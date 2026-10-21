#!/usr/bin/env python3
"""Verify the exact trained AUDONIC model shipped with the Android APK."""
import hashlib,json
from pathlib import Path
import numpy as np
import onnxruntime as ort

root=Path(__file__).resolve().parents[1]/'app/src/main/assets/models'
manifest=json.loads((root/'audonic-manifest.json').read_text())
weights=root/'audonic-student.onnx'
assert manifest['model']=='AUDONIC neural creative-FX student'
assert manifest['gradient_updates']>=100
assert manifest['apex10m_teacher_distillation']
assert manifest['three_b_teacher_trained'] is False
assert manifest['real_music_quality_verified'] is False
assert 10000<manifest['parameter_count']<200000
assert hashlib.sha256(weights.read_bytes()).hexdigest()==manifest['onnx_sha256']
session=ort.InferenceSession(str(weights),providers=['CPUExecutionProvider'])
assert [i.name for i in session.get_inputs()]==['features']
assert [o.name for o in session.get_outputs()]==['fx_controls']
x=np.zeros((1,8,20),dtype=np.float32)
x[:,0]=.3;x[:,1]=.1;x[:,2]=.12;x[:,3]=.8;x[:,5]=.1;x[:,6]=.55
base=session.run(None,{'features':x})[0]
assert base.shape==(1,6,20)
assert np.isfinite(base).all() and np.min(base)>=0 and np.max(base)<=1
y=x.copy();y[:,1]=.92;y[:,3]=-.6;y[:,6]=.05;y[:,5]=.85
alternate=session.run(None,{'features':y})[0]
assert np.max(np.abs(base-alternate))>1e-4,'Model must react to differing sounds'
print(f"PASS: real AUDONIC trained ONNX, sha256 and six adaptive FX controls, {manifest['gradient_updates']} gradient updates")

# Pass actual neural output to the native PCM regression, not invented controls.
import sys
if len(sys.argv)>1:
    base[0,:,-1].astype("<f4").tofile(sys.argv[1])
