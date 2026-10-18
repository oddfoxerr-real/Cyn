import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const read = p => readFileSync(p, "utf8");
const controller = read("fxsound/Source/GUI/FxController.cpp");
const controls = read("fxsound/Source/GUI/FxAudioControls.cpp");
const equalizer = read("fxsound/Source/GUI/FxEqualizer.cpp");
const view = read("fxsound/Source/GUI/FxProView.cpp");
const follower = read("fxsound/Source/GUI/CynDspUiFollow.h");
const controllers = read("fxsound/Source/GUI/FxController.h");
const cyclingPolicy=read("fxsound/Source/GUI/CynPresetCycler.h");
const dspPreset=read("dsp/DfxDspPreset.cpp");
assert(cyclingPolicy.includes("const CynAudioScene& scene") &&
       cyclingPolicy.includes("stable_>=7") &&
       cyclingPolicy.includes("lastSwitch_+1800"),"Cycling needs real music analysis and anti-flutter behavior");
assert(dspPreset.includes("DfxDspPrivate::readPresetSnapshot(") &&
       dspPreset.includes("valsGetGraphicEq(") &&
       !dspPreset.slice(dspPreset.indexOf("bool DfxDspPrivate::readPresetSnapshot"),dspPreset.indexOf("int DfxDspPrivate::loadPreset(")).includes("loadPreset("),
       "Preset snapshots must read real saved EQ without reloading live DSP");
assert(controllers.includes("setSmartPresetCycling(bool enabled)") &&
       controller.includes('settings_.setBool("cyn_auto_preset_cycling"') &&
       controller.includes("cycling_policy_.update(") &&
       controller.includes("cycling_effects_[(size_t)e]") &&
       controller.includes("cycling_eq_[(size_t)b]"),
       "Auto cycling must be persisted and control actual DSP effect/EQ baselines");
assert(view.includes("preset_cycling_.onClick=") &&
       view.includes("preset_cycling_.setBounds(left+20,629,208,24)") &&
       view.includes("getSmartCyclingPreset()"),
       "Smart Listening must contain a wired toggle and live preset status");
const headroomPolicy=read("fxsound/Source/GUI/CynSmartRange.h");
assert(headroomPolicy.includes("static float expressiveScore(") &&
       headroomPolicy.includes("static float headroomGate("),
       "Full-range Music AI must support score contrast and adjustable headroom");
assert(controllers.includes("setSmartHeadroomCap(float percent)") &&
       controllers.includes("getSmartHeadroomCap()"),
       "Headroom cap must be a real persisted native setting");
assert(controller.includes('settings_.setDouble("cyn_music_headroom_cap"') &&
       controller.includes("CynSmartRange::headroomGate(") &&
       controller.includes("CynSmartRange::expressiveScore("),
       "Native Music AI DSP must actually use the adjustable cap and score mapping");
assert(view.includes('headroom_cap_.onValueChange=') &&
       view.includes('headroom_cap_.setBounds(left+542,603,259,24)') &&
       view.includes('txt("HEADROOM CAP"') &&
       view.includes("controller.getSmartListeningMode()==0"),
       "Headroom Cap slider must be visible, wired and restricted to Music AI");
const smartRange = read("fxsound/Source/GUI/CynSmartRange.h");
assert(smartRange.includes("static float effectTarget("), "Missing full-range Music AI authority map");
assert(smartRange.includes("static float freedom(") && smartRange.includes("return 0.0f;"),"Movie AI must preserve its original range");
assert(controller.includes("CynSmartRange::effectTarget(") &&
       controller.includes("CynSmartRange::eqAmount(") &&
       controller.includes("CynSmartRange::scoreFollower(") &&
       controller.includes("CynSmartRange::effectStep("),
       "Live DSP actuator must use sensitivity-aware effect and EQ authority");

const body = (source, start, end) => {
    const a = source.indexOf(start), b = source.indexOf(end, a + start.length);
    assert(a >= 0 && b > a, "Missing implementation: " + start);
    return source.slice(a, b);
};

// The stored baseline must remain distinct from the live effect and EQ states.
assert(controllers.includes("getAppliedEffectValue") && controllers.includes("getAppliedEqBandBoostCut"));
const effectsGetter = body(controller, "float FxController::getAppliedEffectValue(", "float FxController::getEffectValue(");
const eqGetter = body(controller, "float FxController::getAppliedEqBandBoostCut(", "float FxController::getEqBandBoostCut(");
assert(effectsGetter.includes("dfx_dsp_.getEffectValue") && !effectsGetter.includes("smart_effects_"));
assert(eqGetter.includes("dfx_dsp_.getEqBandBoostCut") && !eqGetter.includes("smart_eq_"));
assert(body(controller, "void FxController::updateSmartListening()", "String FxController::getAppliedMixSummary()").includes("dfx_dsp_.setEffectValue"));
// UI only reads the actual DSP adjustments that the real AI made; never writes sound/preset.
const effectSync = body(controls, "void FxEffects::syncAppliedDsp(", "void FxEffects::showValues(");
const eqSync = body(equalizer, "void FxEqualizer::syncAppliedDsp(", "void FxEqualizer::update(");
for (const source of [effectSync, eqSync]) {
    assert(source.includes("CynDspUiFollow::towardApplied"));
    assert(source.includes("isMouseButtonDown()"));
    assert(!source.includes("dfx_dsp_.set"));
    assert(!source.includes("controller.setEffectValue"));
    assert(!source.includes("controller.setEqBandBoostCut"));
}
assert(effectSync.includes("getAppliedEffectValue"));
assert(eqSync.includes("getAppliedEqBandBoostCut"));
assert(eqSync.includes("if (visualChanged) repaint()"));
const eqKeys = body(equalizer, "bool FxEqualizer::FxEqSlider::keyPressed(", "void FxEqualizer::FxEqSlider::mouseDown(");
const effectKeys = body(controls, "bool FxEffects::FxEffectSlider::keyPressed(", "FxEqualizerControl::FxEqualizerControl(");
assert(eqKeys.includes("getValue() + 1.0") && eqKeys.includes("getValue() - 1.0"),
       "Manual EQ arrow keys must keep their original 1 dB step");
assert(effectKeys.includes("getValue() + 1.0") && effectKeys.includes("getValue() - 1.0"),
       "Manual effect arrow keys must keep their original integer step");
assert(equalizer.includes("target - gain") && equalizer.includes("std::abs(gain - target)"),
       "Alt-drag must converge correctly from fractional AI values");
assert(controls.includes("->setRange(0, 10, 0.01)"));
assert((equalizer.match(/->setRange\(-MAX_GAIN, MAX_GAIN, 0.01\)/g) || []).length === 2);
assert(follower.includes("std::exp") && !follower.includes("std::sin"));
assert(!follower.includes("audioEnergy") && !follower.includes("wobble"));
const proSync = body(view, "void FxProView::timerCallback()", "void FxProView::resized()");
assert(proSync.includes("page_ == 0") && proSync.includes("audio_controls_.syncAppliedDsp"));
assert(proSync.includes("equalizer_.syncAppliedDsp"));
assert(proSync.includes("FxTheme::isReducedMotion()"));
assert(proSync.includes("getAppliedMixSummary()"));
assert(controller.includes("dfx_dsp_.getVolumeLeveling() - smart_leveling_"));
// Full Sound Studio refreshes must preserve a live AI display transition.
// Only first render, user-controlled AI-off, power-off and reduced motion snap.
const refresh = body(view, "void FxProView::update()", "void FxProView::timerCallback()");
assert(refresh.includes("!controller.isSmartListening()"));
assert(refresh.includes("FxTheme::isReducedMotion()"));
assert(refresh.includes("audio_controls_.update(snapValues)"));
assert(refresh.includes("equalizer_.update(snapValues)"));
assert(controls.includes("void FxEffects::update(bool snapValues)") &&
       controls.includes("if (snapValues && value >= 0.0"));
assert(equalizer.includes("void FxEqualizer::update(bool snapValues)") &&
       equalizer.includes("if (snapValues && value >= -MAX_GAIN"));

// When the DSP deviates from the saved preset, show a real baseline marker.
// No visual overlay may write to the DSP, alter the user's preset, or
// fabricate values from unrelated audio energy/sine waves.
const effectMark = body(controls,
    "void FxEffects::FxEffectSlider::paintOverChildren(Graphics& g)",
    "void FxEffects::FxEffectSlider::resized()");
const eqMark = body(equalizer, "void FxEqualizer::paintOverChildren(Graphics& g)",
    "FxEqualizer::FxEqSlider::FxEqSlider(");
for (const marker of [effectMark, eqMark]) {
    assert(marker.includes("isSmartListening()"));
    assert(marker.includes("getApplied"));
    assert(marker.includes("getEffectValue") || marker.includes("getEqBandBoostCut"));
    assert(marker.includes("std::abs(applied - preset)"));
    assert(!marker.includes("setEffectValue(") && !marker.includes("setEqBandBoostCut("));
    assert(!marker.includes("std::sin(") && !marker.includes("audioEnergy"));
}
assert(eqMark.includes("g.drawEllipse"));
assert(effectMark.includes("g.drawLine"));
console.log("PASS: AI mode/preset refresh preserves real display transitions");
console.log("PASS: baseline markers only appear for real applied-vs-preset differences");

console.log("PASS: applied DSP telemetry drives real-value EQ and effects");
console.log("PASS: manual drag protection and read-only visual animation");
console.log("PASS: UI only glides toward actual changes; no fabricated pulse or wobble");
