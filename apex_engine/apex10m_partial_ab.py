"""Offline A/B listening tester for a FROZEN partially trained APEX 10M head.

Processes up to 30 seconds of stereo WAV audio without uploading it. The actual
learned temporal head predicts spatial/bass/kick safety controls from real
audio features. Its outputs are gated by independent DSP safeguards, so this
is a research preview, not evidence of superior sound or a live mobile plugin.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import sys
import numpy as np
import soundfile as sf
from scipy import signal

from apex10m_spatial import analyze_stereo


def real_features(audio: np.ndarray, rate: int, frame_seconds: float = 0.10):
    frame = max(256, int(rate * frame_seconds))
    count = (len(audio) + frame - 1) // frame
    output = np.zeros((8, count), dtype=np.float32)
    last_rms = 0.0
    last_low = 0.0
    for idx in range(count):
        section = audio[idx * frame:(idx + 1) * frame]
        if not len(section):
            continue
        left, right = section[:, 0].astype(np.float64), section[:, 1].astype(np.float64)
        mid = (left + right) * 0.5
        side = (left - right) * 0.5
        window = np.hanning(len(mid))
        freqs = np.fft.rfftfreq(len(mid), 1 / rate)
        power = np.abs(np.fft.rfft(mid * window)) ** 2
        total = float(np.sum(power)) + 1e-12
        bass = float(np.sum(power[freqs < 160]) / total)
        vocal = float(np.sum(power[(freqs >= 350) & (freqs < 3500)]) / total)
        l_rms = np.sqrt(np.mean(left**2) + 1e-12)
        r_rms = np.sqrt(np.mean(right**2) + 1e-12)
        m_rms = np.sqrt(np.mean(mid**2) + 1e-12)
        s_rms = np.sqrt(np.mean(side**2) + 1e-12)
        rms = float(np.sqrt(np.mean(mid**2)))
        corr = float(np.clip(np.mean(left * right) / max(l_rms * r_rms, 1e-10), -1, 1))
        # transient proxy: low-band onset + full-band RMS attack
        kick = float(np.clip(max(0.0, bass-last_low) * 2.0 +
                             max(0.0, rms-last_rms) * 4.0, 0, 1))
        flux = float(np.clip(max(0.0, rms-last_rms) * 6, 0, 1))
        peak = float(np.max(np.abs(section)))
        headroom = float(np.clip(1.0 - peak, 0, 1))
        width = float(np.clip(s_rms / max(s_rms + m_rms, 1e-6), 0, 1))
        motion = float(np.clip((l_rms-r_rms) / max(l_rms+r_rms, 1e-8), -1, 1))
        output[:, idx] = [bass,kick,width,corr,flux,vocal,headroom,motion]
        last_low, last_rms = bass, rms
    return output, frame


def process(audio: np.ndarray, rate: int, model, max_strength: float):
    import torch
    if not (0 <= max_strength <= 1):
        raise ValueError("Strength must be in 0..1")
    if max_strength == 0 or np.array_equal(audio[:, 0], audio[:, 1]):
        return audio.copy(), {"reason":"bypass","width_mean":0.0}
    stereo = analyze_stereo(audio, rate)
    if stereo["correlation"] < -.3:
        return audio.copy(), {"reason":"strong-antiphase bypass","width_mean":0.0}
    features, frame = real_features(audio, rate)
    # inference uses the actual trained temporal GRUs, not fabricated control curves
    with torch.inference_mode():
        feat = torch.from_numpy(features).unsqueeze(0)
        _, controls = model(feat)
        predicted = controls.squeeze(0).cpu().numpy()
    if not np.isfinite(predicted).all():
        raise ValueError("Neural inference produced non-finite controls")
    # Safety is enforced outside the model: antiphase, peak, speech and bass
    # remain restricted even if an out-of-domain model confidently disagrees.
    coherence = np.clip((features[3] + .30) / .55, 0, 1)
    dialogue = 1 - .35 * features[5]
    low_protection = 1 - .45 * predicted[4]
    kick_protection = 1 - .45 * predicted[3]
    width = .20 * max_strength * np.clip(.20 + 2.5 * predicted[2], 0, 1)
    width *= coherence * dialogue * low_protection * kick_protection
    width *= np.clip(1.25 - .50 * stereo["side_to_mid"], 0, 1)
    # low-pass gain trajectory to prevent changes at neural window boundaries
    samples = np.arange(len(audio), dtype=np.float64)
    positions = np.arange(len(width), dtype=np.float64) * frame + frame * .5
    width_per_sample = np.interp(samples, positions, width, left=width[0], right=width[-1]).astype(np.float32)
    centre = .5 * (audio[:, 0] + audio[:, 1])
    side = .5 * (audio[:, 0] - audio[:, 1])
    sos = signal.butter(2, 220, fs=rate, btype="highpass", output="sos")
    side_high = signal.sosfilt(sos, side.astype(np.float64)).astype(np.float32)
    new_side = side + width_per_sample * side_high
    result = np.column_stack([centre + new_side, centre - new_side]).astype(np.float32)
    peak = float(np.max(np.abs(result)))
    if peak > .999:
        result *= .999 / peak
    report = {
        "reason":"partial neural policy gated by safe stereo DSP",
        "width_mean":float(np.mean(width)),
        "width_max":float(np.max(width)),
        "kick_protection_mean":float(np.mean(predicted[3])),
        "bass_protection_mean":float(np.mean(predicted[4])),
        "neural_spatial_decision_mean":float(np.mean(predicted[2])),
        "stereo_before":stereo,
        "stereo_after":analyze_stereo(result, rate),
        "real_music_quality_validated":False,
        "on_device_android_inference":False,
    }
    return result, report


def read_frozen(checkpoint: pathlib.Path, expected_sha256: str):
    import torch
    from apex10m_model import SpatialReasoner
    actual = hashlib.sha256(checkpoint.read_bytes()).hexdigest()
    if expected_sha256 and expected_sha256.lower() != actual:
        raise ValueError("Checkpoint SHA-256 mismatch")
    data = torch.load(checkpoint, map_location="cpu", weights_only=True)
    if data.get("format") != "cyn-apex10m-spatial-curriculum-v1" or not data.get("synthetic_only"):
        raise ValueError("Unsupported or unverified model format")
    model = SpatialReasoner()
    model.load_state_dict(data["model"], strict=True)
    model.eval()
    return model, data, actual


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("input", type=pathlib.Path, help="Input stereo WAV up to 30 seconds")
    p.add_argument("--checkpoint",type=pathlib.Path,required=True)
    p.add_argument("--output",type=pathlib.Path)
    p.add_argument("--strength",type=float,default=.7)
    p.add_argument("--sha256",default="",help="Optional pinned snapshot checksum")
    args=p.parse_args()
    audio,rate=sf.read(args.input,always_2d=True,dtype="float32")
    if rate<8000 or rate>192000 or audio.shape[1]!=2:
        p.error("A valid stereo WAV at 8–192 kHz is required")
    if not np.isfinite(audio).all() or not len(audio) or len(audio)/rate>30:
        p.error("Audio must be finite, nonempty and at most 30 seconds")
    import torch
    torch.set_num_threads(2)
    model,meta,digest=read_frozen(args.checkpoint,args.sha256)
    processed,report=process(audio,rate,model,args.strength)
    output=args.output or args.input.with_name(args.input.stem+"-apex10m-partial.wav")
    if output.resolve()==args.input.resolve():
        p.error("Refusing to overwrite the original WAV")
    sf.write(str(output),processed,rate,subtype="PCM_16")
    report.update({
        "input":str(args.input),"output":str(output),
        "snapshot_actual_training_seconds":meta["training_seconds"],
        "snapshot_actual_optimizer_steps":meta["steps"],
        "snapshot_sha256":digest,"synthetic_only":True,
        "production_use_approved":False
    })
    report_path=output.with_suffix(".metrics.json")
    report_path.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
    print(json.dumps(report,indent=2),flush=True)


if __name__=="__main__":
    main()
