#!/usr/bin/env python3
"""Guard connected-app AI mind mapping against falling back to library-only visuals."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/"app/src/main/java/io/github/oddfoxerr/cyn"
service=(root/"AudioService.java").read_text(encoding="utf-8")
view=(root/"ApexSpatialBrainView.java").read_text(encoding="utf-8")
ui=(root/"MainActivity.java").read_text(encoding="utf-8")
checks={
 "External output-mix FFT produces real neural snapshots":"publishConnectedMind(features,rms,trace,t);" in service,
 "Session-specific FFT produces real neural snapshots":"publishConnectedMind(x,rms,trace,lastCapture);" in service,
 "Connected telemetry is gated by permission and actual live spectrum":"mindExternal()" in service and "hasSessionSpectrum()" in service,
 "Real neural weights are captured":"trace.outputConnections[i]" in service,
 "Real neural activations are captured":"trace.hidden[i]" in service,
 "Monitor remains independent of internal library":"return playing||monitor;" in service,
 "External signal is allowed to animate":"service.mindLive()" in ui,
 "Brain shows when connected, regardless of profile":"||service.monitor" in ui,
 "Neural graph uses weighted connections":"drawNeuralPathways(canvas,source.hidden()" in view,
 "Scanner continuously refreshes on screen":"postInvalidateDelayed(40)" in view,
 "User permission stays in place":"Manifest.permission.RECORD_AUDIO" in service,
 "Technical safety labels distinguish external CYN AI":"CYN NEURAL MIND  /  CONNECTED" in view,
}
for title,valid in checks.items():
    if not valid:raise AssertionError(title)
    print("PASS",title)
