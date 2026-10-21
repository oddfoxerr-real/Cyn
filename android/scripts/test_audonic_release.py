#!/usr/bin/env python3
"""Guard experimental AUDONIC release wiring before signing CYN mobile APK."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/"app/src"
src=root/"main/java/io/github/oddfoxerr/cyn"
native=root/"main/cpp"
main=(src/"MainActivity.java").read_text()
service=(src/"AudioService.java").read_text()
neural=(src/"DesktopAi.java").read_text()
selection=(src/"AiSelection.java").read_text()
engine=(src/"NativeEngine.java").read_text()
processor=(native/"phone_engine.cpp").read_text()
header=(native/"AudonicMischief.h").read_text()
checks={
    "Fourth mutually exclusive profile":'static final String AUDONIC = "audonic";' in selection,
    "Experimental AI picker":'"AUDONIC · Neural Mischief AI (Experimental)"' in main,
    "Creative intensity slider":'slider(engine,"Creative intensity"' in main,
    "Saved intensity":'putFloat("audonic_intensity",audonicIntensity)' in service,
    "Primary engine selection":'AiSelection.AUDONIC.equals(aiProfile)' in service,
    "Background neural inference":'private void inferAudonic(long epoch)' in neural,
    "Verified model asset":'models/audonic-manifest.json' in neural,
    "Strict checksum":'audonicSession=open("audonic-student.onnx",hash)' in neural,
    "Realtime isolation":'inferAudonic(epoch);' in neural,
    "Independent policy expiry":'SystemClock.elapsedRealtime()-decision.at<2500' in service,
    "Fail-closed DSP controls":'NativeEngine.audonicControls(desktopAi.engine,creative,audonicReady)' in service,
    "Device runtime control":'static native void audonicControls(' in engine,
    "Native C++ processing":'audonic.process(out[0],out[1],controls);' in processor,
    "No ONNX in audio thread":'onnx' not in header.lower(),
    "AI map active":'AiSelection.AUDONIC.equals(service.aiProfile)' in main,
    "Safety code present":'float widthGate=' in header and 'std::tanh' in header,
}
for label,ok in checks.items():
    if not ok:raise AssertionError(label)
    print("PASS",label)
