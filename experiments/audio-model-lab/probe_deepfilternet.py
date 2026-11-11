#!/usr/bin/env python3
"""Real pretrained DeepFilterNet CPU speech-cleaning smoke test; no user audio."""
from __future__ import annotations
import argparse
import json
import math
import time
from pathlib import Path

import numpy as np
import torch
from df import enhance, init_df


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("model-lab-reports/deepfilternet.json"))
    args = parser.parse_args()
    torch.set_num_threads(2)
    start = time.perf_counter()
    model, state, _ = init_df()
    load_s = time.perf_counter() - start
    rate = int(state.sr())
    seconds = 2
    t = np.arange(rate * seconds, dtype=np.float32) / rate
    rng = np.random.default_rng(83411)
    # A noisy harmonic test, not a substitute for actual clean human speech.
    clean = 0.08 * np.sin(2 * np.pi * 195 * t) + 0.035 * np.sin(2 * np.pi * 390 * t)
    noisy = np.clip(clean + 0.045 * rng.standard_normal(t.size), -0.95, 0.95).astype(np.float32)
    start = time.perf_counter()
    with torch.inference_mode():
        output = enhance(model, state, torch.from_numpy(noisy).unsqueeze(0))
    infer_s = time.perf_counter() - start
    audio = output.detach().cpu().numpy().reshape(-1)
    assert len(audio) > 0 and np.isfinite(audio).all()
    def rms(v: np.ndarray) -> float:
        return float(np.sqrt(np.mean(np.square(v, dtype=np.float64))))
    report = {
        "model": "DeepFilterNet (pretrained)",
        "real_pretrained_inference": True,
        "cpu_only": True,
        "sample_rate": rate,
        "probe_duration_s": seconds,
        "model_load_s": round(load_s, 5),
        "inference_s": round(infer_s, 5),
        "real_time_factor": round(infer_s / seconds, 5),
        "input_rms": rms(noisy),
        "output_rms": rms(audio),
        "output_samples": len(audio),
        "output_peak": float(np.max(np.abs(audio))),
        "quality_verified": False,
        "integrated_in_cyn_audio": False,
        "limitations": "Synthetic tones are not natural speech. RMS change is not proof of intelligibility, SNR gain or better music sound.",
    }
    assert all(math.isfinite(x) for x in [report["input_rms"], report["output_rms"], report["output_peak"]])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
