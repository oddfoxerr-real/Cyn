"""Cyn 11-12-5 base model: timed synthetic-only Adam training, 209 native weights."""
import argparse
import json
import time
from pathlib import Path
import numpy as np
import generate_starter as synth

ROOT = Path(__file__).resolve().parents[1]


def normalized(rows):
    x = np.array([r["x"] for r in rows], dtype=np.float64)
    x[:, :8] = (np.clip(x[:, :8], -12, 0) + 6) / 6
    x[:, 8] = np.clip(x[:, 8], 0, 4) / 2 - 1
    x[:, 9] = np.clip(x[:, 9], 0, 2) - 1
    x[:, 10] = (np.clip(x[:, 10], -8, 0) + 4) / 4
    y = np.array([[r["clarity"], r["bass"]] for r in rows], dtype=np.float64)
    return x, y


def forward(x, w):
    h = np.tanh(x @ w[:132].reshape(12, 11).T + w[132:144])
    return h, h @ w[144:204].reshape(5, 12).T + w[204:209]


def metrics(x, y, w):
    z = forward(x, w)[1][:, [0, 4]]
    p = 1 / (1 + np.exp(-np.clip(z, -30, 30)))
    report = {}
    for i, name in enumerate(("clarity", "bass")):
        actual, pred = y[:, i] > .5, p[:, i] >= .5
        pos, neg = actual.sum(), (~actual).sum()
        report[name] = {
            "accuracy": float(np.mean(actual == pred)),
            "balanced_accuracy": float(((actual & pred).sum()/pos + ((~actual)&(~pred)).sum()/neg)/2),
            "log_loss": float(np.mean(np.logaddexp(0, z[:, i]) - y[:, i]*z[:, i])),
            "false_positive_rate": float(((~actual)&pred).sum()/neg),
            "false_negative_rate": float((actual&(~pred)).sum()/pos)
        }
    report["mean_log_loss"] = (report["clarity"]["log_loss"] + report["bass"]["log_loss"])/2
    return report


def run(seconds, destination):
    current = json.loads((ROOT/"models/cyn-starter.json").read_text())
    initial = np.asarray(current["weights"], dtype=np.float64)
    assert initial.shape == (209,) and np.isfinite(initial).all()
    prepare = time.monotonic()
    x,y = normalized(synth.dataset(910012, 1800))
    xv,yv = normalized(synth.dataset(910013, 500))
    xt,yt = normalized(synth.dataset(910014, 500))
    print(f"Prepared independent synthetic train/validation/test sets in {time.monotonic()-prepare:.1f}s", flush=True)
    old_v, old_t = metrics(xv,yv,initial), metrics(xt,yt,initial)
    rng = np.random.default_rng(20261010)
    w, m, v = initial.copy(), np.zeros(209), np.zeros(209)
    counts = np.array([[(y[:,i]==j).sum() for j in (0,1)] for i in range(2)])
    assert np.all(counts > 0)
    best, best_validation, score = None, None, old_v["mean_log_loss"]
    steps, improvements, last_log = 0, 0, -60
    start = time.monotonic()
    # The clock starts with optimization; every iteration executes backpropagation.
    while True:
        ids = rng.integers(0,len(x),256)
        xb = x[ids] + rng.normal(0,[.05]*8+[.04,.02,.05],size=(256,11))
        yb = y[ids]
        h,z = forward(xb,w)
        p = 1/(1+np.exp(-np.clip(z[:,[0,4]],-30,30)))
        class_weights = np.column_stack([
            np.where(yb[:,i]>.5,len(x)/(2*counts[i,1]),len(x)/(2*counts[i,0]))
            for i in range(2)
        ])
        delta = np.zeros((256,5))
        delta[:,[0,4]] = (p-yb)*class_weights/512
        g = np.zeros(209)
        g[144:204] = (delta.T@h).ravel()
        g[204:209] = delta.sum(axis=0)
        dh = (delta@w[144:204].reshape(5,12))*(1-h*h)
        g[:132] = (dh.T@xb).ravel()
        g[132:144] = dh.sum(axis=0)
        g[156:192] = 0
        g[205:208] = 0
        g += .00002*(w-initial)
        steps += 1
        m = .9*m+.1*g
        v = .999*v+.001*g*g
        w -= .0012*(m/(1-.9**steps))/(np.sqrt(v/(1-.999**steps))+1e-8)
        if not np.isfinite(w).all():
            raise RuntimeError("Non-finite trained weights")
        if steps%128==0:
            check=metrics(xv,yv,w)
            safe=all(check[name]["balanced_accuracy"]>=old_v[name]["balanced_accuracy"]-.01
                     for name in ("clarity","bass"))
            if safe and check["mean_log_loss"]<score-1e-9:
                best,best_validation,score=w.copy(),check,check["mean_log_loss"]
                improvements+=1
            elapsed=time.monotonic()-start
            if elapsed-last_log>=60:
                print(f"Optimizer active {elapsed:.1f}/{seconds}s; {steps} gradient steps; "
                      f"{improvements} improved checkpoints; val loss {score:.6f}",flush=True)
                last_log=elapsed
        if time.monotonic()-start>=seconds:
            break

    elapsed=time.monotonic()-start
    if best is None:
        raise RuntimeError("No checkpoint improved on the original starter after training")
    final=metrics(xt,yt,best)
    for name in ("clarity","bass"):
        if final[name]["balanced_accuracy"]+.015<old_t[name]["balanced_accuracy"]:
            raise RuntimeError("Final held-out evaluation degraded "+name)
    if final["mean_log_loss"]>old_t["mean_log_loss"]*1.02:
        raise RuntimeError("Final held-out loss regressed")
    updated=dict(current)
    updated["name"]="Cyn starter 2 (hour-trained)"
    updated["weights"]=[float(i) for i in best]
    updated["ready"]=[True,False,False,False,True]
    report={"training":{"actual_gradient_seconds":elapsed,"optimizer_steps":steps,
                        "validated_improvements":improvements,
                        "synthetic_train_mixtures":len(x),
                        "synthetic_validation_mixtures":len(xv),
                        "synthetic_test_mixtures":len(xt)},
            "baseline_validation":old_v,"trained_validation":best_validation,
            "baseline_test":old_t,"trained_test":final,
            "limitation":"Synthetic signal mixtures, not real songs or personal listening preferences."}
    destination.mkdir(exist_ok=True,parents=True)
    (destination/"cyn-starter.json").write_text(json.dumps(updated,indent=2)+"\n")
    (destination/"starter-evaluation.json").write_text(json.dumps(report,indent=2)+"\n")
    md=["## Hour-trained experimental Cyn base",
        f"Gradient optimization: **{elapsed:.1f}s**; **{steps:,} gradient updates**; {improvements} validated checkpoints.",
        "","| Synthetic held-out metric | Original | Hour-trained |","|---|---:|---:|"]
    for name in ("clarity","bass"):
        md.append(f"| {name.title()} balanced accuracy | {old_t[name]['balanced_accuracy']:.2%} | {final[name]['balanced_accuracy']:.2%} |")
        md.append(f"| {name.title()} log loss | {old_t[name]['log_loss']:.6f} | {final[name]['log_loss']:.6f} |")
    md.extend(["","**Limitations:** All examples were synthetically generated. No real music, human preferences, "
               "vocal detection or source separation was evaluated. Only Bass and Clarity have starter labels.",
               "Existing personal learning remains intact. Select **Reset to starter** in Smart listening "
               "to activate the new bundled base model on an existing installation."])
    (destination/"training-report.md").write_text("\n".join(md)+"\n")
    print("One-hour neural training finished: "+json.dumps(report),flush=True)


if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--duration",type=int,default=3600)
    p.add_argument("--output",type=Path,default=ROOT/"training-output")
    a=p.parse_args()
    if a.duration<=0:p.error("duration must be positive")
    run(a.duration,a.output)
