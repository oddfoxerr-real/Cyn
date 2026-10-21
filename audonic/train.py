"""Real gradient training for the AUDONIC mobile creative-policy student.

Training labels combine safe procedural DSP objectives and optional distillation
from CYN's *actual* APEX10M ONNX policy (bass/kick/phase/headroom teacher).
Public Slakh features may be added only through verified CC BY 4.0 manifests.
No fictional 3B checkpoint, fabricated training duration, or automatic audio FX.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time

import numpy as np
import torch
from torch.nn import functional as F
from model import AudonicStudent, OUTPUT_NAMES, count_parameters

def make_scenes(batch,frames,generator):
    v=torch.rand(batch,8,frames,generator=generator)
    v[:,3]=v[:,3]*2-1
    v[:,7]=v[:,7]*2-1
    # Create short coherent events; otherwise there is no useful temporal
    # structure for an FX-scheduling model to learn.
    beat=torch.rand(batch,1,frames,generator=generator)<.17
    v[:,1]=(v[:,1]*.15+beat[:,0]*.8).clamp(0,1)
    v[:,4]=(v[:,4]*.18+beat[:,0]*.6).clamp(0,1)
    v[:,0]=F.avg_pool1d(v[:,0:1],5,1,2)[:,0]
    v[:,5]=F.avg_pool1d(v[:,5:6],7,1,3)[:,0]
    return v

def targets(x,teacher=None):
    bass,kick,side,corr,flux,vocal,room,direction=[x[:,i] for i in range(8)]
    safe=((corr+.25)/.8).clamp(0,1)*room*(1-.83*vocal)
    prev=F.pad(kick[:,:-1],(1,0))
    onset=(kick-prev).relu()
    local=F.avg_pool1d(kick[:,None],5,1,2)[:,0]
    # Bass affects the harmonic strategy and protects fundamental/mono bass.
    bass_protection=1-.75*bass
    kick_protection=1-.70*local
    if teacher is not None:
        # APEX10M's learned temporal controls are distilled rather than
        # copying incompatible parameter tensors into the AUDONIC student.
        spatial=teacher[:,2].clamp(0,1)
        kick_protection*=1-.30*teacher[:,3].clamp(0,1)
        bass_protection*=1-.30*teacher[:,4].clamp(0,1)
    else:
        spatial=safe
    outputs=[
        (.50*onset+.22*flux+.10*bass).clamp(0,1)*room*(1-.8*vocal),
        (.50*onset+.30*flux).clamp(0,1)*(.35+.65*room),
        (.46*spatial+.21*direction.abs()).clamp(0,1)*safe*(1-.5*side),
        (.26*onset+.21*flux).clamp(0,1)*room*(1-.70*vocal),
        (.14*onset+.13*flux).clamp(0,1)*room*(1-.7*vocal),
        (.35*bass+.18*onset).clamp(0,1)*bass_protection*room,
    ]
    return torch.stack(outputs,dim=1)

def main():
    p=argparse.ArgumentParser()
    p.add_argument("--steps",type=int,default=600)
    p.add_argument("--teacher",type=Path,default=None,
                   help="Frozen APEX10M release ONNX; no teacher, no falsely claimed distillation")
    p.add_argument("--public-features",type=Path,default=None,
                   help="License-verified Slakh features from public_data.py")
    p.add_argument("--output",type=Path,default=Path("dist/audonic"))
    args=p.parse_args()
    if args.steps<5 or args.steps>150000:p.error("Invalid update count")
    torch.set_num_threads(2)
    torch.manual_seed(261021)
    rng=torch.Generator().manual_seed(261021)
    model=AudonicStudent()
    optimizer=torch.optim.AdamW(model.parameters(),lr=.001,weight_decay=.0002)
    teacher=None
    if args.teacher:
        import onnxruntime as ort
        teacher=ort.InferenceSession(str(args.teacher),providers=["CPUExecutionProvider"])
        if [i.name for i in teacher.get_inputs()]!=["features"] or            "policy_controls" not in [o.name for o in teacher.get_outputs()]:
            raise RuntimeError("Unsupported APEX10M teacher ONNX outputs")
    public=None
    if args.public_features:
        from public_data import load_verified_features
        public=load_verified_features(args.public_features)
        if len(public)==0:raise RuntimeError("Empty approved Slakh manifest")
    begun=time.monotonic()
    loss_value=float("nan")
    for step in range(args.steps):
        x=make_scenes(12,20,rng)
        if public is not None and step%3==0:
            ids=torch.randint(len(public),(4,),generator=rng).tolist()
            x[:4]=torch.from_numpy(public[ids])
        teacher_controls=None
        if teacher:
            teacher_controls=torch.from_numpy(
                teacher.run(["policy_controls"],{"features":x.numpy()})[0])
            assert tuple(teacher_controls.shape)==(12,8,20)
        expected=targets(x,teacher_controls)
        predicted=model(x)
        mse=F.mse_loss(predicted,expected)
        # Discourage flicker and dramatic parameter jumps.
        smooth=(predicted[:,:,1:]-predicted[:,:,:-1]).square().mean()
        loss=mse+smooth*.03
        if not bool(torch.isfinite(loss)):raise RuntimeError("Nonfinite AUDONIC loss")
        optimizer.zero_grad(set_to_none=True)
        loss.backward()
        torch.nn.utils.clip_grad_norm_(model.parameters(),1.0)
        optimizer.step()
        loss_value=float(loss.item())
        if (step+1)%100==0:print(f"AUDONIC gradient updates={step+1} loss={loss_value:.6f}",flush=True)
    model.eval()
    # Validation never reuses the training generator.
    holdout=make_scenes(12,20,torch.Generator().manual_seed(77555))
    teacher_holdout=None
    if teacher:teacher_holdout=torch.from_numpy(teacher.run(
        ["policy_controls"],{"features":holdout.numpy()})[0])
    with torch.no_grad():
        validation=float(F.mse_loss(model(holdout),targets(holdout,teacher_holdout)).item())
    args.output.mkdir(parents=True,exist_ok=True)
    weights=args.output/"audonic-student.pt"
    torch.save({"state_dict":model.state_dict(),"steps":args.steps,
                "seed":261021,"teacher_used":bool(teacher),
                "public_dataset_used":bool(public is not None)},weights)
    path=args.output/"audonic-student.onnx"
    dummy=torch.zeros(1,8,20)
    torch.onnx.export(model,dummy,path,opset_version=17,
                      input_names=["features"],output_names=["fx_controls"],
                      dynamic_axes={"features":{2:"frames"},
                                    "fx_controls":{2:"frames"}},
                      do_constant_folding=True,dynamo=False)
    import onnxruntime as ort
    session=ort.InferenceSession(str(path),providers=["CPUExecutionProvider"])
    maxdiff=0
    for frames in (20,31):
        sample=make_scenes(1,frames,torch.Generator().manual_seed(11))
        with torch.no_grad():ref=model(sample).numpy()
        actual=session.run(["fx_controls"],{"features":sample.numpy()})[0]
        maxdiff=max(maxdiff,float(np.max(np.abs(ref-actual))))
        if maxdiff>5e-4:raise RuntimeError("ONNX parity failed")
    metrics={"model":"AUDONIC neural creative-FX student",
             "parameter_count":count_parameters(model),
             "claimed_3b_parameters":False,"three_b_teacher_trained":False,
             "gradient_updates":args.steps,"training_seconds":round(time.monotonic()-begun,2),
             "synthetic_policy_training":True,
             "apex10m_teacher_distillation":bool(teacher),
             "apex6m_base_reused_as_existing_cyn_engine":True,
             "public_slakh_features_used":bool(public is not None),
             "real_music_quality_verified":False,"loss":loss_value,
             "validation_mse":validation,"onnx_max_abs_parity_error":maxdiff,
             "fx_controls":list(OUTPUT_NAMES),
             "onnx_sha256":hashlib.sha256(path.read_bytes()).hexdigest()}
    (args.output/"audonic-manifest.json").write_text(json.dumps(metrics,indent=2)+"\n")
    print(json.dumps(metrics,indent=2),flush=True)

if __name__=="__main__":main()
