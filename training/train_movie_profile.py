"""Train Cyn Cinema AI with reproducible *synthetic* dialogue/effects mixtures.

This is not speech recognition, source separation, or validation on real movies.
The profile preserves the 11x12x5 architecture and trains ONLY Clarity and Bass.
A separate train/validation/test split guards against shipping an unchanged starter.
"""
from pathlib import Path
import json
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "ai-hub" / "community"
INPUT_MODEL = ROOT / "models" / "cyn-starter.json"
SPEECH = np.array([.05,.08,.17,.40,.82,1.00,.38,.11])
LFE = np.array([1.60,1.28,.76,.22,.09,.04,.03,.02])
EFFECTS = np.array([.26,.45,.61,.78,.90,.86,1.05,.77])
FREQUENCIES = np.array([45,75,120,200,400,900,2200,5000])

def samples(seed, count):
    rng = np.random.default_rng(seed)
    speech = rng.beta(2.0,2.0,count)
    bass = rng.beta(1.6,2.2,count)
    effects = rng.beta(2.0,2.2,count)
    dull = rng.beta(1.7,2.2,count)
    energy = (.10+speech[:,None]*SPEECH
              + bass[:,None]*LFE
              + effects[:,None]*EFFECTS)
    energy *= np.exp(rng.normal(0.0,.12,(count,8)))
    energy *= 1.0-dull[:,None]*(FREQUENCIES[None,:]/5200.0)**.58*.82
    energy = np.maximum(energy,1e-6)
    bandlog = np.log(energy/energy.sum(axis=1,keepdims=True))
    crest = np.clip(1.05+.5*effects+.25*bass+rng.normal(0,.2,count),0,4)
    stereo = np.clip(.24+.55*effects-.15*speech+rng.normal(0,.11,count),0,2)
    log_rms = np.clip(-2.9+1.2*effects+.4*bass+rng.normal(0,.32,count),-8,0)
    x=np.column_stack([bandlog,crest,stereo,log_rms])
    # Independent heuristic preference policy (not ground-truth listener labels).
    # Cinematic speech can benefit from more clarity, but booming LFE should NOT
    # always trigger greater Bass. Keep effects available without maximal boost.
    clarity=np.clip(.48+.32*speech+.12*dull-.08*effects*(1-speech),.28,.87)
    controlled_bass=np.clip(.49-.18*bass+.035*effects*(1-bass),.24,.56)
    target=np.column_stack([clarity,controlled_bass])
    normalized=x.copy()
    normalized[:,:8]=(np.clip(x[:,:8],-12,0)+6)/6
    normalized[:,8]=np.clip(x[:,8],0,4)/2-1
    normalized[:,9]=np.clip(x[:,9],0,2)-1
    normalized[:,10]=(np.clip(x[:,10],-8,0)+4)/4
    return normalized,target

def forward(x,w):
    h=np.tanh(x @ w[:132].reshape(12,11).T + w[132:144])
    z=h @ w[144:204].reshape(5,12).T + w[204:209]
    p=1/(1+np.exp(-np.clip(z,-30,30)))
    return h,p

def metrics(x,y,w):
    p=forward(x,w)[1][:,[0,4]]
    losses=np.mean((p-y)**2,axis=0)
    return {"clarity_mse":float(losses[0]),
            "bass_mse":float(losses[1]),
            "mean_mse":float(np.mean(losses)),
            "mean_clarity_prediction":float(p[:,0].mean()),
            "mean_bass_prediction":float(p[:,1].mean()),
            "max_bass_prediction":float(p[:,1].max()),
            "percentage_bass_above_0_65":float(np.mean(p[:,1]>.65))}

def run():
    starter=json.loads(INPUT_MODEL.read_text(encoding="utf-8"))
    initial=np.asarray(starter["weights"],dtype=np.float64)
    assert initial.shape==(209,) and np.isfinite(initial).all()
    train_x,train_y=samples(74510,2200)
    valid_x,valid_y=samples(74511,500)
    test_x,test_y=samples(74512,500)
    baseline=metrics(test_x,test_y,initial)
    rng=np.random.default_rng(74513)
    w=initial.copy()
    momentum=np.zeros(209)
    variance=np.zeros(209)
    best=initial.copy()
    bestscore=float("inf")
    snapshots=0
    for step in range(1,6001):
        take=rng.integers(0,len(train_x),256)
        x=train_x[take]; y=train_y[take]
        h,p=forward(x,w)
        grad_output=np.zeros((len(x),5))
        # BCE with soft heuristic targets; 2 supervised heads, weighted evenly.
        grad_output[:,[0,4]]=(p[:,[0,4]]-y)/(2*len(x))
        grad=np.zeros(209)
        grad[144:204]=(grad_output.T@h).reshape(-1)
        grad[204:209]=grad_output.sum(axis=0)
        hidden_gradient=(grad_output@w[144:204].reshape(5,12))*(1-h*h)
        grad[:132]=(hidden_gradient.T@x).reshape(-1)
        grad[132:144]=hidden_gradient.sum(axis=0)
        # Preserve unsupervised output head weights, while permitting shared hidden updates.
        grad[156:192]=0
        grad[205:208]=0
        grad+=1e-6*(w-initial)
        momentum=.9*momentum+.1*grad
        variance=.999*variance+.001*grad*grad
        correction=(momentum/(1-.9**step))/(np.sqrt(variance/(1-.999**step))+1e-8)
        w-=.0025*correction
        if not np.isfinite(w).all():
            raise ValueError("Nonfinite training weights")
        if step%100==0:
            observed=metrics(valid_x,valid_y,w)
            score=observed["mean_mse"]
            if (score<bestscore
                    and observed["max_bass_prediction"]<.66
                    and observed["mean_clarity_prediction"]>.52):
                best=w.copy();bestscore=score;snapshots+=1
    final=metrics(test_x,test_y,best)
    assert snapshots>0, "No validated profile checkpoint"
    assert final["clarity_mse"] < baseline["clarity_mse"]*.92, "Clarity did not improve"
    assert final["bass_mse"] < baseline["bass_mse"]*.92, "Bass policy did not improve"
    assert final["max_bass_prediction"]<.68, "Bass predicted near maximum"
    assert final["percentage_bass_above_0_65"]==0, "Bass runaway"
    assert final["mean_clarity_prediction"]>.52, "No dialogue-focused clarity behavior"
    profile={
      "format":"cyn-ai-profile",
      "schema":1,
      "profile":{
        "name":"Cyn Cinema AI",
        "author":"Cyn contributors",
        "category":"cinema",
        "description":"Experimental dialogue-forward sound tuning with controlled bass for movies and series. Trained on generated spectral feature mixtures, not real film soundtracks. Does not identify voices or understand dialogue. A starting profile to personalize with More/Less.",
        "license":"CC-BY-4.0"
      },
      "model":{
        "schema":1,
        "architecture":"cyn-audio-11x12x5",
        "weights":[round(float(v),9) for v in best],
        "ready":[True,False,False,False,True]
      }
    }
    assert len(profile["model"]["weights"])==209
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/"cyn-cinema.cynai.json").write_text(json.dumps(profile,indent=2)+"\n",encoding="utf-8")
    index=json.loads((OUT/"index.json").read_text(encoding="utf-8"))
    assert index["schema"]==1
    entry={
      "id":"cyn-cinema",
      "name":"Cyn Cinema AI",
      "author":"Cyn contributors",
      "category":"cinema",
      "description":"Experimental Movie & Series profile: dialogue-forward Clarity and restrained Bass; synthetic-feature-trained, not speech-aware. Start here and teach your preferences with More/Less.",
      "file":"cyn-cinema.cynai.json",
      "issue":0,
      "license":"CC-BY-4.0"
    }
    index["profiles"]=[p for p in index["profiles"] if p.get("id")!="cyn-cinema"]+[entry]
    (OUT/"index.json").write_text(json.dumps(index,indent=2)+"\n",encoding="utf-8")
    report={
      "description":"Experimental synthetic-feature training for Movie & Series profile",
      "optimizer_steps":6000,
      "dataset":{"train":2200,"validation":500,"test":500,
                 "all_synthetic":True,"independent_seeds":True},
      "baseline_heldout":baseline,
      "cinema_heldout":final,
      "limit":"Synthetic heuristics only. Not tested against real movie dialogue, listener choices, source separation, or loudness standards. No speech understanding."
    }
    (OUT/"cyn-cinema-evaluation.json").write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print("PASS Cinema AI movie profile generated and held-out synthetic tests passed")
    print(json.dumps(report,indent=2))

if __name__=="__main__":
    run()
