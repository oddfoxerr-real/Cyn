"""Smoke tests: identity STFT, fully masked audio, wet bypass. No neural training."""
from __future__ import annotations
import pathlib, tempfile
import numpy as np
import soundfile as sf
from audio import process_audio

class MaskSession:
    def __init__(self, mask):self.mask=mask;self.count=0
    def run(self,names,inputs):
        self.count+=1
        x=inputs["log_magnitude"]
        assert x.ndim==3 and x.shape[1]==513
        return [np.ones_like(x,dtype=np.float32)*self.mask,np.zeros((x.shape[0],6),np.float32)]

with tempfile.TemporaryDirectory() as d:
    root=pathlib.Path(d)
    sample_rate=48000
    t=np.arange(48000,dtype=np.float32)/sample_rate
    wave=(0.25*np.sin(2*np.pi*220*t))[:,None].astype(np.float32)
    input_path=root/"input.wav"
    sf.write(input_path,wave,sample_rate,subtype="PCM_24")
    out=root/"identity.wav"
    session=MaskSession(1.0)
    process_audio(session,input_path,out,wet=1)
    result,rate=sf.read(out,dtype="float32")
    assert rate==48000
    assert session.count>0
    assert np.max(np.abs(wave.squeeze()-result))<0.0005,(wave.squeeze()-result).max()
    quiet=root/"quiet.wav"
    process_audio(MaskSession(0),input_path,quiet,wet=1)
    silenced,_=sf.read(quiet,dtype="float32")
    assert float(np.max(np.abs(silenced)))<0.001
    dry=root/"dry.wav"
    process_audio(MaskSession(0),input_path,dry,wet=0)
    bypass,_=sf.read(dry,dtype="float32")
    assert np.max(np.abs(bypass-wave.squeeze()))<0.00001
    print("PASS: actual 48 kHz WAV STFT/ISTFT, model mask, wet bypass and PCM output")
