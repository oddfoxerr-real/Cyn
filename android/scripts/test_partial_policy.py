#!/usr/bin/env python3
"""Check frozen 37.33% ONNX model is real inference and has documented shapes."""
import hashlib
import json
from pathlib import Path

import numpy as np
import onnxruntime as ort

root = Path(__file__).resolve().parents[1] / "app/src/main/assets/models"
manifest = json.loads((root / "apex10m-partial-manifest.json").read_text())
weights = root / "apex10m-partial-policy.onnx"
assert hashlib.sha256(weights.read_bytes()).hexdigest() == manifest["onnx_sha256"]
assert manifest["trained_spatial_head_parameters"] == 3693992
assert 37.3 <= manifest["frozen_percent_of_15h"] <= 37.4
assert not manifest["trained_on_real_music"] and not manifest["neural_playback_approved"]
session = ort.InferenceSession(str(weights),providers=["CPUExecutionProvider"])
assert [i.name for i in session.get_inputs()] == ["features"]
names = [o.name for o in session.get_outputs()]
assert "policy_controls" in names and "band_controls" in names
x = np.zeros((1,8,20),dtype=np.float32)
x[:,0,:]=.4
x[:,1,:]=.2
x[:,2,:]=.15
x[:,3,:]=.8
x[:,4,:]=.12
x[:,5,:]=.25
x[:,6,:]=.6
x[:,7,:]=.05
data=dict(zip(names,session.run(None,{"features":x})))
assert data["policy_controls"].shape==(1,8,20)
assert data["band_controls"].shape==(1,2,32,20)
assert np.isfinite(data["policy_controls"]).all()
assert ((data["policy_controls"]>=0)&(data["policy_controls"]<=1)).all()
# The model must react to different scene features, not be a placebo.
y=x.copy()
y[:,0,:]=.93; y[:,1,:]=.92; y[:,3,:]=-.8; y[:,6,:]=.08
changed=session.run(["policy_controls"],{"features":y})[0]
delta=float(np.max(np.abs(data["policy_controls"]-changed)))
assert delta > .0001, "Frozen neural policy didn't react to input changes"
print(f"PASS frozen neural ONNX real inference, sha256, 8x20 controls, distinct scenes (max delta={delta:.5f})")
