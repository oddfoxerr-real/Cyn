"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const read=file=>fs.readFileSync(path.resolve(__dirname,"../..",file),"utf8");

test("Material CYN exposes FXSound engine and direct Settings/Updates without DSP connection",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const app=read("riff-cyn/renderer/renderer.js");
  const settings=read("riff-cyn/renderer/settings.js");
  for(const id of ["toolbar-settings","toolbar-updates","fx-engine-reconnect","fx-engine-repair","fx-engine-detail","fx-engine-output"])
    assert(html.includes('id="'+id+'"'),"Missing visible engine or update control: "+id);
  assert(html.indexOf('id="toolbar-settings"')<html.indexOf('class="app-layout"'));
  assert(html.indexOf('id="fx-engine-detail"')<html.indexOf('class="eq-hero"'),"Show native DSP state before scrollable EQ");
  assert(app.includes('$("toolbar-settings").addEventListener("click",()=>transition("settings"))'));
  assert(app.includes('$("fx-engine-reconnect").addEventListener("click",async()=>'));
  assert(settings.includes('toolbarUpdates.addEventListener("click",async()=>'));
  assert(settings.includes("window.cynUpdate.open()"));
  assert(html.includes('data-setting="updates"'));
  assert(html.includes('data-setting="release"'));
});

test("engine health is emitted by genuine native FxSound DSP, not fabricated in renderer",()=>{
  const controller=read("fxsound/Source/GUI/FxController.cpp");
  const bridge=read("riff-cyn/native-bridge.cjs");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const startup=read("fxsound/Source/Main.cpp");
  assert(controller.includes('engine->setProperty("backend","FxSound/CYN native DSP")'));
  assert(controller.includes('engine->setProperty("ready",dfx_enabled_)'));
  assert(controller.includes('engine->setProperty("processing",dfx_enabled_ && audio_process_on_ && model.getPowerState())'));
  assert(controller.includes('settings->setProperty("fx_engine",var(engine))'));
  assert(controller.includes("engine-startup-error.txt"));
  assert(startup.includes("engineStartupError("));
  assert(bridge.includes("function startupFailure()"));
  assert(bridge.includes('Get-Process -Name Cyn,FxSound'));
  assert(renderer.includes("status.fx_engine"));
  assert(renderer.includes('fx.processing===true'));
  assert(renderer.includes('status.selected_output'));
  assert(bridge.includes('maxBuffer:262144'));

  assert(!renderer.includes("AudioContext"));
  assert(!renderer.includes("navigator.mediaDevices"));
});

test("production Windows release checks the actual native engine, models and updater payload",()=>{
  const build=read("BUILD_CYN.ps1");
  const installer=read("Installer/Cyn/Cyn.iss");
  const workflow=read(".github/workflows/windows-release.yml");
  assert(build.includes("Copy-Item -LiteralPath $exe -Destination (Join-Path $dist 'Cyn.exe')"));
  assert(build.includes("Copy-Item -LiteralPath (Join-Path $root 'bin\\x64\\Factsoft')"));
  assert(installer.includes('Source: "{#PayloadDir}\\*"'));
  for(const name of ["dist/Cyn/Cyn.exe","dist/Cyn/CynUpdater.ps1",
      "dist/Cyn/onnxruntime.dll","dist/Cyn/apex6m-225.onnx",
      "dist/Cyn/CYN-Interface/CYN Audio.exe"]){
    assert(workflow.includes(name),"Windows release must fail on missing core component: "+name);
  }
  assert(workflow.includes("Critical native CYN component missing"));
  assert(workflow.includes("nativePresets.Count -lt 8"));
});
