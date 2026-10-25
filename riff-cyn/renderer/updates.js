/* CYN Material updater: read-only release check in the interface, followed by
 * the ORIGINAL native CYN WPF updater for verified downloads and installation.
 * Electron never downloads an installer or changes the native DSP. */
"use strict";
(() => {
  const $=id=>document.getElementById(id);
  const dialog=$("cyn-updates-dialog");
  const title=$("cyn-updates-title");
  const detail=$("cyn-updates-detail");
  const current=$("cyn-updates-current");
  const latest=$("cyn-updates-latest");
  const retry=$("cyn-updates-retry");
  const install=$("cyn-updates-install");
  const done=$("cyn-updates-done");
  const close=$("cyn-updates-close");
  let generation=0,checking=false,openingInstaller=false,available=false;
  const setPhase=(phase,heading,message)=>{
    dialog.dataset.phase=phase;
    title.textContent=heading;
    detail.textContent=message;
  };
  const allowClose=()=>!openingInstaller;
  function dismiss(){
    if(allowClose()&&dialog.open){++generation;dialog.close()}
  }
  done.addEventListener("click",dismiss);
  close.addEventListener("click",dismiss);
  dialog.addEventListener("cancel",event=>{
    if(!allowClose()){event.preventDefault();return}
    ++generation;
  });
  dialog.addEventListener("close",()=>{++generation});
  async function inspect(){
    if(checking||openingInstaller)return;
    const token=++generation;
    checking=true;available=false;
    current.textContent="—";latest.textContent="—";
    retry.hidden=true;install.hidden=true;
    setPhase("checking","Checking for updates","Contacting CYN's official Windows GitHub releases…");
    try{
      const data=await window.cyn.inspectUpdates();
      if(token!==generation||!dialog.open)return;
      current.textContent="v"+data.currentVersion;
      latest.textContent="v"+data.latestVersion;
      if(data.phase==="available"){
        available=true;
        setPhase("available","Update available","CYN "+data.latestVersion+" is ready. Download and install it using the original secure CYN updater.");
        install.hidden=false;
        install.disabled=false;
      }else if(data.phase==="current"){
        setPhase("current","You're up to date","CYN "+data.currentVersion+" is the latest stable Windows release.");
      }else throw new Error("Unexpected updater response");
    }catch(error){
      if(token!==generation||!dialog.open)return;
      setPhase("error","Update check failed",String(error.message||"CYN could not check for updates."));
      retry.hidden=false;
    }finally{checking=false}
  }
  async function startInstall(){
    if(openingInstaller||!available||dialog.dataset.phase!=="available")return;
    openingInstaller=true;
    retry.hidden=true;
    install.disabled=true;
    setPhase("opening","Opening original CYN updater","The native updater handles real download progress, SHA-256 verification, safe audio shutdown, installation and restart.");
    try{
      const result=await window.cyn.checkUpdates();
      if(!result?.started)throw new Error("The original CYN updater did not confirm it started.");
      openingInstaller=false;
      dismiss();
    }catch(error){
      openingInstaller=false;
      setPhase("error","Updater couldn't start",String(error.message||"Try the official release installer."));
      retry.hidden=false;
      install.hidden=false;
      install.disabled=false;
    }
  }
  retry.addEventListener("click",inspect);
  install.addEventListener("click",startInstall);
  function open(){
    if(!dialog.open)dialog.showModal();
    inspect();
  }
  window.cynUpdate=Object.freeze({open});
})();
