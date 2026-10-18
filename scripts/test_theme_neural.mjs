import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const file=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const themeH=file("fxsound/Source/GUI/FxTheme.h");
const theme=file("fxsound/Source/GUI/FxTheme.cpp");
const atmosphere=file("fxsound/Source/GUI/CynThemeAtmosphere.h");
const neural=file("fxsound/Source/GUI/CynNeuralActivity.h");
const view=file("fxsound/Source/GUI/FxProView.cpp");
assert(themeH.includes("NeuralCore, DarkMesh, NumModes"),"Legacy theme enum retained for compatibility while canonical theme is DarkMesh");
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
assert(view.includes("smart_toggle_.setBounds(1041,478,233,26)") &&
       view.includes("neural_activity_.setBounds(993,246,291,194)") &&
       view.includes("learning_timeline_.setBounds(left+13,246,722,194)"),
       "Smart Listening switch must be below the timeline and separate from neural visualization");
assert(view.includes('txt("LIVE NEURAL ACTIVITY",1011'),"Smart Listening must have an independently framed live neural inspector");
console.log("PASS: dark Dark Theme theme, true live neural path weighting, model colors and header spacing");

const paletteRow=theme.match(/\/\/ Dark Theme:[^\n]*\n\s*\{([^}]+)\}/);
assert(paletteRow,"Dark Theme RGB palette missing");
const colors=(paletteRow[1].match(/0x[0-9a-f]{6}/gi)||[]).map(v=>parseInt(v,16));
assert.equal(colors.length,27,"Theme must cover all palette roles");
assert(colors[0]<=0x08090a && colors[1]<=0x101216 && colors[15]<=0x12161a,
       "Background, chrome and cards must be near-black instead of gray");
assert(atmosphere.includes("constexpr int columns=33, rows=9") &&
       atmosphere.includes("node(col+1,row)") &&
       atmosphere.includes("node(col,row+1)"),
       "Connected dotted web mesh must cover workspace lower corners");
assert(atmosphere.includes("if(theme==FxThemeMode::DarkMesh)") &&
       atmosphere.includes("g.setColour(Colour(0xff000000))"),
       "Dark Theme must fill near-black with no opaque blue-gray gradient");
assert(view.includes("ColourGradient side(Colour(0xff07121f)") &&
       view.includes('g.fillRect(0,0,228,getHeight())'),
       "Dark navigation must be one continuous native black-blue sidebar");
console.log("PASS: pure-black palette, fluid retained mesh, model colors and controls");

const blackRoles=[0,1,2,5,9,10,15,24];
for(const n of blackRoles)
    assert(colors[n]<=0x030407,`Dark Theme surface ${n} must be black, not gray`);
assert(atmosphere.includes('const float phase=FxTheme::isReducedMotion()?0.0f:t;'),
       "Liquid web motion must freeze on reduced motion");
assert(atmosphere.includes('const float liquid=.68f+.32f*std::sin') &&
       atmosphere.includes('node(col+1,row)') &&
       atmosphere.includes('node(col,row+1)'),
       "Keep low-speed flowing connected neural mesh lines and dots");
assert(theme.includes('getThemeMode()==FxThemeMode::DarkMesh?base:base.brighter(0.035f)'),
       "Black dropdowns must not be brightened into gray");
assert(view.includes("const Colour base(0xff03070d)") &&
       view.includes("Colour(0xff07121f)"),
       "Stable shell must remain near-black instead of gray");
assert(view.includes("const Colour border(0xff20354b)") &&
       view.includes('g.fillRoundedRectangle(x,y,width,height,r)'),
       "Cards must share the subtle dark Depth hierarchy");
console.log("PASS: mesh retained and flowing, surfaces pure black, no gray bloom");

const studio=file("fxsound/Source/GUI/CynThemeStudio.h");
const fxController=file("fxsound/Source/GUI/FxController.cpp");
assert(theme.includes("theme_mode_ = FxThemeMode::DarkMesh") &&
       theme.includes('return "Dark Theme";') &&
       fxController.includes('settings_.setInt("cyn_previous_theme_mode",previousTheme)') &&
       fxController.includes("mode=FxThemeMode::DarkMesh;"),
       "Stable dark-only redesign must safely migrate legacy colours without deleting user data");
assert(!studio.includes('next_.onClick =') &&
       !studio.includes('previous_.onClick =') &&
       studio.includes('g.drawText("Dark Theme"'),
       "CYN Studio should keep AI Touch/button/close controls, not the obsolete theme carousel");
console.log("PASS: Dark Theme migration, no theme carousel, preserve studio customization");
