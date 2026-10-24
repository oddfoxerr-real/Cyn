"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const {commandFor}=require("../commands.cjs");
const status={
  cyn_build:"identity-2",power:true,
  presets:{built_in:[{name:"Bass Boost"}],user_defined:[{name:"Custom"}]},
  output_devices:["Speakers (HD Audio)"],
  equalizer:{bands:Array.from({length:39},(_,index)=>({index,gain:0,frequency:20*1.25**index}))},
  ai_profiles:[{id:"personal",name:"Personal"},{id:"audonic",name:"AUDONIC"}]
};
test("native commands are validated and only write approved DSP values",()=>{
  assert.equal(commandFor({type:"power",value:true},status),"--power=1");
  assert.equal(commandFor({type:"smart",value:false},status),"--smart_listening=0");
  assert.equal(commandFor({type:"pin",value:true},status),"--pin_output=1");
  assert.equal(commandFor({type:"sensitivity",value:89},status),"--smart_sensitivity=89");
  assert.equal(commandFor({type:"effect",name:"bass",value:7.25},status),"--set_effect=bass:7.25");
  assert.equal(commandFor({type:"band",index:38,value:-9},status),"--set_band_gain=38:-9.00");
  assert.equal(commandFor({type:"preset",value:"Bass Boost"},status),'--preset="Bass Boost"');
  assert.equal(commandFor({type:"output",value:"Speakers (HD Audio)"},status),'--output="Speakers (HD Audio)"');
  assert.equal(commandFor({type:"aiModel",value:"audonic"},status),'--select_ai="audonic"');
});
test("reject unrecognized commands, values and profile impersonation",()=>{
  const bad=[
    {type:"band",index:39,value:1},{type:"band",index:-1,value:1},
    {type:"band",index:2,value:Infinity},{type:"effect",name:"shell",value:1},
    {type:"effect",name:"bass",value:11},{type:"sensitivity",value:101},
    {type:"sensitivity",value:-1},{type:"preset",value:"Not Present"},
    {type:"preset",value:"Bass Boost\"\r\n--power=0"},
    {type:"output",value:"Other"},{type:"aiModel",value:"not-installed"},
    {type:"exec",value:"powershell.exe"}
  ];
  for(const action of bad)assert.throws(()=>commandFor(action,status),TypeError);
  assert.throws(()=>commandFor({type:"power",value:true},{cyn_build:"wrong"}),TypeError);
});
test("native CYN status and AI choices have real controller backing",()=>{
  const root=path.resolve(__dirname,"../..");
  const native=fs.readFileSync(path.join(root,"fxsound/Source/GUI/FxController.cpp"),"utf8");
  assert(native.includes('settings->setProperty("cyn_build", "identity-2")'));
  assert(native.includes('settings->setProperty("active_ai_profile",getActiveAIProfileId())'));
  assert(native.includes('settings->setProperty("ai_profiles",ai_profile_options)'));
  assert(native.includes('getValueForOption("--select_ai")'));
  assert(native.includes("selectAIProfile(requestedAI,selectionError)"));
});
test("renderer uses isolated Riff UI and never starts a second audio engine",()=>{
  const root=path.resolve(__dirname,"..");
  const main=fs.readFileSync(path.join(root,"main.cjs"),"utf8");
  const preload=fs.readFileSync(path.join(root,"preload.cjs"),"utf8");
  const html=fs.readFileSync(path.join(root,"renderer/index.html"),"utf8");
  const app=fs.readFileSync(path.join(root,"renderer/renderer.js"),"utf8");
  assert(main.includes("contextIsolation:true,nodeIntegration:false,sandbox:true"));
  assert(main.includes('setWindowOpenHandler(()=>({action:"deny"}))'));
  assert(html.includes('href="vendor/riff.css"'));
  assert(html.includes('class="nav-active-indicator"'));
  assert(html.includes('class="content-view active"'));
  assert(app.includes("window.cyn.apply(action)"));
  assert(app.includes("status.smart_listening?.enabled"));
  assert(app.includes("b.effective_gain??b.gain"));
  assert(!html.includes("<audio"));
  assert(!app.includes("AudioContext"));
  assert(preload.includes("contextBridge.exposeInMainWorld"));
});
