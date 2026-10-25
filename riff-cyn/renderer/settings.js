"use strict";
(() => {
  const $=id=>document.getElementById(id);
  const items=[...document.querySelectorAll("[data-setting]")];
  const dialog=$("settings-dialog"),form=$("settings-dialog-form"),
    name=$("settings-dialog-name"),hint=$("settings-dialog-hint"),
    title=$("settings-dialog-title"),accept=$("settings-dialog-accept");
  let selectedAction=null,busy=false;
  const notify=(message)=>{
    const banner=$("error-banner");
    if(!banner)return;
    banner.textContent=message;
    banner.hidden=!message;
    if(message)banner.scrollIntoView({block:"nearest",behavior:"instant"});
  };
  const getStatus=async()=>{
    const result=await window.cyn.status();
    if(!result.connected)throw new Error(result.error||"Start native CYN before editing presets");
    return result.status;
  };
  const apply=async(action)=>{
    const result=await window.cyn.apply(action);
    if(!result.connected)throw new Error(result.error||"Native CYN did not confirm the change");
    return result.status;
  };
  async function ask(action){
    selectedAction=action;
    const destructive=action==="delete",rename=action==="rename";
    title.textContent=({saveNew:"Save New Preset",rename:"Rename Preset",delete:"Delete Preset"})[action];
    hint.textContent=destructive
      ?"Delete the selected user preset? The preset cannot be restored."
      :rename?"Choose a new name for the selected user preset.":"Save the current sound as a new user preset.";
    $("settings-dialog-label").hidden=destructive;
    name.hidden=destructive;
    name.required=!destructive;
    name.value="";
    accept.textContent=destructive?"Delete":rename?"Rename":"Save";
    dialog.showModal();
    if(!destructive)name.focus();else accept.focus();
  }
  function reduced(){
    return localStorage.getItem("cyn-riff-reduced-motion")==="1"||
      window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  }
  const smooth=$("smooth-animations");
  smooth.checked=localStorage.getItem("cyn-riff-reduced-motion")!=="1";
  smooth.addEventListener("change",()=>{
    const on=!smooth.checked;
    localStorage.setItem("cyn-riff-reduced-motion",on?"1":"0");
    const actual=$("reduced-motion");actual.checked=on;
    document.body.classList.toggle("reduce-motion",on);
  });
  const nativePrefs=[
    ["automatic-updates","automaticUpdates","automatic_updates"],
    ["launch-on-startup","launchOnStartup","launch_on_startup"],
    ["hotkeys-enabled","hotkeysEnabled","hotkeys_enabled"]
  ];
  const syncNativePreferences=status=>{
    for(const [id,,property] of nativePrefs){
      const control=$(id);
      control.checked=Boolean(status?.[property]);
      control.disabled=!status||(id==="hotkeys-enabled"&&!status.hotkeys_supported);
    }
  };
  window.addEventListener("cyn-native-status",event=>syncNativePreferences(event.detail));
  for(const [id,type] of nativePrefs){
    const control=$(id);
    control.addEventListener("change",async()=>{
      const selected=control.checked;
      control.disabled=true;
      try{
        const current=await apply({type,value:selected});
        syncNativePreferences(current);
        if(id==="automatic-updates"&&selected)
          notify("Automatic update checks enabled. CYN will check for verified releases.");
      }catch(e){
        control.checked=!selected;
        notify(e.message);
      }finally{
        if(id!=="hotkeys-enabled")control.disabled=false;
        else {
          try{const result=await window.cyn.status();syncNativePreferences(result.connected?result.status:null)}
          catch{control.disabled=true}
        }
      }
    });
  }
  const onTop=$("always-on-top");
  onTop.addEventListener("change",async()=>{
    try{onTop.checked=await window.cyn.alwaysOnTop(onTop.checked)}
    catch(e){onTop.checked=!onTop.checked;notify(e.message)}
  });
  const actions={
    "save-new":()=>ask("saveNew"),
    overwrite:async()=>{
      const st=await getStatus(),preset=st.selected_preset;
      if(!(st.presets?.user_defined||[]).some(p=>p.name===preset))throw new Error("Select a user preset before overwriting");
      await apply({type:"overwrite"});notify("Preset overwritten successfully.");
    },
    undo:async()=>{await apply({type:"undo"});notify("Preset changes undone.");},
    rename:()=>ask("rename"),
    delete:()=>ask("delete"),
    native:()=>window.cyn.openNative(),
    mobile:()=>window.cyn.openLink("mobile"),
    updates:()=>window.cyn.checkUpdates(),
    theme:()=>window.cynRail.open("appearance"),
    credits:()=>window.cyn.openLink("credits"),
    support:()=>window.cyn.openLink("support"),
    exit:()=>window.cyn.window("close")
  };
  items.forEach(button=>{
    const action=actions[button.dataset.setting];
    if(!action)return;
    button.addEventListener("click",async()=>{
      if(busy)return;
      busy=true;button.disabled=true;
      try{await action()}catch(e){notify(e.message)}
      finally{busy=false;button.disabled=false}
    });
  });
  $("settings-dialog-cancel").addEventListener("click",()=>dialog.close("cancel"));
  form.addEventListener("submit",async event=>{
    event.preventDefault();
    if(!selectedAction)return;
    const action={type:selectedAction};
    if(selectedAction!=="delete")action.value=name.value.trim();
    accept.disabled=true;
    try{
      await apply(action);
      dialog.close("success");
      notify(({saveNew:"New preset saved.",rename:"Preset renamed.",delete:"Preset deleted."})[selectedAction]);
    }catch(e){hint.textContent=e.message;hint.setAttribute("role","alert")}
    finally{accept.disabled=false}
  });
  dialog.addEventListener("close",()=>{selectedAction=null;hint.removeAttribute("role")});
  // Exit animation is triggered for both the title-bar X and Windows close.
  // No timeout in the host process: the renderer explicitly acknowledges completion.
  let closing=false;
  window.cyn.onClosing(()=>{
    if(closing)return;closing=true;
    window.cynRail.close();
    const finish=()=>window.cyn.closeComplete();
    if(reduced()){finish();return;}
    document.body.classList.add("cyn-closing");
    document.documentElement.style.pointerEvents="none";
    const veil=document.createElement("div");
    veil.className="cyn-close-veil";veil.setAttribute("aria-hidden","true");
    document.body.append(veil);
    const animation=veil.animate([
      {clipPath:"circle(0% at 97% 3%)",opacity:.25},
      {clipPath:"circle(150% at 97% 3%)",opacity:1}
    ],{duration:420,easing:"cubic-bezier(.15,.8,.22,1)",fill:"forwards"});
    animation.finished.then(finish,finish);
  });
})();
