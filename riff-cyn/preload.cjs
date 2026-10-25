"use strict";
const {contextBridge,ipcRenderer}=require("electron");
contextBridge.exposeInMainWorld("cyn",Object.freeze({
  status:()=>ipcRenderer.invoke("cyn:status"),
  apply:(action)=>ipcRenderer.invoke("cyn:apply",action),
  start:()=>ipcRenderer.invoke("cyn:start"),
  selectNative:()=>ipcRenderer.invoke("cyn:select-native"),
  window:(action)=>ipcRenderer.send("cyn:window",action),
  onClosing:(callback)=>{
    if(typeof callback!=="function")return;
    ipcRenderer.on("cyn:animate-close",()=>callback());
  },
  closeComplete:()=>ipcRenderer.send("cyn:close-complete"),
  alwaysOnTop:(enabled)=>ipcRenderer.invoke("cyn:on-top",enabled),
  openNative:()=>ipcRenderer.invoke("cyn:open-native"),
  openLink:(type)=>ipcRenderer.invoke("cyn:open-link",type),
  checkUpdates:()=>ipcRenderer.invoke("cyn:check-updates")
}));
