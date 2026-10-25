#!/usr/bin/env python3
"""Reproducible LIBERTY kick/onset training on real, openly licensed recorded drums.

Source: Google Magenta Groove MIDI Dataset v1.0.0 (CC BY 4.0).
Uses WAV audio and temporally aligned MIDI drum-onset labels. No generated songs.
Candidate weights are for evaluation only: they are NOT bundled into Android.
"""
import argparse
import bisect
import csv
import hashlib
import io
import json
import math
import os
from pathlib import Path
import random
import time
import subprocess

import mido
import numpy as np
import soundfile as sf
from scipy.signal import lfilter
from remotezip import RemoteZip

SOURCE = "https://storage.googleapis.com/magentadata/datasets/groove/groove-v1.0.0.zip"
DATASET_SHA256 = "21559feb2f1c96ca53988fd4d7060b1f2afe1d854fb2a8dcea5ff95cf3cce7e9"
KICK_PITCHES = {35, 36}
SEED = 20261025


def midi_kicks(payload):
    midi = mido.MidiFile(file=io.BytesIO(payload))
    t = 0.0
    hits = []
    # MidiFile iteration applies tempo changes and yields seconds, not ticks.
    for message in midi:
        t += message.time
        if message.type == "note_on" and message.note in KICK_PITCHES and message.velocity:
            hits.append(t)
    return hits


def clamp(array, lo=0.0, hi=1.0):
    return np.clip(array, lo, hi)


def sound_features(audio, sample_rate):
    """The eight 100-ms stereo features computed by phone_engine.cpp/collectPolicy.

    Filter alpha must account for sample-rate variation. Frame duration is 100ms.
    feature order: bass ratio, onset, side level, correlation, transient,
    vocal band, headroom, left-right balance.
    """
    if len(audio) < sample_rate * 3:
        return np.empty((0, 8), dtype=np.float32)
    data = np.asarray(audio, dtype=np.float32)
    if data.ndim == 1:
        data = np.column_stack([data, data])
    if data.shape[1] == 1:
        data = np.repeat(data, 2, axis=1)
    left, right = data[:, 0], data[:, 1]
    mid, side = (left+right)*.5, (left-right)*.5
    low_alpha = 1.0 - (1.0-.018)**(48000.0/sample_rate)
    high_alpha = 1.0 - (1.0-.20)**(48000.0/sample_rate)
    low = lfilter([low_alpha], [1.0, -(1-low_alpha)], mid)
    high = lfilter([high_alpha], [1.0, -(1-high_alpha)], mid)
    frame = max(1, round(.1*sample_rate))
    num = len(mid)//frame
    if num <= 20:
        return np.empty((0, 8), dtype=np.float32)

    def squared(signal):
        return np.square(signal[:num*frame].reshape(num, frame), dtype=np.float32).mean(axis=1)

    rms = np.sqrt(squared(mid))
    bass = np.sqrt(squared(low))
    side_level = np.sqrt(squared(side))
    le = np.sqrt(squared(left))
    ri = np.sqrt(squared(right))
    correlation = (left[:num*frame]*right[:num*frame]).reshape(num,frame).mean(axis=1)
    vocal = np.sqrt(squared(high-low))
    peak = np.maximum(np.abs(left), np.abs(right))[:num*frame].reshape(num,frame).max(axis=1)
    ratio = clamp(bass/np.maximum(rms, 1e-5))
    previous_bass = np.r_[0.0, ratio[:-1]]
    previous_rms = np.r_[0.0, rms[:-1]]
    attack = np.maximum(0.0, rms-previous_rms)
    features = np.stack([
        ratio,
        clamp(np.maximum(0.0,ratio-previous_bass)*2.0+attack*4.0),
        clamp(side_level/np.maximum(side_level+rms,1e-5)),
        clamp(correlation/np.maximum(le*ri,1e-9), -1, 1),
        clamp(attack*6),
        clamp(vocal/np.maximum(rms,1e-5)),
        clamp(1.0-peak),
        clamp((le-ri)/np.maximum(le+ri,1e-5),-1,1)
    ], axis=1)
    return np.nan_to_num(features,nan=0,posinf=0,neginf=0).astype(np.float32)


def examples(audio, sample_rate, kicks):
    features = sound_features(audio, sample_rate)
    x, y, base = [], [], []
    for ending in range(19, len(features), 2):
        seconds=(ending+.5)*.1
        loc=bisect.bisect_left(kicks,seconds-.09)
        label = loc<len(kicks) and kicks[loc]<=seconds+.06
        window = features[ending-19:ending+1].T
        x.append(window.reshape(160))
        y.append(float(label))
        base.append(float(np.max(features[max(0,ending-2):ending+1,1])))
    return x,y,base


def read_examples(args):
    random.seed(SEED)
    collections={split:{"x":[],"y":[],"base":[],"files":0} for split in ("train","validation","test")}
    attempts=0
    with RemoteZip(SOURCE) as archive:
        names={member.filename:member for member in archive.infolist()}
        info_name=next((p for p in names if p.endswith("/info.csv") or p=="info.csv"),None)
        if info_name is None:
            raise RuntimeError("Official Groove metadata absent")
        root=info_name[:-len("info.csv")]
        table=list(csv.DictReader(io.StringIO(archive.read(info_name).decode("utf-8-sig"))))
        if len(table)<900:
            raise RuntimeError("Incomplete online dataset")
        random.Random(SEED).shuffle(table)
        for row in table:
            if attempts>=args.max_files:
                break
            split=row["split"].strip().lower()
            if split not in collections:
                continue
            wav=root+row["audio_filename"].strip().lstrip("/")
            midi=root+row["midi_filename"].strip().lstrip("/")
            if wav not in names or midi not in names:
                continue
            if names[wav].file_size>args.max_wav_bytes:
                continue
            attempts+=1
            try:
                # RemoteZip issues HTTPS byte-range requests. It does not copy the
                # whole 4.76GB archive to the runner, nor store source WAV audio.
                raw=archive.read(wav)
                try:
                    with sf.SoundFile(io.BytesIO(raw)) as source:
                        sample_rate=source.samplerate
                        audio=source.read(frames=int(sample_rate*args.seconds_per_file),
                            dtype="float32",always_2d=True)
                except (sf.LibsndfileError,ValueError):
                    # Some Groove archive WAV-named files contain codecs libsndfile
                    # does not recognize. ffmpeg decodes via headers without trusting
                    # the extension. No copies of source audio are retained.
                    if attempts<=2:
                        print("Audio probe",wav,"magic",raw[:24].hex(),flush=True)
                    decoded=subprocess.run([
                        "ffmpeg","-nostdin","-hide_banner","-loglevel","error",
                        "-i","pipe:0","-t",str(args.seconds_per_file),
                        "-ar","48000","-ac","2","-f","f32le","pipe:1"
                    ],input=raw,capture_output=True,timeout=90)
                    if decoded.returncode:
                        raise ValueError("ffmpeg decode: "+decoded.stderr[:250].decode(errors="replace"))
                    sample_rate=48000
                    audio=np.frombuffer(decoded.stdout,dtype="<f4").reshape(-1,2)
                kicks=midi_kicks(archive.read(midi))
                xx,yy,bb=examples(audio,sample_rate,kicks)
                if len(xx)<5 or not (np.isfinite(xx).all()):
                    continue
                data=collections[split]
                data["x"].extend(xx);data["y"].extend(yy);data["base"].extend(bb)
                data["files"]+=1
            except (OSError,ValueError,EOFError,sf.LibsndfileError,subprocess.TimeoutExpired) as error:
                print("SKIP corrupted/unreadable recording:",type(error).__name__,flush=True)
            if attempts%40==0:
                print("Downloaded/decoded",attempts,"licensed recordings",
                      {k:v["files"] for k,v in collections.items()},flush=True)
    for split,v in collections.items():
        if v["files"]<(5 if split!="train" else 30):
            raise RuntimeError("Too little labeled real audio for "+split)
        for key in ("x","y","base"):
            v[key]=np.asarray(v[key],dtype=np.float32)
        if v["y"].sum()<10:
            raise RuntimeError("Kick labels missing from "+split)
    return collections


def sigmoid(z):
    return 1/(1+np.exp(-np.clip(z,-20,20)))


def predict(x,model):
    h=np.maximum(x @ model["w1"]+model["b1"],0)
    return sigmoid(h @ model["w2"]+model["b2"]).reshape(-1)


def f1_at(truth,scores,threshold):
    truth=np.asarray(truth)>0
    predicted=np.asarray(scores)>=threshold
    tp=int((truth&predicted).sum())
    fp=int((~truth&predicted).sum())
    fn=int((truth&~predicted).sum())
    p=tp/(tp+fp) if tp+fp else 0
    r=tp/(tp+fn) if tp+fn else 0
    return {"f1":2*p*r/(p+r) if p+r else 0,"precision":p,
            "recall":r,"tp":tp,"fp":fp,"fn":fn}


def best_threshold(truth,scores):
    return max(((f1_at(truth,scores,float(t))["f1"],float(t))
                for t in np.linspace(.05,.9,35)),key=lambda x:x[0])[1]


def optimize(data,epochs):
    rng=np.random.default_rng(SEED)
    model={"w1":rng.normal(0,.055,(160,32)).astype(np.float32),
           "b1":np.zeros(32,np.float32),
           "w2":rng.normal(0,.05,(32,1)).astype(np.float32),
           "b2":np.zeros(1,np.float32)}
    m={k:np.zeros_like(v) for k,v in model.items()}
    v={k:np.zeros_like(q) for k,q in model.items()}
    x=data["train"]["x"]; y=data["train"]["y"][:,None]
    xval=data["validation"]["x"]; yval=data["validation"]["y"]
    balance=min(8,max(1.0,float(len(y)-y.sum())/max(1.0,float(y.sum()))))
    best_score=-1;best_model=None;history=[];step=0
    for epoch in range(epochs):
        indices=rng.permutation(len(x))
        for ids in np.array_split(indices,max(1,math.ceil(len(x)/256))):
            xb=x[ids];yb=y[ids]
            z1=xb @ model["w1"]+model["b1"];h=np.maximum(z1,0)
            z2=h @ model["w2"]+model["b2"];p=sigmoid(z2)
            grad=(p-yb)*np.where(yb>0, balance, 1)/len(ids)
            gradients={"w2":h.T@grad,"b2":grad.sum(axis=0)}
            dh=(grad@model["w2"].T)*(z1>0)
            gradients["w1"]=xb.T@dh
            gradients["b1"]=dh.sum(axis=0)
            step+=1
            for k in model:
                g=np.clip(gradients[k],-2,2)
                m[k]=.9*m[k]+.1*g
                v[k]=.999*v[k]+.001*g*g
                model[k]-=.0015*(m[k]/(1-.9**step))/(np.sqrt(v[k]/(1-.999**step))+1e-8)
        predictions=predict(xval,model)
        threshold=best_threshold(yval,predictions)
        score=f1_at(yval,predictions,threshold)["f1"]
        history.append({"epoch":epoch+1,"validation_f1":round(score,5)})
        if score>best_score:
            best_model={k:q.copy() for k,q in model.items()}
            best_score=score
        if (epoch+1)%10==0:
            print(f"Epoch {epoch+1}/{epochs}, held-out validation F1={score:.4f}, best={best_score:.4f}",flush=True)
    return best_model,history


def export_onnx(model,path):
    from onnx import TensorProto, helper, numpy_helper, save
    constants=[numpy_helper.from_array(weights,name)
               for name,weights in model.items()]
    nodes=[
        helper.make_node("MatMul",["audio_window","w1"],["z1"]),
        helper.make_node("Add",["z1","b1"],["z1b"]),
        helper.make_node("Relu",["z1b"],["hidden"]),
        helper.make_node("MatMul",["hidden","w2"],["z2"]),
        helper.make_node("Add",["z2","b2"],["logit"]),
        helper.make_node("Sigmoid",["logit"],["kick_probability"])
    ]
    graph=helper.make_graph(nodes,"LIBERTY real-audio kick classifier",
       [helper.make_tensor_value_info("audio_window",TensorProto.FLOAT,[None,160])],
       [helper.make_tensor_value_info("kick_probability",TensorProto.FLOAT,[None,1])],constants)
    network=helper.make_model(graph,producer_name="CYN-LIBERTY-groove-training",
        opset_imports=[helper.make_opsetid("",13)])
    network.ir_version=9
    save(network,path)
    import onnxruntime as ort
    session=ort.InferenceSession(str(path),providers=["CPUExecutionProvider"])
    sample=np.zeros((1,160),dtype=np.float32)
    result=session.run(None,{"audio_window":sample})[0]
    assert result.shape==(1,1) and np.isfinite(result).all()


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--max-files",type=int,default=850)
    parser.add_argument("--seconds-per-file",type=int,default=30)
    parser.add_argument("--max-wav-bytes",type=int,default=65_000_000)
    parser.add_argument("--epochs",type=int,default=90)
    parser.add_argument("--output",type=Path,default=Path("liberty-training-output"))
    args=parser.parse_args()
    started=time.monotonic()
    collected=read_examples(args)
    network,history=optimize(collected,args.epochs)
    valid=collected["validation"];test=collected["test"]
    candidate_val=predict(valid["x"],network)
    candidate_test=predict(test["x"],network)
    baseline_threshold=best_threshold(valid["y"],valid["base"])
    candidate_threshold=best_threshold(valid["y"],candidate_val)
    baseline=f1_at(test["y"],test["base"],baseline_threshold)
    trained=f1_at(test["y"],candidate_test,candidate_threshold)
    # Improvement on an unseen test set is a measurement, not a proof of better listening.
    report={
      "dataset":"Google Magenta Groove MIDI Dataset v1.0.0 (real aligned drum audio and MIDI)",
      "source_url":SOURCE,"source_sha256_full_archive":DATASET_SHA256,
      "license":"CC BY 4.0; cite Gillick et al., ICML 2019",
      "dataset_files_used":{k:v["files"] for k,v in collected.items()},
      "examples":{k:len(v["y"]) for k,v in collected.items()},
      "train_kick_positive_rate":float(collected["train"]["y"].mean()),
      "training_epochs_completed":len(history),"trained_weights":sum(q.size for q in network.values()),
      "baseline_test":baseline,"trained_test":trained,
      "validation_threshold_baseline":baseline_threshold,
      "validation_threshold_trained":candidate_threshold,
      "candidate_beats_baseline_f1":trained["f1"]>baseline["f1"]+.01,
      "seconds_wall_clock":round(time.monotonic()-started,1),
      "limits":"Drum-only recordings, not vocals/full mixes; kick detection is not an audio enhancement quality benchmark. Never deploy automatically."
    }
    args.output.mkdir(parents=True,exist_ok=True)
    onnx_path=args.output/"liberty-kick-candidate.onnx"
    export_onnx(network,onnx_path)
    report["onnx_sha256"]=hashlib.sha256(onnx_path.read_bytes()).hexdigest()
    (args.output/"evaluation.json").write_text(json.dumps(report,indent=2)+"\n")
    (args.output/"training_history.json").write_text(json.dumps(history,indent=2)+"\n")
    (args.output/"README.txt").write_text(
      "LIBERTY experimental real-audio kick/transient recognition weights.\n"
      "Dataset Google Magenta Groove v1.0.0 under CC BY 4.0, "
      "Gillick et al., ICML 2019.\n"
      "Not shipped in CYN. No full-mix audio quality or perceptual gain is claimed.\n"
      "Compare to baseline_test and trained_test in evaluation.json.\n")
    print("TRAINING RESULTS:",json.dumps(report,sort_keys=True),flush=True)


if __name__=="__main__":
    main()
