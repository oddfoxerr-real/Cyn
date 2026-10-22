#!/usr/bin/env python3
"""Material 3 BrainScan visual regression without changing DSP or AI inputs."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=root/"app/src/main/java/io/github/oddfoxerr/cyn"
view=(src/"ApexSpatialBrainView.java").read_text(encoding="utf-8")
activity=(src/"MainActivity.java").read_text(encoding="utf-8")
style=(root/"app/src/main/res/values/styles.xml").read_text(encoding="utf-8")

for token in (
    "colorSurfaceContainerLow", "colorSurfaceContainerHigh",
    "colorOnSurface", "colorOnSurfaceVariant", "colorOutlineVariant",
    "colorPrimaryContainer",
):
    assert token in view, f"BrainScan missing Material 3 theme role: {token}"
    assert token in style, f"CYN palette missing required colour role: {token}"
for token in (
    "drawMaterialSurface(canvas,w,h)",
    "drawMaterialHeader(canvas,playing,connected)",
    "drawMaterialFooter(canvas,playing,connected,external,bass,kick,spatial,decision)",
    "canvas.drawRoundRect(stageBounds",
    "canvas.drawRoundRect(tileBounds",
    "canvas.drawRoundRect(tileProgress",
    "CREATIVE_NAMES[i]",
    "materialCreativeHues[i]",
):
    assert token in view, f"BrainScan Material 3 component missing: {token}"
assert "dp(audonicMind?466:368)" in activity, "Material channel cards need allocated space"
assert "6 CREATIVE EFFECTS" in view
# Keep actual model/PCM decisions and no synthetic activity.
for token in (
    "decisions=live?source.creative():null",
    "source.creativeStrength()",
    "if(live&&!reduced&&target>.07f",
    "source.decisionAt()",
    "if(playing)drawWaves(canvas,level)",
    "if(audonic)drawCreative(canvas,cx,cy,scale)",
    "if(!reduced)",
    "postInvalidateDelayed(40)",
):
    assert token in view, f"Telemetered activity or reduced motion regressed: {token}"
assert "int columns=getWidth()<dp(306)?2:3;" in view, "Narrow phone cards must use two columns"
assert "onInitializeAccessibilityNodeInfo" in view, "Channel values need screen-reader access"
assert "info.setContentDescription(detail.toString())" in view, "Real channel levels need accessible labels"
assert "new ApexSpatialBrainView(this" in activity
assert "service.audonicVisualTelemetry()" in activity
print("PASS Material 3 palettes, rounded BrainScan viewport, tonal channel tiles, real telemetry, motion accessibility")
