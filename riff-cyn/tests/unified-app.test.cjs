"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const read=p=>fs.readFileSync(path.resolve(__dirname,"../..",p),"utf8");
const {commandFor}=require("../commands.cjs");

test("CYN has exactly one visible audio-power control, no engine launcher",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const preload=read("riff-cyn/preload.cjs");
  const main=read("riff-cyn/main.cjs");
  assert.match(html,/id="power"[^>]*aria-pressed="false"/);
  assert(html.includes('id="power-label"'));
  assert.equal((html.match(/id="power"/g)||[]).length,1);
  for(const id of ["start-native","fx-engine-reconnect","select-native","fx-engine-classic"])
    assert(!html.includes('id="'+id+'"'),"Obsolete extra app control: "+id);
  assert(renderer.includes('$("power-label").textContent=on?"On":"Off"'));
  assert(renderer.includes('apply({type:"power",value:requested})'));
  assert(renderer.includes('setAttribute("aria-pressed",String(on))'));
  assert(!renderer.includes("window.cyn.start("));
  assert(!renderer.includes("window.cyn.selectNative("));
  assert(!preload.includes('cyn:start'));
  assert(!preload.includes('cyn:select-native'));
  assert(!main.includes('ipcMain.handle("cyn:start"'));
  assert(!main.includes('ipcMain.handle("cyn:select-native"'));
});

test("power changes DSP state without killing its Windows service or erasing presets",()=>{
  const state={cyn_build:"identity-2"};
  assert.equal(commandFor({type:"power",value:true},state),"--power=1");
  assert.equal(commandFor({type:"power",value:false},state),"--power=0");
  const bridge=read("riff-cyn/native-bridge.cjs");
  const main=read("riff-cyn/main.cjs");
  assert(bridge.includes("commandFor(action,state.status)"));
  assert(bridge.includes("async function ensureRunning()"));
  assert(bridge.includes('spawn(exe,["--engine-host"]'));
  assert(!bridge.includes('spawn(exe,["--classic"]'));
  assert(main.includes('ipcMain.handle("cyn:status",()=>bridge.ensureRunning())'));
  assert(main.includes("bridge.launch().then(result=>"));
  assert(!main.includes("process.kill("));
});

test("packaged CYN always uses its bundled headless native FxSound host",()=>{
  const bridge=read("riff-cyn/native-bridge.cjs");
  const main=read("riff-cyn/main.cjs");
  const native=read("fxsound/Source/Main.cpp");
  const shell=read("fxsound/Source/GUI/CynMaterialShell.h");
  const installer=read("Installer/Cyn/Cyn.iss");
  assert(bridge.includes("if(packaged)return validExe(bundled)?bundled:null;"));
  assert(main.includes("makeBridge({packaged:app.isPackaged})"));
  assert(!native.includes("std::make_unique<FxMainWindow>()"));
  assert(native.includes("FxController::getInstance().init(nullptr, system_tray_view_.get(), audio_passthru_.get())"));
  assert(shell.includes('trimmed.equalsIgnoreCase("--classic")'));
  assert(native.includes('const bool headlessHost = commandline.trim().equalsIgnoreCase("--engine-host")'));
  assert((native.match(/if \(!headlessHost\) AlertWindow::showMessageBox/g)||[]).length===3,
    "Automatic CYN audio recovery must not spawn a second warning window");
  assert(installer.includes('Name: "{autoprograms}\\Cyn"; Filename: "{app}\\CYN-Interface\\CYN Audio.exe"'));
});
