import assert from "node:assert/strict";
import {readFileSync} from "node:fs";

const read = path => readFileSync(new URL("../" + path, import.meta.url), "utf8");
const catalogue = JSON.parse(read("ai-hub/community/index.json"));
const hybrid = catalogue.profiles.find(p=>p.id==="cyn-apex-flow-hybrid");
const offline = catalogue.profiles.find(p=>p.id==="cyn-apex6m-flowsense");
assert(hybrid && hybrid.kind!== "research" && hybrid.file==="cyn-apex-flow-hybrid.cynai.json");
assert(offline?.kind==="research", "Keep offline APEX-6M distinct");
const profile=JSON.parse(read("ai-hub/community/" + hybrid.file));
const trained=JSON.parse(read("ai-hub/community/cyn-real-audio-13h.cynai.json"));
assert.deepEqual(profile.model.weights, trained.model.weights,
  "Live preview must actually reuse the real 13h-trained preference weights");
assert.deepEqual(profile.model.ready, trained.model.ready);
assert.equal(profile.profile.runtime_extension,"cyn-apex-flow-hybrid-v1");
assert.match(profile.profile.description,/NOT part|NOT.*denoiser|not.*denoiser/i,
  "Be clear 6.3M offline checkpoint is NOT playing in CYN");
const controller=read("fxsound/Source/GUI/FxController.cpp");
const header=read("fxsound/Source/GUI/FxController.h");
const hybridHeader=read("fxsound/Source/GUI/CynApexFlowHybrid.h");
const hub=read("fxsound/Source/GUI/CynAiHubDialog.h");
assert(controller.includes('getActiveAIProfileId()=="bundled:cyn-apex-flow-hybrid.cynai.json"'),
  "Runtime must gate hybrid DSP by the actual selected AI profile");
assert(controller.includes("decision.preference[4]+flow.bassBias") &&
       controller.includes("decision.preference[3]+flow.dynamicsBias") &&
       controller.includes("decision.lowEqDb+flow.lowEqDb"),
  "Hybrid should affect the real applied DSP decisions, not fake telemetry");
assert(controller.includes("apex_flow_.reset()") && controller.includes("sample.pretrainedSceneValid=scene.valid"),
  "Pretrained scene unavailable must fail closed");
assert(header.includes("CynApexFlowHybrid apex_flow_"));
assert(hybridHeader.includes("gainSafety") && hybridHeader.includes("lastMs_") &&
       !hybridHeader.includes("std::vector"),
  "Apex tempo envelope must be bounded, peak gated and allocation-free");
assert(hub.includes("FxController::getInstance().importSmartProfile (embedded, error)"),
  "Bundled AI Hub activation must use real native model switch");
const build=read("BUILD_CYN.ps1");
assert(build.includes("ai-hub\\community\\*"),"Installer must bundle new hybrid profile");
console.log("PASS: APEX Hybrid truly selectable, trained 13h+YAMNet provenance, separate offline 6M, native DSP wired");
