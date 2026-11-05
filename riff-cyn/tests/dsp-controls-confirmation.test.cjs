"use strict";
const test=require("node:test"),assert=require("node:assert/strict");
const fs=require("node:fs"),path=require("node:path");
const read=relative=>fs.readFileSync(path.resolve(__dirname,"../..",relative),"utf8");
const {actionConfirmed}=require("../action-result.cjs");
const status={
 cyn_build:"identity-2",power:true,output_pinned:false,
 selected_output:"Headphones (JBL Tune 520BT)",active_ai_profile:"apex",
 output_devices:["Headphones (JBL Tune 520BT)"],
 fx_engine:{physical_output_targeted:true},
 effects:{clarity:4.3,ambience:2.4,surround:3,dynamicboost:3,bass:8},
 equalizer:{bands:[{index:0,gain:3.5}],balance:1,master_gain:2,volume_leveling:1.4,filter_q:1.8},
 smart_listening:{enabled:true,sensitivity:45,headroom_cap:80,audonic_intensity:35,listening_mode:0,preset_cycling:true},
 ai_dj:{installed:true,enabled:true,intensity:.4,spacing:18,stereo:false}
};
test("native confirmations reject silently ignored DSP changes",()=>{
 for(const [action,field] of [
  [{type:"effect",name:"bass",value:8},"effects"],
  [{type:"band",index:0,value:3.5},"equalizer"],
  [{type:"sensitivity",value:45},"smart_listening"],
  [{type:"headroom",value:80},"smart_listening"],
  [{type:"audonicIntensity",value:35},"smart_listening"],
  [{type:"masterGain",value:2},"equalizer"],
  [{type:"filterQ",value:1.8},"equalizer"],
  [{type:"djIntensity",value:40},"ai_dj"],
  [{type:"djSpacing",value:18},"ai_dj"],
  [{type:"power",value:true},"power"]
 ]){
   assert(actionConfirmed(action,status),action.type+" should confirm");
   const modified=structuredClone(status);
   if(field==="effects")modified.effects.bass=1;
   else if(field==="equalizer") {
     if(action.type==="band")modified.equalizer.bands[0].gain=-5;
     if(action.type==="masterGain")modified.equalizer.master_gain=0;
     if(action.type==="filterQ")modified.equalizer.filter_q=2.5;
   }else if(field==="smart_listening"){
     if(action.type==="sensitivity")modified.smart_listening.sensitivity=3;
     if(action.type==="headroom")modified.smart_listening.headroom_cap=3;
     if(action.type==="audonicIntensity")modified.smart_listening.audonic_intensity=3;
   }else if(field==="ai_dj"){
     if(action.type==="djIntensity")modified.ai_dj.intensity=.1;
     else modified.ai_dj.spacing=4;
   }else modified.power=false;
   assert(!actionConfirmed(action,modified),action.type+" must reject stale native value");
 }
});
test("Windows physical output requires confirmed passthrough",()=>{
 const a={type:"output",value:"Headphones (JBL Tune 520BT)"};
 assert(actionConfirmed(a,status));
 assert(!actionConfirmed(a,{...status,fx_engine:{physical_output_targeted:false}}));
 assert(!actionConfirmed(a,{...status,selected_output:"Speakers"}));
});
test("range state is locked while dragging or waiting for the real native effect",()=>{
 const renderer=read("riff-cyn/renderer/renderer.js");
 for(const fragment of [
  "const activeRanges=new Set(),pendingRanges=new Set(),actionQueue=[]",
  'input.addEventListener("input",()=>{',
  'input.addEventListener("change",async()=>{',
  'activeRanges.add(id)',
  'pendingRanges.add(id)',
  "if(isRangeEditing(input))return",
  'syncBoundRange("sensitivity"',
  'syncBoundRange("headroom"',
  'syncBoundRange("audonic-intensity"',
  'syncBoundRange("dj-intensity"',
  'syncBoundRange("dj-spacing"',
  "while(actionQueue.length)",
  "renderConnected(result.status)"
 ])assert(renderer.includes(fragment),"Missing control lifecycle: "+fragment);
});
