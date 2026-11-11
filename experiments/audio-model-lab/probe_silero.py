#!/usr/bin/env python3
"""Real pretrained Silero VAD ONNX CPU inference. Synthetic smoke test only."""
from __future__ import annotations
import argparse
import json
import math
import time
from pathlib import Path

import numpy as np
import torch
from silero_vad import get_speech_timestamps, load_silero_vad


def synth(rate: int = 16000, seconds: int = 4) -> dict[str, torch.Tensor]:
    rng = np.random.default_rng(14528)
    t = np.arange(rate * seconds, dtype=np.float32) / rate
    envelope = ((t > 0.65) & (t < 3.25)).astype(np.float32)
    # Harmonic tones are NOT spoken language; no accuracy claims from this probe.
    voiced = envelope * (0.16 * np.sin(2 * np.pi * 165 * t)
                         + 0.065 * np.sin(2 * np.pi * 330 * t)
                         + 0.02 * rng.standard_normal(t.size).astype(np.float32))
    return {
        "silence": torch.zeros(t.size, dtype=torch.float32),
        "harmonics": torch.from_numpy(np.clip(voiced, -1, 1).astype(np.float32)),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path("model-lab-reports/silero-vad.json"))
    args = parser.parse_args()
    torch.set_num_threads(2)
    load_start = time.perf_counter()
    model = load_silero_vad(onnx=True)
    load_s = time.perf_counter() - load_start
    results = {}
    for name, audio in synth().items():
        start = time.perf_counter()
        timestamps = get_speech_timestamps(audio, model, sampling_rate=16000, return_seconds=True)
        elapsed = time.perf_counter() - start
        duration = len(audio) / 16000
        assert math.isfinite(elapsed) and elapsed >= 0
        assert all(0 <= float(t["start"]) <= float(t["end"]) <= duration for t in timestamps)
        results[name] = {
            "duration_s": duration,
            "inference_s": round(elapsed, 5),
            "real_time_factor": round(elapsed / duration, 5),
            "speech_intervals": timestamps,
        }
    report = {
        "model": "Silero VAD (ONNX pretrained)",
        "real_pretrained_inference": True,
        "cpu_only": True,
        "model_load_s": round(load_s, 5),
        "probes": results,
        "quality_verified": False,
        "integrated_in_cyn_audio": False,
        "limitations": "Synthetic harmonics are not real dialogue, and smoke-test timings do not prove speech detection accuracy or sound enhancement.",
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
