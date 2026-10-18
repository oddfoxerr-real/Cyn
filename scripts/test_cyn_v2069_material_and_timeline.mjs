// Native CYN 2.0.69 material, timeline and four-agent geometry contract.
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const header=read("fxsound/Source/GUI/FxTheme.h");
const theme=read("fxsound/Source/GUI/FxTheme.cpp");
const atmosphere=read("fxsound/Source/GUI/CynThemeAtmosphere.h");
const studio=read("fxsound/Source/GUI/CynThemeStudio.h");
const view=read("fxsound/Source/GUI/FxProView.cpp");
const ctrl=read("fxsound/Source/GUI/FxAudioControls.cpp");
const timeline=read("fxsound/Source/GUI/AudioLearningTimeline.h");
const spectrum=read("fxsound/Source/GUI/FxVisualizer.cpp");
const app=read("fxsound/Source/GUI/FxController.cpp");
for(const mode of ["AetherGlass","IonViolet","LiquidTitanium","JadeCircuit","SolarInk","PolarMist"]) {
  assert(header.includes(mode),mode+" must have its own saved theme ID");
  assert(theme.includes("case FxThemeMode::"+mode+":"),mode+" missing native palette integration");
  assert(atmosphere.includes("case FxThemeMode::"+mode+":"),mode+" missing native artwork");
  assert(studio.includes("FxThemeMode::"+mode+",\""),mode+" missing from the selector");
  assert(app.includes("case FxThemeMode::"+mode+":"),mode+" cannot persist from controller");
}
assert(view.includes("const Colour base=colour(WindowBackground)") &&
       view.includes("const Colour panel=colour(PanelBackground)") &&
       view.includes("const Colour border=colour(RowOutline)"),
       "Native workspace must consume the same actual palette as JUCE controls");
assert(ctrl.includes("equalizer_control_.setBounds(0,0,getWidth(),getHeight())") &&
       ctrl.includes("if(getWidth()>=540)"),
       "Output Tuning must use full width with responsive control positions");
assert(view.includes("std::array<std::array<Candidate,39>,2> ranked") &&
       view.includes("agent.targetId") &&
       view.includes("getDistanceFrom(claimed") &&
       view.includes("<39.0f"),
       "Four flowers must choose separated, exclusive active control targets");
assert(view.includes("!audio_controls_.isEffectsShown()") &&
       view.includes("!equalizer_.isVisible()"),
       "Inactive audio focus panes must never draw overlay agents");
assert(timeline.includes("mouseWheelMove(") &&
       timeline.includes("recordLabeledSelection(") &&
       timeline.includes("setActiveEffect(") &&
       timeline.includes("historyMs=360000") &&
       view.includes("learning_timeline_.recordLabeledSelection("),
       "Timeline must scroll retained audio and color real labeled training ranges");
assert(spectrum.includes("ColourGradient spectrum(") &&
       spectrum.includes("spectrum.addColour(.33f") &&
       spectrum.includes("spectrum.addColour(.69f"),
       "Spectrum should shift chromatic stops across real FFT bands");
console.log("PASS: 6 native themes, separate flowers, responsive tuning, scrollable color-coded learning");
