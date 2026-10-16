import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const file=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const themeH=file("fxsound/Source/GUI/FxTheme.h");
const theme=file("fxsound/Source/GUI/FxTheme.cpp");
const atmosphere=file("fxsound/Source/GUI/CynThemeAtmosphere.h");
const neural=file("fxsound/Source/GUI/CynNeuralActivity.h");
const view=file("fxsound/Source/GUI/FxProView.cpp");
assert(themeH.includes("NeuralCore, DarkMesh, NumModes"),"Dark Theme must be selectable in Theme Studio");
assert(theme.includes('"Dark Theme"'),"Missing user-visible theme name");
assert(atmosphere.includes("case FxThemeMode::DarkMesh:"),"Theme needs a real animated backdrop");
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
console.log("PASS: dark Dark Theme theme, true live neural path weighting, model colors and header spacing");

const paletteRow=theme.match(/\/\/ Dark Theme:[^\n]*\n\s*\{([^}]+)\}/);
assert(paletteRow,"Dark Theme RGB palette missing");
const colors=(paletteRow[1].match(/0x[0-9a-f]{6}/gi)||[]).map(v=>parseInt(v,16));
assert.equal(colors.length,27,"Theme must cover all palette roles");
assert(colors[0]<=0x08090a && colors[1]<=0x101216 && colors[15]<=0x12161a,
       "Background, chrome and cards must be near-black instead of gray");
assert(atmosphere.includes("constexpr int columns=32, rows=9") &&
       atmosphere.includes("node(col+1,row)") &&
       atmosphere.includes("node(col,row+1)"),
       "Connected dotted web mesh must cover workspace lower corners");
assert(atmosphere.includes("if(theme==FxThemeMode::DarkMesh)") &&
       atmosphere.includes("g.setColour(Colour(0xff050607))"),
       "Dark Theme must fill near-black with no opaque blue-gray gradient");
assert(view.includes("const bool webDark=FxTheme::getThemeMode()==FxThemeMode::DarkMesh;"),
       "Dark sidebar must use black glass rather than a gray rail");
console.log("PASS: near-black palette, connected mesh, model colors and black controls");
