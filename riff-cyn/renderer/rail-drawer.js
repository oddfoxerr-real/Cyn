/* CYN dock drawer — adapted from rootscripts/riff/app.js (MIT).
 * Source: https://github.com/rootscripts/riff
 * Carries Riff's open/close, 120ms section switch, 200ms exit motion,
 * original rail CSS class contract, and click-away/Escape semantics.
 * The media-player/playlist dependencies have been replaced with native
 * CYN controls; no fake playlists or audio sessions are started. */
"use strict";
(() => {
  const root=document.getElementById("rail-drawer");
  const sidebar=document.getElementById("sidebar");
  const title=document.getElementById("rail-drawer-title");
  const closeButton=document.getElementById("btn-close-rail-drawer");
  const buttons=[...document.querySelectorAll("[data-drawer]")];
  const sections=Object.fromEntries(["presets","ai","appearance"].map(mode=>
    [mode,document.getElementById("drawer-section-"+mode)]));
  const presetBox=document.getElementById("presets-container");
  let openDrawerState=null,activeDrawerMode=null;
  let railDrawerCloseTimeout=null,switchTimer=null,sequence=0;
  const names={presets:"Presets",ai:"AI Hub",appearance:"Appearance"};
  const reduce=()=>document.body.classList.contains("reduce-motion")||
    window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  function syncButtonState(){
    buttons.forEach(b=>{
      const active=b.dataset.drawer===openDrawerState;
      b.classList.toggle("active",active);
      b.setAttribute("aria-expanded",String(active));
    });
    const active=buttons.find(b=>b.dataset.drawer===openDrawerState);
    buttons.forEach(b=>{if(!active||b!==active)b.removeAttribute("aria-current")});
    if(active)active.setAttribute("aria-current","page");
    document.dispatchEvent(new CustomEvent("cyn:drawer-change",{detail:{mode:openDrawerState}}));
  }
  function setSection(mode){
    for(const [key,section] of Object.entries(sections)){
      section.style.display=key===mode?"flex":"none";
      section.classList.remove("section-switching-in","section-switching-out");
    }
    title.textContent=names[mode];
    root.classList.toggle("drawer-presets-mode",mode==="presets");
  }
  function applyDrawerState(){
    if(!root)return;
    sequence++;
    const snapshot=sequence;
    if(railDrawerCloseTimeout){clearTimeout(railDrawerCloseTimeout);railDrawerCloseTimeout=null}
    if(switchTimer){clearTimeout(switchTimer);switchTimer=null}
    syncButtonState();
    if(!openDrawerState){
      if(activeDrawerMode!==null){
        activeDrawerMode=null;
        sidebar.classList.remove("has-drawer-open");
        root.classList.remove("open");
        root.setAttribute("aria-hidden","true");
        railDrawerCloseTimeout=setTimeout(()=>{
          if(sequence===snapshot)root.classList.add("hidden");
          railDrawerCloseTimeout=null;
        },reduce()?0:200);
      }
      return;
    }
    const isNew=activeDrawerMode===null;
    const isChanged=!isNew&&activeDrawerMode!==openDrawerState;
    const oldMode=activeDrawerMode;
    activeDrawerMode=openDrawerState;
    root.classList.remove("hidden");
    root.setAttribute("aria-hidden","false");
    sidebar.classList.add("has-drawer-open");
    if(isNew){
      setSection(openDrawerState);
      // Read the layout before applying the Riff 'open' class to animate entry.
      void root.offsetWidth;
      root.classList.add("open");
    }else if(isChanged){
      const outgoing=sections[oldMode],incoming=sections[openDrawerState];
      if(outgoing)outgoing.classList.add("section-switching-out");
      const target=openDrawerState;
      switchTimer=setTimeout(()=>{
        if(sequence!==snapshot)return;
        setSection(target);
        if(incoming){
          incoming.classList.add("section-switching-in");
          setTimeout(()=>{if(sequence===snapshot)incoming.classList.remove("section-switching-in")},reduce()?0:200);
        }
        switchTimer=null;
      },reduce()?0:120);
    }
  }
  function open(mode){
    if(!(mode in sections))throw new TypeError("Unknown CYN drawer");
    openDrawerState=mode;applyDrawerState();
  }
  function close(){openDrawerState=null;applyDrawerState()}
  function toggle(mode){if(mode===openDrawerState)close();else open(mode)}
  buttons.forEach(button=>button.addEventListener("click",event=>{
    event.stopPropagation();toggle(button.dataset.drawer);
  }));
  closeButton.addEventListener("click",close);
  document.addEventListener("keydown",event=>{
    if(event.key==="Escape"&&openDrawerState){
      event.preventDefault();close();
    }
  });
  document.addEventListener("pointerdown",event=>{
    if(!openDrawerState)return;
    if(root.contains(event.target)||buttons.some(b=>b.contains(event.target)))return;
    close();
  });
  let presetSignature="",presetSelect=()=>{};
  function setPresets(status,onSelect){
    presetSelect=onSelect;
    const presets=[...(status?.presets?.built_in||[]),...(status?.presets?.user_defined||[])];
    const signature=JSON.stringify({presets,selected:status?.selected_preset,power:status?.power});
    if(signature===presetSignature)return;
    presetSignature=signature;
    presetBox.replaceChildren();
    const badge=document.getElementById("presets-count");
    badge.hidden=presets.length===0;
    badge.textContent=presets.length>99?"99+":String(presets.length);
    for(const item of presets){
      const button=document.createElement("button");
      button.type="button";
      button.className="preset-chip";
      button.textContent=item.name+(item.modified?" *":"");
      button.disabled=!status?.power;
      const active=item.name===status?.selected_preset;
      button.classList.toggle("active",active);
      button.setAttribute("aria-pressed",String(active));
      button.addEventListener("click",()=>presetSelect(item.name));
      presetBox.append(button);
    }
    if(!presets.length){
      const empty=document.createElement("p");
      empty.className="drawer-helper";
      empty.textContent=status?"No native presets found.":"Connect CYN to load your presets.";
      presetBox.append(empty);
    }
  }
  window.cynRail=Object.freeze({open,close,toggle,getMode:()=>openDrawerState,setPresets});
})();
