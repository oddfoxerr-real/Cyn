import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const view=readFileSync(new URL("../fxsound/Source/GUI/FxProView.cpp",import.meta.url),"utf8");
const win=readFileSync(new URL("../fxsound/Project/resources.rc",import.meta.url),"utf8");
const juce=readFileSync(new URL("../fxsound/JuceLibraryCode/JuceHeader.h",import.meta.url),"utf8");
const jucer=readFileSync(new URL("../fxsound/FxSound.jucer",import.meta.url),"utf8");
const shell=readFileSync(new URL("../fxsound/Source/GUI/FxWindow.cpp",import.meta.url),"utf8");
const eq=readFileSync(new URL("../fxsound/Source/GUI/FxEqualizer.cpp",import.meta.url),"utf8");
const required=[
    "preset_list_.setBounds(left+14,139,315,36)",
    "endpoint_list_.setBounds(left+350,139,341,36)",
    "ai_status_card_.setBounds(968,139,314,36)",
    'txt("PRESET",262,114,260,18',
    'txt("OUTPUT DEVICE",603,114,260,18',
    'txt("ACTIVE AI",972,114,230,18',
    "// Three quiet glass tiles",
    "// Tabs sit directly on the workbench",
    "overview_tab_.setBounds(left+12,203,126,34)",
    "const float navY=104.0f+49.0f*(float)page_",
    "sound_nav_.setBounds(navX,104,navW,42)",
    "card(247,252,686,205)",
    "card(944,252,356,205)",
    "card(247,468,1053,220)",
    "dj_download_.setBounds(left+24,329,205,38)",
    "dj_intensity_.setBounds(left+209,522,565,31)",
    "dj_spacing_.setBounds(left+209,574,565,31)",
    "embedded_ai_hub_.setBounds(247,250,1053,435)",
    "learning_timeline_.setBounds(left+13,288,722,164)",
    "equalizer_.setBounds(eqX,288,eqW,193)"
];
for(const anchor of required)assert(view.includes(anchor),"UI overlap/layout regression: "+anchor);
assert(view.includes('g.drawText("WORKSPACES"') &&
       !view.includes('g.drawText("CYN",28,22'),
       "Don't show two different CYN title bars");
assert(view.includes('String("AI ACTIVE  /  ")') &&
       !view.includes('String("AI ACTIVE  •  ")'),
       "Avoid corrupting UTF-8 status separators in Windows build");
assert(win.includes("FILEVERSION  2,0,70,0") &&
       juce.includes('versionString  = "2.0.70.0"') &&
       juce.includes('versionNumber  = 0x2004600') &&
       jucer.includes('version="2.0.70.0"'),
       "Installed CYN executable and runtime must report correct version, not 2.0.29");
assert(shell.includes('g.drawRoundedRectangle(rim,radius-0.7f') &&
       shell.includes('g.strokePath(corner,PathStrokeType(1.6f))'),
       "Restore the original rounded window-corner frame and accent.");
assert(eq.includes("const int compactBandHeight=juce::jlimit(65") &&
       eq.includes("const int tunedBandHeight=juce::jlimit(55") &&
       eq.includes("Y_MARGIN, thumbWidth, compactBandHeight)") &&
       eq.includes("Y_MARGIN, thumbWidth, tunedBandHeight )"),
       "Native EQ sliders must fit their panel with working labels at all supported band counts");
assert(!view.includes("railOutline") && !view.includes("Path railFill(spine)"), "Remove stray curved sidebar flourish entirely");
assert(view.includes("card(247,253,795,235,23.0f)") && view.includes("card(1053,500,247,188,23.0f)"), "Spectrum and AI status need their own aligned panels");
assert(view.includes("ColourGradient navGlow(") && view.includes("card(955,108,345,78,19.0f)"),"Gradient selection and native header tile geometry required");
console.log("PASS: clean black workspace, bottom sidebar sweep, aligned audio panels and version metadata");
