"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const root=path.resolve(__dirname,"../..");
const read=p=>fs.readFileSync(path.join(root,p),"utf8");
const {commandFor}=require("../commands.cjs");

const status={cyn_build:"identity-2",fx_engine:{
  virtual_device_active:true,physical_output_ready:true
}};
test("restore original system audio has a typed command and refuses a missing driver",()=>{
  assert.equal(commandFor({type:"restoreAudio"},status),"--restore_audio");
  assert.throws(()=>commandFor({type:"restoreAudio"},{
    cyn_build:"identity-2",fx_engine:{virtual_device_active:false,physical_output_ready:true}
  }),/virtual audio driver/);
  assert.throws(()=>commandFor({type:"restoreAudio"},{
    cyn_build:"identity-2",fx_engine:{virtual_device_active:true,physical_output_ready:false}
  }),/speaker or headphone/);
  assert.throws(()=>commandFor({type:"restoreAudio",value:';malicious'},null),/Native CYN status/);
});
test("new Material UI is only a controller of existing FxSound DSP",()=>{
  const html=read("riff-cyn/renderer/index.html");
  const renderer=read("riff-cyn/renderer/renderer.js");
  const main=read("riff-cyn/main.cjs");
  const bridge=read("riff-cyn/native-bridge.cjs");
  assert(html.includes('id="fx-engine-route"'));
  assert(renderer.includes('apply({type:"restoreAudio"})'));
  assert(renderer.includes("fx.virtual_device_default")===false ||
    renderer.includes("fx.route_enabled"),"Cannot invent audio route");
  assert(renderer.includes('fx.route_enabled===true'));
  assert(renderer.includes('fx.processing===true'));
  assert(renderer.includes('$("engine-state").classList.toggle("on",routed)'));
  assert(main.includes("bridge.launch()"));
  assert(bridge.includes('spawn(exe,["--run_minimized"]'));
  assert(!renderer.includes("AudioContext"));
  assert(!renderer.includes("getUserMedia"));
});
test("audio restoration uses original virtual FxSound device and native capture/processing pipeline",()=>{
  const ctrl=read("fxsound/Source/GUI/FxController.cpp");
  const wrapper=read("audiopassthru/src/AudioPassthru/AudioPassthru.cpp");
  const native=read("audiopassthru/src/AudioPassthru/AudioPassthruPrivate.cpp");
  const rules=read("audiopassthru/src/sndDevices/sndDevicesImplementDeviceRules.cpp");
  assert(ctrl.includes('if (arg_list.containsOption("--restore_audio"))'));
  assert(ctrl.includes("audio_passthru_->restoreCynRouting()"));
  assert(ctrl.includes("setPowerState(true);"));
  assert(ctrl.includes("setOutput(String(physical.pwszID.c_str()),false)"));
  assert(ctrl.includes('engine->setProperty("virtual_device_default",virtualDefault)'));
  assert(ctrl.includes('engine->setProperty("route_enabled",virtualActive && virtualDefault'));
  assert(ctrl.includes('engine->setProperty("processing",virtualActive && virtualDefault'));
  assert(ctrl.includes("audio_passthru_->setDspProcessingModule(&dfx_dsp_)"));
  assert(wrapper.includes("data_->restoreCynRouting()"));
  assert(native.includes("sndDevicesSetDefaultDeviceSelectionMode("));
  assert(native.includes("SND_DEVICES_AUTO_SELECT_DEFAULT_DEVICE_ON"));
  assert(native.includes("killProcessingThread(&timedOut)"));
  assert(native.includes("s_sndDevices_.playbackDeviceIsUnavailable = FALSE"));
  assert(native.includes("p_dfx_dsp_->processAudio"));
  assert(rules.includes("sndDevicesSetDeviceType(hp_sndDevices, SND_DEVICES_DEFAULT, cast_handle->pwszID[cast_handle->captureDeviceNum]"));
});
