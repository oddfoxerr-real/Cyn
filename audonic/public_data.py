"""License-gated feature extraction for AUDONIC's optional Slakh corpus.

No audio is packaged into CYN or uploaded. You must supply an authorized local
Slakh / BabySlakh WAV or FLAC track and a manifest with exact SHA-256 and
attribution. Only CC0 and CC-BY-4.0 are accepted by default. A Slakh
whole-dataset license does not authorize unrelated third-party clips.
"""
from __future__ import annotations
import argparse,hashlib,json
from pathlib import Path
import numpy as np

ALLOWED={"CC0-1.0","CC-BY-4.0"}

def verify_manifest(path):
    manifest=json.loads(Path(path).read_text(encoding="utf-8"))
    if manifest.get("source") not in ("Slakh2100","BabySlakh"):
        raise ValueError("Unrecognized dataset; separate license audit required")
    entries=manifest.get("items",[])
    if not isinstance(entries,list) or not entries:raise ValueError("No permitted audio items")
    base=Path(path).resolve().parent
    for item in entries:
        if item.get("license") not in ALLOWED:
            raise ValueError("Unapproved license: "+str(item.get("license")))
        if item.get("attribution")!="Manilow et al. (2019), Slakh":
            raise ValueError("Source attribution required")
        name=item.get("path","")
        if not name or Path(name).is_absolute() or ".." in Path(name).parts:
            raise ValueError("Unsafe input path")
        source=(base/name).resolve()
        if not source.is_relative_to(base) or not source.is_file():
            raise ValueError("Source must be local and under manifest folder")
        expected=item.get("sha256","").lower()
        if len(expected)!=64 or any(x not in "0123456789abcdef" for x in expected):
            raise ValueError("Full SHA-256 of audio is required")
        actual=hashlib.sha256(source.read_bytes()).hexdigest()
        if actual!=expected:raise ValueError("Audio checksum does not match manifest")
        yield source

def features(audio,rate):
    """20 real 100-ms frames, approximate native CYN feature ordering."""
    frame=int(rate*.1)
    if frame<80:raise ValueError("Sample rate too low")
    if audio.ndim==1:audio=np.column_stack([audio,audio])
    if audio.ndim!=2 or audio.shape[1]!=2:raise ValueError("Expected mono or stereo")
    if not np.isfinite(audio).all():raise ValueError("Invalid samples")
    groups=[]
    previous_low=previous_rms=0.
    for i in range(20):
        segment=audio[i*frame:(i+1)*frame]
        if len(segment)<frame:break
        left=segment[:,0];right=segment[:,1]
        mid=(left+right)*.5;side=(left-right)*.5
        fft=np.abs(np.fft.rfft(mid*np.hanning(len(mid))))
        freq=np.fft.rfftfreq(len(mid),1/rate)
        e=fft*fft
        total=float(np.sum(e))+1e-12
        bass=float(np.sum(e[freq<170])/total)
        voice=float(np.sum(e[(freq>=350)&(freq<3500)])/total)
        rms=float(np.sqrt(np.mean(mid**2)))
        side_rms=float(np.sqrt(np.mean(side**2)))
        correlation=float(np.sum(left*right)/(np.sqrt(np.sum(left*left)*np.sum(right*right))+1e-9))
        kick=float(np.clip(max(0,bass-previous_low)*2+max(0,rms-previous_rms)*4,0,1))
        flux=float(np.clip(max(0,rms-previous_rms)*6,0,1))
        l=float(np.sqrt(np.mean(left*left)));r=float(np.sqrt(np.mean(right*right)))
        groups.append([bass,kick,float(np.clip(side_rms/max(side_rms+rms,1e-8),0,1)),
            float(np.clip(correlation,-1,1)),flux,voice,
            float(np.clip(1-np.max(np.abs(segment)),0,1)),
            float(np.clip((l-r)/max(l+r,1e-8),-1,1))])
        previous_low,previous_rms=bass,rms
    if len(groups)!=20:return None
    return np.asarray(groups,dtype=np.float32).T

def load_verified_features(archive):
    with np.load(archive,allow_pickle=False) as data:
        result=data["features"]
        if "licenses" not in data or "sources" not in data:raise ValueError("Missing license provenance")
        licenses=list(data["licenses"])
        sources=list(data["sources"])
        if any(x not in ALLOWED for x in licenses) or any(x not in ("Slakh2100","BabySlakh") for x in sources):
            raise ValueError("Unapproved corpus provenance")
        if len(licenses)!=len(result) or len(sources)!=len(result):
            raise ValueError("Missing per-item provenance")
    if result.ndim!=3 or result.shape[1:]!=(8,20) or not np.isfinite(result).all():
        raise ValueError("Invalid feature tensor")
    return result

def main():
    import soundfile as sf
    p=argparse.ArgumentParser()
    p.add_argument("--manifest",required=True,type=Path)
    p.add_argument("--output",required=True,type=Path)
    args=p.parse_args()
    vectors=[]
    for source in verify_manifest(args.manifest):
        audio,rate=sf.read(source,always_2d=True,dtype="float32")
        if audio.shape[1]==1:audio=np.repeat(audio,2,axis=1)
        elif audio.shape[1]!=2:continue
        # Up to 6 deterministic 2s chunks per song, no sample upload.
        for offset in range(0,min(len(audio)-int(rate*2)+1,int(rate*30)),int(rate*4)):
            block=features(audio[offset:offset+int(rate*2)],rate)
            if block is not None:vectors.append(block)
    if not vectors:raise RuntimeError("No 2-second audio frames found")
    manifest=json.loads(args.manifest.read_text())
    args.output.parent.mkdir(parents=True,exist_ok=True)
    np.savez_compressed(args.output,features=np.stack(vectors),
        licenses=np.asarray([manifest["items"][0]["license"]]*len(vectors)),
        sources=np.asarray([manifest["source"]]*len(vectors)))
    print(f"Prepared {len(vectors)} verified local Slakh windows; no audio redistributed")

if __name__=="__main__":main()
