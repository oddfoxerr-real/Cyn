#!/usr/bin/env python3
"""Select the best candidate on an unseen, seeded holdout and make an importable Cyn profile."""
from __future__ import annotations
import argparse
import json
import math
from pathlib import Path
import numpy as np
from train_overnight import generate, report

ROOT = Path(__file__).resolve().parents[1]

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--inputs", type=Path, default=Path("training-candidates"))
    p.add_argument("--output", type=Path, default=Path("training-selected"))
    args = p.parse_args()
    x, y, scenes = generate(20541017, 12000)
    base = json.loads((ROOT / "models/cyn-starter.json").read_text())
    profile = json.loads((ROOT / "ai-hub/community/cyn-starter.cynai.json").read_text())
    baseline = report(np.asarray(base["weights"]), x, y, scenes)
    eligible = []
    all_tests = []
    for f in sorted(args.inputs.glob("*/weights.json")):
        weights = np.asarray(json.loads(f.read_text()), dtype=np.float64)
        if weights.shape != (209,) or not np.isfinite(weights).all():
            raise ValueError(f"Invalid model weights in {f}")
        metrics = report(weights, x, y, scenes)
        valid = (metrics["mean_mse"] < baseline["mean_mse"] * .99
                 and metrics["clarity_mse"] < baseline["clarity_mse"] * .99
                 and metrics["bass_mse"] < baseline["bass_mse"] * .99
                 and metrics["max_bass"] < .86
                 and all(metrics["scene_mse"][str(k)] <= baseline["scene_mse"][str(k)] * 1.05
                         for k in range(4)))
        candidate = {"name": f.parent.name, "metrics": metrics, "eligible": valid}
        all_tests.append(candidate)
        if valid:
            eligible.append((metrics["mean_mse"], f.parent.name, weights))
    summary = {
        "schema": 1, "baseline_test": baseline, "candidate_tests": all_tests,
        "heldout_seed": 20541017, "heldout_rows": len(x),
        "quality_claim": "synthetic-target-prediction-only",
        "real_audio_listening_verified": False,
        "pretrained_models_directly_distilled": False,
        "notes": "These tests compare synthetic heuristics only. No result proves improved audio."
    }
    args.output.mkdir(parents=True, exist_ok=True)
    if eligible:
        score, name, weights = min(eligible, key=lambda item: item[0])
        profile["profile"]["name"] = "Cyn Neural Experimental - Synthetic Training"
        profile["profile"]["category"] = "experimental"
        profile["profile"]["description"] = (
            "CPU-trained synthetic audio control candidate. Existing live YAMNet and "
            "Cyn Balance continue supplying scene and spectral context. "
            "NOT verified to improve music/dialogue; listening A/B is required.")
        profile["model"]["weights"] = [float(v) for v in weights]
        profile["model"]["ready"] = [True, False, False, False, True]
        assert profile["format"] == "cyn-ai-profile" and profile["model"]["architecture"] == "cyn-audio-11x12x5"
        (args.output / "cyn-neural-experimental.cynai.json").write_text(json.dumps(profile, indent=2) + "\n")
        summary["selection"] = name
        summary["status"] = "passed-synthetic-evaluation"
    else:
        summary["selection"] = None
        summary["status"] = "rejected-all-candidates"
    (args.output / "training-report.json").write_text(json.dumps(summary, indent=2) + "\n")
    (args.output / "status.txt").write_text(summary["status"] + "\n")
    print(json.dumps(summary, indent=2))
    if not eligible:
        raise SystemExit("No candidate beat original on the independent synthetic test; no AI profile exported.")


if __name__ == "__main__":
    main()
