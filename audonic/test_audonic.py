"""Preflight for the real AUDONIC student and conservative dataset gate."""
import hashlib,json,tempfile
from pathlib import Path
import numpy as np
import torch
import soundfile as sf
from model import AudonicStudent,count_parameters,OUTPUT_NAMES
from public_data import verify_manifest,features,load_verified_features
from train import make_scenes,targets

def check():
    torch.manual_seed(7)
    student=AudonicStudent()
    n=count_parameters(student)
    assert 10000<n<200000 and n!=3_000_000_000
    x=make_scenes(3,20,torch.Generator().manual_seed(19))
    y=targets(x)
    predicted=student(x)
    assert tuple(predicted.shape)==(3,len(OUTPUT_NAMES),20)
    assert tuple(y.shape)==tuple(predicted.shape)
    assert bool(torch.isfinite(y).all())
    assert bool(((predicted>=0)&(predicted<=1)).all())
    first=student.stem.weight.detach().clone()
    loss=(predicted-y).square().mean()
    loss.backward()
    torch.optim.SGD(student.parameters(),lr=.1).step()
    assert torch.max(torch.abs(first-student.stem.weight.detach())).item()>0
    with tempfile.TemporaryDirectory() as temp:
        directory=Path(temp)
        audio=np.sin(np.arange(2*16000)*.13).astype("float32")*.15
        sample=directory/"slakh-clip.wav"
        sf.write(sample,audio,16000)
        template={"source":"BabySlakh","items":[{
            "path":sample.name,"license":"CC-BY-4.0",
            "attribution":"Manilow et al. (2019), Slakh",
            "sha256":hashlib.sha256(sample.read_bytes()).hexdigest()
        }]}
        meta=directory/"manifest.json"
        meta.write_text(json.dumps(template))
        assert len(list(verify_manifest(meta)))==1
        tensor=features(np.column_stack([audio,audio]),16000)
        assert tensor.shape==(8,20) and np.isfinite(tensor).all()
        template["items"][0]["license"]="CC-BY-NC-4.0"
        meta.write_text(json.dumps(template))
        try:list(verify_manifest(meta))
        except ValueError:pass
        else:raise AssertionError("Commercially restricted sample erroneously allowed")
    print(f"PASS AUDONIC neural gradient step, {n} real student parameters, license filter and audio features")

if __name__=="__main__":check()
