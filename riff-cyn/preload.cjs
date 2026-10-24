"use strict";
const {contextBridge,ipcRenderer}=require("electron");
contextBridge.exposeInMainWorld("cyn",Object.freeze({
  status:()=>ipcRenderer.invoke("cyn:status"),
  apply:(action)=>ipcRenderer.invoke("cyn:apply",action),
  start:()=>ipcRenderer.invoke("cyn:start"),
  selectNative:()=>ipcRenderer.invoke("cyn:select-native"),
  window:(action)=>ipcRenderer.send("cyn:window",action)
}));
