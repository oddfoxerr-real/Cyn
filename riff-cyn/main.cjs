"use strict";
const {app,BrowserWindow,ipcMain,dialog,shell}=require("electron");
const fs=require("node:fs");
const path=require("node:path");
const {makeBridge}=require("./native-bridge.cjs");
if(process.platform==="win32")app.commandLine.appendSwitch("disable-features","CalculateNativeWinOcclusion");
let win;
let bridge;
let overridePath=null;
const settingsFile=()=>path.join(app.getPath("userData"),"native-engine.json");
function loadConfig(){
  try{
    const v=JSON.parse(fs.readFileSync(settingsFile(),"utf8"));
    return typeof v.executable==="string"?v.executable:null;
  }catch{return null;}
}
function saveConfig(executable){
  fs.mkdirSync(path.dirname(settingsFile()),{recursive:true});
  fs.writeFileSync(settingsFile(),JSON.stringify({executable},null,2));
}
function createWindow(){
  win=new BrowserWindow({
    width:1370,height:840,minWidth:760,minHeight:580,
    show:false,backgroundColor:"#121214",title:"CYN Riff",
    frame:false,
    webPreferences:{
      preload:path.join(__dirname,"preload.cjs"),
      contextIsolation:true,nodeIntegration:false,sandbox:true,
      webSecurity:true
    }
  });
  win.loadFile(path.join(__dirname,"renderer","index.html"));
  win.once("ready-to-show",()=>win.show());
  win.webContents.setWindowOpenHandler(()=>({action:"deny"}));
  win.webContents.on("will-navigate",event=>event.preventDefault());
}
app.whenReady().then(()=>{
  bridge=makeBridge(loadConfig());
  ipcMain.handle("cyn:status",()=>bridge.getStatus());
  ipcMain.handle("cyn:apply",async (_e,action)=>bridge.apply(action));
  ipcMain.handle("cyn:start",()=>bridge.launch());
  ipcMain.handle("cyn:select-native",async()=>{
    const result=await dialog.showOpenDialog(win,{
      title:"Select your installed native CYN engine",
      filters:[{name:"CYN native executable",extensions:["exe"]}],
      properties:["openFile"]
    });
    if(result.canceled||!result.filePaths.length)return {selected:false};
    const executable=result.filePaths[0];
    bridge.selectNative(executable);
    saveConfig(executable);
    return {selected:true};
  });
  ipcMain.on("cyn:window",(_e,action)=>{
    if(!win||win.isDestroyed())return;
    if(action==="minimize")win.minimize();
    else if(action==="maximize")win.isMaximized()?win.unmaximize():win.maximize();
    else if(action==="close")win.close();
  });
  createWindow();
});
app.on("window-all-closed",()=>{app.quit();});
