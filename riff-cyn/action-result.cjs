"use strict";
// Pure verification of CYN's authoritative native status after a UI action.
// Never show success merely because a secondary command process exited.
const near=(actual,expected,tolerance=0.15)=>
  actual!==undefined&&actual!==null&&Number.isFinite(Number(actual))&&
  Math.abs(Number(actual)-Number(expected))<=tolerance;
function actionConfirmed(action,status){
  if(!status||status.cyn_build!=="identity-2")return false;
  const smart=status.smart_listening||{},eq=status.equalizer||{},dj=status.ai_dj||{};
  switch(action.type){
    case "power":return status.power===action.value;
    case "output":return status.selected_output===action.value &&
      (status.power!==true||status.fx_engine?.physical_output_targeted===true);
    case "effect":return near(status.effects?.[action.name],action.value);
    case "band":return near(eq.bands?.[action.index]?.gain,action.value);
    case "eqBandCount":return Array.isArray(eq.bands)&&eq.bands.length===action.value;
    case "balance":return near(eq.balance,action.value);
    case "masterGain":return near(eq.master_gain,action.value);
    case "volumeLeveling":return near(eq.volume_leveling,action.value);
    case "filterQ":return near(eq.filter_q,action.value);
    case "smart":return smart.enabled===action.value;
    case "sensitivity":return near(smart.sensitivity,action.value,1);
    case "headroom":return near(smart.headroom_cap,action.value,1);
    case "audonicIntensity":return near(smart.audonic_intensity,action.value,1);
    case "listeningMode":return smart.listening_mode===action.value;
    case "presetCycling":return smart.preset_cycling===action.value;
    case "pin":return status.output_pinned===action.value;
    case "preset":return status.selected_preset===action.value;
    case "aiModel":return status.active_ai_profile===action.value;
    case "djEnable":return dj.enabled===action.value;
    case "djIntensity":return near(Number(dj.intensity)*100,action.value,1);
    case "djSpacing":return near(dj.spacing,action.value,1);
    case "djStereo":return dj.stereo===action.value;
    case "automaticUpdates":return status.automatic_updates===action.value;
    case "launchOnStartup":return status.launch_on_startup===action.value;
    case "hotkeysEnabled":return status.hotkeys_enabled===action.value;
    // Long-running training, downloads and preset-management commands have
    // asynchronous/native-specific completion semantics; they still require a
    // connected, freshly read native snapshot.
    default:return true;
  }
}
module.exports={actionConfirmed};
