"""CPU-capable training and honest isolated evaluation of APEX-6M.

Avoid treating synthetic denoising benchmarks as evidence of real music superiority.
"""
from __future__ import annotations
import argparse, json, time
from pathlib import Path
import torch
import torch.nn.functional as F
from .model import Apex6M, count_parameters, forward_audio, stft
from .data import batch


def si_sdr(estimate: torch.Tensor, target: torch.Tensor) -> torch.Tensor:
    eps=1e-8
    e=estimate-estimate.mean(-1,keepdim=True)
    t=target-target.mean(-1,keepdim=True)
    aligned=(e*t).sum(-1,keepdim=True)*t/(t.square().sum(-1,keepdim=True)+eps)
    remaining=e-aligned
    return 10*torch.log10((aligned.square().sum(-1)+eps)/(remaining.square().sum(-1)+eps))


def evaluate(model: Apex6M, *, examples: int=20, seed: int=998):
    model.eval()
    values=[]
    with torch.no_grad():
        for i in range(0,examples,4):
            audio,clean,_=batch(seed+i, min(4,examples-i), "test")
            out,_,_=forward_audio(model,audio)
            original=si_sdr(audio,clean)
            denoised=si_sdr(out,clean)
            values.append(torch.stack((original,denoised),dim=-1))
    scores=torch.cat(values).mean(0)
    return {"input_si_sdr_db":round(float(scores[0]),3),"apex_si_sdr_db":round(float(scores[1]),3),"change_db":round(float(scores[1]-scores[0]),3),"test_examples":examples,"data":"held-out synthetic waveform recipes only"}


def train(args):
    torch.manual_seed(args.seed)
    torch.set_num_threads(args.threads)
    model=Apex6M()
    completed_steps=0
    if args.resume is not None:
        state=torch.load(args.resume,map_location="cpu",weights_only=True)
        if state.get("status")!="synthetic-only" or state.get("sample_rate")!=48000:
            raise ValueError("Only prior CYN APEX synthetic-only, 48 kHz checkpoints can resume")
        model.load_state_dict(state["weights"])
        completed_steps=int(state["training_steps"])
    params=count_parameters(model)
    if params<5_000_000: raise RuntimeError("APEX must have at least five million trainable parameters")
    opt=torch.optim.AdamW(model.parameters(),lr=args.lr,weight_decay=1e-4)
    output=Path(args.output);output.mkdir(parents=True,exist_ok=True)
    checkpoint=output/"apex6m-225.pt"
    before=evaluate(model,examples=args.test_examples)
    start=time.perf_counter()
    history=[]
    for step in range(1,args.steps+1):
        model.train()
        audio,clean,scene=batch(completed_steps+step+args.seed,args.batch_size)
        spec=stft(audio); target=stft(clean).abs()
        mask,logits=model(torch.log1p(spec.abs()))
        estimate=mask*spec.abs()
        # normalized, bounded spectral loss with a weak scene supervision term.
        spectral=F.l1_loss(torch.log1p(estimate),torch.log1p(target))
        scene_loss=F.binary_cross_entropy_with_logits(logits,scene)
        loss=spectral+0.05*scene_loss
        opt.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(),2.0)
        opt.step()
        if step==1 or step==args.steps or step%max(1,args.steps//5)==0:
            history.append({"step":step,"loss":round(float(loss.item()),5)})
            print(f"step {step}/{args.steps}: training_loss={loss.item():.5f}",flush=True)
        if step%25==0:
            torch.save({"weights":model.state_dict(),"params":params,"training_steps":completed_steps+step,"sample_rate":48000,"status":"synthetic-only"},checkpoint)
    after=evaluate(model,examples=args.test_examples)
    torch.save({"weights":model.state_dict(),"params":params,"training_steps":completed_steps+args.steps,"sample_rate":48000,"status":"synthetic-only"},checkpoint)
    report={"model":"CYN APEX-6M","parameters":params,"trained_steps":completed_steps+args.steps,"new_training_steps":args.steps,"resumed_from_steps":completed_steps,"elapsed_seconds":round(time.perf_counter()-start,2),"initial":before,"trained":after,"loss_history":history,"weights_file":checkpoint.name,"quality_proven_on_real_music":False,"beats_moises_or_lalal":False,"cyn_13h_comparison_available":False,"limitation":"Synthetic noise removal is a separate objective from Cyn's 13h bass/clarity preference model; SI-SDR improvement does not indicate a real-music superiority or user preference."}
    (output/"evaluation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2),flush=True)
    return report


def main():
    p=argparse.ArgumentParser()
    p.add_argument("--steps",type=int,default=80)
    p.add_argument("--resume",type=Path,default=None)
    p.add_argument("--batch-size",type=int,default=4)
    p.add_argument("--test-examples",type=int,default=20)
    p.add_argument("--threads",type=int,default=2)
    p.add_argument("--lr",type=float,default=0.0002)
    p.add_argument("--seed",type=int,default=20261017)
    p.add_argument("--output",type=str,default="reports")
    args=p.parse_args()
    if args.steps<1 or args.batch_size<1 or args.test_examples<1: p.error("Steps and batch sizes must be positive")
    train(args)

if __name__=="__main__":main()