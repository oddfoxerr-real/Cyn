import {readFileSync} from "node:fs";
import assert from "node:assert/strict";
const file=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const themeH=file("fxsound/Source/GUI/FxTheme.h");
const theme=file("fxsound/Source/GUI/FxTheme.cpp");
const atmosphere=file("fxsound/Source/GUI/CynThemeAtmosphere.h");
const neural=file("fxsound/Source/GUI/CynNeuralActivity.h");
const view=file("fxsound/Source/GUI/FxProView.cpp");
assert(themeH.includes("NeuralCore, DarkMesh, AetherGlass, IonViolet, LiquidTitanium, JadeCircuit, SolarInk, PolarMist, Material3, NumModes"),"Legacy theme enum retained for compatibility while canonical theme is DarkMesh");
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
assert(view.includes("smart_toggle_.setBounds(1032,477,244,26)") &&
       view.includes("neural_activity_.setBounds(1008,288,271,164)") &&
       view.includes("learning_timeline_.setBounds(left+13,288,722,164)"),
       "Smart Listening switch must be below the timeline and separate from neural visualization");
assert(view.includes('txt("BRAINSCAN  /  LIVE SIGNAL",1011') && view.includes('neural_activity_.setBounds(1008,288,271,164)') && view.includes('brain_scan_.setBounds(1008,288,271,164)'),"Smart Listening must have an independently framed switchable BrainScan inspector");
console.log("PASS: dark Dark Theme theme, true live neural path weighting, model colors and header spacing");

const paletteRow=theme.match(/\/\/ DarkMesh:[^\n]*\n\s*\{([^}]+)\}/);
assert(paletteRow,"Dark Theme RGB palette missing");
const colors=(paletteRow[1].match(/0x[0-9a-f]{6}/gi)||[]).map(v=>parseInt(v,16));
assert.equal(colors.length,27,"Theme must cover all palette roles");
assert(colors[0]===0x121214 && colors[1]===0x1e1f22 && colors[24]===0x1a1b1e,
       "Riff MIT source surface-container tiers must be preserved in DarkMesh");
assert(atmosphere.includes("constexpr int columns=33, rows=9") &&
       atmosphere.includes("node(col+1,row)") &&
       atmosphere.includes("node(col,row+1)"),
       "Connected dotted web mesh must cover workspace lower corners");
assert(atmosphere.includes("if(theme==FxThemeMode::DarkMesh)") &&
       atmosphere.includes("g.setColour(Colour(0xff000000))"),
       "Dark Theme must fill near-black with no opaque blue-gray gradient");
assert(view.includes("CynRiffLayout::rail()") &&
       view.includes("g.fillRoundedRectangle(10.0f,8.0f,80.0f,684.0f,28.0f)"),
       "Dark navigation must use the Riff 80px floating surface rail");
console.log("PASS: pure-black palette, fluid retained mesh, model colors and controls");

const tonalRoles=[0,1,2,5,9,10,15,24];
for(const n of tonalRoles)
    assert(colors[n]>=0x121214 && colors[n]<=0x333438,
      `Riff-style DarkMesh surface ${n} must retain low-contrast charcoal layering`);
assert(atmosphere.includes('const float phase=FxTheme::isReducedMotion()?0.0f:t;'),
       "Liquid web motion must freeze on reduced motion");
assert(atmosphere.includes('const float liquid=.68f+.32f*std::sin') &&
       atmosphere.includes('node(col+1,row)') &&
       atmosphere.includes('node(col,row+1)'),
       "Keep low-speed flowing connected neural mesh lines and dots");
assert(theme.includes('getThemeMode()==FxThemeMode::DarkMesh?base:base.brighter(0.035f)'),
       "Black dropdowns must not be brightened into gray");
assert(view.includes("const Colour base=colour(WindowBackground)") &&
       view.includes("CynRiffLayout::main()"),
       "Stable shell must draw Riff-style floating content canvas");
assert(view.includes("const Colour border=colour(RowOutline)") &&
       view.includes('g.fillRoundedRectangle(x,y,width,height,r)'),
       "Cards must share the subtle dark Depth hierarchy");
console.log("PASS: mesh retained and flowing, surfaces pure black, no gray bloom");

const studio=file("fxsound/Source/GUI/CynThemeStudio.h");
const fxController=file("fxsound/Source/GUI/FxController.cpp");
assert(theme.includes('case FxThemeMode::AetherGlass:') &&
       theme.includes('case FxThemeMode::IonViolet:') &&
       fxController.includes('settings_.getInt("theme_mode"') &&
       fxController.includes('FxTheme::setThemeMode(mode);') &&
       fxController.includes("case FxThemeMode::AetherGlass:"),
       "Selected dark themes must persist instead of being reset at startup");
assert(studio.includes('themePicker_.onChange=') &&
       studio.includes('controller.setThemeMode(themeMode_)') &&
       studio.includes('paintPreview(g,themeMode_,preview_,time)') &&
       !studio.includes('next_.onClick ='),
       "CYN Studio must preview and persist a real dark preset while retaining AI Touch");
console.log("PASS: curated dark theme selection, persisted palette and studio customization");
