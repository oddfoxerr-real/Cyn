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

test("Riff's old playback-bar height does not collapse CYN's real controls",()=>{
  const css=fs.readFileSync(path.resolve(__dirname,"../renderer/cyn.css"),"utf8");
  assert(css.includes("height:calc(100dvh - 46px)!important"));
  assert(css.includes(".workspace-header,.control-deck,.content-view{flex-shrink:0}"));
  assert(css.includes(".control-deck{min-height:100px}"));
});

test("upstream Riff drawer implementation is connected to real CYN actions",()=>{
  const root=path.resolve(__dirname,"..");
  const html=fs.readFileSync(path.join(root,"renderer/index.html"),"utf8");
  const drawer=fs.readFileSync(path.join(root,"renderer/rail-drawer.js"),"utf8");
  const app=fs.readFileSync(path.join(root,"renderer/renderer.js"),"utf8");
  const css=fs.readFileSync(path.join(root,"renderer/cyn.css"),"utf8");
  assert(html.includes('src="rail-drawer.js"'));
  assert(html.indexOf('src="rail-drawer.js"')<html.indexOf('src="renderer.js"'));
  for(const id of ["rail-drawer","btn-toggle-presets-drawer","btn-toggle-ai-drawer",
    "btn-toggle-theme-drawer","drawer-section-presets","drawer-section-ai",
    "drawer-section-appearance","btn-close-rail-drawer","drawer-ai-model"]){
    assert(html.includes('id="'+id+'"'),"Missing functional Riff/CYN control: "+id);
  }
  assert(drawer.includes("railDrawerCloseTimeout"));
  assert(drawer.includes('classList.add("section-switching-out")'));
  assert(drawer.includes("120")&&drawer.includes("200"));
  assert(drawer.includes('event.key==="Escape"'));
  assert(app.includes("window.cynRail.setPresets(status"));
  assert(app.includes('type:"aiModel"'));
  assert(css.includes("#rail-drawer"));
  assert(css.includes("grid-auto-rows:minmax(112px,auto)"));
  assert(css.includes("repeat(var(--band-count,31),minmax(23px,1fr))"));
});
test("CYN is the visible app identity and preview stays separate from native CYN",()=>{
  const root=path.resolve(__dirname,"..");
  const html=fs.readFileSync(path.join(root,"renderer/index.html"),"utf8");
  const pkg=JSON.parse(fs.readFileSync(path.join(root,"package.json"),"utf8"));
  const win=fs.readFileSync(path.join(root,"main.cjs"),"utf8");
  assert.equal(pkg.build.productName,"CYN Audio");
  assert.equal(pkg.name,"cyn-audio-desktop");
  assert(pkg.build.artifactName.startsWith("CYN-Audio-Preview-"));
  assert(win.includes('title:"CYN"'));
  assert(html.includes("<title>CYN — Personal Audio</title>"));
  assert(html.includes('CYN<span class="brand-slash"> / AUDIO</span>'));
  assert(!html.includes("CYN Riff"));
  assert(pkg.build.nsis.deleteAppDataOnUninstall===false);
});


test("Riff graphite dock: workspaces above separator, all flyouts below, neutral square selection",()=>{
  const root=path.resolve(__dirname,"..");
  const html=fs.readFileSync(path.join(root,"renderer/index.html"),"utf8");
  const css=fs.readFileSync(path.join(root,"renderer/cyn.css"),"utf8");
  const app=fs.readFileSync(path.join(root,"renderer/renderer.js"),"utf8");
  const navStart=html.indexOf('<nav class="sidebar-nav"');
  const navEnd=html.indexOf("</nav>",navStart);
  const toolsStart=html.indexOf('<div class="rail-action-group">',navEnd);
  const drawerStart=html.indexOf('<div class="rail-drawer hidden"',toolsStart);
  assert(navStart>=0&&navEnd>navStart&&toolsStart>navEnd&&drawerStart>toolsStart);
  const nav=html.slice(navStart,navEnd);
  const tools=html.slice(toolsStart,drawerStart);
  for(const mode of ["overview","devices","ai"])
    assert(nav.includes('data-view="'+mode+'"'),"Main workspace missing from top: "+mode);
  for(const id of ["btn-toggle-presets-drawer","btn-toggle-ai-drawer","btn-toggle-theme-drawer"]){
    assert(!nav.includes('id="'+id+'"'),"Drawer tool incorrectly placed in top navigation: "+id);
    assert(tools.includes('id="'+id+'"'),"Drawer tool must sit below divider: "+id);
  }
  assert(html.indexOf('class="rail-divider rail-lower-divider"')<toolsStart,
    "Sidebar separator must precede drawer tools");
  assert(tools.includes('class="rail-divider rail-tools-divider"'),
    "Bottom shortcuts must be separated from flyout tools");
  assert(css.includes("background:#3b3c40!important"),"Top dock selection must be charcoal, not accent purple");
  assert(css.includes("border-radius:12px!important"),"Top selected tile must be a rounded square");
  assert(css.includes("background:#1e1f22!important"),"Riff drawer must use original graphite surface");
  assert(css.includes("background:#414247!important"),"Flyout selected circle must be neutral gray");
  assert(app.includes(".sidebar .rail-action-group [data-drawer="),
    "Drawer selection must target below-divider controls");
});
