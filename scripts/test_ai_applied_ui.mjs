import assert from "node:assert/strict";
import { readFileSync } from "node:fs";

const read = p => readFileSync(p, "utf8");
const controller = read("fxsound/Source/GUI/FxController.cpp");
const controls = read("fxsound/Source/GUI/FxAudioControls.cpp");
const equalizer = read("fxsound/Source/GUI/FxEqualizer.cpp");
const view = read("fxsound/Source/GUI/FxProView.cpp");
const follower = read("fxsound/Source/GUI/CynDspUiFollow.h");
const controllers = read("fxsound/Source/GUI/FxController.h");

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
console.log("PASS: applied DSP telemetry drives real-value EQ and effects");
console.log("PASS: manual drag protection and read-only visual animation");
console.log("PASS: UI only glides toward actual changes; no fabricated pulse or wobble");
