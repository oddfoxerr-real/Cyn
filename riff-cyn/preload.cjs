"use strict";
const {contextBridge,ipcRenderer}=require("electron");
contextBridge.exposeInMainWorld("cyn",Object.freeze({
  status:()=>ipcRenderer.invoke("cyn:status"),
  scanDevices:()=>ipcRenderer.invoke("cyn:scan-devices"),
  telemetry:()=>ipcRenderer.invoke("cyn:telemetry"),
  apply:(action)=>ipcRenderer.invoke("cyn:apply",action),
  window:(action)=>ipcRenderer.send("cyn:window",action),
  onClosing:(callback)=>{
    if(typeof callback!=="function")return;
    ipcRenderer.on("cyn:animate-close",()=>callback());
  },
  closeComplete:()=>ipcRenderer.send("cyn:close-complete"),
  alwaysOnTop:(enabled)=>ipcRenderer.invoke("cyn:on-top",enabled),
  openLink:(type)=>ipcRenderer.invoke("cyn:open-link",type),
  inspectUpdates:()=>ipcRenderer.invoke("cyn:updates-inspect"),
  checkUpdates:()=>ipcRenderer.invoke("cyn:check-updates")
}));
