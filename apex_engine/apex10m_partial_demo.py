"""Generate short original royalty-free synthetic WAV for mobile A/B listening."""
from __future__ import annotations
from pathlib import Path
import numpy as np
import soundfile as sf

def create(out: Path, seconds: float = 12.0, rate: int = 48000):
    t=np.arange(int(seconds*rate),dtype=np.float64)/rate
    beat=t*2.0  # 120 BPM
    onset=np.mod(beat,1.0)
    kick_env=np.exp(-onset*31.0)
    kick_phase=(2*np.pi*(52*t+15/31*(1-np.exp(-onset*31.0))))
    kick=.37*kick_env*np.sin(kick_phase)
    sub=.16*np.sin(2*np.pi*(55*t+.03*np.sin(.35*t)))
    pad=(.10*np.sin(2*np.pi*330*t)+.10*np.sin(2*np.pi*550*t))*(.65+.35*np.sin(2*np.pi*.15*t)**2)
    # already-stereo, high-frequency location cues that a side-only DSP can change
    side=(.115*np.sin(2*np.pi*3600*t + 1.2*np.sin(t*1.3))
           +.04*np.sin(2*np.pi*6200*t+np.sin(t*.8)))
    side *= .4+.6*np.sin(np.pi*(t/seconds))**2
    dry=np.column_stack([kick+sub+pad+side,kick+sub+pad-side])
    peak=np.max(np.abs(dry))
    if peak>.79:dry*=.79/peak
    out.parent.mkdir(parents=True,exist_ok=True)
    sf.write(out,dry.astype(np.float32),rate,subtype="PCM_16")
    return out

if __name__=="__main__":
    import sys
    create(Path(sys.argv[1] if len(sys.argv)>1 else "demo-dry.wav"))
