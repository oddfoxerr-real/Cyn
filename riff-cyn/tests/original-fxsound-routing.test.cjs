"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const root=path.resolve(__dirname,"../..");
const read=p=>fs.readFileSync(path.join(root,p),"utf8");
const {commandFor}=require("../commands.cjs");

test("Material is a UI only: no custom Windows routing commands",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const native=read("fxsound/Source/GUI/FxController.cpp");
  const bridge=read("riff-cyn/native-bridge.cjs");
  const engine=read("audiopassthru/src/AudioPassthru/AudioPassthruPrivate.cpp");
  assert(!html.includes('id="fx-engine-route"'));
  assert(!renderer.includes("restoreAudio"));
  assert(!read("riff-cyn/commands.cjs").includes("--restore_audio"));
  assert(!native.includes("--restore_audio"));
  assert(!native.includes("restoreCynRouting"));
  assert(!engine.includes("restoreCynRouting"));
  assert.throws(()=>commandFor({type:"restoreAudio"},{cyn_build:"identity-2"}),/Unknown|Unsupported|Invalid/i);
  assert(!html.includes('id="fx-engine-classic"'),"No duplicated legacy CYN interface");
  assert(!renderer.includes("window.cyn.openNative()"));
  assert(bridge.includes('spawn(exe,["--engine-host"]'),"Riff starts the FxSound host headlessly");
  assert(!renderer.includes("AudioContext"));
  assert(!renderer.includes("getUserMedia"));
});

test("native FxSound owns its unmodified audio capture, DSP and physical playback",()=>{
  const ctrl=read("fxsound/Source/GUI/FxController.cpp");
  const publicWrapper=read("audiopassthru/src/AudioPassthru/AudioPassthru.cpp");
  const native=read("audiopassthru/src/AudioPassthru/AudioPassthruPrivate.cpp");
  const rules=read("audiopassthru/src/sndDevices/sndDevicesImplementDeviceRules.cpp");
  assert(ctrl.includes("audio_passthru_->setDspProcessingModule(&dfx_dsp_)"));
  assert(ctrl.includes("setOutput(default_output.pwszID.c_str())"));
  assert(ctrl.includes('engine->setProperty("processing",virtualActive && virtualDefault'));
  assert(native.includes("p_dfx_dsp_->processAudio"));
  assert(native.includes("sndDevicesReInit("));
  assert(!native.includes("restoreCynRouting"));
  assert(!publicWrapper.includes("restoreCynRouting"));
  assert(rules.includes("sndDevicesSetDeviceType(hp_sndDevices, SND_DEVICES_DEFAULT, cast_handle->pwszID[cast_handle->captureDeviceNum]"));
});
test("Material reports original engine output rather than inventing audio-routing success",()=>{
  const renderer=read("riff-cyn/renderer/renderer.js");
  assert(renderer.includes("status.selected_output"));
  assert(renderer.includes("engine.processing===true"));
  assert(renderer.includes('"ENGINE ON"'));
  assert(!renderer.includes('"ROUTED"'));
  assert(!renderer.includes('"Windows via CYN: "'));
  assert(!renderer.includes('window.cyn.openNative()'));
});
