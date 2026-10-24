import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const file=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const l=file("fxsound/Source/GUI/CynRiffLayout.h"),
      v=file("fxsound/Source/GUI/FxProView.cpp"),
      h=file("fxsound/Source/GUI/FxProView.h"),
      m=file("fxsound/Source/GUI/CynExpressiveMotion.h"),
      t=file("fxsound/Source/GUI/FxTheme.cpp");
const license=file("third_party/riff-LICENSE.txt");
assert(license.includes("MIT License")&&license.includes("Copyright (c) 2026 rootscripts"));
for(const x of ["railWidth=80.0f","railIconSize=48.0f","contentLeft=112.0f",
                "0xff121214","0xff1e1f22","0xff1a1b1e","0xff333438","visualTransform()"])
  assert(l.includes(x),"Riff CSS token missing: "+x);
assert(v.includes("CynRiffLayout::visualTransform()") &&
       v.includes("CynRiffLayout::x(b.getX())")&&v.includes("CynRiffLayout::width(b.getWidth())"),
       "Drawn workspace and interactive native controls must reflow together");
assert(v.includes("g.fillRoundedRectangle(10.0f,8.0f,80.0f,684.0f,28.0f)") &&
       v.includes("g.fillRoundedRectangle(102.0f,8.0f,1208.0f,684.0f,28.0f)"),
       "Riff main and sidebar large floating surfaces missing");
assert(v.includes('set("riff_icon_rail",true)')&&h.includes('CynRailButton sound_nav_')&&
       h.includes('CynRailButton misc_nav_'),"Native icon hit targets missing");
assert(m.includes("104.0f+58.0f") && m.includes("260.0f") && m.includes("340.0f"),
       "Actual Riff two-stage stretch/settle motion must drive the icon rail");
for(const x of ["equalizer_.setBounds(eqX,314,eqW,210)",
                "audio_controls_.setBounds(eqX,555,766,130)",
                "brain_scan_.setBounds(1008,288,271,164)",
                "embedded_ai_hub_.setBounds(247,250,1053,435)"])
  assert(v.includes(x),"Real DSP/AI native component missing: "+x);
assert(t.includes("// DarkMesh: Riff MIT CSS neutral surface tiers"));
console.log("PASS real MIT Riff source tokens, floating UI, native controls and DSP preservation");
