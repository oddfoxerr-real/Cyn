import assert from "node:assert/strict";
import { readFileSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

// Visual layout geometry derived from native JUCE source, not a JUCE screenshot.
// No FxController, settings, audio interface or user data is ever initialized.
const root=resolve(dirname(fileURLToPath(import.meta.url)),"..");
const read=p=>readFileSync(join(root,p),"utf8");
const view=read("fxsound/Source/GUI/FxProView.cpp");
const layout=read("fxsound/Source/GUI/CynRiffLayout.h");
const eq=read("fxsound/Source/GUI/FxEqualizer.cpp");
const eqh=read("fxsound/Source/GUI/FxEqualizer.h");
const header=read("fxsound/Source/GUI/FxProView.h");
function value(src,pattern,label) {
  const match=src.match(pattern);
  assert(match,"Native layout token missing: "+label);
  return Number(match[1]);
}
const W=value(header,/WIDTH\s*=\s*(\d+)/,"window width");
const H=value(header,/HEIGHT\s*=\s*(\d+)/,"window height");
const oldX=value(layout,/legacyContentLeft\s*=\s*([\d.]+)f/,"legacy content left");
const newX=value(layout,/contentLeft\s*=\s*([\d.]+)f/,"new content left");
const right=value(layout,/contentRight\s*=\s*([\d.]+)f/,"content right");
const rail=value(layout,/railWidth\s*=\s*([\d.]+)f/,"floating icon rail");
const eqWidth=value(view,/eqW\s*=\s*(\d+)/,"equalizer workbench width");
const eqMargin=value(eqh,/X_MARGIN\s*=\s*(\d+)/,"equalizer margin");
const eqTop=value(eqh,/Y_MARGIN\s*=\s*(\d+)/,"equalizer top");
const s=(right-newX)/(right-oldX);
const tx=x=>Math.round(x*s+newX-oldX*s);
const tw=w=>Math.round(w*s);
assert.equal(W,1320);
assert.equal(H,700);
assert.equal(rail,80);
assert(view.includes("CynRiffLayout::visualTransform()"));
assert(view.includes("CynRiffLayout::x(b.getX())"));
assert(view.includes("CynRiffLayout::width(b.getWidth())"));
assert(eq.includes("CynEqUiCallbacks::dispatchGain("));
assert(eq.includes("getNumEqBands()") && eq.includes("setEqBandBoostCut("));

const pages=[
  ["sound-studio","Sound Studio",[[247,253,1053,282],[247,547,790,141],[1046,547,254,141]]],
  ["smart-listening","Smart Listening",[[247,253,743,210],[996,253,304,210],[247,472,1053,216]]],
  ["output-devices","Output Devices",[[247,253,1053,200],[247,465,1053,223]]],
  ["ai-hub","AI Hub",[[247,250,1053,435]]],
  ["ai-dj","Misc / AI DJ",[[247,252,686,205],[944,252,356,205],[247,468,1053,220]]]
];
const topCards=[[247,108,1053,78]];
for (const c of [...topCards,...pages.flatMap(p=>p[2])]) {
  assert(view.includes("card("+c.join(",")) ||
   (c[0]===247 && c[1]===250 && view.includes("embedded_ai_hub_.setBounds(247,250,1053,435)")),
   "Native page geometry diverged from preview: "+c.join(","));
}
function bounds(rect) {
  const [x,y,w,h]=rect, X=tx(x), width=tw(w);
  assert(X>=102 && X+width<=1312 && y>=8 && y+h<=692,
    "Panel escaped the actual 1320x700 native workbench: "+rect.join(","));
  return [X,y,width,h];
}
const drawRect=(r,color,radius=20)=>'<rect x="'+r[0]+'" y="'+r[1]+'" width="'+r[2]+'" height="'+r[3]+'" rx="'+radius+'" fill="'+color+'"/>';
const drawLabel=(x,y,t,size=15)=>'<text x="'+x+'" y="'+y+'" font-family="Segoe UI,Arial,sans-serif" font-size="'+size+'" fill="#d7dce5">'+t+'</text>';
function render(page,bands) {
  const [id,title,cards]=page;
  const svg=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1320 700">'];
  svg.push(drawRect([0,0,W,H],"#121214",0));
  svg.push(drawRect([10,8,80,684],"#1e1f22",28));
  svg.push(drawRect([102,8,1208,684],"#1a1b1e",28));
  const nav=["sound-studio","smart-listening","output-devices","ai-hub","ai-dj","studio"];
  nav.forEach((item,i)=>{
    const y=104+i*58;
    if(item===id) svg.push(drawRect([26,y,48,48],"#3c4251",24));
    svg.push(drawLabel(32,y+29,["EQ","AI","OUT","HUB","DJ","CFG"][i],12));
  });
  svg.push(drawLabel(tx(246),73,title,30));
  svg.push(drawLabel(tx(246),97,"SOURCE-LINKED GEOMETRY PREVIEW - NOT A NATIVE SCREENSHOT",11));
  topCards.forEach((c,i)=>{
    svg.push(drawRect(bounds(c),"#25262a",19));
    for(const [x,label] of [[262,"PRESET"],[603,"OUTPUT DEVICE"],[972,"ACTIVE AI"]])
      svg.push(drawLabel(tx(x),135,label,13));
  });
  if(id==="sound-studio"){
    svg.push(drawRect(bounds([254,200,442,40]),"#292c34",20));
    svg.push(drawLabel(tx(266),225,"OVERVIEW       EQUALIZER       ENHANCEMENTS",14));
  }
  cards.forEach(c=>svg.push(drawRect(bounds(c),"#222327",22)));
  if(id==="sound-studio"){
    const eqBounds=bounds([259,314,eqWidth,210]);
    svg.push(drawRect(eqBounds,"#101217",15));
    const cell=Math.floor((eqBounds[2]-2*eqMargin)/bands);
    assert(cell>=20,"EQ band hit area too narrow for "+bands+" bands");
    for(let i=0;i<bands;++i){
      const x=eqBounds[0]+eqMargin+i*cell+cell/2;
      const y=eqBounds[1]+eqTop+5;
      svg.push('<line x1="'+x+'" y1="'+y+'" x2="'+x+'" y2="'+(y+118)+'" stroke="#363c47" stroke-width="2"/>');
      svg.push(drawRect([x-4,y+40+(i%7)*7,8,12],"#92b9ff",4));
    }
    svg.push(drawLabel(eqBounds[0]+14,eqBounds[1]+17,bands+" EQUALIZER BANDS",11));
    svg.push(drawLabel(tx(1040),286,"SPECTRUM",12));
    svg.push(drawLabel(tx(266),571,"ENHANCEMENTS",13));
  }else{
    svg.push(drawLabel(tx(267),287,
      id==="smart-listening"?"NEURAL / LEARNING":id==="output-devices"?"DEVICE ROUTING":id==="ai-hub"?"EMBEDDED AI CATALOGUE":"AI DJ CONTROLS",16));
  }
  svg.push("</svg>");
  return svg.join("\n")+"\n";
}
const folder=resolve(root,process.env.CYN_RENDER_OUT||"artifacts/riff-render-check");
mkdirSync(folder,{recursive:true});
let generated=0;
for(const page of pages){
  for(const bands of [31,35,39]){
    writeFileSync(join(folder,page[0]+"-"+bands+"-bands.svg"),render(page,bands),"utf8");
    generated++;
  }
}
assert.equal(generated,15);
console.log("PASS 5 native CYN pages x 3 EQ modes: 15 offscreen source-linked geometry SVG previews. No audio/settings accessed.");
