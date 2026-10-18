// Close-to-tray visual regression and non-DSP integration contract.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read = p => readFileSync(new URL("../" + p, import.meta.url), "utf8");
const windowCpp = read("fxsound/Source/GUI/FxMainWindow.cpp");
const windowHeader = read("fxsound/Source/GUI/FxMainWindow.h");
const parent = read("fxsound/Source/GUI/FxWindow.h");
const liquid = read("fxsound/Source/GUI/CynLiquidCloseOverlay.h");
const theme = read("fxsound/Source/GUI/FxTheme.cpp");
const power = read("fxsound/Source/GUI/CynPowerCurtain.h");
const controller = read("fxsound/Source/GUI/FxController.cpp");

assert(windowHeader.includes("CynLiquidCloseOverlay liquid_close_overlay_"),
       "Liquid overlay must be owned by the existing native window");
assert(parent.includes("getCloseButtonBounds()"),
       "Collapse must target the real X position instead of a guessed corner");
assert(windowCpp.includes("createComponentSnapshot(getLocalBounds())") &&
       windowCpp.includes("getScreenBounds(), getCloseButtonBounds()"),
       "Use one real application frame and anchor its motion to the X button");
assert(windowCpp.includes("liquid_close_overlay_.begin(") &&
       windowCpp.includes("FxController::getInstance().hideMainWindow()"),
       "Liquid animation must preserve the existing close-to-tray behavior");
assert(windowCpp.includes("liquid_close_overlay_.cancel();") &&
       windowCpp.includes("void FxMainWindow::show()"),
       "Reopening during collapse must cancel the overlay");
assert(windowCpp.includes("void FxMainWindow::userTriedToCloseWindow()") &&
       windowCpp.includes("void FxMainWindow::closeButtonPressed()") &&
       (windowCpp.match(/closeToTrayWithLiquid\(\);/g) || []).length >= 2,
       "Both native close paths must animate");
assert(liquid.includes("windowIsSemiTransparent") &&
       liquid.includes("windowIgnoresMouseClicks") &&
       liquid.includes("windowIgnoresKeyPresses") &&
       liquid.includes("setOpaque(false)"),
       "Close animation should be truly desktop-transparent and click-through");
assert(liquid.includes("FxTheme::isReducedMotion()") &&
       liquid.includes("startTimerHz(60)") &&
       liquid.includes("durationMs = 460.0") &&
       liquid.includes("snapshot_ = juce::Image()"),
       "Animation must be brief, respect accessibility, and release the snapshot");
assert(liquid.includes("phaseExpandEnd = 0.28f") &&
       liquid.includes("phaseHoldEnd = 0.37f") &&
       liquid.includes("easeOutQuint(") &&
       liquid.includes("g.drawImageAt(snapshot_, 0, 0)") &&
       liquid.includes("renderVortexMask(melt)") &&
       liquid.includes("reduceClipRegion(vortexMask_") &&
       liquid.includes("drawVortexHighlights(") &&
       !liquid.includes("ribbonCount") &&
       liquid.indexOf("if (t < phaseExpandEnd)") <
           liquid.indexOf("if (t < phaseHoldEnd)") &&
       liquid.indexOf("if (t < phaseHoldEnd)") <
           liquid.indexOf("renderVortexMask(melt)"),
       "Actual X must expand to fill CYN before the opaque surface vortex-dissolves");
assert(liquid.includes("maskWidth = 192") &&
       liquid.includes("maskHeight = 108") &&
       liquid.includes("Image::BitmapData::writeOnly"),
       "Vortex field must stay low-resolution and CPU-bounded");
assert(!liquid.includes("setPowerState(") &&
       !windowCpp.slice(windowCpp.indexOf("void FxMainWindow::closeToTrayWithLiquid()"),
             windowCpp.indexOf("void FxMainWindow::userTriedToCloseWindow()"))
           .includes("setPowerState("),
       "Hiding the UI must never power off or mute the audio processor");
assert(controller.includes('settings_.setBool("run_minimized", true)'),
       "Existing tray-minimized preference must be preserved");
assert(theme.includes("frameMs/16.666667f") &&
       theme.includes("std::pow(1.0f-rate") &&
       theme.includes("it->value[i]+=delta*mix;"),
       "Shared button/toggle transitions must be responsive and frame-rate independent");
assert(power.includes("irisDurationMs = 410.0") &&
       power.includes("curtain.setUsingNonZeroWinding(false)") &&
       power.includes("curtain.addPath(iris)"),
       "Power swallow must retain its proper mask and gain a fluid edge");
console.log("PASS: fluid X-to-tray, actual power iris, snappy controls, real audio unaffected");
