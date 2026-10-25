"use strict";
const test=require("node:test");
const assert=require("node:assert/strict");
const fs=require("node:fs");
const path=require("node:path");
const {commandFor}=require("../commands.cjs");
const root=path.resolve(__dirname,"../..");
const files={
  controller:fs.readFileSync(path.join(root,"fxsound/Source/GUI/FxController.cpp"),"utf8"),
  header:fs.readFileSync(path.join(root,"fxsound/Source/GUI/FxController.h"),"utf8"),
  bridge:fs.readFileSync(path.join(root,"riff-cyn/native-bridge.cjs"),"utf8"),
  main:fs.readFileSync(path.join(root,"riff-cyn/main.cjs"),"utf8"),
  preload:fs.readFileSync(path.join(root,"riff-cyn/preload.cjs"),"utf8"),
  html:fs.readFileSync(path.join(root,"riff-cyn/renderer/index.html"),"utf8"),
  brain:fs.readFileSync(path.join(root,"riff-cyn/renderer/brain-scan.js"),"utf8"),
  renderer:fs.readFileSync(path.join(root,"riff-cyn/renderer/renderer.js"),"utf8")
};
const status={
  cyn_build:"identity-2",power:true,active_ai_profile:"audonic",
  smart_listening:{training:false,listening_mode:0},
  ai_profiles:[{id:"personal",name:"My saved AI"},{id:"audonic",name:"AUDONIC"}]
};
test("native controls are typed and reject invalid inputs",()=>{
  assert.equal(commandFor({type:"headroom",value:61},status),"--smart_headroom=61");
  assert.equal(commandFor({type:"listeningMode",value:1},status),"--smart_mode=1");
  assert.equal(commandFor({type:"presetCycling",value:true},status),"--preset_cycling=1");
  assert.equal(commandFor({type:"audonicIntensity",value:70},status),"--audonic_intensity=70");
  assert.equal(commandFor({type:"train"},status),"--smart_train");
  assert.equal(commandFor({type:"labelRecent",effect:4,more:true,newest:0,oldest:15},status),"--label_recent=4:1:0:15");
  for(const input of [
    {type:"headroom",value:101},{type:"listeningMode",value:4},
    {type:"presetCycling",value:"yes"},{type:"audonicIntensity",value:Infinity},
    {type:"labelRecent",effect:5,more:true,newest:0,oldest:10},
    {type:"labelRecent",effect:1,more:false,newest:40,oldest:20},
    {type:"labelRecent",effect:1,more:true,newest:-1,oldest:20}
  ])assert.throws(()=>commandFor(input,status),TypeError);
  assert.throws(()=>commandFor({type:"train"},{
    ...status,smart_listening:{training:true,listening_mode:0}}),TypeError);
});
test("measured neural activations and creative DSP are native-only, no fake signal",()=>{
  for(const x of ["void FxController::publishUiTelemetry()",
    "getLiveNeuralActivity(sample)","getLiveAudonicControls(creative)",
    'telemetry->setProperty("input_connections",firstLinks)',
    'telemetry->setProperty("output_connections",secondLinks)',
    'telemetry->setProperty("creative",creativeChannels)',
    'telemetry->setProperty("recent_frames",recent)',
    "Time::currentTimeMillis()-frame.id"
  ])assert(files.controller.includes(x),"Missing live native brain data: "+x);
  assert(files.controller.includes("publishUiTelemetry();"));
  assert(files.controller.includes('getChildFile("ui-telemetry-request")'));
  assert(files.header.includes("void publishUiTelemetry();"));
  assert(files.brain.includes("sample?.active"));
  assert(files.brain.includes("data.input_connections"));
  assert(files.brain.includes("data.output_connections"));
  assert(files.brain.includes("data.creative"));
  assert(!files.brain.includes("Math.random"),"Neural activity may not be fabricated");
  assert(!files.brain.includes("AudioContext"),"Electron cannot be a second sound processor");
  assert(!files.brain.includes("getUserMedia"),"Neural UI must never record audio");
});
test("Electron invokes local read-only schema-checked telemetry, not a remote API",()=>{
  assert(files.bridge.includes("function telemetry()"));
  assert(files.bridge.includes('lastState.ui_telemetry_schema!==1'));
  assert(files.bridge.includes('sample.hidden.length!==12'));
  assert(files.bridge.includes('sample.input_connections.length!==132'));
  assert(files.bridge.includes("Date.now()-stat.mtimeMs>1350"));
  assert(files.main.includes('ipcMain.handle("cyn:telemetry",()=>bridge.telemetry())'));
  assert(files.preload.includes('telemetry:()=>ipcRenderer.invoke("cyn:telemetry")'));
  assert(!files.bridge.includes("http.createServer"));
});
test("no loss of native Music/Movie, AUDONIC, headroom, cycling, training or feedback control",()=>{
  const commands=["--smart_mode","--audonic_intensity","--smart_headroom",
    "--preset_cycling","--smart_train","--label_recent"];
  for(const command of commands)assert(files.controller.includes(command),"Missing native AI action "+command);
  for(const id of ["brain-canvas","learning-waveform","headroom","preset-cycling",
    "listening-mode","audonic-intensity","train-ai","feedback-effect","feedback-more","feedback-less"]){
    assert(files.html.includes('id="'+id+'"'),"AI interface must expose "+id);
  }
  for(const type of ["headroom","presetCycling","listeningMode","audonicIntensity",
    "train","labelRecent"])assert(files.renderer.includes('type:"'+type+'"'),"Renderer lost "+type);
});

test("native AI DJ remains real and installable, never mocked by Electron",()=>{
 const available={...status,ai_dj:{installed:false,busy:false}};
 const ready={...status,ai_dj:{installed:true,busy:false}};
 assert.equal(commandFor({type:"djCommand",value:"download"},available),"--dj_command=download");
 assert.equal(commandFor({type:"djCommand",value:"remove"},ready),"--dj_command=remove");
 assert.equal(commandFor({type:"djEnable",value:true},ready),"--dj_enable=1");
 assert.equal(commandFor({type:"djIntensity",value:72},ready),"--dj_intensity=72");
 assert.equal(commandFor({type:"djSpacing",value:18},ready),"--dj_spacing=18");
 assert.equal(commandFor({type:"djStereo",value:false},ready),"--dj_stereo=0");
 for(const item of [{type:"djEnable",value:true},{type:"djSpacing",value:46},
   {type:"djIntensity",value:101},{type:"djCommand",value:"remove"}]){
   assert.throws(()=>commandFor(item,available),TypeError);
 }
 for(const option of ["--dj_command","--dj_enable","--dj_intensity","--dj_spacing","--dj_stereo"])
   assert(files.controller.includes(option),"Native CYN DJ is missing "+option);
 for(const id of ["dj-download","dj-remove","dj-cancel","dj-enable","dj-intensity","dj-spacing","dj-stereo"])
   assert(files.html.includes('id="'+id+'"'),"Material UI has no "+id);
 assert(files.renderer.includes('type:"djEnable"'));
});
