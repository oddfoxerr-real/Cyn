"""True checkpoint smoke test: process WAV through ONNX's actual 6.3M weights."""
from pathlib import Path
import tempfile
import numpy as np
import onnxruntime as ort
import soundfile as sf
from audio import process_audio

model=Path("dist-apex/apex6m-225.onnx")
assert model.is_file() and model.stat().st_size>20_000_000, "Full model asset missing"
options=ort.SessionOptions(); options.intra_op_num_threads=2
session=ort.InferenceSession(str(model),sess_options=options,providers=["CPUExecutionProvider"])
with tempfile.TemporaryDirectory() as directory:
    root=Path(directory)
    sample_rate=48_000
    t=np.arange(12000,dtype=np.float32)/sample_rate
    waveform=(0.24*np.sin(2*np.pi*190*t)+0.07*np.sin(2*np.pi*2500*t))[:,None]
    sf.write(str(root/"in.wav"),waveform,sample_rate,subtype="PCM_24")
    result=process_audio(session,root/"in.wav",root/"out.wav",wet=0.8)
    audio,rate=sf.read(str(result),always_2d=True,dtype="float32")
    assert rate==sample_rate and audio.shape==waveform.shape
    assert np.isfinite(audio).all()
    assert np.max(np.abs(audio))<1.0
    assert np.max(np.abs(audio-waveform))>0.00001, "Full ONNX mask did not change real waveform"
    print("PASS: full 6,307,477-parameter APEX model ran on actual WAV and changed output")
