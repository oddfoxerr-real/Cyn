#!/usr/bin/env python3
"""AUDONIC BrainScan release gate: measured policy channels and mode isolation."""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
src=root/"app/src/main/java/io/github/oddfoxerr/cyn"
main=(src/"MainActivity.java").read_text(encoding="utf-8")
audio=(src/"AudioService.java").read_text(encoding="utf-8")
brain=(src/"ApexSpatialBrainView.java").read_text(encoding="utf-8")
ai=(src/"DesktopAi.java").read_text(encoding="utf-8")
gradle=(root/"app/build.gradle").read_text(encoding="utf-8")

code=re.search(r"^\\s*versionCode\\s+(\\d+)\\s*$",gradle,re.MULTILINE)
name=re.search(r"^\\s*versionName\\s+'(0\\.2\\.\\d+-preview)'\\s*$",gradle,re.MULTILINE)
assert code is not None and name is not None, "Android preview version metadata missing"
assert int(code.group(1))>=30, "AUDONIC BrainScan needs 0.2.26 or newer app identity"
assert "||audonicMind||service.monitor" in main, "AUDONIC scan hidden in Library mode"
assert "Creative BrainScan" in main
assert "new ApexSpatialBrainView" in main
assert "},accent,secondary,audonicMind)" in main
assert "service.audonicVisualTelemetry()" in main
assert "service.audonicIntensity" in main
assert "service.capturedAudioFresh()" in main

assert "DesktopAi.AudonicPolicy policy" in audio
assert "SystemClock.elapsedRealtime()-policy.at<2500" in audio, "Stale decisions must expire"
assert "(!playing&&!capturedAudioFresh())" in audio, "Unsupported Connect must not show live AUDONIC"
assert "return policy!=null" in audio and "?policy.effects:null" in audio
assert "audonicEnabled=AiSelection.AUDONIC.equals(aiProfile)" in audio
assert "final float[] effects;" in ai, "Native ONNX policy remains source of truth"
assert "effects=six.clone()" in ai, "Snapshot must be immutable"

for channel in ("HARMONICS","ATTACK","WIDTH","COLOUR","RHYTHM","OVERTONES"):
    assert channel in brain, f"Missing AUDONIC channel {channel}"
assert "updateCreative(dt,playing,now,reduced)" in brain
assert "decisions=live?source.creative():null" in brain
assert "source.creativeStrength()" in brain
assert "if(audonic)drawCreative(canvas,cx,cy,scale)" in brain
assert "if(external&&!audonic)" in brain, "Do not relabel personal model as AUDONIC"
assert "if(live&&!reduced&&target>.07f" in brain, "No synthetic AUDONIC signals"
assert "postInvalidateDelayed(40)" in brain
assert "if(!reduced)" in brain
assert "Math.floorMod(channel,4)" in brain
assert "source.decisionAt()" in brain
print("PASS AUDONIC six-channel telemetry, freshness, Library/Connect routes, scene isolation and versioning")
