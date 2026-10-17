// Source/contract checks for CYN's actual EQ and input-free DJ hand.
// The Windows CI job additionally compiles the full native app.
import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const read=path=>readFileSync(new URL("../"+path,import.meta.url),"utf8");
const controllerHeader=read("fxsound/Source/GUI/FxController.h");
const controller=read("fxsound/Source/GUI/FxController.cpp");
const uiHeader=read("fxsound/Source/GUI/FxAudioControls.h");
const ui=read("fxsound/Source/GUI/FxAudioControls.cpp");
const sos=read("dsp/ptutil/include/sos.h");
const init=read("dsp/ptutil/DspUtil/GraphicEq/GraphicEqInit.cpp");
const resize=read("dsp/ptutil/DspUtil/GraphicEq/GraphicEqSet.cpp");
const restore=read("dsp/DfxDspEq.cpp");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const touch=read("fxsound/Source/GUI/CynDjTouch.h");
const view=read("fxsound/Source/GUI/FxProView.cpp");
const viewHeader=read("fxsound/Source/GUI/FxProView.h");

assert(controllerHeader.includes("DEFAULT_NUM_EQ_BANDS = 31;"),"31-band default required");
assert(uiHeader.includes("equalizer_bands_ = { 31, 35, 39 };"),"EQ menu must have only 31/35/39");
assert(["case 31:","case 35:","case 39:"].every(x=>ui.includes(x)),
  "All three band count choices must display");
assert(!controller.includes("nb != 5 && nb != 10"),"Old low-band default must migrate");
assert(controller.includes("if (num_bands != 31 && num_bands != 35 && num_bands != 39) return;"),
  "Never silently select unsupported EQ layout");
assert(sos.includes("SOS_MAX_NUM_SOS_SECTIONS 40"),"Real SOS audio backend MUST have capacity for 39");
assert(init.includes("NULL, GRAPHIC_EQ_MAX_NUM_BANDS"),
  "Allocate all 40 section slots before playback, not during active UI switching");
assert(init.includes("sosSetNumActiveSections(cast_handle->sos_hdl, i_num_bands)"),
  "Default EQ MUST initially activate only the selected 31/35/39 sections");
assert(resize.includes("num_bands > GRAPHIC_EQ_MAX_NUM_BANDS") &&
       resize.includes("sosSetNumActiveSections(cast_handle->sos_hdl, num_bands)"),
  "Switching to 39 requires REAL extra sections; not just 39 rendered controls");
assert(restore.includes("r_boost_cut_original[SOS_MAX_NUM_SOS_SECTIONS+1]") &&
       restore.includes("r_boost_cut_interpolated[SOS_MAX_NUM_SOS_SECTIONS+1]"),
  "Preset conversion must never overflow with 39-band input");
assert(controller.includes('settings_.setInt("num_bands", dfx_dsp_.getNumEqBands())'),
  "Persist actual DSP value, not a failed request");

for(const x of ["None=0","Cursor=1","DjHand=2","GhostHand=3","Flowers=4","CustomPng=5"])
  assert(touch.includes(x),"Missing AI Touch option: "+x);
assert(studio.includes('setInt("cyn_dj_touch_style",touchStyle_)') &&
       studio.includes('touchPicker_.onChange=') &&
       studio.includes('chooseCustomTouch()'),
       "Theme Studio must offer separately saved AI touch styles");
assert(studio.includes("file.getSize()>2*1024*1024") &&
       studio.includes("image.getWidth()>1024") &&
       studio.includes('customChoice_.copyFileTo(target)'),
       "Custom PNG must be size-bounded and copied to a private local asset");
assert(view.includes('controller.getSettings().getInt("cyn_dj_touch_style",0)') &&
       view.includes('CynDjTouch::paint(g,dj_touch_style_'),
       "Selected touch style must actually render over real CYN controls");
assert(view.includes("controller.getAppliedEffectValue(") &&
       view.includes("controller.getAppliedEqBandBoostCut(") &&
       view.includes("std::abs(value-preset)>0.08f") &&
       view.includes("std::abs(value-preset)>0.09f"),
       "DJ Touch must follow measured AI DSP changes, not meaningless looping animation");
assert(view.includes("if(chosen==CynDjTouch::None || FxTheme::isReducedMotion()") &&
       view.includes("dj_touch_seeded_=false;"),
       "Touch off and reduced motion must not animate");
assert(!touch.includes("std::thread") && !touch.includes("dfx_dsp_.set"),
       "DJ touch is visual only. Preserve APEX and the native DSP.");
assert(viewHeader.includes("updateDjTouch(double now)"));
console.log("PASS: full DSP 31/35/39 EQ, safe remapping, AI DJ touch selector and truthful native animation");
