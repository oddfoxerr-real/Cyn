// Native Cyn AI Hub catalogue and offline model-format regression checks.
// No webpage or hosted server is required.
import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const path = p => new URL("../" + p, import.meta.url);
const catalogue=JSON.parse(readFileSync(path("ai-hub/community/index.json"),"utf8"));
assert.equal(catalogue.schema,1);
assert(Array.isArray(catalogue.profiles)&&catalogue.profiles.length>0);
for(const entry of catalogue.profiles){
  assert.match(entry.file,/^[a-z0-9-]+\.cynai\.json$/);
  const profile=JSON.parse(readFileSync(path("ai-hub/community/"+entry.file),"utf8"));
  assert.equal(profile.format,"cyn-ai-profile");
  assert.equal(profile.schema,1);
  assert.equal(profile.model.schema,1);
  assert.equal(profile.model.architecture,"cyn-audio-11x12x5");
  assert.equal(profile.model.weights.length,209);
  assert(profile.model.weights.every(w=>typeof w==="number"&&Number.isFinite(w)&&Math.abs(w)<=100));
  assert.equal(profile.model.ready.length,5);
  assert(profile.model.ready.every(b=>typeof b==="boolean"));
  assert(!("examples" in profile)&&!("examples" in profile.model),"Never ship private audio labels");
}
const native=readFileSync(path("fxsound/Source/GUI/CynAiHubDialog.h"),"utf8");
assert(native.includes("juce::DialogWindow::LaunchOptions"),"Hub must run inside Cyn as native UI");
assert(native.includes("CynAiProfiles/index.json"),"Missing bundled offline catalogue");
assert(native.includes("refreshCommunity"),"Missing optional GitHub refresh");
assert(native.includes("importSmartProfile"),"Missing native model import");
const view=readFileSync(path("fxsound/Source/GUI/FxProView.cpp"),"utf8");
assert(view.includes("browse_ai_.onClick = [] { CynAiHubDialog::show(); };"),"AI Hub button must open native dialog");
assert(!view.includes('URL("https://github.com/oddfoxerr-real/Cyn/tree/main/ai-hub").launchInDefaultBrowser()'),"Website link still exists");
const build=readFileSync(path("BUILD_CYN.ps1"),"utf8");
assert(build.includes("CynAiProfiles"),"Installer must bundle the offline community catalogue");
console.log("PASS: "+catalogue.profiles.length+" native AI profiles, offline catalogue, no AI website");
