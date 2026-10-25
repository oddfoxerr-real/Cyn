"use strict";
// Typed allowlist for the existing CYN single-instance command interface.
// No renderer string becomes a shell command.
function n(x,min,max){return typeof x==="number"&&Number.isFinite(x)&&x>=min&&x<=max;}
function assert(s,message){if(!s)throw new TypeError(message);}
function safeName(value){return typeof value==="string"&&value.length>0&&value.length<=160&&!/["\r\n\x00]/.test(value);}
const effectNames=new Set(["clarity","ambience","surround","dynamicboost","bass"]);
function commandFor(action,status){
  assert(action&&typeof action==="object"&&!Array.isArray(action),"Invalid action");
  assert(status&&status.cyn_build==="identity-2","Native CYN status is required");
  switch(action.type){
    case "power": assert(typeof action.value==="boolean","Power must be boolean");return "--power="+(action.value?"1":"0");
    case "smart": assert(typeof action.value==="boolean","Smart listening must be boolean");return "--smart_listening="+(action.value?"1":"0");
    case "pin": assert(typeof action.value==="boolean","Output pin must be boolean");return "--pin_output="+(action.value?"1":"0");
    case "sensitivity": assert(Number.isInteger(action.value)&&n(action.value,0,100),"Sensitivity out of range");return "--smart_sensitivity="+action.value;
    case "automaticUpdates":
      assert(typeof action.value==="boolean","Automatic updates must be boolean");
      return "--cyn_automatic_updates="+(action.value?"1":"0");
    case "launchOnStartup":
      assert(typeof action.value==="boolean","Launch on startup must be boolean");
      return "--cyn_startup="+(action.value?"1":"0");
    case "hotkeysEnabled":
      assert(typeof action.value==="boolean","Keyboard shortcuts must be boolean");
      assert(!action.value||status.hotkeys_supported===true,"Windows keyboard shortcuts unavailable");
      return "--cyn_hotkeys="+(action.value?"1":"0");
    case "headroom":
      assert(Number.isInteger(action.value)&&n(action.value,0,100),"Headroom must be 0–100%");
      return "--smart_headroom="+action.value;
    case "listeningMode":
      assert(Number.isInteger(action.value)&&(action.value===0||action.value===1),"Unknown listening mode");
      assert(status.smart_listening?.training!==true,"Cannot switch AI modes during training");
      return "--smart_mode="+action.value;
    case "presetCycling":
      assert(typeof action.value==="boolean","Preset cycling must be boolean");
      assert(status.smart_listening?.listening_mode===0,"Preset cycling requires Music AI");
      return "--preset_cycling="+(action.value?"1":"0");
    case "audonicIntensity":
      assert(Number.isInteger(action.value)&&n(action.value,0,100),"AUDONIC intensity must be 0–100%");
      assert(status.active_ai_profile==="audonic","AUDONIC model is not selected");
      return "--audonic_intensity="+action.value;
    case "djCommand":
      assert(["download","cancel","remove"].includes(action.value),"Unknown AI DJ command");
      assert(status.ai_dj&&typeof status.ai_dj==="object","Native AI DJ unavailable");
      if(action.value==="download")assert(!status.ai_dj.installed&&!status.ai_dj.busy,"AI DJ is already installed or downloading");
      if(action.value==="remove")assert(status.ai_dj.installed&&!status.ai_dj.busy,"AI DJ cannot be removed now");
      if(action.value==="cancel")assert(status.ai_dj.busy,"AI DJ is not downloading");
      return "--dj_command="+action.value;
    case "djEnable":
      assert(typeof action.value==="boolean","AI DJ enabled state must be boolean");
      assert(status.ai_dj?.installed&&!status.ai_dj?.busy,"AI DJ is not installed or is busy");
      return "--dj_enable="+(action.value?"1":"0");
    case "djIntensity":
      assert(Number.isInteger(action.value)&&n(action.value,0,100),"AI DJ intensity must be 0–100");
      assert(status.ai_dj?.installed,"Install AI DJ first");
      return "--dj_intensity="+action.value;
    case "djSpacing":
      assert(Number.isInteger(action.value)&&n(action.value,4,45),"AI DJ spacing must be 4–45 seconds");
      assert(status.ai_dj?.installed,"Install AI DJ first");
      return "--dj_spacing="+action.value;
    case "djStereo":
      assert(typeof action.value==="boolean","AI DJ stereo must be boolean");
      assert(status.ai_dj?.installed,"Install AI DJ first");
      return "--dj_stereo="+(action.value?"1":"0");
    case "labelRecent": {
      assert(Number.isInteger(action.effect)&&action.effect>=0&&action.effect<5,
        "Choose a valid CYN DSP effect");
      assert(typeof action.more==="boolean","Feedback must be More or Less");
      assert(Number.isInteger(action.newest)&&Number.isInteger(action.oldest)&&
        action.newest>=0&&action.oldest>action.newest&&action.oldest<=60,
        "Select a valid recent audio window");
      assert(status.smart_listening?.training!==true,"Finish training before adding labels");
      return "--label_recent="+action.effect+":"+(action.more?"1":"0")+":"+
        action.newest+":"+action.oldest;
    }
    case "train":
      assert(status.smart_listening?.training!==true,"AI training already running");
      return "--smart_train";
    case "preset": {
      const names=[...(status.presets?.built_in||[]),...(status.presets?.user_defined||[])].map(p=>p.name);
      assert(safeName(action.value)&&names.includes(action.value),"Preset must exist in CYN");
      return '--preset="'+action.value+'"';
    }
    case "saveNew": {
      assert(status.power===true,"Enable native CYN before saving presets");
      assert(typeof action.value==="string"&&/^[^<>:"/\\|?*\x00-\x1f]{1,64}$/.test(action.value)&&
        action.value.trim()===action.value,"Choose a valid preset name (1–64 characters)");
      assert(![...(status.presets?.built_in||[]),...(status.presets?.user_defined||[])]
        .some(p=>p.name.toLowerCase()===action.value.toLowerCase()),"Preset name already exists");
      return '--save_preset="'+action.value+'"';
    }
    case "overwrite":
    case "undo":
    case "rename":
    case "delete": {
      assert(status.power===true,"Enable CYN before modifying presets");
      const all=[...(status.presets?.built_in||[]),...(status.presets?.user_defined||[])];
      const selected=all.find(p=>p.name===status.selected_preset);
      assert(selected,"Active preset is unavailable");
      const own=(status.presets?.user_defined||[]).some(p=>p.name===selected.name);
      if(action.type==="undo"){
        assert(selected.modified===true,"Preset has no changes to undo");
        return "--undo_preset";
      }
      assert(own,"Only your saved presets can be changed");
      if(action.type==="overwrite"){
        assert(selected.modified===true,"Preset has no changes to overwrite");
        return "--overwrite_preset";
      }
      if(action.type==="delete")return "--delete_preset";
      assert(selected.modified!==true,"Save or undo changes before renaming");
      assert(typeof action.value==="string"&&/^[^<>:"/\\|?*\x00-\x1f]{1,64}$/.test(action.value)&&
        action.value.trim()===action.value,"Choose a valid preset name");
      assert(!all.some(p=>p.name.toLowerCase()===action.value.toLowerCase()),"Preset name already exists");
      return '--rename_preset="'+action.value+'"';
    }
    case "output": {
      assert(safeName(action.value)&&(status.output_devices||[]).includes(action.value),"Unknown output device");
      return '--output="'+action.value+'"';
    }
    case "aiModel": {
      const profiles=Array.isArray(status.ai_profiles)?status.ai_profiles:[];
      assert(safeName(action.value)&&profiles.some(p=>p.id===action.value),"AI model is not installed in native CYN");
      return '--select_ai="'+action.value+'"';
    }
    case "effect": {
      assert(effectNames.has(action.name),"Unknown effect");
      assert(n(action.value,0,10),"Effect must be 0–10");
      return "--set_effect="+action.name+":"+action.value.toFixed(2);
    }
    case "band": {
      const bands=status.equalizer?.bands;
      assert(Array.isArray(bands)&&Number.isInteger(action.index)&&action.index>=0&&
        action.index<bands.length,"EQ index out of range");
      assert(n(action.value,-12,12),"EQ gain must be -12 to +12 dB");
      return "--set_band_gain="+action.index+":"+action.value.toFixed(2);
    }
    default: throw new TypeError("Unsupported native action: "+String(action.type));
  }
}
module.exports={commandFor};
