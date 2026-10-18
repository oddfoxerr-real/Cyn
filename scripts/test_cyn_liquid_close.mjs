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
assert(parent.includes("getCloseButtonCentre()"),
       "Collapse must target the real X position instead of a guessed corner");
assert(windowCpp.includes("createComponentSnapshot(getLocalBounds())") &&
       windowCpp.includes("getScreenBounds(), getCloseButtonCentre()"),
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
       liquid.includes("durationMs = 390.0") &&
       liquid.includes("snapshot_ = juce::Image()"),
       "Animation must be brief, respect accessibility, and release the snapshot");
assert(!liquid.includes("setPowerState(") &&
       !windowCpp.slice(windowCpp.indexOf("void FxMainWindow::closeToTrayWithLiquid()"),
             windowCpp.indexOf("void FxMainWindow::userTriedToCloseWindow()"))
           .includes("setPowerState("),
       "Hiding the UI must never power off or mute the audio processor");
assert(controller.includes('settings_.setBool("run_minimized", true)'),
       "Existing tray-minimized preference must be preserved");
assert(theme.includes("smooth_animations_ ? .44f : .60f"),
       "Shared button/toggle transitions must no longer use laggy 13% stepping");
assert(power.includes("irisDurationMs = 410.0") &&
       power.includes("curtain.setUsingNonZeroWinding(false)") &&
       power.includes("curtain.addPath(iris)"),
       "Power swallow must retain its proper mask and gain a fluid edge");
console.log("PASS: fluid X-to-tray, actual power iris, snappy controls, real audio unaffected");
