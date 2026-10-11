#!/usr/bin/env python3
"""Cyn candidate benchmark: run REAL pretrained CPJKU Beat This! small0 on CPU.

This is an isolated research probe; the real-time Cyn audio DSP and installer
are unchanged. Generated beat audio is not evidence of real-music accuracy.
"""
from __future__ import annotations
import json
import math
import statistics
import struct
import time
import wave
from pathlib import Path

ROOT=Path("music-ai-benchmark")
ROOT.mkdir(exist_ok=True)
AUDIO=ROOT/"synthetic_phonk_rhythm.wav"
REPORT=ROOT/"beat_this_small0.json"

def make_probe(rate=22050,seconds=24,bpm=130):
    # Synthetic bass-kick/downbeat pulse train, bright snare on offbeats,
    # plus very low amplitude steady musical tone. Not copyrighted material.
    total=rate*seconds
    samples=[0.0]*total
    quarter=60.0/bpm
    kick_times=[]
    for i in range(int(seconds/quarter)):
        position=i*quarter
        kick_times.append(position)
        start=round(position*rate)
        for n in range(min(int(.22*rate),total-start)):
            t=n/rate
            envelope=math.exp(-t*24.0)
            # Downward pitch slide approximating synthetic phonk kick, plus click
            kick=.57*envelope*math.sin(2*math.pi*(54*t+92*t*math.exp(-t*20)))
            click=.12*math.exp(-t*160)*math.sin(2*math.pi*1500*t)
            samples[start+n]+=kick+click
        if i%4 in (1,3):
            snare_start=round((position+.012)*rate)
            for n in range(min(int(.08*rate),total-snare_start)):
                t=n/rate
                samples[snare_start+n]+=.09*math.exp(-t*75)*math.sin(2*math.pi*800*t)
    vals=[]
    for i,v in enumerate(samples):
        t=i/rate
        x=max(-.93,min(.93,v+.01*math.sin(2*math.pi*220*t)))
        vals.append(int(x*32767))
    with wave.open(str(AUDIO),"wb") as out:
        out.setnchannels(1)
        out.setsampwidth(2)
        out.setframerate(rate)
        out.writeframes(struct.pack("<"+"h"*len(vals),*vals))
    return kick_times,seconds

def main():
    import torch
    from beat_this.inference import File2Beats
    torch.set_num_threads(2)
    expected,duration=make_probe()
    load_start=time.perf_counter()
    model=File2Beats(checkpoint_path="small0",device="cpu",dbn=False)
    load_seconds=time.perf_counter()-load_start
    start=time.perf_counter()
    with torch.inference_mode():
        beats,downbeats=model(str(AUDIO))
    inference_seconds=time.perf_counter()-start

    predictions=[float(x) for x in beats]
    down=[float(x) for x in downbeats]
    assert all(math.isfinite(x) and 0<=x<=duration+1 for x in predictions)
    assert all(math.isfinite(x) and 0<=x<=duration+1 for x in down)
    def coverage(truth,pred,window=.10):
        return sum(any(abs(p-t)<=window for p in pred) for t in truth)/len(truth)
    observed_bpm=(
        60/statistics.median([y-x for x,y in zip(predictions,predictions[1:])])
        if len(predictions)>2 and all(y>x for x,y in zip(predictions,predictions[1:]))
        else None
    )
    report={
      "candidate":"CPJKU/beat_this small0",
      "pretrained_model_loaded":True,
      "audio_type":"synthetic rhythm only (not a real-music benchmark)",
      "duration_seconds":duration,
      "sample_rate_hz":22050,
      "expected_bpm":130,
      "predicted_beat_count":len(predictions),
      "predicted_downbeat_count":len(down),
      "estimated_bpm":observed_bpm,
      "synthetic_beats_within_100ms_fraction":coverage(expected,predictions),
      "model_load_seconds":round(load_seconds,3),
      "inference_seconds":round(inference_seconds,3),
      "inference_real_time_factor":round(inference_seconds/duration,3),
      "head_beats_seconds":predictions[:8],
      "torch_version":torch.__version__,
      "production_integration":False,
      "limitations":"This is offline whole-file beat tracking on a CPU runner; it cannot establish low-latency streaming or performance on a Windows PC. No claim of bass/mastering sound improvement. Test on permission-cleared real music before integration."
    }
    REPORT.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))
    print("PASS: original pretrained Beat This! small0 checkpoint loaded and inferred offline audio")

if __name__=="__main__":
    main()
