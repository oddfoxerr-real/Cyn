"""Apply experimental APEX denoiser to a WAV; no live system audio interception."""
from __future__ import annotations
import argparse
from pathlib import Path
import numpy as np
import soundfile as sf
import torch
from .model import Apex6M, SAMPLE_RATE, forward_audio


def apply(checkpoint: Path, source: Path, output: Path, wet: float = 1.0):
    if not (0.0 <= wet <= 1.0):
        raise ValueError("wet strength must be between zero and one")
    torch.set_num_threads(2)
    data, rate = sf.read(source, always_2d=True, dtype="float32")
    if rate != SAMPLE_RATE:
        raise ValueError("Experimental audio inference accepts 8 kHz WAV only; resample with an explicit high-quality converter")
    if not np.isfinite(data).all() or data.size == 0:
        raise ValueError("Audio must be finite, nonempty")
    if data.shape[1] > 2: raise ValueError("Only mono/stereo supported")
    state=torch.load(checkpoint,map_location="cpu",weights_only=True)
    model=Apex6M()
    model.load_state_dict(state["weights"])
    model.eval()
    # Overlap-add chunking for boundary-safe audio continuity.
    frames=rate
    hop=rate//2
    fade=np.hanning(frames).astype(np.float32)
    result=np.zeros_like(data)
    norm=np.zeros((data.shape[0],1),np.float32)
    with torch.no_grad():
        for start in range(0,data.shape[0],hop):
            end=min(data.shape[0],start+frames)
            segment=np.zeros((frames,data.shape[1]),np.float32)
            segment[:end-start]=data[start:end]
            waveform=torch.from_numpy(segment.T.copy())
            out,_,_=forward_audio(model,waveform)
            weights=fade[:end-start].copy()
            # Boundary of file cannot be windowed to 0 without losing audio.
            if start == 0: weights[:hop]=1
            if end == data.shape[0]:weights[-min(hop,len(weights)):]=1
            result[start:end] += out.T.numpy()[:end-start]*weights[:,None]
            norm[start:end] += weights[:,None]
    result/=np.maximum(norm,1e-6)
    result=(1-wet)*data+wet*result
    peak=np.max(np.abs(result))
    if peak>0.999: result*=0.999/peak
    output.parent.mkdir(parents=True,exist_ok=True)
    sf.write(output,result,rate,subtype="PCM_24")
    return output


def main():
    p=argparse.ArgumentParser()
    p.add_argument("source",type=Path)
    p.add_argument("output",type=Path)
    p.add_argument("--checkpoint",type=Path,default=Path("weights/apex6m-225.pt"))
    p.add_argument("--wet",type=float,default=1.0,help="Denoising strength 0..1 (0=bypass)")
    args=p.parse_args()
    print(apply(args.checkpoint,args.source,args.output,wet=args.wet))

if __name__=="__main__":main()