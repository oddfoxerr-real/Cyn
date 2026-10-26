"use strict";
/* Track what CYN last selected without pretending a disconnected device is active.
   Windows Bluetooth/USB enumeration can momentarily return an empty device list,
   even while the native FxSound host is still running. */
(function(root,factory){
  const api=factory();
  if(typeof module==="object"&&module.exports)module.exports=api;
  else root.cynDeviceState=api;
})(typeof window!=="undefined"?window:globalThis,function(){
  function makeTracker(){
    let knownDevices=[],selected="";
    function validName(name){return typeof name==="string"&&name.length>0&&name.length<=256}
    function view(names,choice,confirmed,canSelect){
      const items=names.map(name=>({value:name,label:name}));
      if(choice&&!names.includes(choice))
        items.unshift({value:choice,label:choice+" (temporarily unavailable)"});
      if(!items.length)items.push({value:"",label:"No active output detected"});
      return {items,selected:choice,confirmed,canSelect:Boolean(canSelect),stale:!confirmed};
    }
    function update(status){
      const live=Array.isArray(status?.output_devices)
        ? [...new Set(status.output_devices.filter(validName))]:[];
      const native=validName(status?.selected_output)?status.selected_output:"";
      // A selected endpoint is authoritative only if it appears in the SAME
      // native device snapshot. A blank/partial scan must never erase a known choice.
      const confirmed=Boolean(native&&live.includes(native));
      if(live.length)knownDevices=live;
      // An explicit native engine selection always supersedes a cached UI
      // value. The UI may keep a known name across an empty Windows scan,
      // but may never label JBL as selected when the engine says Speakers.
      if(native)selected=native;
      return view(live.length?live:knownDevices,confirmed?native:selected,confirmed,live.length>0);
    }
    function offline(){return view(knownDevices,selected,false,false)}
    return {update,offline};
  }
  return Object.freeze({makeTracker});
});
