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
    path.join(process.env.ProgramFiles||"C:\\Program Files","Cyn","Cyn.exe"),
    path.join(process.env.LOCALAPPDATA||"","Programs","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","dist","Cyn","Cyn.exe"),
    path.resolve(__dirname,"..","..","dist","Cyn","Cyn.exe")
  ];
  return paths.filter(Boolean);
}
function validExe(p){return typeof p==="string"&&path.win32.basename(p).toLowerCase()==="cyn.exe"&&fs.existsSync(p);}
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
   if(!(await running(exe)))return {connected:false,error:"Native CYN is not running. Start the engine to connect."};
   const statusPath=path.join(process.env.APPDATA||"","Cyn","status.json");
   const previous=fs.existsSync(statusPath)?fs.statSync(statusPath).mtimeMs:0;
   await exec(exe,["--status"],{windowsHide:true,timeout:8500,maxBuffer:4096});
   for(let i=0;i<45;i++){
     await pause(110);
     if(!fs.existsSync(statusPath)||fs.statSync(statusPath).mtimeMs<=previous)continue;
     const data=JSON.parse(fs.readFileSync(statusPath,"utf8"));
     if(data.cyn_build!=="identity-2")throw new Error("The running audio engine is not CYN.");
     lastState=data;
     return {connected:true,status:data};
   }
   throw new Error("Native CYN did not return a fresh status report.");
 }
 function getStatus(){
   if(!pending)pending=statusImpl().catch(e=>({connected:false,error:e.message})).finally(()=>{pending=null;});
   return pending;
 }
 async function launch(){
   if(!isWin())throw new Error("CYN audio processing is Windows-only");
   const exe=locate();
   if(!exe)throw new Error("Select an installed Cyn.exe in Settings");
   if(await running(exe))return getStatus();
   const child=spawn(exe,["--run_minimized"],{detached:true,windowsHide:true,stdio:"ignore"});
   child.unref();
   await pause(1800);
   return getStatus();
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
 return {getStatus,launch,apply,selectNative,findExe:()=>locate()};
}
module.exports={makeBridge};
