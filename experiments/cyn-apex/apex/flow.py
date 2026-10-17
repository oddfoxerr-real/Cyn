"""CYN APEX FlowSense: objective energy, pulse, build and breakdown analysis.
This is signal analysis, NOT neural understanding of emotion or ideal mastering.
Works independently of the denoising checkpoint and outside the audio callback.
"""
from __future__ import annotations
import argparse
import json
from pathlib import Path
from math import gcd
import numpy as np
import soundfile as sf
from scipy.signal import resample_poly, stft, fftconvolve

SAMPLE_RATE=12000
FFT_SIZE=1024
HOP=256

def _smooth(x:np.ndarray, frames:int)->np.ndarray:
    frames=max(1,frames)
    window=np.ones(frames)/frames
    return np.convolve(np.pad(x,(frames//2,frames-frames//2-1),mode="edge"),window,"valid")

def estimate_pulse(onsets:np.ndarray, fps:float)->tuple[float|None,float]:
    if onsets.size < fps*5:
        return None,0.0
    x=np.maximum(onsets-np.median(onsets),0.0)
    x-=x.mean()
    power=float(x@x)
    if power<1e-6:
        return None,0.0
    lo=max(1,round(fps*60/200))
    hi=min(len(x)-1,round(fps*60/65))
    if lo>=hi:
        return None,0.0
    corr=fftconvolve(x,x[::-1],mode="full")[len(x)-1:]
    lag=lo+int(np.argmax(corr[lo:hi+1]))
    conf=float(np.clip(corr[lag]/power,0,1))
    return round(fps*60/lag,2),round(conf,3)

def analyze_audio(waveform:np.ndarray,sample_rate:int,*,seconds_per_segment:float=1.0)->dict:
    if sample_rate<=0 or seconds_per_segment<=0:
        raise ValueError("Invalid sample rate or segment length")
    signal=np.asarray(waveform,dtype=np.float64)
    if signal.ndim==2:
        if signal.shape[1] not in (1,2):
            raise ValueError("Expected mono or stereo audio")
        signal=signal.mean(axis=1)
    if signal.ndim!=1 or signal.size==0 or not np.isfinite(signal).all():
        raise ValueError("Expected finite, nonempty audio")
    if sample_rate!=SAMPLE_RATE:
        g=gcd(sample_rate,SAMPLE_RATE)
        signal=resample_poly(signal,SAMPLE_RATE//g,sample_rate//g)
    if len(signal)<SAMPLE_RATE:
        raise ValueError("Need at least one second of audio")
    freqs,times,spectrum=stft(signal,fs=SAMPLE_RATE,nperseg=FFT_SIZE,
                             noverlap=FFT_SIZE-HOP,boundary=None,padded=False)
    power=np.abs(spectrum)**2
    magnitude=np.sqrt(power)
    level_db=20*np.log10(np.maximum(np.sqrt(power.mean(axis=0)),1e-9))
    logmag=np.log1p(magnitude*800)
    flux=np.maximum(np.diff(logmag,axis=1,prepend=logmag[:,:1]),0).mean(axis=0)
    flux=np.clip(flux/max(float(np.percentile(flux,95)),1e-9),0,3)
    total=np.maximum(power.sum(axis=0),1e-12)
    low=power[freqs<=180].sum(axis=0)/total
    high=power[freqs>=3300].sum(axis=0)/total
    fps=SAMPLE_RATE/HOP
    smooth_level=_smooth(level_db,round(fps*.7))
    pulse,confidence=estimate_pulse(flux,fps)
    segments=[]
    duration=len(signal)/SAMPLE_RATE
    for i in range(int(np.ceil(duration/seconds_per_segment))):
        start=i*seconds_per_segment
        end=min(duration,(i+1)*seconds_per_segment)
        mask=(times>=start)&(times<end)
        if not mask.any():
            continue
        segments.append({
            "from_s":round(start,3),"to_s":round(end,3),
            "intensity":round(float(np.clip((smooth_level[mask]+52).mean()/42,0,1)),4),
            "level_db":round(float(np.median(level_db[mask])),2),
            "onset_activity":round(float(flux[mask].mean()),4),
            "bass_fraction":round(float(low[mask].mean()),4),
            "high_fraction":round(float(high[mask].mean()),4)
        })
    events=[]
    for i in range(1,len(segments)):
        curr=segments[i]; prev=segments[i-1]
        jump=curr["level_db"]-prev["level_db"]
        earlier=segments[max(0,i-3):i]
        ascending=len(earlier)>=2 and all(
            earlier[j]["level_db"]<earlier[j+1]["level_db"]+.7
            for j in range(len(earlier)-1))
        if jump>7.5 and curr["onset_activity"]>.08 and prev["level_db"]>-70:
            name="impact_transition"
        elif jump>2 and ascending:
            name="build"
        elif jump< -8:
            name="breakdown"
        else:
            continue
        events.append({"at_s":curr["from_s"],"kind":name,"contrast_db":round(jump,2)})
    return {
        "schema":1,"analyzer":"CYN APEX FlowSense v0.1","duration_s":round(duration,3),
        "estimated_pulse_bpm":pulse,"pulse_confidence":confidence,
        "segments":segments,"events":events,
        "ai_model_inference":False,"offline_only":True,
        "interpretation":"Signal-derived changes, not emotion, listener preferences or verified mastering choices"
    }

def main():
    parser=argparse.ArgumentParser(description="APEX experimental music flow analysis")
    parser.add_argument("source",type=Path)
    parser.add_argument("--output",type=Path,default=Path("reports/flow.json"))
    args=parser.parse_args()
    data,sample_rate=sf.read(args.source,dtype="float32",always_2d=False)
    result=analyze_audio(data,sample_rate)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
    print(json.dumps({key:value for key,value in result.items() if key!="segments"},indent=2))

if __name__=="__main__":
    main()
