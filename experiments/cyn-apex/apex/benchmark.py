"""Reproducible matched synthetic-waveform denoising benchmark.

Model is not evaluated on real songs; this is NOT competitive SOTA evidence.
"""
from __future__ import annotations
import argparse, json, time
from pathlib import Path
import torch
from .model import Apex6M, forward_audio, istft, stft, count_parameters
from .data import batch
from .train import si_sdr


def spectral_gate(audio: torch.Tensor) -> torch.Tensor:
    x=stft(audio)
    magnitude=x.abs()
    # Blind, per-frequency stationary-noise estimate (no reference/clean leakage).
    floor=magnitude.quantile(0.20,dim=-1,keepdim=True)
    gain=(1-(1.2*floor/(magnitude+1e-6))**1.25).clamp(0.07,1.0)
    return istft(x*gain,audio.shape[-1])


def compare(weights: Path, *, trials:int=96, threads:int=2, seed:int=910100):
    torch.set_num_threads(threads)
    state=torch.load(weights,map_location="cpu",weights_only=True)
    model=Apex6M().eval()
    model.load_state_dict(state["weights"])
    results=[]
    with torch.inference_mode():
        for i in range(0,trials,4):
            noisy,clean,_=batch(seed+i,min(4,trials-i),"test")
            apex,_,_=forward_audio(model,noisy)
            gate=spectral_gate(noisy)
            scores=torch.stack([si_sdr(noisy,clean),si_sdr(gate,clean),si_sdr(apex,clean)],dim=1)
            results.extend(scores.tolist())
        probe,_clean,_scene=batch(seed+100000,1,"test")
        _=forward_audio(model,probe)
        timings=[]
        for _ in range(20):
            t=time.perf_counter()
            _=forward_audio(model,probe)
            timings.append((time.perf_counter()-t)*1000)
    t=torch.tensor(results)
    def value(tensor):return round(float(tensor),4)
    return {"model":"CYN APEX-6M","params":count_parameters(model),"test_examples":trials,
      "seed":seed,"trained_steps":state["training_steps"],"sample_rate":48000,
      "mean_si_sdr_db":{"unprocessed":value(t[:,0].mean()),"classical_spectral_gate":value(t[:,1].mean()),"apex":value(t[:,2].mean())},
      "mean_improvement_vs_input_db":value((t[:,2]-t[:,0]).mean()),
      "mean_improvement_vs_gate_db":value((t[:,2]-t[:,1]).mean()),
      "apex_better_than_gate_fraction":value((t[:,2]>t[:,1]).float().mean()),
      "1s_cpu_forward_milliseconds":{"median":value(torch.tensor(timings).median()),"max":value(max(timings))},
      "actual_real_world_quality_proven":False,
      "baseline_cyn_13h": "not comparable: separate bass/clarity controller objective, no denoised waveform",
      "limitation":"Fresh held-out procedural bass/lead/drum/harmonic/noise mixtures, not real songs, voices, Moises, or LALAL.AI."}


def main():
    p=argparse.ArgumentParser();p.add_argument("--checkpoint",type=Path,default=Path("weights/apex6m-225.pt"));p.add_argument("--trials",type=int,default=96);p.add_argument("--output",type=Path,default=Path("reports/benchmark.json"));args=p.parse_args()
    report=compare(args.checkpoint,trials=args.trials)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))

if __name__=="__main__":main()