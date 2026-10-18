// Native CYN Button Studio: persistent, independent material and motion choices.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const visuals=read("fxsound/Source/GUI/CynButtonVisuals.h");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const theme=read("fxsound/Source/GUI/FxTheme.cpp");
const header=read("fxsound/Source/GUI/FxTheme.h");
const controller=read("fxsound/Source/GUI/FxController.cpp");
const close=read("fxsound/Source/GUI/CynCloseMotion.h");
const overlay=read("fxsound/Source/GUI/CynLiquidCloseOverlay.h");
for(const name of ["Classic","Quick Pop","Soft Float","Ripple Press","Liquid Morph",
                   "Subtle","Neon Edge","Soft Glow","Liquid Edge","Glass Sweep",
                   "Liquid Gel Press","Flowing Ink","Wet Gloss","Spray Paint",
                   "Elastic Bounce","Viscous Slide","Ink Pulse","Aerosol Mist",
                   "Liquid Chrome","Ink Marble","Electric Droplets"])
    assert(visuals.includes('return "'+name+'"'),"Missing native style: "+name);
for(const prop of ["cyn_button_motion_style","cyn_button_highlight_style",
                   "cyn_button_motion_speed","cyn_button_highlight_intensity"])
    assert(studio.includes('setInt("'+prop+'"') && controller.includes('"'+prop+'"'),
           "Preview must save, and native CYN load, "+prop);
assert(header.includes("setButtonVisualSettings") && theme.includes("FxTheme::setButtonVisualSettings("));
assert(theme.includes("CynButtonVisuals::paint(g,bounds") &&
       studio.includes("CynButtonVisuals::paint(g,getLocalBounds().toFloat()"),
       "Real native buttons and interactive preview must draw via same shared renderer");
assert(studio.includes('buttonTab_("Buttons")') &&
       studio.includes("void showButtonPage()") &&
       studio.includes("paintButtonPage(g);") &&
       studio.includes("ButtonSample buttonDemo_") &&
       studio.includes("buttonMotionPicker_.onChange") &&
       studio.includes("buttonHighlightPicker_.onChange") &&
       studio.includes("buttonSpeedSlider_.onValueChange") &&
       studio.includes("buttonIntensitySlider_.onValueChange"),
       "Studio must have functional Buttons tab, live preview and controls");
assert(visuals.includes("if(reducedMotion || strength<.01f || (over<.01f && pushed<.01f)) return;") &&
       theme.includes("buttonMix : mix);") &&
       theme.includes("button_speed_"),
       "Effects must honor reduced motion and adjust button response without changing DSP");
assert(close.includes("phaseExpandEnd = 0.43f") &&
       close.includes("phaseHoldEnd = 0.51f") &&
       close.includes("baseDurationMs = 700.0") &&
       close.includes("expandEase(") &&
       overlay.includes("CynCloseMotion::expandEase(t / phaseExpandEnd)") &&
       !overlay.includes("easeOutQuint"),
       "The full-window X expansion must be visibly paced before liquid dissolve");
assert(visuals.includes("case SprayPaint:") &&
       visuals.includes("for(int i=0;i<84;++i)") &&
       visuals.includes("g.reduceClipRegion(mask)") &&
       visuals.includes("case WetGloss:"),
       "Spray particles and liquid gloss must render natively inside the button");
assert(!visuals.includes("AudioProcessor") &&
       !studio.includes("setPowerState("),
       "UI styles must not touch audio processing");
console.log("PASS: CYN Button Studio native styles, material highlights, live preview and visible X expansion");
