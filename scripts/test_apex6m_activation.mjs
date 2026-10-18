import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read = p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const source=read("fxsound/Source/GUI/FxController.cpp");
const native=read("fxsound/Source/GUI/CynAiHubDialog.h");
const worker=read("fxsound/Source/GUI/CynApex6MWorker.h");
const dsp=read("dsp/include/SourceAudioAnalyzer.h");
const build=read("BUILD_CYN.ps1");
const exportScript=read("apex_engine/export_onnx.py");

assert(native.includes('selectAIProfile ("apex6m", error)'),
  "Full APEX Hub entry must activate actual model, not research-only URL");
assert(native.includes('ctrl.apex6MInstalled()'),
  "Model activation requires installed full checkpoint");
assert(source.includes('settings_.setString("cyn_active_ai_profile_music","apex6m")'),
  "Full ONNX inference must have a distinct activated model ID");
assert(source.includes('settings_.setString("cyn_apex_previous_music_profile",getActiveAIProfileId())'),
  "Old private AI selection must be restorable");
assert(source.includes('CynApex6MWorker::interpolate(neuralBands,freq)'),
  "Full ONNX mask statistics must change the actual applied EQ target");
assert(source.includes('dfx_dsp_.setEqBandBoostCut(b,'),
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
assert(source.includes("const bool apexFlowActive=apex_hybrid_active_ || apex6m_active_") &&
       source.includes("CynFlagshipMixer::musicEqCorrection(") &&
       source.includes("CynApex6MWorker::interpolate(neuralBands,freq)"),
       "Native APEX must retain the existing live music, bass, and passage control engine");
const diagnostic = read("DIAGNOSE_CYN_APEX.ps1");
assert(worker.includes("reportFailure(") &&
       worker.includes("ONNX Runtime: ") &&
       worker.includes("error 126") &&
       worker.includes("startIfAvailable(bool explicitRetry=false)") &&
       worker.includes("diagnostic() const"),
       "Cross-PC APEX must report actual ONNX errors and allow a manual retry");
assert(native.includes("apexActivationPending_") &&
       native.includes("void timerCallback() override") &&
       native.includes("ctrl.retryApex6M()") &&
       native.includes("apex6MDiagnostic()"),
       "AI Hub must wait for slow ONNX startup, then report success or a clear failure");
assert(source.includes("error=apex6MDiagnostic()") &&
       !source.includes("Full APEX model is missing or still loading; try again shortly."),
       "Selecting APEX must not confuse missing weights, slow startup and DLL failure");
assert(build.includes("DIAGNOSE_CYN_APEX.ps1") &&
       diagnostic.includes("Get-FileHash") &&
       diagnostic.includes("LoadLibraryEx") &&
       diagnostic.includes("vc_redist.x64.exe"),
       "The installed application must include a local, read-only diagnostic tool");
assert(source.includes("restorePreviousApexMusicAI(") &&
       native.includes("Switch to previous AI"),
       "Native APEX must be deactivatable without losing private learned model");
assert(!source.includes("apex6m_weights=Array(209)"),
       "Never rebrand a 209-parameter weight profile as full APEX");
console.log("PASS: activatable native APEX 6M requires full ONNX real inference and reversible DSP corrections");
