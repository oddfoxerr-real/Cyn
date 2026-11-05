"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const read=name=>fs.readFileSync(path.resolve(__dirname,"../..",name),"utf8");

test("connected output discovery stays live and devices are accessible",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  assert(renderer.includes("},1600);"),"Foreground audio poll must refresh more frequently than four seconds");
  assert(renderer.includes('window.addEventListener("focus"'));
  assert(renderer.includes('document.addEventListener("visibilitychange"'));
  for(const id of ["device-refresh-top","device-refresh-page","device-list","device-list-status"]){
    assert(html.includes('id="'+id+'"'),"Missing audio device discovery control: "+id);
  }
  assert(html.includes('role="group" aria-label="Connected Windows audio outputs"'));
  assert(renderer.includes('button.setAttribute("aria-pressed",String(selected))'));
  assert(renderer.includes('button.disabled=!writable()||!view.canSelect'));
  assert(renderer.includes("const signature=JSON.stringify(live)"));
  assert(renderer.includes("list.dataset.signature!==signature"),
    "A refresh must preserve keyboard focus if the device list is unchanged");
  assert(renderer.includes('button.addEventListener("click",()=>apply({type:"output",value:name}))'));
});
test("power switch responds immediately and rolls back on native errors",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const css=read("riff-cyn/renderer/cyn.css");
  assert(html.includes('class="cyn-power-switch"'));
  assert(html.includes('id="power-hint"'));
  const click=renderer.slice(renderer.indexOf('setPowerView(requested,true)'));
  assert(click.indexOf('setPowerView(requested,true)')<click.indexOf('await apply({type:"power"'));
  assert(renderer.includes('pendingPowerState=null;'));
  assert(renderer.includes('setPowerView(current?.status?.power===true)'));
  assert(css.includes(".cyn-power-knob")&&css.includes(".cyn-power-switch.is-on"));
});
test("host checks are cached but device scans still request actual native status",()=>{
  const bridge=read("riff-cyn/native-bridge.cjs");
  assert(bridge.includes("let lastHostCheck=0"));
  assert(bridge.includes("Date.now()-lastHostCheck>12000"));
  assert(bridge.includes('await exec(exe,["--status"]'));
  assert(bridge.includes('data=JSON.parse(fs.readFileSync(statusPath,"utf8"))'));
});

test("manual refresh really forces Windows native MMDevice enumeration",()=>{
 const renderer=read("riff-cyn/renderer/renderer.js");
 const main=read("riff-cyn/main.cjs");
 const bridge=read("riff-cyn/native-bridge.cjs");
 const preload=read("riff-cyn/preload.cjs");
 const native=read("fxsound/Source/GUI/FxController.cpp");
 const pub=read("audiopassthru/include/AudioPassthru.h");
 const priv=read("audiopassthru/src/AudioPassthru/AudioPassthruPrivate.cpp");
 assert(renderer.includes("await window.cyn.scanDevices()"));
 assert(preload.includes('scanDevices:()=>ipcRenderer.invoke("cyn:scan-devices")'));
 assert(main.includes('ipcMain.handle("cyn:scan-devices",()=>bridge.scanDevices())'));
 assert(bridge.includes('await exec(exe,["--rescan_outputs"]'));
 assert(native.includes('arg_list.containsOption("--rescan_outputs")'));
 assert(pub.includes("bool force_rescan = false"));
 assert(priv.includes("if(changed || force_rescan)"));
});
