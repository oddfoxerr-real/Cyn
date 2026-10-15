#!/usr/bin/env python3
"""Perform actual gradient updates for a specified wall-clock period.

Stages are independent GitHub Actions jobs; real numerical optimization (not
sleeping) runs during each stage. Best weights and run metrics are checkpointed.
"""
from __future__ import annotations
import argparse
import json
import math
import time
from pathlib import Path
import numpy as np

ROOT=Path(__file__).resolve().parents[1]


def infer(w,x):
    h=np.tanh(x@w[:132].reshape(12,11).T+w[132:144])
    z=h@w[144:204].reshape(5,12).T+w[204:209]
    p=1/(1+np.exp(-np.clip(z[:,[0,4]],-30,30)))
    return h,p


def metrics(w, x,y,role):
    p=infer(w,x)[1]
    mse=np.mean((p-y)**2,axis=0)
    return {"clarity_mse":float(mse[0]),"bass_mse":float(mse[1]),
            "mean_mse":float(mse.mean()),
            "max_bass":float(p[:,1].max()),
            "role_mse":{str(k):float(np.mean((p[role==k]-y[role==k])**2))
                        for k in sorted(set(role.tolist()))}}


def run(duration, input_model, output, seed):
    output.mkdir(parents=True,exist_ok=True)
    source=ROOT/"models/cyn-starter.json"
    initial=np.asarray(json.loads(source.read_text())["weights"],dtype=np.float64)
    if input_model is None:
        w=initial.copy()
        cumulative_s=0.; cumulative_steps=0
    else:
        state=json.loads(input_model.read_text())
        w=np.asarray(state["weights"],dtype=np.float64)
        cumulative_s=float(state["optimization_seconds"])
        cumulative_steps=int(state["gradient_steps"])
    if w.shape!=(209,) or not np.isfinite(w).all():raise ValueError("Malformed weights")
    data={}
    for name in ("train","validation","test"):
        with np.load(Path("real-audio-data")/(name+".npz")) as d:
            data[name]=(d["x"].astype(np.float64),d["y"].astype(np.float64),d["role"])
    x,y,_=data["train"]
    xv,yv,rv=data["validation"]
    baseline=metrics(initial,xv,yv,rv)
    best=w.copy()
    bestm=metrics(best,xv,yv,rv)
    bestscore=bestm["mean_mse"]
    rng=np.random.default_rng(seed)
    momentum=np.zeros(209)
    variance=np.zeros(209)
    a=.0;b=.0
    start=time.monotonic()
    last_log=start
    last_save=start
    steps=0
    observations=0
    improvements=0

    def persist(done=False):
        info={"schema":1,"weights":[float(v) for v in best],
              "optimization_seconds":cumulative_s+(time.monotonic()-start),
              "gradient_steps":cumulative_steps+steps,
              "seed":seed, "best_validation":bestm,
              "baseline_validation":baseline,
              "validation_improvements":improvements,
              "complete":done,
              "training_data":"CC BY 4.0 authentic speech and instrument recordings",
              "training_targets":"Heuristic preferences, not listening-quality ground truth"}
        tmp=output/"checkpoint.json.tmp"
        tmp.write_text(json.dumps(info)+"\n")
        tmp.replace(output/"checkpoint.json")
        (output/"metrics.json").write_text(json.dumps({k:v for k,v in info.items() if k!="weights"},indent=2)+"\n")

    # Never spend time sleeping to fake progress. Every loop runs optimizer math.
    while True:
        elapsed=time.monotonic()-start
        if elapsed>=duration:break
        ids=rng.integers(0,len(x),size=256)
        xb=x[ids]+rng.normal(0,.017,size=(256,11))
        yb=y[ids]
        h,p=infer(w,xb)
        delta=np.zeros((256,5),dtype=np.float64)
        delta[:,[0,4]]=2*(p-yb)*p*(1-p)/256
        g=np.zeros(209,dtype=np.float64)
        g[144:204]=(delta.T@h).ravel()
        g[204:209]=delta.sum(axis=0)
        dh=(delta@w[144:204].reshape(5,12))*(1-h*h)
        g[:132]=(dh.T@xb).ravel()
        g[132:144]=dh.sum(axis=0)
        g[156:192]=0
        g[205:208]=0
        g+=.000002*(w-initial)
        steps+=1
        a=.9*a+.1
        b=.999*b+.001
        momentum=.9*momentum+.1*g
        variance=.999*variance+.001*g*g
        lr=.0011 if cumulative_s+elapsed<4*3600 else .00035
        w-=lr*(momentum/a)/(np.sqrt(variance/b)+1e-8)
        if not np.isfinite(w).all():raise RuntimeError("Optimizer produced nonfinite weights")

        if steps%300==0:
            now=time.monotonic()
            if now-last_log>=60:
                m=metrics(w,xv,yv,rv)
                observations+=1
                stable=(m["max_bass"]<.86 and m["clarity_mse"]<baseline["clarity_mse"]*1.05
                        and m["bass_mse"]<baseline["bass_mse"]*1.05)
                if stable and m["mean_mse"]<bestscore-1e-9:
                    best=w.copy();bestm=m;bestscore=m["mean_mse"]
                    improvements+=1
                print(json.dumps({"elapsed_s":round(elapsed),"step":cumulative_steps+steps,
                                  "current_val_mse":m["mean_mse"],
                                  "best_val_mse":bestscore,
                                  "baseline_val_mse":baseline["mean_mse"]}),flush=True)
                last_log=now
            if now-last_save>=120:
                persist(False)
                last_save=now
    persist(True)
    result=metrics(best,*data["test"])
    print(json.dumps({"stage_gradient_seconds":round(time.monotonic()-start,2),
        "total_gradient_seconds":round(cumulative_s+time.monotonic()-start,2),
        "gradient_steps":cumulative_steps+steps,"heldout_observation_not_for_selection":result,
        "best_validation":bestm}),flush=True)


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--seconds",type=int,required=True)
    p.add_argument("--seed",type=int,required=True)
    p.add_argument("--input-checkpoint",type=Path)
    p.add_argument("--output",type=Path,required=True)
    args=p.parse_args()
    if args.seconds<1:raise ValueError("seconds must be positive")
    run(args.seconds,args.input_checkpoint,args.output,args.seed)


if __name__=="__main__":
    main()
