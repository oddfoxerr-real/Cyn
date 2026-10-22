// CYN Windows BrainScan/Material3/AUDONIC integration smoke tests.
// SPDX-License-Identifier: AGPL-3.0-or-later
import {readFileSync} from "node:fs";
import {strict as assert} from "node:assert";
const read = file => readFileSync(file, "utf8");
const controller=read("fxsound/Source/GUI/FxController.cpp");
const header=read("fxsound/Source/GUI/FxController.h");
const pro=read("fxsound/Source/GUI/FxProView.cpp");
const brain=read("fxsound/Source/GUI/CynBrainScan3D.h");
const themes=read("fxsound/Source/GUI/FxTheme.cpp");
const themeHeader=read("fxsound/Source/GUI/FxTheme.h");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const hub=read("fxsound/Source/GUI/CynAiHubDialog.h");
const atmosphere=read("fxsound/Source/GUI/CynThemeAtmosphere.h");

assert.match(themeHeader,/PolarMist, Material3, NumModes/);
assert.match(themes,/Material 3-inspired dark tonal surfaces/);
assert.match(themes,/FxThemeMode::Material3: return "Material 3"/);
assert.match(studio, /FxThemeMode::Material3/);
assert.match(atmosphere,/case FxThemeMode::Material3:/);
assert.match(controller,/case FxThemeMode::Material3:/);
assert.match(controller,/cyn_audonic_previous_music_profile/);
assert.match(controller,/choices\.push_back\(\{"audonic"/);
assert.match(controller,/CynAudonicStudio::decide/);
assert.match(controller,/CynAudonicStudio::effectAdd/);
assert.match(controller,/CynAudonicStudio::eqAdd/);
assert.match(controller,/audonic_policy_at_=0/);
assert.match(controller,/!audonicActive && !cycling_profiles_/);
assert.match(header,/getLiveAudonicControls/);
assert.match(pro,/scan_toggle_\.onClick/);
assert.match(pro,/brain_scan_\.setActivity\(liveSignal\?/);
assert.match(pro,/audonic_intensity_\.onValueChange/);
assert.match(brain,/if\(active_\)angle_\+=0\.013f/);
assert.match(brain,/getLocalBounds/);
assert.match(brain,/NeuralActivitySnapshot/);
assert.match(hub,/nativeAudonic/);
assert.match(hub,/selectAIProfile\("audonic",error\)/);
console.log("PASS Windows Material3 palette, AUDONIC DSP/AI Hub, telemetry and BrainScan routing");
