"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const os=require("node:os");
const path=require("node:path");
const {chooseNative,bundledNative}=require("../native-bridge.cjs");
const read=p=>fs.readFileSync(path.resolve(__dirname,"../..",p),"utf8");

test("packaged Material always selects its adjacent native FxSound engine over an old Classic/preview path",()=>{
  const root=fs.mkdtempSync(path.join(os.tmpdir(),"cyn-material-native-"));
  try{
    const directory=path.join(root,"Cyn");
    const old=path.join(root,"Legacy");
    fs.mkdirSync(path.join(directory,"CYN-Interface"),{recursive:true});
    fs.mkdirSync(old);
    const ui=path.join(directory,"CYN-Interface","CYN Audio.exe");
    const correct=path.join(directory,"Cyn.exe");
    const stale=path.join(old,"Cyn.exe");
    for(const file of [ui,correct,stale])fs.writeFileSync(file,"test executable");
    assert.equal(bundledNative(ui),correct);
    assert.equal(chooseNative(correct,stale,[stale,correct]),correct);
    fs.rmSync(correct);
    assert.equal(chooseNative(correct,stale,[stale]),stale,
      "A portable/unbundled development preview may still select its manual native host");
  }finally{fs.rmSync(root,{recursive:true,force:true});}
});
test("Material boots/reconnects the original native host without ever launching Classic as its DSP dependency",()=>{
  const bridge=read("riff-cyn/native-bridge.cjs");
  const main=read("riff-cyn/main.cjs");
  const classic=read("fxsound/Source/Main.cpp");
  assert(bridge.includes("return chooseNative(bundledNative(),nativeExe,candidates())"));
  assert(bridge.includes("async function ensureRunning()"));
  assert(bridge.includes("autoRestartPending=launch().catch"));
  assert(bridge.includes("lastAutoRestart<30000"));
  assert(bridge.includes('spawn(exe,["--run_minimized"]'));
  assert(bridge.includes('cwd:path.dirname(exe)'));
  assert(bridge.includes('const state=await ensureRunning()'));
  assert(main.includes('ipcMain.handle("cyn:status",()=>bridge.ensureRunning())'));
  assert(main.includes("bridge.launch().then(result=>"));
  assert(classic.includes("FxController::getInstance().init(main_window_.get(), system_tray_view_.get(), audio_passthru_.get())"));
  assert(!bridge.includes('spawn(exe,["--classic"]'));
});
test("Native power and selected-output settings remain managed only by the original FxSound controller",()=>{
  const bridge=read("riff-cyn/native-bridge.cjs");
  const main=read("riff-cyn/main.cjs");
  const native=read("fxsound/Source/GUI/FxController.cpp");
  assert(native.includes('setPowerState(settings_.getBool("power"))'));
  assert(native.includes('audio_passthru_->setDspProcessingModule(&dfx_dsp_)'));
  assert(native.includes("initOutputs(output_devices_)"));
  assert(main.includes('bridge.launch().then'));
  assert(bridge.includes("commandFor(action,state.status)"));
  assert(!main.includes("AudioContext"));
  assert(!bridge.includes("getUserMedia"));
});
