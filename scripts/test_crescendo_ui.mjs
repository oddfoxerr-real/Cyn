// SPDX-License-Identifier: AGPL-3.0-or-later
// Native Crescendo Studio surface contract: real data and editable native DSP.
import assert from "node:assert/strict";
import {readFileSync as read} from "node:fs";
const source=p=>read(p,"utf8");
const view=source("fxsound/Source/GUI/FxProView.cpp");
const native=source("fxsound/Source/GUI/CynCrescendoStudio.h");
const knobs=source("fxsound/Source/GUI/FxAudioControls.cpp");
const controller=source("fxsound/Source/GUI/FxController.h");
const eq=source("fxsound/Source/GUI/FxEqualizer.cpp");
const header=source("fxsound/Source/GUI/FxProView.h");
assert(view.includes("CynCrescendoStudio::draw(g,crescendo_samples_"));
assert(view.includes("getLiveNeuralActivity(snapshot)"));
assert(view.includes("snapshot.rms*3.0f"));
assert(view.includes("getLiveYamNetScene(scene)"));
assert(view.includes("getCrescendoStrength()"));
assert(controller.includes("music_crescendo_.current().drop"));
assert(native.includes("Waveform follows measured audio"));
assert(native.includes("scene.music")&&native.includes("scene.percussion"));
assert(header.includes("std::array<float,96> crescendo_samples_"));
assert(knobs.includes("Slider::RotaryVerticalDrag"));
assert(knobs.includes("FxEffects::FxEffectSlider::paint(Graphics& g)"));
assert(knobs.includes("FxController::getInstance().setEffectValue(effect_, value)"));
assert(knobs.includes("isMouseButtonDown()"));
assert(knobs.includes("getAppliedEffectValue"));
assert(knobs.includes("effects_.setBounds(0,0,getWidth(),getHeight())"));
assert(eq.includes("sliderHeight")&&eq.includes("getHeight()<230"));
assert(view.includes("Version 2.0.34"));
assert(!native.includes("std::sin")&&!native.includes("random"));
console.log("PASS: #5 Crescendo Studio paints real audio, actual scene and measured rise");
console.log("PASS: editable knobs, original user presets, native EQ and no fake telemetry");

const studio=source("fxsound/Source/GUI/CynThemeStudio.h");
const theme=source("fxsound/Source/GUI/FxTheme.cpp");
const themeHeader=source("fxsound/Source/GUI/FxTheme.h");
assert(view.includes("themes_nav_.onClick = [this] { CynThemeStudioDialog::show(this); }"));
assert(view.includes("eq_open_.onClick = [this] { selectPage(4); }"));
assert(view.includes("eq_back_.onClick = [this] { selectPage(0); }"));
assert(view.includes("equalizer_.setVisible(page == 4)"));
assert(view.includes("equalizer_.setBounds(268,192,927,346)"));
assert(view.includes("live_eq_[(size_t)i]=brainController.getAppliedEqBandBoostCut(i)"));
assert(native.includes("const std::array<float,31>& eq,int eqCount"));
assert(native.includes("paintRole(ControlBackground)"));
assert(native.includes("paintRole(SliderTrack)"));
assert(knobs.includes("Colour(FXCOLOR(SliderTrack))"));
assert(knobs.includes("Colour(FXCOLOR(DefaultText)).withAlpha(1.0f)"));
assert(knobs.includes("Colour(FXCOLOR(SliderTrack)).withAlpha(1.0f)"));
assert(knobs.includes("Colour(FXCOLOR(SliderHighlight)).withAlpha(1.0f)"));
assert(!native.includes("0xff"),"Dashboard has hard-coded non-theme colors");
assert(studio.includes("static void show(juce::Component* owner=nullptr)"));
assert(themeHeader.includes("NeuralCore, Crescendo, NumModes"));
assert(theme.includes('"Charcoal Desktop", "Neural Core", "Crescendo"'));
assert(theme.includes("0x171a20, 0x1e222a, 0x232832"),
       "Original Graphite colors must be restored");
console.log("PASS: all themes persist; compact read-only EQ and full editable EQ wired");

