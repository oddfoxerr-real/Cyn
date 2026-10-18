import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const source=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const fx=source("fxsound/Source/GUI/FxProView.cpp");
const header=source("fxsound/Source/GUI/FxProView.h");
const liquid=source("fxsound/Source/GUI/CynLiquidScreen.h");
const motion=source("fxsound/Source/GUI/CynUiMotion.h");
assert(header.includes("Image transition_image_, transition_to_image_;"),
       "Separate old/new page snapshots must be retained");
assert(fx.includes("transition_to_image_=createComponentSnapshot(content)") &&
       fx.includes("theme_to_image_=createComponentSnapshot(getLocalBounds())"),
       "Capture destination screen after actual page/theme update");
assert(fx.includes("CynLiquidScreen::render(g,transition_image_,transition_to_image_") &&
       fx.includes("CynLiquidScreen::render(g,theme_from_image_,theme_to_image_"),
       "Tabs and themes must share real liquid compositing");
assert(fx.includes("capturing_liquid_=true") &&
       fx.includes("if(!capturing_liquid_ &&"),
       "No recursive snapshot animation while capturing destination screen");
assert(!fx.includes("g.drawImageAt(theme_from_image_,-offset") &&
       !fx.includes("g.reduceClipRegion(remnants)"),
       "Old slide and rectangular strip wipe must be gone");
assert(liquid.includes("reveal.addEllipse(") &&
       liquid.includes("reveal.lineTo(left+front(y),top+y)") &&
       liquid.includes("g.reduceClipRegion(reveal)") &&
       liquid.includes("g.drawImageAt(incoming,bounds.getX(),bounds.getY())"),
       "Actually reveal destination pixels in connected liquid and spray portals");
assert(liquid.includes("rawProgress>=1.0f"),
       "No screenshot compositing may persist at completion");
assert(motion.includes("pageDurationMs = 405.0") &&
       motion.includes("themeLiquidDurationMs = 450.0"),
       "Liquid screen transition needs visible movement time");
assert(fx.includes("!FxTheme::isReducedMotion()"),
       "Keep accessibility reduced-motion bypass");
console.log("PASS: native liquid mask reveals new tab/theme content during switch");
