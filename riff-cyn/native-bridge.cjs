"use strict";
const fs=require("node:fs");
const path=require("node:path");
const {execFile,spawn}=require("node:child_process");
const {promisify}=require("node:util");
const {commandFor}=require("./commands.cjs");
const exec=promisify(execFile);
const pause=ms=>new Promise(resolve=>setTimeout(resolve,ms));
const isWin=()=>process.platform==="win32";

function bundledNative(executable=process.execPath){
  // The combined installer owns one *authoritative* DSP executable. A saved
  // manual path from an old preview must never override the adjacent Cyn.exe.
  return path.resolve(path.dirname(executable),"..","Cyn.exe");
}
function candidates(){
  const paths=[
    bundledNative(),
    process.env.CYN_NATIVE_EXE,
    path.join(process.env.ProgramFiles||"C:\\Program Files","Cyn","Cyn.exe"),
    path.join(process.env.LOCALAPPDATA||"","Programs","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","dist","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","..","dist","Cyn","Cyn.exe")
  ];
  return paths.filter(Boolean);
}
function validExe(p){
  return typeof p==="string"&&path.win32.basename(p).toLowerCase()==="cyn.exe"&&
    fs.existsSync(p)&&fs.statSync(p).isFile();
}
// A bundled release must never silently redirect its IPC, updater or DSP
// commands to an unrelated/old Cyn.exe saved by standalone previews.
function chooseNative(bundled,saved,paths){
  if(validExe(bundled))return bundled;
  if(validExe(saved))return saved;
  return paths.find(validExe)||null;
}
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
 let launchPending=null;
 let autoRestartPending=null;
 let lastAutoRestart=0;
 function locate(){return chooseNative(bundledNative(),nativeExe,candidates());}
 function selectNative(p){
   if(!validExe(p))throw new Error("Select the real Cyn.exe, not another program");
   const bundle=bundledNative();
   if(validExe(bundle)&&path.resolve(p).toLowerCase()!==path.resolve(bundle).toLowerCase())
     throw new Error("Installed CYN always uses its bundled native FxSound engine. An older standalone Cyn.exe cannot replace it.");
   nativeExe=p;lastState=null;return true;
 }
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
   // Process enumeration occasionally misses an existing host on slower
   // Windows machines. Confirm once more before telling the UI to disconnect.
   if(!(await running(exe))){
     await pause(200);
     if(!(await running(exe)))return {connected:false,
       error:startupFailure()||"Native FxSound/CYN is not running. Close other FxSound instances from the system tray, then press Connect engine."};
   }
   const statusPath=path.join(process.env.APPDATA||"","Cyn","status.json");
   const previous=fs.existsSync(statusPath)?fs.statSync(statusPath).mtimeMs:0;
   // The 39-band DSP state, presets and neural profile inventory exceed 4 KB.
   // Never treat valid native CYN telemetry as an engine failure due to a
   // tiny stdout buffer on console-launched installations.
   await exec(exe,["--status"],{windowsHide:true,timeout:8500,maxBuffer:262144});
   for(let i=0;i<45;i++){
     await pause(110);
     if(!fs.existsSync(statusPath)||fs.statSync(statusPath).mtimeMs<=previous)continue;
     let data;
     try{
       // CYN writes status.json from the native JUCE thread. Do not treat
       // one in-progress file replacement as an engine disconnection.
       data=JSON.parse(fs.readFileSync(statusPath,"utf8"));
     }catch{
       await pause(80);
       continue;
     }
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
 async function launchImpl(){
   if(!isWin())throw new Error("CYN audio processing is Windows-only");
   const exe=locate();
   if(!exe)throw new Error("The original Cyn.exe DSP executable is missing. Repair the complete CYN Windows installation.");
   if(await running(exe))return getStatus();
   const child=spawn(exe,["--run_minimized"],{
     detached:true,windowsHide:true,stdio:"ignore",cwd:path.dirname(exe)
   });
   // Do not report a successful launch if Windows refused to start the host.
   await new Promise((resolve,reject)=>{
     child.once("error",reject);
     child.once("spawn",resolve);
   });
   child.unref();
   let result=null;
   for(let attempt=0;attempt<8;attempt++){
     await pause(attempt===0?1100:750);
     result=await getStatus();
     if(result.connected)return result;
     if(result.error&&/another (fxsound|cyn|process|audio host)/i.test(result.error))break;
   }
   return result||{connected:false,error:"The original FxSound/CYN audio host did not become ready."};
 }
 function launch(){
   // Starting the Material window and connecting from its controls may race.
   // One native host must own the signed virtual audio driver.
   if(!launchPending)launchPending=launchImpl().finally(()=>{launchPending=null;});
   return launchPending;
 }
 async function ensureRunning(){
   const status=await getStatus();
   if(status.connected||!locate())return status;
   if(/another (fxsound|cyn|audio host)/i.test(status.error||""))return status;
   if(autoRestartPending)return autoRestartPending;
   // If the native engine disappears, reconnect automatically; do not require
   // opening Classic. Cap retries to avoid spawning failures in a loop.
   if(Date.now()-lastAutoRestart<30000)return status;
   lastAutoRestart=Date.now();
   autoRestartPending=launch().catch(error=>({connected:false,error:error.message}))
     .finally(()=>{autoRestartPending=null;});
   return autoRestartPending;
 }
 async function apply(action){
   if(!isWin())throw new Error("This backend only runs on Windows");
   const exe=locate();
   if(!exe)throw new Error("Original Cyn.exe is not installed.");
   // An action from Material must connect the exact bundled native engine,
   // instead of requiring the separate Classic window to bootstrap it.
   const state=await ensureRunning();
   if(!state.connected)throw new Error(state.error||"Original CYN engine could not start");
   const arg=commandFor(action,state.status);
   await exec(exe,[arg],{windowsHide:true,timeout:8500,maxBuffer:4096});
   // A renderer poll can have started before this native command. Never
   // return that old pending status as proof of the newly selected device.
   if(pending)await pending;
   let result=null;
   for(let attempt=0;attempt<3;attempt++){
     if(attempt)await pause(260);
     result=await getStatus();
     const observed=result.connected&&result.status;
     if(!observed)continue;
     if(action.type==="power"&&result.status.power!==action.value)continue;
     if(action.type==="output"&&result.status.selected_output!==action.value)continue;
     return result;
   }
   return result||{connected:false,error:"CYN did not return fresh native audio state."};
 }
 return {getStatus,ensureRunning,telemetry,launch,apply,selectNative,findExe:()=>locate()};
}
module.exports={makeBridge,chooseNative,bundledNative};
