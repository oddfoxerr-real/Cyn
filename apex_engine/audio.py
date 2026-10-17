"""Actual full-model ONNX spectral mask inference, no training.
WAV input, level-preserving blend, 48-kHz inference, CPU safe fallback.
"""
from __future__ import annotations
from math import gcd
from pathlib import Path
import numpy as np
import scipy.signal as signal
import soundfile as sf

SAMPLE_RATE=48_000
FFT=1024
HOP=256

def process_audio(session, incoming: Path, destination: Path, wet: float = 0.65,
                  progress=None):
    if not (0<=wet<=1) or not np.isfinite(wet):
        raise ValueError("Wet mix must be a finite value between 0 and 1")
    source, sample_rate = sf.read(str(incoming), dtype="float32", always_2d=True)
    if sample_rate < 8_000 or sample_rate > 192_000:
        raise ValueError("Sample rate must be 8 kHz to 192 kHz")
    if source.ndim != 2 or source.shape[1] not in (1,2) or source.size==0:
        raise ValueError("WAV must be nonempty mono or stereo")
    if not np.isfinite(source).all():
        raise ValueError("Nonfinite input samples")
    if len(source)/sample_rate > 600:
        raise ValueError("Limit audio to 10 minutes per processing run")
    if sample_rate != SAMPLE_RATE:
        g=gcd(sample_rate,SAMPLE_RATE)
        x=signal.resample_poly(source,SAMPLE_RATE//g,sample_rate//g,axis=0).astype(np.float32)
    else:
        x=source.copy()
    if wet == 0:
        sf.write(str(destination),source,sample_rate,subtype="PCM_24")
        if progress: progress(1,1)
        return destination
    original_length=x.shape[0]
    # 1-second windows, half-second overlap; zero-pad short end windows.
    window_size=SAMPLE_RATE
    step=window_size//2
    positions=list(range(0,original_length,step))
    result=np.zeros_like(x)
    normalizer=np.zeros((len(x),1),np.float32)
    fade=np.hanning(window_size).astype(np.float32)
    for index,start in enumerate(positions):
        end=min(original_length,start+window_size)
        length=end-start
        segment=np.zeros((window_size,x.shape[1]),np.float32)
        segment[:length,:]=x[start:end,:]
        # The model was trained on torch.stft with a Hann window. SciPy STFT
        # has a normalizing factor of sum(window); restore torch-like scale.
        freq,time,spectrum=signal.stft(segment.T,fs=SAMPLE_RATE,
                                       window="hann",nperseg=FFT,
                                       noverlap=FFT-HOP,
                                       boundary="even",padded=True,
                                       axis=-1)
        spectrum=np.asarray(spectrum,dtype=np.complex64)
        torch_scale=FFT/2.0
        features=np.log1p(np.abs(spectrum)*torch_scale).astype(np.float32)
        neural_mask,scene_logits=session.run(["mask","scene_logits"],
                                               {"log_magnitude":features})
        if neural_mask.shape!=spectrum.shape or not np.isfinite(neural_mask).all():
            raise RuntimeError("APEX model returned invalid output")
        restored=signal.istft(spectrum*neural_mask,fs=SAMPLE_RATE,
                               window="hann",nperseg=FFT,
                               noverlap=FFT-HOP,boundary=True)[1]
        restored=np.asarray(restored[:,:window_size].T,dtype=np.float32)
        weight=fade[:length].copy()
        if start==0: weight[:min(step,length)]=1
        if end==original_length:weight[max(0,length-step):]=1
        result[start:end] += restored[:length]*weight[:,None]
        normalizer[start:end] += weight[:,None]
        if progress: progress(index+1,len(positions))
    result /= np.maximum(normalizer,1.0e-6)
    result=(1-wet)*x+wet*result
    if sample_rate!=SAMPLE_RATE:
        g=gcd(sample_rate,SAMPLE_RATE)
        result=signal.resample_poly(result,sample_rate//g,SAMPLE_RATE//g,axis=0)
        result=result[:len(source)]
        if len(result)<len(source):
            result=np.pad(result,((0,len(source)-len(result)),(0,0)))
    if not np.isfinite(result).all():
        raise RuntimeError("Invalid nonfinite processed audio")
    peak=float(np.max(np.abs(result)))
    if peak>0.999:
        result*=0.999/peak
    destination.parent.mkdir(parents=True,exist_ok=True)
    sf.write(str(destination),result,sample_rate,subtype="PCM_24")
    return destination
