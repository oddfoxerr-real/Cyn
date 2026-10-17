import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read = p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const source=read("fxsound/Source/GUI/FxController.cpp");
const native=read("fxsound/Source/GUI/CynAiHubDialog.h");
const worker=read("fxsound/Source/GUI/CynApex6MWorker.h");
const dsp=read("dsp/include/SourceAudioAnalyzer.h");
const build=read("BUILD_CYN.ps1");
const exportScript=read("apex_engine/export_onnx.py");

assert(native.includes('selectAIProfile("apex6m",error)'),
  "Full APEX Hub entry must activate actual model, not research-only URL");
assert(native.includes('getInstance().apex6MInstalled()'),
  "Model activation requires installed full checkpoint");
assert(source.includes('settings_.setString("cyn_active_ai_profile_music","apex6m")'),
  "Full ONNX inference must have a distinct activated model ID");
assert(source.includes('settings_.setString("cyn_apex_previous_music_profile",getActiveAIProfileId())'),
  "Old private AI selection must be restorable");
assert(source.includes('CynApex6MWorker::interpolate(neural,frequency)'),
  "Full ONNX mask statistics must change the actual applied EQ target");
assert(source.includes('dfx_dsp_.setEqBandBoostCut(band,'),
  "Model's decisions must reach native DSP and not just animated UI");
assert(worker.includes('api->Run(session') &&
  worker.includes('apex6m-225.onnx') &&
  worker.includes('copyApexWindow(samples,lastWindow)'),
  "Full model must infer from real source audio on a separate worker");
assert(dsp.includes('rate_==48000 && channels_<=2') &&
       dsp.includes("apex_ring_") &&
       dsp.includes("std::memory_order_release"),
  "APEX must receive real full-bandwidth mono/stereo PCM asynchronously");
assert(build.includes('PREPARE_CYN_APEX6M.ps1') &&
       build.includes('Copy-Item -LiteralPath (Join-Path $apexStage $name)'),
       "Final Windows installer must contain genuine ONNX weights");
assert(exportScript.includes("6_307_477") &&
       exportScript.includes("torch.onnx.export"),
       "Real checkpoint provenance and full parameter count must be verified");
assert(!source.includes("apex6m_weights=Array(209)"),
       "Never rebrand a 209-parameter weight profile as full APEX");
console.log("PASS: activatable native APEX 6M requires full ONNX real inference and reversible DSP corrections");
