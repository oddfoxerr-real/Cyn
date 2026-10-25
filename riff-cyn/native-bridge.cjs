"use strict";
const fs=require("node:fs");
const path=require("node:path");
const {execFile,spawn}=require("node:child_process");
const {promisify}=require("node:util");
const {commandFor}=require("./commands.cjs");
const exec=promisify(execFile);
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const isWin=()=>process.platform==="win32";

function candidates(){
  const paths=[
    process.env.CYN_NATIVE_EXE,
    // Packaged under {app}/CYN-Interface; native CYN lives one folder higher.
    path.resolve(path.dirname(process.execPath),"..","Cyn.exe"),
    path.join(process.env.ProgramFiles||"C:\\Program Files","Cyn","Cyn.exe"),
    path.join(process.env.LOCALAPPDATA||"","Programs","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","dist","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","..","dist","Cyn","Cyn.exe")
  ];
  return paths.filter(Boolean);
}
function validExe(p){return typeof p==="string"&&path.win32.basename(p).toLowerCase()==="cyn.exe"&&fs.existsSync(p);}
function startupFailure(){
  // A diagnostic written by the real JUCE host. Do not present stale
  // diagnostics after a successful launch or leak private user files.
  try{
    const file=path.join(process.env.APPDATA||"","Cyn","engine-startup-error.txt");
    const stats=fs.statSync(file);
    if(stats.size<12||stats.size>2048||Date.now()-stats.mtimeMs>300000)return null;
    const reason=fs.readFileSync(file,"utf8").trim();
    return reason.length>0&&reason.length<1000?reason:null;
  }catch{return null;}
}
function makeBridge(override){
 let nativeExe=validExe(override)?override:null;
 let pending=null;
 let lastState=null;
 function locate(){if(nativeExe&&validExe(nativeExe))return nativeExe;return candidates().find(validExe)||null;}
 function selectNative(p){if(!validExe(p))throw new Error("Select the real Cyn.exe, not another program");nativeExe=p;lastState=null;return true;}
 async function running(exe){
   const command='$found=@(Get-Process -Name Cyn,FxSound -ErrorAction SilentlyContinue | Where-Object { $_.Path -and [string]::Equals($_.Path,$env:CYN_BRIDGE_EXE,[StringComparison]::OrdinalIgnoreCase) }); if($found.Count -gt 0) { Write-Output "RUNNING" }';
   const {stdout}=await exec("powershell.exe",["-NoProfile","-NonInteractive","-Command",command],{
     windowsHide:true,timeout:6500,maxBuffer:2048,env:{...process.env,CYN_BRIDGE_EXE:exe}});
   return stdout.includes("RUNNING");
 }
 async function statusImpl(){
   if(!isWin())return {connected:false,error:"CYN's native audio engine runs on Windows."};
   const exe=locate();
   if(!exe)return {connected:false,error:"Cyn.exe not found. Locate your installed native CYN engine."};
   if(!(await running(exe)))return {connected:false,
     error:startupFailure()||"Native FxSound/CYN is not running. Close other FxSound instances from the system tray, then press Connect engine."};
   const statusPath=path.join(process.env.APPDATA||"","Cyn","status.json");
   const previous=fs.existsSync(statusPath)?fs.statSync(statusPath).mtimeMs:0;
   // The 39-band DSP state, presets and neural profile inventory exceed 4 KB.
   // Never treat valid native CYN telemetry as an engine failure due to a
   // tiny stdout buffer on console-launched installations.
   await exec(exe,["--status"],{windowsHide:true,timeout:8500,maxBuffer:262144});
   for(let i=0;i<45;i++){
     await pause(110);
     if(!fs.existsSync(statusPath)||fs.statSync(statusPath).mtimeMs<=previous)continue;
     const data=JSON.parse(fs.readFileSync(statusPath,"utf8"));
     if(data.cyn_build!=="identity-2")throw new Error("The running audio engine is not CYN.");
     lastState=data;
     return {connected:true,status:data};
   }
   throw new Error(startupFailure()||"The FxSound DSP host is running but not responding. Try reconnecting or repairing CYN.");
 }
 function getStatus(){
   if(!pending)pending=statusImpl().catch(e=>({connected:false,error:e.message})).finally(()=>{pending=null;});
   return pending;
 }
 function telemetry(){
   // Opt-in local stream only. No raw audio, no network and no need to spawn
   // a native process every frame on slower Windows machines.
   if(!isWin()||!lastState||lastState.cyn_build!=="identity-2"||
      lastState.ui_telemetry_schema!==1)return null;
   const root=path.join(process.env.APPDATA||"","Cyn");
   try{
     fs.mkdirSync(root,{recursive:true});
     const watch=path.join(root,"ui-telemetry-request");
     if(!fs.existsSync(watch))fs.writeFileSync(watch,"1",{mode:0o600});
     const now=new Date();
     fs.utimesSync(watch,now,now);
     const file=path.join(root,"ui-telemetry.json");
     const stat=fs.statSync(file);
     if(stat.size<16||stat.size>65536||Date.now()-stat.mtimeMs>1350)return null;
     const sample=JSON.parse(fs.readFileSync(file,"utf8"));
     if(sample.schema!==1||typeof sample.sampled_at_ms!=="number"||
        Date.now()-sample.sampled_at_ms>1350||
        !Array.isArray(sample.hidden)||sample.hidden.length!==12||
        !Array.isArray(sample.inputs)||sample.inputs.length!==11||
        !Array.isArray(sample.outputs)||sample.outputs.length!==5||
        !Array.isArray(sample.input_connections)||sample.input_connections.length!==132||
        !Array.isArray(sample.output_connections)||sample.output_connections.length!==60)return null;
     return sample;
   }catch{return null;}
 }
 async function launch(){
   if(!isWin())throw new Error("CYN audio processing is Windows-only");
   const exe=locate();
   if(!exe)throw new Error("The native FxSound DSP executable Cyn.exe is missing. Repair CYN using the official combined Windows installer.");
   if(await running(exe))return getStatus();
   const child=spawn(exe,["--run_minimized"],{detached:true,windowsHide:true,stdio:"ignore"});
   child.unref();
   // Windows audio initialization and first-run signed driver activation
   // can take longer than a fixed 1.8 s delay. Return a real status or error.
   let result=null;
   for(let attempt=0;attempt<7;attempt++){
     await pause(attempt===0?1100:700);
     result=await getStatus();
     if(result.connected)return result;
     if(result.error&&result.error.includes("another process"))break;
   }
   return result||{connected:false,error:"The native FxSound DSP did not start."};
 }
 async function apply(action){
   if(!isWin())throw new Error("This backend only runs on Windows");
   const exe=locate();
   if(!exe||!await running(exe))throw new Error("Start native CYN before changing audio settings");
   const state=await getStatus();
   if(!state.connected)throw new Error(state.error);
   const arg=commandFor(action,state.status);
   await exec(exe,[arg],{windowsHide:true,timeout:8500,maxBuffer:4096});
   return getStatus();
 }
 return {getStatus,telemetry,launch,apply,selectNative,findExe:()=>locate()};
}
module.exports={makeBridge};
