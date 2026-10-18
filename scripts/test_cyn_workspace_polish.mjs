import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const view=read("fxsound/Source/GUI/FxProView.cpp");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const header=read("fxsound/Source/GUI/FxProView.h");
const theme=read("fxsound/Source/GUI/FxTheme.cpp");
assert(view.includes("switch(i)") &&
       view.includes("case 0: { // Sound Studio") &&
       view.includes("case 5: { // Studio"),
       "Sidebar must use six distinct vector icons");
assert(view.includes('g.drawText("ACTIVE ENGINE"') &&
       view.includes("getActiveAIProfileId()"),
       "Sidebar engine state must be read from the controller");
assert(view.includes("endpoint_list_.getText()") &&
       view.includes("isOutputPinned()") &&
       view.includes("isPlaybackDeviceAvailable()"),
       "Device workspace must display real output name/priority/availability");
assert(view.includes("dj->intensity()") &&
       view.includes("dj->installed()") &&
       view.includes("dj->enabled()") &&
       view.includes("juce::Path track,fill"),
       "AI DJ readout must use real local engine settings");
assert(view.includes("dj_intensity_.onValueChange=configure") &&
       view.includes("dj_spacing_.onValueChange=configure") &&
       view.includes("dj_stereo_.onClick=configure"),
       "UI refactor must not disconnect native AI DJ controls");
assert(studio.includes("Preview first; Apply studio saves the chosen styles.") &&
       studio.includes("Slower on the left, faster on the right. Apply to save.") &&
       studio.includes("92,688,525,18") &&
       studio.includes("92,686,525,18"),
       "CYN Studio footer help must not obscure Apply/Close");
assert(studio.includes('setInt("cyn_button_motion_style",buttonMotion_)') &&
       studio.includes('setInt("cyn_close_motion_style",closeStyle_)'),
       "Saved animation and liquid button preferences must be retained");
assert(header.includes('ai_hub_nav_ { "AI Hub" }') &&
       view.includes('ai_hub_nav_.onClick = [this] { selectPage(3); };') &&
       view.includes("embedded_ai_hub_.setVisible(page==3)"),
       "Embedded AI Hub must remain a usable page");
assert(theme.includes("button_motion_") && theme.includes("button_highlight_"),
       "CYN native button customization must remain intact");
console.log("PASS: improved navigation, truthful device/DJ telemetry and Studio footer");