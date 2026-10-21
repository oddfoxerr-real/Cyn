#!/usr/bin/env python3
"""Release smoke checks for APEX 10M's reactive UI and category placement."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]/"app/src/main/java/io/github/oddfoxerr/cyn"
main=(root/"MainActivity.java").read_text(encoding="utf-8")
service=(root/"AudioService.java").read_text(encoding="utf-8")
ai=(root/"DesktopAi.java").read_text(encoding="utf-8")
scan=(root/"ApexSpatialBrainView.java").read_text(encoding="utf-8")
assert "Try frozen" not in main, "Experimental release naming regressed"
assert "APEX 10M · Experimental AI Release" in main
assert "APEX 10M neural control" in main
assert 'if(AiSelection.APEX_10M.equals(aiProfile))partialNeuralPreview=true;' in service
assert "postInvalidateDelayed(40)" in scan and "onAttachedToWindow" in scan
assert 'angle+=dt*(playing?.58f+level*.90f+decision*.38f:.42f)' in scan
assert 'source.decisionAt()' in scan
for observed in ("bass", "kick", "spatial", "decision"):
    assert observed in scan and observed in service, f"No real {observed} channel"
assert 'NativeEngine.spatialPolicyWindow(desktopAi.engine)' in service
assert 'NativeEngine.spatialActivity(desktopAi.engine)' in service
assert 'inferPartial(epoch);' in ai and ai.index('inferPartial(epoch);') < ai.index('float[] wave=NativeEngine.yamnetWindow')
assert 'page=4' in main, "Updates category must remain separate"
assert 'brainScan=new ApexSpatialBrainView(this' in main, "Scan must remain in APEX AI Hub"
print("PASS APEX release identity, telemetry wiring, inference triggers, rotating mesh, category ownership")
