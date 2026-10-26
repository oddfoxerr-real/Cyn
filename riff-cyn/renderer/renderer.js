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
let refreshInFlight=null, actionRevision=0, actionError="", hadNativeSnapshot=false;
const deviceTracker=window.cynDeviceState.makeTracker();
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
  // Never replace a select's children while the user is choosing an item.
  if(document.activeElement===select)return;
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
  const realAppliedLine=svg.querySelector("#eq-ai-applied-curve");
  svg.innerHTML='<defs><linearGradient id="eqfill" x1="0" y1="0" x2="0" y2="1"><stop stop-color="#88baff" stop-opacity=".3"/><stop offset="1" stop-color="#88baff" stop-opacity="0"/></linearGradient><linearGradient id="eqstroke"><stop stop-color="#85b6ff"/><stop offset="1" stop-color="#67e2c5"/></linearGradient></defs>'+
    lines+'<path d="'+closed+'" fill="url(#eqfill)"/><path d="'+path+'" stroke="url(#eqstroke)" stroke-width="3.5" fill="none" stroke-linecap="round" stroke-linejoin="round"/>'+
    list.filter((_,i)=>i%Math.max(1,Math.floor(list.length/12))===0).map(p=>'<circle cx="'+p.x+'" cy="'+p.y+'" r="4" fill="#b8d6ff"/>').join("");
  if(realAppliedLine)svg.append(realAppliedLine);
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
      const live=document.createElement("p");live.className="live-effect-label";
      live.textContent="Native AI applied: ";
      const liveValue=document.createElement("output");liveValue.id="effect-applied-"+name;
      liveValue.textContent="—";live.append(liveValue);
      article.append(h,output,slider,desc,live);box.append(article);
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
      const aiApplied=document.createElement("span");
      aiApplied.id="band-applied-"+b.index;
      aiApplied.className="eq-applied-marker";
      aiApplied.setAttribute("aria-hidden","true");
      aiApplied.style.opacity="0";
      item.append(val,input,frequency,aiApplied);box.append(item);
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
  const eq=status?.equalizer||{};
  const mode=$("eq-band-select");
  mode.disabled=!writable()||!bands.length;
  if(bands.length)mode.value=String(bands.length);
  const advanced=[
    ["eq-balance","balance"],
    ["eq-master-gain","master_gain"],
    ["eq-volume-leveling","volume_leveling"],
    ["eq-filter-q","filter_q"]
  ];
  for(const [id,key] of advanced){
    const control=$(id),output=$(id+"-value");
    control.disabled=!writable()||!bands.length;
    const numeric=Number(eq[key]);
    if(document.activeElement!==control&&Number.isFinite(numeric)){
      control.value=numeric;
      output.textContent=numeric.toFixed(1);
    }
  }
}
function renderConnected(status){
  window.dispatchEvent(new CustomEvent("cyn-native-status",{detail:status}));
  const fx=status.fx_engine;
  const hasNativeEngineReport=fx&&typeof fx.ready==="boolean";
  const ready=hasNativeEngineReport&&fx.ready===true;
  const routed=hasNativeEngineReport&&fx.route_enabled===true;
  const processing=hasNativeEngineReport&&fx.processing===true;
  const virtualActive=hasNativeEngineReport&&fx.virtual_device_active===true;
  const physicalReady=hasNativeEngineReport&&fx.physical_output_ready===true;
  $("fx-engine-detail").textContent=!hasNativeEngineReport
    ?"Original CYN connected; update the native host for Windows audio-routing diagnostics."
    :!virtualActive
      ?"Original FxSound virtual audio driver is missing or disabled. Repair the CYN installation."
      :!physicalReady
        ?"Select an active speaker or headphone output. CYN cannot forward audio without one."
        :!status.power
          ?"Original FxSound DSP is switched off. Restore system audio to enable it."
          :!routed
            ?"Windows audio is bypassing CYN. Restore system audio to select the original FxSound virtual device."
            :processing
              ?"Original FxSound DSP is processing live Windows audio."
              :"Windows is routed through CYN. Waiting for sound from an app.";
  $("fx-engine-output").textContent="Playback: "+String(status.selected_output||"Not selected")+
    " · Windows via CYN: "+(routed?"Yes":"No");
  $("fx-engine-route").hidden=!hasNativeEngineReport;
  $("fx-engine-route").disabled=!writable()||!virtualActive||!physicalReady||Boolean(routed);
  $("fx-engine-reconnect").textContent=ready?"Refresh engine":"Reconnect engine";
  $("fx-engine-reconnect").disabled=false;
  setEngineState(true,(routed?"ROUTED":ready?"NOT ROUTED":"DRIVER ISSUE")+" / v"+String(status.version||"?"));
  // Green status means Windows is really routed via FxSound, not merely
  // that Electron can reach a native process.
  $("engine-state").classList.toggle("on",routed);
  $("rail-led").classList.toggle("on",routed);
  if(typeof status.power==="boolean")$("power").checked=status.power;
  $("smart").checked=Boolean(status.smart_listening?.enabled);
  $("pin").checked=Boolean(status.output_pinned);
  $("sensitivity").value=Number(status.smart_listening?.sensitivity)||0;
  $("sensitivity-value").textContent=Math.round(Number(status.smart_listening?.sensitivity)||0)+"%";
  const smartMeta=status.smart_listening||{};
  $("listening-mode").value=String(smartMeta.listening_mode===1?1:0);
  $("listening-mode").disabled=!writable()||Boolean(smartMeta.training);
  $("headroom").value=Number(smartMeta.headroom_cap)||0;
  $("headroom-value").textContent=Math.round(Number(smartMeta.headroom_cap)||0)+"%";
  $("headroom").disabled=!writable();
  $("preset-cycling").checked=Boolean(smartMeta.preset_cycling);
  $("preset-cycling").disabled=!writable()||smartMeta.listening_mode===1;
  $("audonic-intensity").value=Number(smartMeta.audonic_intensity)||0;
  $("audonic-intensity-value").textContent=Math.round(Number(smartMeta.audonic_intensity)||0)+"%";
  $("audonic-intensity").disabled=!writable()||status.active_ai_profile!=="audonic";
  $("training-fill").style.width=(Math.max(0,Math.min(1,Number(smartMeta.training_progress)||0))*100).toFixed(1)+"%";
  $("train-ai").disabled=!writable()||Boolean(smartMeta.training);
  $("training-status").textContent=smartMeta.training?"Native CYN is learning from saved feedback":"Training ready";
  for(const id of ["feedback-more","feedback-less"])$(id).disabled=!writable()||Boolean(smartMeta.training);
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
  const view=deviceTracker.update(status);
  for(const select of [$("device-top"),$("device-page")]){
    opts(select,view.items,view.selected);
    select.disabled=!writable()||!view.canSelect;
  }
  if(view.stale&&view.selected){
    $("fx-engine-output").textContent="Last selected output: "+view.selected+
      " · Device connection not yet confirmed by Windows";
  }
  for(const id of ["preset","power","smart","pin","sensitivity"])
    $(id).disabled=!writable();
  const dj=status.ai_dj||{};
  $("dj-status").textContent=String(dj.status||"Native AI DJ unavailable");
  $("dj-enable").checked=Boolean(dj.enabled);
  $("dj-enable").disabled=!writable()||!dj.installed||Boolean(dj.busy);
  $("dj-download").hidden=Boolean(dj.installed)||Boolean(dj.busy);
  $("dj-download").disabled=!writable()||Boolean(dj.installed)||Boolean(dj.busy);
  $("dj-cancel").hidden=!dj.busy;
  $("dj-remove").hidden=!dj.installed||Boolean(dj.busy);
  $("dj-progress-track").hidden=!dj.busy;
  $("dj-progress-fill").style.width=(Math.max(0,Math.min(1,Number(dj.progress)||0))*100).toFixed(1)+"%";
  $("dj-recent").textContent=String(dj.last_effect||"AI DJ ready")+" · "+String(dj.remixes||0)+" remixes";
  $("dj-intensity").value=Math.round((Number(dj.intensity)||0)*100);
  $("dj-intensity-value").textContent=Math.round((Number(dj.intensity)||0)*100)+"%";
  $("dj-spacing").value=Math.round(Number(dj.spacing)||18);
  $("dj-spacing-value").textContent=$("dj-spacing").value+" s";
  $("dj-stereo").checked=Boolean(dj.stereo);
  for(const id of ["dj-intensity","dj-spacing","dj-stereo"])$(id).disabled=!writable()||!dj.installed||Boolean(dj.busy);
  renderEffects(status);
  renderBands(status);
}
function renderDisconnected(message){
  window.dispatchEvent(new CustomEvent("cyn-native-status",{detail:null}));
  $("fx-engine-detail").textContent=String(message||"Native CYN audio engine is offline.");
  $("fx-engine-output").textContent="Audio effects and neural processing require the real Cyn.exe engine.";
  $("fx-engine-route").hidden=true;
  $("fx-engine-route").disabled=true;
  $("fx-engine-reconnect").textContent="Reconnect engine";
  $("fx-engine-reconnect").disabled=false;
  setEngineState(false,"Engine offline");
  for(const id of ["preset","device-top","device-page","power","smart","pin","sensitivity","ai-model"])$(id).disabled=true;
  for(const id of ["listening-mode","headroom","preset-cycling","audonic-intensity","train-ai","feedback-more","feedback-less"])$(id).disabled=true;
  $("training-fill").style.width="0%";
  $("training-status").textContent="Connect native CYN for training";
  $("dj-status").textContent="Native AI DJ unavailable";
  $("dj-recent").textContent="Connect CYN to read AI DJ status";
  for(const id of ["dj-enable","dj-download","dj-remove","dj-cancel","dj-intensity","dj-spacing","dj-stereo"])$(id).disabled=true;
  $("feedback-result").textContent="Connect CYN to label recent audio activity";
  $("smart-summary").textContent=String(message||"Connect native CYN to access processing");
  $("active-model").textContent="Not connected";
  $("learning-status").textContent="Start native CYN to read its actual learning state.";
  $("sensitivity-value").textContent="—";
  opts($("preset"),[{value:"",label:"Connect CYN"}],"");
  const lastKnown=deviceTracker.offline();
  for(const id of ["device-top","device-page"]){
    if(hadNativeSnapshot)opts($(id),lastKnown.items,lastKnown.selected);
    else opts($(id),[{value:"",label:"Connect CYN"}],"");
    $(id).disabled=true;
  }
  for(const id of ["ai-model","drawer-ai-model"])opts($(id),[{value:"",label:"Unavailable"}],"");
  $("drawer-ai-model").disabled=true;
  $("drawer-ai-summary").textContent="Start CYN to load installed AI models.";
  window.cynRail.setPresets(null,()=>{});
  if(!hadNativeSnapshot){
    renderEffects(null);
    renderBands(null);
  }else{
    // A momentary IPC failure is not permission to erase the user's 39-band
    // EQ, selected devices or knob positions. Disable editing until verified.
    for(const [name] of fx){
      const slider=$("effect-"+name);if(slider)slider.disabled=true;
    }
    for(const input of $("eq-sliders").querySelectorAll("input"))input.disabled=true;
    for(const id of ["eq-band-select","eq-balance","eq-master-gain","eq-volume-leveling","eq-filter-q"])
      $(id).disabled=true;
  }
}
async function refresh(){
  if(busy)return;
  if(refreshInFlight)return refreshInFlight;
  const revision=actionRevision;
  const work=(async()=>{
    try{
      const result=await window.cyn.status();
      // A pre-click poll must never overwrite the user's subsequent output
      // or power change while the native command is being applied.
      if(busy||revision!==actionRevision)return;
      current=result;
      if(result.connected&&result.status?.cyn_build==="identity-2"){
        hadNativeSnapshot=true;
        renderConnected(result.status);
        if(!actionError)showError("");
      }else renderDisconnected(result.error||"CYN's native status could not be verified.");
    }catch(error){
      if(busy||revision!==actionRevision)return;
      current={connected:false,error:error.message};
      renderDisconnected(error.message);
    }
  })();
  refreshInFlight=work;
  try{return await work}
  finally{if(refreshInFlight===work)refreshInFlight=null}
}
async function apply(action){
  if(!writable())return false;
  ++actionRevision; // invalidate an in-flight read before changing Windows audio
  busy=true;
  let succeeded=false;
  try{
    const result=await window.cyn.apply(action);
    if(!result.connected||!result.status)throw new Error(result.error||"Native audio command did not complete");
    // The command process exiting only means Windows received the CLI command.
    // Verify the actual native response before accepting a power/output change.
    if(action.type==="power"&&result.status.power!==action.value)
      throw new Error("CYN did not confirm the processing switch. Check the native engine and audio driver.");
    if(action.type==="output"&&result.status.selected_output!==action.value)
      throw new Error("Windows did not confirm the selected output. The previous device is preserved.");
    current=result;
    succeeded=true;
    actionError="";
    showError("");
  }catch(error){
    actionError=String(error.message||error);
    showError(actionError);
  }finally{
    busy=false;
    if(refreshInFlight)await refreshInFlight;
    await refresh();
    if(actionError)showError(actionError);
  }
  return succeeded;
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
$("toolbar-settings").addEventListener("click",()=>transition("settings"));
$("go-eq").addEventListener("click",()=>transition("equalizer"));
$("fx-engine-classic").addEventListener("click",()=>{
  window.cyn.openNative().catch(error=>showError("Could not open original CYN audio controls: "+error.message));
});
$("fx-engine-repair").addEventListener("click",()=>{
  window.cyn.openLink("updates").catch(error=>showError("Could not open the CYN installer page: "+error.message));
});
$("fx-engine-route").addEventListener("click",async()=>{
  const control=$("fx-engine-route");
  control.disabled=true;
  $("fx-engine-detail").textContent="Restoring original FxSound Windows audio routing…";
  const result=await apply({type:"restoreAudio"});
  if(result) {
    const report=current?.status?.fx_engine;
    if(report?.virtual_device_active!==true||report?.physical_output_ready!==true)
      showError("CYN audio device is unavailable. Use Repair install or select a connected output.");
    // Audio endpoint and actual DSP activity are confirmed on the next
    // native status refresh, never simulated from a successful button click.
  }
  await refresh();
});
$("fx-engine-reconnect").addEventListener("click",async()=>{
  const control=$("fx-engine-reconnect");
  control.disabled=true;
  $("fx-engine-detail").textContent="Connecting to the native CYN engine…";
  try{
    const result=await window.cyn.start();
    if(!result.connected)showError(result.error||"Native FX engine could not start");
  }catch(error){showError("Native FX engine: "+error.message)}
  finally{control.disabled=false;await refresh()}
});
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
$("eq-band-select").addEventListener("change",e=>apply({type:"eqBandCount",value:Number(e.target.value)}));
for(const [id,type] of [
  ["eq-balance","balance"],["eq-master-gain","masterGain"],
  ["eq-volume-leveling","volumeLeveling"],["eq-filter-q","filterQ"]
]){
  $(id).addEventListener("input",event=>{
    $(id+"-value").textContent=Number(event.target.value).toFixed(1);
  });
  $(id).addEventListener("change",event=>apply({type,value:Number(event.target.value)}));
}
$("listening-mode").addEventListener("change",e=>apply({type:"listeningMode",value:Number(e.target.value)}));
$("headroom").addEventListener("input",e=>$("headroom-value").textContent=Math.round(Number(e.target.value))+"%");
$("headroom").addEventListener("change",e=>apply({type:"headroom",value:Number(e.target.value)}));
$("preset-cycling").addEventListener("change",e=>apply({type:"presetCycling",value:e.target.checked}));
$("audonic-intensity").addEventListener("input",e=>$("audonic-intensity-value").textContent=Math.round(Number(e.target.value))+"%");
$("audonic-intensity").addEventListener("change",e=>apply({type:"audonicIntensity",value:Number(e.target.value)}));
$("train-ai").addEventListener("click",()=>apply({type:"train"}));
$("dj-download").addEventListener("click",()=>apply({type:"djCommand",value:"download"}));
$("dj-cancel").addEventListener("click",()=>apply({type:"djCommand",value:"cancel"}));
$("dj-remove").addEventListener("click",()=>apply({type:"djCommand",value:"remove"}));
$("dj-enable").addEventListener("change",e=>apply({type:"djEnable",value:e.target.checked}));
$("dj-intensity").addEventListener("input",e=>$("dj-intensity-value").textContent=Math.round(Number(e.target.value))+"%");
$("dj-intensity").addEventListener("change",e=>apply({type:"djIntensity",value:Number(e.target.value)}));
$("dj-spacing").addEventListener("input",e=>$("dj-spacing-value").textContent=Math.round(Number(e.target.value))+" s");
$("dj-spacing").addEventListener("change",e=>apply({type:"djSpacing",value:Number(e.target.value)}));
$("dj-stereo").addEventListener("change",e=>apply({type:"djStereo",value:e.target.checked}));
function rangeLabel(){
  const newer=$("feedback-from"),older=$("feedback-to");
  if(Number(newer.value)>=Number(older.value)){
    if(document.activeElement===newer)older.value=Math.min(60,Number(newer.value)+1);
    else newer.value=Math.max(0,Number(older.value)-1);
  }
  $("feedback-from-value").textContent=newer.value+" s";
  $("feedback-to-value").textContent=older.value+" s";
}
for(const id of ["feedback-from","feedback-to"])$(id).addEventListener("input",rangeLabel);
for(const [id,more] of [["feedback-more",true],["feedback-less",false]]){
  $(id).addEventListener("click",async()=>{
    $("feedback-result").textContent="Saving private CYN feedback…";
    const success=await apply({type:"labelRecent",effect:Number($("feedback-effect").value),
      more,newest:Number($("feedback-from").value),oldest:Number($("feedback-to").value)});
    const response=String(current?.status?.smart_listening?.learning||"");
    $("feedback-result").textContent=success
      ?(response||"Native CYN processed the label request")
      :"CYN could not save that feedback";
  });
}
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
