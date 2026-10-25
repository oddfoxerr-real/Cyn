"use strict";
const $=id=>document.getElementById(id);
const pages={
  overview:["Sound Studio","Your audio, with CYN's real DSP underneath."],
  equalizer:["Equalizer","Precision control of every active frequency band."],
  ai:["Smart Listening","Native neural processing and adaptive sound."],
  devices:["Output Devices","Control your actual Windows audio hardware."],
  settings:["Settings","Manage your native CYN connection and appearance."]
};
const fx=[
  ["clarity","Clarity","Detail and vocal presence"],
  ["ambience","Ambience","Space and atmosphere"],
  ["surround","Surround","Stereo field"],
  ["dynamicboost","Dynamic boost","Energy and contrast"],
  ["bass","Bass boost","Low-frequency weight"]
];
let current=null, activeView="overview", busy=false;
const value=(n,fallback="—")=>(n==null||n==="")?fallback:(Number.isFinite(Number(n))?Number(n).toFixed(1):fallback);
const writable=()=>Boolean(current?.connected)&&!busy;
function showError(message){
  $("error-banner").textContent=String(message||"");
  $("error-banner").hidden=!message;
}
function setEngineState(on,label){
  $("engine-state").classList.toggle("on",on);
  $("rail-led").classList.toggle("on",on);
  $("engine-label").textContent=label;
  $("start-native").hidden=on;
}
function opts(select,items,selected){
  const normalized=items.map(item=>typeof item==="string"?{value:item,label:item}:item);
  const signature=JSON.stringify(normalized);
  if(select.dataset.signature!==signature){
    select.replaceChildren(...normalized.map(item=>new Option(item.label,item.value)));
    select.dataset.signature=signature;
  }
  select.value=selected;
}
function renderGraph(bands){
  const svg=$("eq-curve");
  if(!bands.length){
    svg.replaceChildren();
    const text=document.createElementNS("http://www.w3.org/2000/svg","text");
    text.setAttribute("x","30");
    text.setAttribute("y","116");
    text.setAttribute("fill","#8493a7");
    text.setAttribute("font-size","18");
    text.textContent="Connect CYN to view your actual equalizer settings";
    svg.append(text);
    return;
  }
  const list=bands.map((b,i)=>({
    x:25+i*(950/Math.max(1,bands.length-1)),
    y:112.5-(Math.max(-12,Math.min(12,Number(b.effective_gain??b.gain)||0))*7.8)
  }));
  const path=list.map((p,i)=>(i?"L":"M")+p.x.toFixed(1)+","+p.y.toFixed(1)).join(" ");
  const closed=path+" L975,206 L25,206 Z";
  const lines=[-12,-6,0,6,12].map(db=>{
    const y=112.5-db*7.8;
    return '<line x1="25" x2="975" y1="'+y+'" y2="'+y+'" stroke="#60728c" stroke-opacity="'+(db===0?".4":".13")+'"/>'
  }).join("");
  svg.innerHTML='<defs><linearGradient id="eqfill" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#88baff" stop-opacity=".3"/><stop offset="1" stop-color="#88baff" stop-opacity="0"/></linearGradient><linearGradient id="eqstroke"><stop stop-color="#85b6ff"/><stop offset="1" stop-color="#67e2c5"/></linearGradient></defs>'+
    lines+'<path d="'+closed+'" fill="url(#eqfill)"/><path d="'+path+'" stroke="url(#eqstroke)" stroke-width="3.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/>'+
    list.filter((_,i)=>i%Math.max(1,Math.floor(list.length/12))===0).map(p=>'<circle cx="'+p.x+'" cy="'+p.y+'" r="4" fill="#b8d6ff"/>').join("");
}
function renderEffects(status){
  const box=$("effects-grid");
  if(!box.childElementCount){
    for(const [name,label,description] of fx){
      const article=document.createElement("article");
      article.className="effect-card";
      const h=document.createElement("h3");h.textContent=label;
      const output=document.createElement("div");output.className="effect-value";output.id="effect-value-"+name;
      const slider=document.createElement("input");
      slider.type="range";slider.min="0";slider.max="10";slider.step=".1";slider.id="effect-"+name;
      slider.setAttribute("aria-label",label+" gain");
      slider.addEventListener("input",()=>{output.textContent=value(slider.value)});
      slider.addEventListener("change",()=>apply({type:"effect",name,value:Number(slider.value)}));
      const desc=document.createElement("p");desc.textContent=description;
      article.append(h,output,slider,desc);box.append(article);
    }
  }
  for(const [name] of fx){
    const slider=$("effect-"+name),output=$("effect-value-"+name);
    slider.disabled=!writable();
    if(document.activeElement!==slider){
      const val=status?.effects?.[name];
      slider.value=Number.isFinite(Number(val))?Number(val):0;
      output.textContent=value(val);
    }
  }
}
function renderBands(status){
  const box=$("eq-sliders"),bands=Array.isArray(status?.equalizer?.bands)?status.equalizer.bands:[];
  $("eq-band-mode").textContent=bands.length?bands.length+" BANDS":"NO NATIVE EQ";
  $("eq-mode-label").textContent=bands.length?bands.length+" BAND MODE":"CONNECT ENGINE";
  const signature=bands.map(b=>String(b.index)+":"+String(b.frequency)).join("|");
  box.style.setProperty("--band-count",String(Math.max(1,bands.length)));
  if(box.dataset.signature!==signature){
    box.replaceChildren();
    for(const b of bands){
      const item=document.createElement("label");item.className="eq-band";
      const val=document.createElement("output");val.id="band-value-"+b.index;val.textContent=value(b.gain);
      const input=document.createElement("input");
      input.type="range";input.min="-12";input.max="12";input.step=".1";
      input.value=Number(b.gain)||0;input.id="band-"+b.index;
      input.setAttribute("aria-label","EQ band "+(b.index+1)+" at "+b.frequency+" Hz");
      input.addEventListener("input",()=>{
        val.textContent=value(input.value);
      });
      input.addEventListener("change",()=>apply({type:"band",index:b.index,value:Number(input.value)}));
      const frequency=document.createElement("span");frequency.className="freq";
      const freq=Number(b.frequency);
      frequency.textContent=freq>=1000?(freq/1000).toFixed(freq%1000?1:0)+"k":String(Math.round(freq));
      item.append(val,input,frequency);box.append(item);
    }
    box.dataset.signature=signature;
  }
  for(const b of bands){
    const slider=$("band-"+b.index),val=$("band-value-"+b.index);
    if(!slider)continue;
    slider.disabled=!writable();
    if(document.activeElement!==slider){slider.value=Number(b.gain)||0;val.textContent=value(b.gain)}
  }
  renderGraph(bands);
}
function renderConnected(status){
  setEngineState(true,(status.power?"ON":"OFF")+" / v"+String(status.version||"?"));
  $("power").checked=Boolean(status.power);
  $("smart").checked=Boolean(status.smart_listening?.enabled);
  $("pin").checked=Boolean(status.output_pinned);
  $("sensitivity").value=Number(status.smart_listening?.sensitivity)||0;
  $("sensitivity-value").textContent=Math.round(Number(status.smart_listening?.sensitivity)||0)+"%";
  $("smart-summary").textContent=String(status.smart_listening?.summary||"Native CYN adaptive processing.");
  $("learning-status").textContent=String(status.smart_listening?.learning||"No learning telemetry available.");
  const profiles=Array.isArray(status.ai_profiles)?status.ai_profiles:[];
  const activeProfile=profiles.find(p=>p.id===status.active_ai_profile);
  $("active-model").textContent=String(activeProfile?.name||status.active_ai_profile||"Native CYN AI");
  const aiOptions=profiles.length?profiles.map(p=>({value:p.id,label:p.name})):[{value:"",label:"Model selection in native CYN"}];
  for(const select of [$("ai-model"),$("drawer-ai-model")]){
    select.disabled=!writable()||!profiles.length;
    opts(select,aiOptions,status.active_ai_profile||"");
  }
  $("drawer-ai-summary").textContent=activeProfile?.name
    ? "Selected: "+activeProfile.name
    : "AI model selection remains in native CYN until this engine exposes its installed profiles.";
  const presets=[...(status.presets?.built_in||[]),...(status.presets?.user_defined||[])];
  opts($("preset"),presets.map(p=>({value:p.name,label:p.name+(p.modified?" *":"")})),status.selected_preset||"");
  window.cynRail.setPresets(status,name=>apply({type:"preset",value:name}));
  const dev=Array.isArray(status.output_devices)?status.output_devices:[];
  for(const select of [$("device-top"),$("device-page")])opts(select,dev,status.selected_output||"");
  for(const id of ["preset","device-top","device-page","power","smart","pin","sensitivity"])
    $(id).disabled=!writable();
  renderEffects(status);
  renderBands(status);
}
function renderDisconnected(message){
  setEngineState(false,"Engine offline");
  for(const id of ["preset","device-top","device-page","power","smart","pin","sensitivity","ai-model"])$(id).disabled=true;
  $("smart-summary").textContent=String(message||"Connect native CYN to access processing");
  $("active-model").textContent="Not connected";
  $("learning-status").textContent="Start native CYN to read its actual learning state.";
  $("sensitivity-value").textContent="—";
  opts($("preset"),[{value:"",label:"Connect CYN"}],"");
  for(const id of ["device-top","device-page"])opts($(id),[{value:"",label:"Connect CYN"}],"");
  for(const id of ["ai-model","drawer-ai-model"])opts($(id),[{value:"",label:"Unavailable"}],"");
  $("drawer-ai-model").disabled=true;
  $("drawer-ai-summary").textContent="Start CYN to load installed AI models.";
  window.cynRail.setPresets(null,()=>{});
  renderEffects(null);
  renderBands(null);
}
async function refresh(){
  if(busy)return;
  try{
    const result=await window.cyn.status();
    current=result;
    if(result.connected){renderConnected(result.status);showError("")}
    else renderDisconnected(result.error);
  }catch(error){
    current={connected:false,error:error.message};
    renderDisconnected(error.message);
  }
}
async function apply(action){
  if(!writable())return;
  busy=true;
  try{
    const result=await window.cyn.apply(action);
    current=result;
    if(!result.connected)throw new Error(result.error||"Native audio command did not complete");
    showError("");
  }catch(error){showError(error.message)}
  finally{
    busy=false;
    await refresh();
  }
}
/* Riff's first/middle/last dock geometry, isolated from animation.
 * The previous approach set inline radius during transitions, but the
 * actual Electron computed radius could still resolve to all-14px.
 * Explicit indicator states now own the corner geometry through CSS,
 * while movement uses the original Riff 260/340ms stretch/settle timings.
 */
const stackClasses=["stack-only","stack-first","stack-mid","stack-last"];
let navTimer=0,navReady=false,navSequence=0,navTargetTop=null;
function refreshNavStack(){
  const nav=document.querySelector(".sidebar-nav");
  if(!nav)return;
  const visible=[...nav.children].filter(el=>
    el.classList.contains("nav-item") &&
    el.getBoundingClientRect().height>1 &&
    getComputedStyle(el).display!=="none");
  visible.forEach((el,i)=>{
    const desired=visible.length===1?"stack-only":
      i===0?"stack-first":i===visible.length-1?"stack-last":"stack-mid";
    for(const name of stackClasses)el.classList.toggle(name,name===desired);
  });
}
function navShape(active){
  if(active.classList.contains("stack-first"))return "first";
  if(active.classList.contains("stack-last"))return "last";
  if(active.classList.contains("stack-only"))return "only";
  return "middle";
}
function updateNavIndicator(animate){
  refreshNavStack();
  const ind=$("nav-active-indicator");
  if(!ind)return;
  const active=document.querySelector(".sidebar-nav .nav-item.active");
  clearTimeout(navTimer);
  const sequence=++navSequence;
  if(!active||!active.offsetParent){
    ind.classList.add("is-hidden");
    ind.dataset.cynShape="none";
    navReady=false;
    navTargetTop=null;
    return;
  }
  const nav=active.closest(".sidebar-nav");
  const nr=nav.getBoundingClientRect();
  const r=active.getBoundingClientRect();
  if(!r.height)return;
  const top=Math.round(r.top-nr.top+(r.height-48)/2);
  const shape=navShape(active);
  ind.classList.remove("is-hidden");
  const reduce=document.body.classList.contains("reduce-motion")||
    window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const computed=getComputedStyle(ind);
  const curTop=computed.transform==="none"?top:new DOMMatrix(computed.transform).m42;
  const curH=ind.getBoundingClientRect().height||48;
  if(!animate||!navReady||reduce||navTargetTop===top){
    ind.style.transition="opacity .2s ease";
    ind.style.height="48px";
    ind.style.transform="translateY("+top+"px)";
    ind.dataset.cynShape=shape;
    navTargetTop=top;
    navReady=true;
    return;
  }
  const newTop=Math.min(curTop,top);
  const newBottom=Math.max(curTop+curH,top+48);
  ind.style.transition="transform .26s cubic-bezier(.3,0,.8,.15),height .26s cubic-bezier(.3,0,.8,.15),opacity .2s ease";
  ind.dataset.cynShape="stretch";
  ind.style.transform="translateY("+newTop+"px)";
  ind.style.height=(newBottom-newTop)+"px";
  navTargetTop=top;
  navTimer=setTimeout(()=>{
    if(sequence!==navSequence)return;
    ind.style.transition="transform .34s cubic-bezier(.2,0,0,1),height .34s cubic-bezier(.2,0,0,1),border-radius .34s cubic-bezier(.2,0,0,1),opacity .2s ease";
    ind.style.transform="translateY("+top+"px)";
    ind.style.height="48px";
    ind.dataset.cynShape=shape;
  },260);
}
window.addEventListener("resize",()=>updateNavIndicator(false));
if(document.fonts?.ready)document.fonts.ready.then(()=>updateNavIndicator(false));
const navStackRoot=document.querySelector(".sidebar-nav");
if(navStackRoot && typeof ResizeObserver!=="undefined"){
  new ResizeObserver(()=>updateNavIndicator(false)).observe(navStackRoot);
}
function updateSelectedNav(animate){
  document.querySelectorAll(".sidebar .nav-item,.sidebar .rail-icon-btn").forEach(button=>{
    button.classList.remove("active");
    button.removeAttribute("aria-current");
  });
  let selected;
  const drawer=window.cynRail.getMode();
  if(drawer)selected=document.querySelector('.sidebar .rail-action-group [data-drawer="'+drawer+'"]');
  else selected=document.querySelector('.sidebar [data-view="'+activeView+'"]');
  if(selected){
    selected.classList.add("active");
    selected.setAttribute("aria-current","page");
  }
  updateNavIndicator(animate);
}
function transition(view){
  if(!pages[view])return;
  window.cynRail.close();
  if(view===activeView){updateSelectedNav(true);return;}
  activeView=view;
  updateSelectedNav(true);
  document.querySelectorAll(".content-view").forEach(v=>v.classList.remove("active"));
  $("view-"+view).classList.add("active");
  $("page-title").textContent=pages[view][0];
  $("page-subtitle").textContent=pages[view][1];
  $("main-content").scrollTop=0;
}
document.addEventListener("cyn:drawer-change",()=>updateSelectedNav(true));
$("drawer-open-smart").addEventListener("click",()=>transition("ai"));
// Appearance is routed through the Settings action list in settings.js.
$("drawer-ai-model").addEventListener("change",event=>
  apply({type:"aiModel",value:event.target.value}));
const accentSetting="cyn-ui-accent";
function setAccent(accent){
  const valid=["blue","mint","violet"];
  const choice=valid.includes(accent)?accent:"blue";
  document.body.dataset.accent=choice;
  document.querySelectorAll(".accent-choice").forEach(button=>{
    button.setAttribute("aria-pressed",String(button.dataset.accent===choice));
  });
  localStorage.setItem(accentSetting,choice);
}
document.querySelectorAll(".accent-choice").forEach(button=>
  button.addEventListener("click",()=>setAccent(button.dataset.accent)));
setAccent(localStorage.getItem(accentSetting)||"blue");
document.querySelectorAll("[data-view]").forEach(b=>b.addEventListener("click",()=>transition(b.dataset.view)));
document.querySelectorAll("[data-window]").forEach(b=>b.addEventListener("click",()=>window.cyn.window(b.dataset.window)));
$("go-eq").addEventListener("click",()=>transition("equalizer"));
$("start-native").addEventListener("click",async()=>{
  $("start-native").disabled=true;
  try{const r=await window.cyn.start();if(!r.connected)showError(r.error||"Native CYN is not yet available")}
  catch(e){showError(e.message)}
  finally{$("start-native").disabled=false;await refresh()}
});
$("select-native").addEventListener("click",async()=>{
  try{
    const selected=await window.cyn.selectNative();
    if(selected.selected){$("native-path-state").textContent="Native CYN path selected.";await refresh()}
  }catch(e){showError(e.message)}
});
$("power").addEventListener("change",e=>apply({type:"power",value:e.target.checked}));
$("smart").addEventListener("change",e=>apply({type:"smart",value:e.target.checked}));
$("pin").addEventListener("change",e=>apply({type:"pin",value:e.target.checked}));
$("sensitivity").addEventListener("input",e=>{$("sensitivity-value").textContent=Math.round(Number(e.target.value))+"%"});
$("sensitivity").addEventListener("change",e=>apply({type:"sensitivity",value:Number(e.target.value)}));
$("preset").addEventListener("change",e=>apply({type:"preset",value:e.target.value}));
for(const id of ["device-top","device-page"])
  $(id).addEventListener("change",e=>apply({type:"output",value:e.target.value}));
$("ai-model").addEventListener("change",e=>apply({type:"aiModel",value:e.target.value}));
$("reduced-motion").checked=localStorage.getItem("cyn-riff-reduced-motion")==="1";
document.body.classList.toggle("reduce-motion",$("reduced-motion").checked);
$("reduced-motion").addEventListener("change",e=>{
  document.body.classList.toggle("reduce-motion",e.target.checked);
  localStorage.setItem("cyn-riff-reduced-motion",e.target.checked?"1":"0");
});
renderDisconnected("Checking native CYN installation…");
requestAnimationFrame(()=>updateSelectedNav(false));
refresh();
setInterval(()=>{if(!busy&&!document.hidden)refresh()},4200);
