// APEX availability is not the same as a loaded ONNX session.
// Prevent false success on slow / older Windows PCs and preserve DSP safety.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const worker=read("fxsound/Source/GUI/CynApex6MWorker.h");
const ctrl=read("fxsound/Source/GUI/FxController.cpp");
const ctrlH=read("fxsound/Source/GUI/FxController.h");
const hub=read("fxsound/Source/GUI/CynAiHubDialog.h");
assert(worker.includes("juce::String diagnostic() const") &&
       worker.includes("std::atomic<int> loadState_") &&
       worker.includes("juce::CriticalSection diagnosticLock_") &&
       worker.includes("recordFailure("),
       "ONNX load failures must have a thread-safe readable diagnostic");
assert(worker.includes("GetErrorMessage(status)") &&
       worker.includes("GetLastError()") &&
       worker.includes("Microsoft Visual C++ x64 runtime") &&
       worker.includes("API version is incompatible") &&
       worker.includes("APEX model output shape") &&
       worker.includes("ONNX Runtime DLL is missing"),
       "Expose meaningful file, dependency, API, shape and inference errors");
assert(worker.includes("if(!installed() || isThreadRunning()) return;") &&
       worker.includes("loadState_.store(1") &&
       worker.includes("startThread(2)") &&
       worker.includes("loadState_.store(2") &&
       worker.includes("loadState_.store(3"),
       "Worker must support a safe on-demand retry without parallel ONNX threads");
assert(ctrl.includes("startApex6MIfAvailable();") &&
       ctrl.includes("error=apex6MDiagnostic()") &&
       ctrl.includes('settings_.setString("cyn_active_ai_profile_music","apex6m")'),
       "Activation must not succeed before ONNX readiness or erase prior learned model");
assert(ctrlH.includes("apex6MFailed()") && ctrlH.includes("apex6MDiagnostic()") &&
       ctrlH.includes("startApex6MIfAvailable()"),
       "UI should access exact failure reason and retry API");
assert(hub.includes("startTimer(400)") && hub.includes("pendingApexActivation_") &&
       hub.includes("ctrl.apex6MReady()") &&
       hub.includes('ctrl.selectAIProfile ("apex6m", error)') &&
       hub.includes("activateReadyApex()") &&
       hub.includes("Retry APEX engine") &&
       hub.includes("APEX could not start:") &&
       hub.includes("pendingApexActivation_=false;"),
       "Slow PCs must finish user-approved activation only after actual load, or show real failure");
assert(!worker.includes("dfx_dsp_.setEqBandBoostCut(") &&
       !worker.includes("setPowerState("),
       "Retry and diagnostics must not modify live audio");
console.log("PASS: APEX x64 load failure diagnostics, single-worker retry, deferred activation");
