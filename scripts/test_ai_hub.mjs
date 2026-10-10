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
assert(view.includes("browse_ai_.onClick = [this] { CynAiHubDialog::show(this); };"),"AI Hub button must open native dialog");
assert(!view.includes('URL("https://github.com/oddfoxerr-real/Cyn/tree/main/ai-hub").launchInDefaultBrowser()'),"Website link still exists");
const build=readFileSync(path("BUILD_CYN.ps1"),"utf8");
assert(build.includes("CynAiProfiles"),"Installer must bundle the offline community catalogue");
const header=readFileSync(path("fxsound/Source/GUI/FxProView.h"),"utf8");
assert(header.includes('ai_hub_nav_ { "AI Hub" }'),"Permanent AI Hub navigation button missing");
for(const key of ["ai_hub_nav_.setBounds(68, 198, 152, 36)", "ai_hub_nav_.onClick = [this] { CynAiHubDialog::show(this); };",
                  "export_ai_.setBounds(685, 545, 96, 35)", "import_ai_.setBounds(791, 545, 96, 35)",
                  "browse_ai_.setBounds(897, 545, 106, 35)", "Version 2.0.12"]){
  assert(view.includes(key),"AI Hub discoverability regression: "+key);
}
assert(view.includes("&ai_hub_nav_, &home_"),"AI Hub navigation missing from component registration");
assert(view.includes("ai_hub_nav_.setVisible(true)"),"AI Hub navigation should be visible on every page");
assert(view.includes("button->toFront(false)"),"AI profile actions need explicit z-order");
console.log("PASS: visible native AI sidebar, profile actions and v2.0.11 version marker");
assert(native.includes("setOpaque (true)"),"AI Hub must paint an opaque content surface");
assert(native.includes("componentToCentreAround = owner"),"AI Hub must be positioned over Cyn");
assert(native.includes("dialogBackgroundColour = juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f)"),"Dialog background is transparent");
assert(native.includes("g.fillAll (juce::Colour (FXCOLOR(WindowBackground)).withAlpha (1.0f))"),"AI Hub content is transparent");
assert(!/juce::Colour\s*\(\s*FXCOLOR\([A-Za-z]+\)\s*\)(?!\.withAlpha)/.test(native),"Some AI Hub colors still have transparent alpha");
const theme=readFileSync(path("fxsound/Source/GUI/FxTheme.cpp"),"utf8");
assert(theme.includes('set("cyn_inline_categories", false)'),"Theme menu must use stable JUCE submenus");
assert(theme.includes('set("cyn_popup_fade_ms", animateMenu ? 215 : 0)'),"Theme submenu should retain native fade");
console.log("PASS: opaque native AI dialog centered above main app and stable theme submenu");
console.log("PASS: "+catalogue.profiles.length+" native AI profiles, offline catalogue, no AI website");
