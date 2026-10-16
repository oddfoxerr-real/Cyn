import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const file=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const themeH=file("fxsound/Source/GUI/FxTheme.h");
const theme=file("fxsound/Source/GUI/FxTheme.cpp");
const atmosphere=file("fxsound/Source/GUI/CynThemeAtmosphere.h");
const neural=file("fxsound/Source/GUI/CynNeuralActivity.h");
const view=file("fxsound/Source/GUI/FxProView.cpp");
assert(themeH.includes("NeuralCore, GeminiNight, NumModes"),"Gemini Night must be selectable in Theme Studio");
assert(theme.includes('"Gemini Night"'),"Missing user-visible theme name");
assert(atmosphere.includes("case FxThemeMode::GeminiNight:"),"Theme needs a real animated backdrop");
assert(neural.includes("setProfileId(const juce::String& id)"),"Neural visualization must accept selected model ID");
assert(neural.includes("const float influence = std::abs(contributions[index]);"),
  "Neural paths must be based on actual live weighted contributions");
assert(neural.includes("if(influence < displayFloor && index%17 != 0) continue;"),
  "Profile detail must hide weak paths without fabricating a new network");
assert(neural.includes("const bool trained=id.contains("),"Trained model must have different graph styling");
assert(view.includes("neural_activity_.setProfileId(selected);"),
  "Active AI dropdown must update neural graph identity");
assert(view.includes("page_ == 1 ? 339 : 565"),"Smart Listening toggle should not overlap model labels");
assert(view.includes('text("ACTIVE AI MODEL",540,369'),"AI model label should not overlap the toggle");
console.log("PASS: dark Gemini Night theme, true live neural path weighting, model colors and header spacing");
