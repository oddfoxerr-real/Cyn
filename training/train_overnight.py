#!/usr/bin/env python3
"""Train a drop-in Cyn 11x12x5 AI Hub profile with measured synthetic holdouts.

The deployed flagship already fuses a Google YAMNet scene worker, Cyn Balance,
and this independent preference head. These generated targets are *heuristic*,
not human listening judgments. Never claim music-quality improvements from MSE.
"""
from __future__ import annotations
import argparse
import json
import math
import time
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
BANDS = np.array([
    [1.25, 1.58, .78, .24, .10, .04, .02, .014],
    [1.32, 1.48, .95, .38, .17, .07, .09, .045],
    [.13, .20, .43, .67, .54, .24, .52, .60],
    [.08, .15, .31, .62, .95, 1.04, .55, .10],
    [.08, .10, .26, .45, .90, 1.10, .78, .60],
    [.035, .035, .06, .12, .26, .49, .80, 1.25],
], dtype=np.float64)


def generate(seed: int, count: int) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    rng = np.random.default_rng(seed)
    src = rng.random((count, 6))
    src[:, 0] = src[:, 0] ** .8 * 1.25
    src[:, 1] = src[:, 1] ** .68 * 1.25
    src[:, 2] *= .75
    src[:, 3] = src[:, 3] ** .9 * 1.15
    src[:, 4] *= 1.05
    src[:, 5] *= .85
    # Additional source mixtures and mode shifts beyond the original v3 test.
    scene = rng.integers(0, 4, size=count)
    src[scene == 1, 0:2] *= 1.40       # kick-led electronic music
    src[scene == 2, 3] *= 1.60         # dialogue and vocals
    src[scene == 2, 0:2] *= .35
    src[scene == 3, 2:4] *= 1.20       # dense percussion / cymbal transients
    src[scene == 3, 5] *= 1.45
    sub, kick, snare, vocal, instrument, hat = src.T
    dull, hiss = rng.random((2, count))
    power = (.025 + src @ BANDS)
    power *= (1 - .62 * dull[:, None] * np.arange(8)[None, :] / 7)
    power[:, 7] += .24 * hiss * vocal
    power[:, 6:] += (.06 * hiss * hat)[:, None]
    power *= np.exp(rng.uniform(-.16, .16, size=(count, 8)))
    power = np.maximum(power, .0001)
    shares = power / np.sum(power, axis=1, keepdims=True)
    logbands = np.log(np.maximum(shares, 1e-8))
    crest = np.clip(.8 + .56 * kick + .34 * snare + .21 * hat
                    + rng.uniform(-.24, .24, count), 0, 4)
    width = np.clip(.17 + .43 * instrument + .27 * hat - .13 * vocal
                    + rng.uniform(-.11, .11, count), 0, 2)
    rms = np.clip(-3.15 + .32 * sub + .29 * kick + .37 * instrument
                  + .25 * vocal + rng.uniform(-.425, .425, count), -8, 0)
    raw = np.column_stack([logbands, crest, width, rms])
    x = raw.copy()
    x[:, :8] = (np.clip(x[:, :8], -12, 0) + 6) / 6
    x[:, 8] = np.clip(x[:, 8], 0, 4) / 2 - 1
    x[:, 9] = np.clip(x[:, 9], 0, 2) - 1
    x[:, 10] = (np.clip(x[:, 10], -8, 0) + 4) / 4
    low = power[:, :3].sum(axis=1)
    mid = power[:, 3:6].sum(axis=1)
    low_presence = low / (low + mid + 1e-9)
    kick_share = kick / (kick + sub + .35 * vocal + .12)
    rumble = sub / (sub + kick + .12)
    thin = np.clip((.41 - low_presence) * 2, 0, 1)
    # Heuristic smooth targets, *not* an assertion of ideal human sound.
    bass = np.clip(.56 + .34 * kick_share * (1 - .35 * rumble)
                   + .10 * thin - .19 * rumble - .06 * vocal
                   - .025 * snare, .27, .78)
    sibilance = np.clip(.55 * hiss + .25 * hat + .2 * (1 - dull), 0, 1)
    clarity = np.clip(.44 + .30 * vocal * (.65 + .35 * dull)
                      - .14 * sibilance * vocal - .075 * hat
                      + .085 * dull + .025 * kick, .27, .80)
    y = np.column_stack([clarity, bass])
    return x, y, scene


def predict(w: np.ndarray, x: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    h = np.tanh(x @ w[:132].reshape(12, 11).T + w[132:144])
    z = h @ w[144:204].reshape(5, 12).T + w[204:209]
    prob = 1. / (1. + np.exp(-np.clip(z[:, [0, 4]], -30, 30)))
    return h, prob


def report(w: np.ndarray, x: np.ndarray, y: np.ndarray, scenes: np.ndarray) -> dict:
    p = predict(w, x)[1]
    result = {"clarity_mse": float(np.mean((p[:, 0] - y[:, 0]) ** 2)),
              "bass_mse": float(np.mean((p[:, 1] - y[:, 1]) ** 2)),
              "mean_mse": float(np.mean((p - y) ** 2)),
              "max_bass": float(p[:, 1].max()),
              "mean_bass": float(p[:, 1].mean())}
    result["scene_mse"] = {str(k): float(np.mean((p[scenes == k] - y[scenes == k]) ** 2))
                            for k in range(4)}
    return result


def train(seed: int, steps: int, samples: int, out: Path) -> None:
    begin = time.perf_counter()
    rng = np.random.default_rng(seed + 187)
    base = json.loads((ROOT / "models/cyn-starter.json").read_text())
    profile = json.loads((ROOT / "ai-hub/community/cyn-starter.cynai.json").read_text())
    original = np.asarray(base["weights"], dtype=np.float64)
    assert original.shape == (209,) and np.isfinite(original).all()
    x, y, scenes = generate(seed + 1, samples)
    xv, yv, sv = generate(seed + 10001, 8000)
    # The final independent holdout is selected in compare_overnight.py.
    baseline = report(original, xv, yv, sv)
    w, m, v = original.copy(), np.zeros(209), np.zeros(209)
    best = original.copy()
    best_score = baseline["mean_mse"]
    best_step = 0
    stale = 0
    checkpoints = 0
    for step in range(1, steps + 1):
        ids = rng.integers(0, samples, 384)
        xb, yb = x[ids], y[ids]
        h, p = predict(w, xb)
        delta = np.zeros((384, 5))
        delta[:, [0, 4]] = (p - yb) / 768
        grad = np.zeros(209)
        grad[144:204] = (delta.T @ h).ravel()
        grad[204:209] = delta.sum(axis=0)
        dh = (delta @ w[144:204].reshape(5, 12)) * (1 - h ** 2)
        grad[:132] = (dh.T @ xb).ravel()
        grad[132:144] = dh.sum(axis=0)
        grad[156:192] = 0
        grad[205:208] = 0
        grad += .00001 * (w - original)
        m = .9 * m + .1 * grad
        v = .999 * v + .001 * grad ** 2
        learning_rate = .0024 if step < 7000 else .0010 if step < 24000 else .0004
        w -= learning_rate * (m / (1 - .9 ** step)) / (np.sqrt(v / (1 - .999 ** step)) + 1e-8)
        if not np.isfinite(w).all():
            raise ValueError("Divergent neural optimization")
        if step % 1000 == 0:
            candidate = report(w, xv, yv, sv)
            allowed = (candidate["max_bass"] <= .86
                       and all(candidate["scene_mse"][str(k)]
                               <= baseline["scene_mse"][str(k)] * 1.05 for k in range(4))
                       and candidate["clarity_mse"] < baseline["clarity_mse"] * 1.02
                       and candidate["bass_mse"] < baseline["bass_mse"] * 1.02)
            if allowed and candidate["mean_mse"] < best_score - 1e-7:
                best, best_step, best_score = w.copy(), step, candidate["mean_mse"]
                stale = 0
                checkpoints += 1
            else:
                stale += 1
            if step % 5000 == 0:
                print(json.dumps({"step": step, "best_step": best_step,
                    "candidate_loss": candidate["mean_mse"], "best_loss": best_score}), flush=True)
            # Keep computing only while the independent validation set improves.
            if stale >= 22:
                print(f"Early stopping: {stale} evaluations without improvement", flush=True)
                break
    end = time.perf_counter()
    metrics = {
        "schema": 1, "architecture": "Cyn native 11x12x5 (209 weights)",
        "seed": seed, "steps_completed": step, "steps_budget": steps,
        "best_step": best_step, "improved_checkpoints": checkpoints,
        "training_seconds": round(end - begin, 3), "train_examples": samples,
        "validation_examples": len(xv), "baseline_validation": baseline,
        "candidate_validation": report(best, xv, yv, sv),
        "model_status": "synthetic-training-candidate",
        "actual_music_better": None,
        "pretrained_context": "Cyn separately uses YAMNet and CynBalance at runtime; these 209 weights have NOT been trained directly on YAMNet, Silero or DeepFilterNet outputs.",
        "limitations": "Targets are synthetic heuristics; not listening preference labels, real music, source separation, or audio-output measurements.",
    }
    out.mkdir(parents=True, exist_ok=True)
    (out / "weights.json").write_text(json.dumps(best.tolist()) + "\n")
    (out / "metrics.json").write_text(json.dumps(metrics, indent=2) + "\n")
    print(json.dumps({"candidate": seed, "steps": step,
                      "old_validation_mse": baseline["mean_mse"],
                      "new_validation_mse": metrics["candidate_validation"]["mean_mse"],
                      "time_s": metrics["training_seconds"]}), flush=True)


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--seed", type=int, required=True)
    p.add_argument("--steps", type=int, default=80000)
    p.add_argument("--samples", type=int, default=60000)
    p.add_argument("--output", type=Path, required=True)
    a = p.parse_args()
    train(a.seed, a.steps, a.samples, a.output)


if __name__ == "__main__":
    main()
