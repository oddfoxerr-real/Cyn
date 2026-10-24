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
    case "preset": {
      const names=[...(status.presets?.built_in||[]),...(status.presets?.user_defined||[])].map(p=>p.name);
      assert(safeName(action.value)&&names.includes(action.value),"Preset must exist in CYN");
      return '--preset="'+action.value+'"';
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
