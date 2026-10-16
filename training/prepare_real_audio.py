#!/usr/bin/env python3
"""Prepare authentic CC-BY-4.0 audio recordings for Cyn's real-audio trial.

Sources: OpenSLR Mini LibriSpeech (spoken language) and Google's NSynth test
(recorded instrument notes). Only numeric features/heuristic targets are saved.
Never upload original recordings. No copyrighted commercial songs are used.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import math
import tarfile
import time
import urllib.request
from pathlib import Path
import numpy as np
import soundfile as sf

SOURCES = {
    "speech": "https://openslr.trmal.net/resources/31/dev-clean-2.tar.gz",
    "notes": "https://storage.googleapis.com/download.magenta.tensorflow.org/datasets/nsynth/nsynth-test.jsonwav.tar.gz",
}
RATE = 16000
N = 4096
FREQ = np.fft.rfftfreq(N, 1 / RATE)
Z = np.exp(-2j * np.pi * FREQ / RATE)
RESPONSES = []
for center in [45,75,120,200,400,900,2200,5000]:
    w=2*np.pi*min(center, RATE*.4)/RATE
    alpha=np.sin(w)/1.6
    b=alpha/(1+alpha); a1=-2*np.cos(w)/(1+alpha); a2=(1-alpha)/(1+alpha)
    RESPONSES.append(np.abs((b-b*Z*Z)/(1+a1*Z+a2*Z*Z))**2)
RESPONSES=np.asarray(RESPONSES)


def stable_hash(text: str) -> int:
    return int.from_bytes(hashlib.sha256(text.encode()).digest()[:8], "big")


def download(url: str, destination: Path, max_bytes: int = 1_600_000_000) -> dict:
    for attempt in range(3):
        try:
            req=urllib.request.Request(url, headers={"User-Agent":"CynAudioResearch/1.0"})
            h=hashlib.sha256()
            with urllib.request.urlopen(req, timeout=180) as response, destination.open("wb") as out:
                total=0
                while True:
                    chunk=response.read(1024*1024)
                    if not chunk:
                        break
                    total+=len(chunk)
                    if total>max_bytes:
                        raise RuntimeError("Archive exceeds safety size limit")
                    out.write(chunk); h.update(chunk)
            if total<5_000_000:
                raise RuntimeError("Unexpectedly small dataset")
            return {"url":url,"sha256":h.hexdigest(),"compressed_bytes":total}
        except Exception as exc:
            print(f"Download attempt {attempt+1} failed: {type(exc).__name__}: {exc}", flush=True)
            destination.unlink(missing_ok=True)
            if attempt==2:
                raise
            time.sleep(5*(attempt+1))
    raise RuntimeError("Download failed")


def features(waveform: np.ndarray, seed: int) -> list[np.ndarray]:
    samples=np.asarray(waveform, dtype=np.float32).reshape(-1)
    if len(samples)<N:
        return []
    rng=np.random.default_rng(seed)
    offsets=np.linspace(0,len(samples)-N,num=4,dtype=np.int64)
    if len(samples)>2*N:
        offsets[1:3] = rng.integers(0,len(samples)-N,size=2)
    rows=[]
    for off in offsets:
        v=np.asarray(samples[int(off):int(off)+N],dtype=np.float64)
        v=np.nan_to_num(v,nan=0,posinf=0,neginf=0)
        rms=float(np.sqrt(np.mean(v*v)))
        if rms<.002: continue
        spectrum=np.fft.rfft(v)
        power=np.abs(spectrum)**2
        power[1:-1]*=2
        full=float(power.sum())
        band=np.log(1e-6+RESPONSES@power/max(full,1e-20))
        crest=math.log(max(1.,float(np.max(np.abs(v)))/rms))
        raw=np.r_[band,crest,0.,math.log(rms)]
        x=raw.copy()
        x[:8]=(np.clip(x[:8],-12,0)+6)/6
        x[8]=np.clip(x[8],0,4)/2-1
        x[9]=np.clip(x[9],0,2)-1
        x[10]=(np.clip(x[10],-8,0)+4)/4
        if np.isfinite(x).all():rows.append(x.astype(np.float32))
    return rows


def fold(group: str) -> int:
    h=stable_hash(group)%100
    return 0 if h<78 else 1 if h<89 else 2


def load_wave(payload: bytes, name: str) -> np.ndarray:
    with sf.SoundFile(io.BytesIO(payload)) as f:
        if f.samplerate!=RATE:
            raise ValueError(f"Unexpected rate in {name}: {f.samplerate}")
        data=f.read(dtype="float32",always_2d=True)
    return data.mean(axis=1)


def from_speech(archive: Path, arrays: list[list], counters: dict) -> None:
    with tarfile.open(archive, "r:gz") as tar:
        for m in tar:
            if not m.isfile() or not m.name.endswith(".flac"):continue
            bits=m.name.split("/")
            group="speaker:"+ (bits[-3] if len(bits)>=3 else bits[0])
            reader=tar.extractfile(m)
            if reader is None: continue
            wave=load_wave(reader.read(),m.name)
            rows=features(wave,stable_hash(m.name))
            if not rows:continue
            f=fold(group)
            for x in rows:
                high=float(np.exp(min(0.,x[7]*6-6)))
                # Preference targets are safety-oriented *heuristics*, NOT listening labels.
                clarity=np.clip(.68-.08*high,.53,.75)
                bass=.35
                arrays[f].append((x,np.array([clarity,bass],dtype=np.float32),0))
            counters["speech_clips"]+=1


def from_notes(archive: Path, arrays: list[list], counters: dict) -> None:
    with tarfile.open(archive,"r:gz") as tar:
        metadata_member=next((m for m in tar.getmembers() if m.name.endswith("/examples.json")),None)
        if metadata_member is None:raise ValueError("NSynth examples.json missing")
        metadata=json.loads(tar.extractfile(metadata_member).read())
        for m in tar:
            if not m.isfile() or not m.name.endswith(".wav"):continue
            name=Path(m.name).stem
            if stable_hash(name)%4==0:continue
            meta=metadata.get(name)
            if not meta:continue
            group="instrument:"+str(meta.get("instrument_str") or "-".join(name.split("-")[:-2]))
            reader=tar.extractfile(m)
            if reader is None:continue
            wave=load_wave(reader.read(),m.name)
            rows=features(wave,stable_hash(m.name))
            if not rows:continue
            family=str(meta.get("instrument_family_str",""))
            qualities=set(meta.get("qualities_str",[]))
            isbass=family=="bass"; isvocal=family=="vocal"
            clarity=.63 if isvocal else .52
            if "bright" in qualities:clarity-=.09
            if "dark" in qualities:clarity+=.04
            bass=.65 if isbass else .48
            if "distortion" in qualities:bass-=.06
            if "percussive" in qualities:bass+=.025
            clarity=float(np.clip(clarity,.34,.75))
            bass=float(np.clip(bass,.28,.75))
            role=2 if isbass else 3 if "percussive" in qualities else 1
            f=fold(group)
            for x in rows:
                arrays[f].append((x,np.array([clarity,bass],dtype=np.float32),role))
            counters["note_clips"]+=1


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",type=Path,default=Path("real-audio-data"))
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    info={}
    arrays=[[],[],[]]
    counters={"speech_clips":0,"note_clips":0}
    for source in ("speech","notes"):
        filename=args.output/(source+".tar.gz")
        info[source]=download(SOURCES[source],filename)
        if source=="speech":from_speech(filename,arrays,counters)
        else:from_notes(filename,arrays,counters)
        filename.unlink()  # never upload copyrighted audio even when licensed
    for f,(name,min_rows) in enumerate((("train",1500),("validation",100),("test",100))):
        rows=arrays[f]
        if len(rows)<min_rows:
            raise ValueError(f"Too few real-audio {name} examples: {len(rows)}")
        x=np.stack([r[0] for r in rows])
        y=np.stack([r[1] for r in rows])
        roles=np.array([r[2] for r in rows],dtype=np.int8)
        if not set(roles).intersection({0}) or not set(roles).intersection({1,2,3}):
            raise ValueError(f"Speech and instrument coverage missing from {name}")
        np.savez_compressed(args.output/(name+".npz"),x=x,y=y,role=roles)
        info[name]={"windows":int(len(x)),
                    "speech_windows":int((roles==0).sum()),
                    "instrument_windows":int((roles!=0).sum())}
    info["counts"]=counters
    info["license"]="CC BY 4.0; attribution to LibriSpeech/OpenSLR and Google NSynth"
    info["reference"]="https://openslr.org/31/ ; https://magenta.tensorflow.org/datasets/nsynth"
    info["sources_are_real_recordings"]=True
    info["input_to_model"]="11 native spectral statistics; aligned with Cyn PersonalAudioModel"
    info["targets"]="Heuristic preference signals from published class tags; no human preference labels."
    info["separation"]="Train/validation/test divided by speaker or instrument identity"
    (args.output/"provenance.json").write_text(json.dumps(info,indent=2)+"\n")
    print(json.dumps(info,indent=2),flush=True)


if __name__=="__main__":
    main()
