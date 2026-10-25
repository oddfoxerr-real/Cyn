"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const vm=require("node:vm");
const source=fs.readFileSync(path.resolve(__dirname,"../renderer/updates.js"),"utf8");
const read=p=>fs.readFileSync(path.resolve(__dirname,"../..",p),"utf8");

function renderHarness(inspectUpdates,checkUpdates=async()=>({started:true})){
  const names=["cyn-updates-dialog","cyn-updates-title","cyn-updates-detail",
    "cyn-updates-current","cyn-updates-latest","cyn-updates-retry",
    "cyn-updates-install","cyn-updates-done","cyn-updates-close"];
  const els=Object.fromEntries(names.map(id=>[id,{
    id,open:false,dataset:{},hidden:false,disabled:false,textContent:"",
    events:{},
    addEventListener(type,callback){(this.events[type]??=[]).push(callback)},
    dispatch(type){for(const fn of this.events[type]||[])fn({preventDefault(){}})},
    click(){this.dispatch("click")},
    showModal(){this.open=true},
    close(){this.open=false;this.dispatch("close")}
  }]));
  const ctx={window:{cyn:{inspectUpdates,checkUpdates}},document:{getElementById:id=>els[id]}};
  vm.runInNewContext(source,ctx,{filename:"updates.js"});
  return {els,app:ctx.window.cynUpdate};
}
const tick=()=>new Promise(resolve=>setImmediate(resolve));
test("CYN checks current version without starting an installer",async()=>{
  let starts=0;
  const {els,app}=renderHarness(async()=>({phase:"current",currentVersion:"2.0.88",latestVersion:"2.0.88"}),async()=>{starts++;return {started:true}});
  app.open();await tick();await tick();
  assert.equal(els["cyn-updates-dialog"].open,true);
  assert.equal(els["cyn-updates-title"].textContent,"You're up to date");
  assert.equal(els["cyn-updates-install"].hidden,true);
  assert.equal(els["cyn-updates-current"].textContent,"v2.0.88");
  assert.equal(starts,0);
});
test("CYN offers verified original native installer only when an update is available",async()=>{
  let starts=0;
  const {els,app}=renderHarness(async()=>({phase:"available",currentVersion:"2.0.88",latestVersion:"2.0.89"}),async()=>{starts++;return {started:true}});
  app.open();await tick();
  assert.equal(els["cyn-updates-title"].textContent,"Update available");
  assert.equal(els["cyn-updates-install"].hidden,false);
  assert.equal(starts,0,"Read-only checking must never download or install");
  els["cyn-updates-install"].click();
  await tick();
  assert.equal(starts,1);
  assert.equal(els["cyn-updates-dialog"].open,false);
});
test("CYN updater reports network errors, allows retry, does not fake latest status",async()=>{
  let attempts=0;
  const {els,app}=renderHarness(async()=>{
    if(++attempts===1)throw new Error("Network unavailable");
    return {phase:"current",currentVersion:"2.0.88",latestVersion:"2.0.88"};
  });
  app.open();await tick();
  assert.equal(els["cyn-updates-title"].textContent,"Update check failed");
  assert.match(els["cyn-updates-detail"].textContent,/Network unavailable/);
  assert.equal(els["cyn-updates-retry"].hidden,false);
  els["cyn-updates-retry"].click();await tick();
  assert.equal(els["cyn-updates-title"].textContent,"You're up to date");
});
test("CYN keeps its actual old updater worker and signed/checksummed installer flow",()=>{
  const main=read("riff-cyn/main.cjs");
  const preload=read("riff-cyn/preload.cjs");
  const worker=read("CynUpdaterWorker.ps1");
  const native=read("CynUpdater.ps1");
  const settings=read("riff-cyn/renderer/settings.js");
  const html=read("riff-cyn/renderer/index.html");
  assert(main.includes('ipcMain.handle("cyn:updates-inspect"'));
  assert(main.includes('"-CheckOnly"'));
  assert(main.includes('ipcMain.handle("cyn:check-updates"'));
  assert(main.includes('"-STA","-ExecutionPolicy","Bypass"'));
  assert(preload.includes('inspectUpdates:()=>ipcRenderer.invoke("cyn:updates-inspect")'));
  assert(worker.includes('[switch]$CheckOnly'));
  assert(worker.includes("phase='available'")&&worker.includes("phase='current'"));
  assert(native.includes('Start-Process -FilePath $script:installerPath -Verb RunAs'));
  assert(native.includes("SHA-256 verified"));
  assert(html.includes('id="cyn-updates-dialog"'));
  assert(settings.includes("updates:()=>window.cynUpdate.open()"));
  assert(settings.includes("window.cynUpdate.open()"));
});
