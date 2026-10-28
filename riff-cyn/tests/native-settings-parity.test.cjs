"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const root=path.resolve(__dirname,"../..");
const read=file=>fs.readFileSync(path.join(root,file),"utf8");
const {commandFor}=require("../commands.cjs");
const status={
  cyn_build:"identity-2",
  hotkeys_supported:true,
  equalizer:{bands:Array.from({length:39},(_,index)=>({index,gain:0}))},
  smart_listening:{training:false}
};

test("advanced native DSP parameters remain available without Classic",()=>{
  for(const [action,expected] of [
    [{type:"eqBandCount",value:31},"--num_bands=31"],
    [{type:"eqBandCount",value:35},"--num_bands=35"],
    [{type:"eqBandCount",value:39},"--num_bands=39"],
    [{type:"balance",value:-12.5},"--balance=-12.50"],
    [{type:"masterGain",value:8.25},"--master_gain=8.25"],
    [{type:"volumeLeveling",value:2.3},"--volume_leveling=2.30"],
    [{type:"filterQ",value:1.8},"--filter_q=1.80"]
  ])assert.equal(commandFor(action,status),expected);
  for(const action of [
    {type:"eqBandCount",value:32},
    {type:"eqBandCount",value:"39"},
    {type:"balance",value:21},
    {type:"balance",value:NaN},
    {type:"masterGain",value:-21},
    {type:"volumeLeveling",value:4.1},
    {type:"filterQ",value:.9}
  ])assert.throws(()=>commandFor(action,status),TypeError);
  const native=read("fxsound/Source/GUI/FxController.cpp");
  for(const option of ["--num_bands","--balance","--master_gain","--volume_leveling","--filter_q"])
    assert(native.includes(option),"Missing CYN native DSP parameter "+option);
});

test("CYN automatic updates, startup and shortcut toggles use saved native settings",()=>{
  const native=read("fxsound/Source/GUI/FxController.cpp");
  const html=read("riff-cyn/renderer/index.html");
  const settings=read("riff-cyn/renderer/settings.js");
  const renderer=read("riff-cyn/renderer/renderer.js");
  for(const [action,argument] of [
    ["automaticUpdates","--cyn_automatic_updates=1"],
    ["launchOnStartup","--cyn_startup=1"],
    ["hotkeysEnabled","--cyn_hotkeys=1"]
  ])assert.equal(commandFor({type:action,value:true},status),argument);
  for(const action of ["automaticUpdates","launchOnStartup","hotkeysEnabled"])
    assert.throws(()=>commandFor({type:action,value:"1"},status),TypeError);
  assert.throws(()=>commandFor({type:"hotkeysEnabled",value:true},{...status,hotkeys_supported:false}),TypeError);
  for(const required of [
    "setAutoUpdates(automaticUpdates==\"1\")",
    "setLaunchOnStartup(startup==\"1\")",
    "enableHotkeys(hotkeys==\"1\" && SysInfo::canSupportHotkeys())",
    'settings->setProperty("automatic_updates",getAutoUpdates())',
    'settings->setProperty("launch_on_startup",isLaunchOnStartup())',
    'settings->setProperty("hotkeys_enabled",model.getHotkeySupport())'
  ])assert(native.includes(required),"Native CYN setting missing: "+required);
  for(const id of ["automatic-updates","launch-on-startup","hotkeys-enabled"])
    assert(html.includes('id="'+id+'"'),"Material Settings missing "+id);
  assert(settings.includes('window.addEventListener("cyn-native-status"'));
  assert(renderer.includes('new CustomEvent("cyn-native-status"'));
});

test("advanced EQ renderer maintains state from native CYN rather than UI defaults",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const commandTypes=["eqBandCount","balance","masterGain","volumeLeveling","filterQ"];
  const ids=["eq-band-select","eq-balance","eq-master-gain","eq-volume-leveling","eq-filter-q"];
  for(const id of ids)assert(html.includes('id="'+id+'"'),"Missing native EQ control: "+id);
  for(const type of commandTypes)assert(renderer.includes('type:"'+type+'"')||renderer.includes('"'+type+'"'),
    "Missing native EQ action: "+type);
  for(const field of ["master_gain","volume_leveling","filter_q","balance"])
    assert(renderer.includes('"'+field+'"'),"Native EQ state not represented: "+field);
});

test("CYN updates remain verified, report installation errors and reopen Material",()=>{
  const main=read("riff-cyn/main.cjs");
  const updater=read("CynUpdater.ps1");
  const worker=read("CynUpdaterWorker.ps1");
  assert(main.includes('ipcMain.handle("cyn:check-updates"'));
  assert(main.includes('"-Interactive"'));
  assert(main.includes("const live=await bridge.getStatus()"));
  assert(main.includes("CYN_NATIVE_EXE:exe"));
  assert(main.includes(".VersionInfo.FileVersion"));
  assert(main.includes('"-NonInteractive","-Command",powershell'));
  assert(!main.includes('if(!live.connected)throw new Error(live.error||"Start CYN to check for updates")'));
  assert(!main.includes('spawn(exe,["--classic"]'),"Riff must not launch the redundant old CYN UI");
  assert(worker.includes('SHA256')||worker.includes('SHA-256'));
  assert(updater.includes("CYN-Interface\\CYN Audio.exe"));
  assert(updater.includes("CYN updated but could not restart the native engine and redesigned interface"));
  assert(updater.includes("Show-Error ('CYN updated but could not restart"));
  assert(!updater.includes("Start-Process -FilePath (Join-Path $PSScriptRoot 'Cyn.exe') -ArgumentList '--view=2'"));
  assert(!updater.includes("} catch {}\n            $script:allowedToClose=$true"));
});

test("native AI movement is rendered from measured applied EQ and clears on stale telemetry",()=>{
  const brain=read("riff-cyn/renderer/brain-scan.js");
  const graph=read("riff-cyn/renderer/renderer.js");
  const native=read("fxsound/Source/GUI/FxController.cpp");
  assert(brain.includes('sample?.applied_bands'));
  assert(brain.includes('line.id="eq-ai-applied-curve"'));
  assert(brain.includes('appliedDisplay=appliedDisplay.map('));
  assert(brain.includes('previous+(Number(values[i])-previous)*.23'));
  assert(brain.includes('if(!actual)'));
  assert(brain.includes('sample=null;lastSample=0'));
  assert(graph.includes('svg.querySelector("#eq-ai-applied-curve")'));
  assert(graph.includes('if(realAppliedLine)svg.append(realAppliedLine)'));
  assert(native.includes('getAppliedEqBandBoostCut(i)'));
  assert(!brain.includes("Math.random"));
});

test("invalid or unavailable Windows startup registry entries never crash native status",()=>{
  const native=read("fxsound/Source/GUI/FxController.cpp");
  assert(native.includes('HKEY hkey = nullptr;'));
  assert(native.includes('if (RegOpenKeyExW('));
  assert(native.includes('return query == ERROR_SUCCESS && type == REG_SZ'));
});
