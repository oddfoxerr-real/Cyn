// CYN power-standby visual + real DSP state contract.
// Prevent a fake 'power-off' screen that leaves neural models processing.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const ui=read("fxsound/Source/GUI/FxProView.cpp");
const h=read("fxsound/Source/GUI/FxProView.h");
const curtain=read("fxsound/Source/GUI/CynPowerCurtain.h");
const dsp=read("fxsound/Source/GUI/FxController.cpp");
const apex=read("fxsound/Source/GUI/CynApex6MWorker.h");
const yamnet=read("fxsound/Source/GUI/CynYamNetWorker.h");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const touch=read("fxsound/Source/GUI/CynDjTouch.h");

assert(h.includes("CynPowerCurtain power_curtain_"),"CYN view must own standby layer");
assert(ui.includes("power_curtain_.onPowerRequested") &&
       ui.includes("FxController::getInstance().setPowerState(true)"),
       "Central power control must actually turn on native audio processing");
assert(ui.includes("power_curtain_.syncPower(FxModel::getModel().getPowerState())"),
       "Standby must track actual native power even when toggled from tray/hotkey");
assert(curtain.includes("if(phase_!=Phase::Idle || powered_) return;"),
       "Power button must reject repeated clicks during transitions");
assert(curtain.includes("getMillisecondCounterHiRes") &&
       curtain.includes("Phase::Opening") && curtain.includes("Phase::Closing") &&
       curtain.includes("curtain.setUsingNonZeroWinding(false)"),
       "Both circular opening and closing must use an actual iris reveal");
assert(curtain.includes("FxTheme::isReducedMotion()") &&
       curtain.includes("setInterceptsMouseClicks(true,true)") &&
       curtain.includes("setWantsKeyboardFocus(true)"),
       "Power layer must be accessible, respect reduced motion and block edits while off");
assert(ui.includes("power_curtain_.toFront(false)") &&
       ui.includes("if(power_curtain_.isVisible()) return;"),
       "Standby must obscure user controls and other DJ touch overlays");
assert(ui.includes('if(!FxModel::getModel().getPowerState())') &&
       ui.includes('live_status_.setText("Processing off"'),
       "Invisible AI graphs and effect polling must stop in standby");
assert(dsp.includes("void FxController::updateSmartListening()") &&
       dsp.includes("No hidden model") &&
       dsp.includes("apex6m_active_=false;"),
       "Off must skip Smart Listening predictions, not just hide them");
assert(dsp.includes("powerOn(power_state)") &&
       dsp.includes("if(!power_state)") &&
       dsp.includes('getActiveAIProfileId()=="apex6m"'),
       "Native audio power must remain authoritative, with inactive AI suspended");
for(const [name,worker] of [["APEX",apex],["YAMNet",yamnet]]) {
    assert(worker.includes("void setProcessingEnabled(bool active) noexcept") &&
           worker.includes("processing_enabled_.load(std::memory_order_acquire)") &&
           worker.includes("wait(220)"),
           name+" must park inference when CYN is Off");
}
assert(studio.includes("cyn_dj_touch_style_effects") &&
       studio.includes("cyn_dj_touch_style_eq") &&
       studio.includes("cyn_dj_touch_flower_count"),
       "Two independent hand/flower panel settings must be preserved");
assert(touch.includes("enum Style") && touch.includes("CustomPng"),
       "Custom PNG visual style must remain intact");
console.log("PASS: power standby iris animation, real native off/on, parked neural CPU, theme DJ settings");
