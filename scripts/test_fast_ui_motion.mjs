import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read = file => readFileSync(new URL("../"+file,import.meta.url),"utf8");
const page=read("fxsound/Source/GUI/FxProView.cpp");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const touch=read("fxsound/Source/GUI/CynDjTouch.h");
const header=read("fxsound/Source/GUI/FxProView.h");
const controller=read("fxsound/Source/GUI/FxController.cpp");
assert(page.includes("createComponentSnapshot(content)"),
  "Transition must capture only content, not a full-window bitmap");
assert(!page.includes("transition_image_=createComponentSnapshot(getLocalBounds())"),
  "A whole-window navigation overlay would cover the active sidebar");
assert(page.includes("if(page == page_ && isShowing()) return;"),
  "Repeated clicks must not restart animation; initial page still initializes");
assert(page.includes("page_ = page;") &&
       page.includes("misc_nav_.setToggleState(page==4") &&
       page.includes("ai_hub_nav_.setToggleState(page==3"),
  "The selected AI Hub/Misc pages and native navigation controls must update immediately");
assert(page.includes("g.reduceClipRegion(content);") &&
       page.includes("CynUiMotion::outgoingOpacity(t)"),
  "Old-page fade should never obscure the live navigation controls");
assert(page.includes("CynUiMotion::pageDurationMs") &&
       studio.includes("CynCloseMotion::expandEase(") &&
       studio.includes("CynButtonVisuals::paint("),
  "Page navigation must retain tested easing; Dark Studio must preview real close and button animations without a removed carousel");
assert(page.includes("FxTheme::isReducedMotion()"),
  "Reduced motion should bypass expensive effects");
assert(studio.includes("effectsTouchPicker_") && studio.includes("eqTouchPicker_") &&
       studio.includes("flowerCount_"),
  "Preserve independent two-hand and flowers-per-panel studio selections");
assert(header.includes("std::array<DjTouchAgent,8>") &&
       page.includes("void FxProView::updateDjTouch(double now)") &&
       touch.includes("enum Style"),
  "Never regress live audio-driven two-panel overlays");
assert(controller.includes("nb != 31 && nb != 35 && nb != 39"),
  "31, 35 and 39 real DSP EQ bands must remain supported");
console.log("PASS: fast non-blocking CYN navigation, theme preview, AI hands, flowers and EQ");
