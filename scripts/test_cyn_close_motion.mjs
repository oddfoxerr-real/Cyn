// Native Theme Studio close-animation preferences and runtime integration.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read = path => readFileSync(new URL("../"+path, import.meta.url),"utf8");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const motion=read("fxsound/Source/GUI/CynCloseMotion.h");
const overlay=read("fxsound/Source/GUI/CynLiquidCloseOverlay.h");
const window=read("fxsound/Source/GUI/FxMainWindow.cpp");

for (const label of ["Vortex","Gravity Melt","Liquid Ripple","Soft Fade","None (instant)","Stardust","Liquid Curtain","Liquid Ink","Spray Dissolve"])
    assert(motion.includes('return "'+label+'"'),"Missing native close effect: "+label);
assert(motion.includes("defaultSpeedPercent = 85") &&
       motion.includes("minSpeedPercent = 55") &&
       motion.includes("maxSpeedPercent = 160") &&
       motion.includes("baseDurationMs = 700.0") &&
       motion.includes("phaseExpandEnd = 0.43f") &&
       motion.includes("phaseHoldEnd = 0.51f") &&
       motion.includes("expandEase(") &&
       motion.includes("baseDurationMs*100.0/(double)clampSpeed(speedPercent)"),
       "Duration formula must make the default slightly slower, and clamp speed to 0.55–1.60x");
assert(motion.includes("inline float coverage(") &&
       motion.includes("if(style==GravityMelt)") &&
       motion.includes("if(style==LiquidRipple)") &&
       motion.includes("if(style==SoftFade)") &&
       motion.includes("if(style==StarDust)") &&
       motion.includes("if(style==Curtain)") &&
       motion.includes("if(style==LiquidInk)") &&
       motion.includes("if(style==SprayDissolve)"),
       "Each style must be backed by a distinct material coverage algorithm");
assert(overlay.includes("CynCloseMotion::coverage(style_,progress") &&
       studio.includes("CynCloseMotion::coverage(closeStyle_,melt"),
       "Preview and real overlay must share the same style computation");
assert(overlay.includes("CynCloseMotion::durationForSpeed(speedPercent)") &&
       overlay.includes("elapsed / durationMs_") &&
       overlay.includes("startedAt_ >= durationMs_"),
       "Speed must affect the entire close animation including completion/cancellation");
assert(overlay.includes("CynCloseMotion::None)") &&
       overlay.includes("FxTheme::isReducedMotion()"),
       "None and reduced motion must suppress overlay creation");
assert(window.includes('"cyn_close_motion_style"') &&
       window.includes('"cyn_close_motion_speed_pct"') &&
       window.includes("FxController::getInstance().hideMainWindow()"),
       "Closing must use saved preferences without disabling audio");
assert(studio.includes('motionTab_("Animations")') &&
       studio.includes('appearanceTab_("Appearance")') &&
       studio.includes("void showMotionPage(bool show)") &&
       studio.includes("paintMotionPage(g);") &&
       studio.includes("replayMotionPreview();"),
       "Styles/speed need a visible native Theme Studio tab and playable preview");
assert(studio.includes("closeStylePicker_.onChange") &&
       studio.includes("closeSpeedSlider_.onValueChange") &&
       studio.includes("closeSpeedSlider_.setRange(.55,1.60,.05)"),
       "Users must be able to change style and speed");
assert(studio.includes('setInt("cyn_close_motion_style",closeStyle_)') &&
       studio.includes('setInt("cyn_close_motion_speed_pct",closeSpeedPercent_)') &&
       studio.includes('cancel_.onClick = [this] { close(); };'),
       "Apply must save both preferences; Close must not save uncommitted edits");
assert(!overlay.includes("setPowerState(") &&
       !studio.includes('settings.setPowerState('),
       "Motion preferences must not change playback");
console.log("PASS: Theme Studio close styles, saved speed, preview, bounds and no-DSP runtime");
