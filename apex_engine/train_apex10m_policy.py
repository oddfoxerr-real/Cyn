"""APEX 10M spatial-policy bootstrap: REAL PyTorch gradient training.

Synthetic scenes intentionally teach *conservative controls*, not music
quality. No 10M live inference until real-audio evaluation passes. The existing
6,307,477-parameter APEX base is NOT retrained or overwritten here. This trains
the real 3,693,992-parameter temporal spatial head in the 10,001,469-param
architecture. CPU training is resumable, including optimizer and RNG state.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import time

import torch
from torch import nn
from torch.nn import functional as F

from apex10m_model import SpatialReasoner, CONTROL_NAMES, APEX10M_PARAMETER_COUNT, APEX6M_PARAMETER_COUNT

POLICY_VERSION = "cyn-apex10m-spatial-curriculum-v1"
HEAD_COUNT = APEX10M_PARAMETER_COUNT - APEX6M_PARAMETER_COUNT


def scene(batch: int, frames: int, rng: torch.Generator):
    """Procedural proxies: sub-bass, kick, side energy, phase, headroom and motion.

    Targets are heuristics with temporal context; they are NOT ground-truth
    human evaluations. No copyrighted music or personal recordings are used.
    """
    r = lambda *shape: torch.rand(*shape, generator=rng)
    category = torch.randint(0, 5, (batch, 1), generator=rng)
    bass = (0.17 + 0.66 * r(batch, frames) +
            0.22 * (category == 0).float() - 0.10 * (category == 2).float()).clamp(0, 1)
    kick_events = (r(batch, frames) < (0.10 + .05 * (category == 0))).float()
    kick = (kick_events * (0.65 + .35 * r(batch, frames)) +
            .09 * r(batch, frames)).clamp(0, 1)
    kick_context = F.max_pool1d(kick[:, None, :], kernel_size=5, stride=1, padding=2).squeeze(1)
    side = (0.06 + 0.80 * r(batch, frames) +
            .18 * (category == 3).float()).clamp(0, 1)
    correlation = (1.9 * r(batch, frames) - .65 -
                   0.70 * (category == 4).float()).clamp(-1, 1)
    flux = (.52 * kick + .48 * r(batch, frames)).clamp(0, 1)
    dialogue = (.04 + .68 * r(batch, frames) +
                .35 * (category == 2).float()).clamp(0, 1)
    headroom = (.18 + .78 * r(batch, frames) -
                .17 * (category == 0).float()).clamp(0, 1)
    motion = (2 * r(batch, frames) - 1).clamp(-1, 1)

    x = torch.stack([bass, kick, side, correlation, flux,
                     dialogue, headroom, motion], dim=1)
    coherent = ((correlation + .3) / .65).clamp(0, 1)
    # Very wide, antiphase, speech, clipping and bass-heavy content are bad
    # candidates for aggressive spatial widening.
    safe_spatial = (coherent * (1 - .85 * dialogue) * (1 - .7 * bass)
                    * (1 - .55 * side) * headroom).clamp(0, 1)
    bass_memory = F.avg_pool1d(bass[:, None, :], 5, 1, 2).squeeze(1)
    dialogue_memory = F.avg_pool1d(dialogue[:, None, :], 5, 1, 2).squeeze(1)
    directional_motion = (motion.abs() * side * coherent).clamp(0, 1)
    targets = torch.stack([
        dialogue_memory,
        side * coherent,
        safe_spatial,
        (.16 + .75 * kick_context + .13 * flux).clamp(0, 1),
        (.14 + .76 * bass_memory + .11 * kick_context).clamp(0, 1),
        (.12 + .58 * dialogue_memory * flux).clamp(0, 1),
        directional_motion,
        (1 - .5 * (correlation.abs() < .22).float()
         - .3 * (headroom < .18).float()).clamp(0, 1)
    ], dim=1)
    if targets.shape[1] != len(CONTROL_NAMES):
        raise AssertionError("Control output shape changed")

    # Two-channel, 32-band moderate equalization policy targets.
    # Bass bins remain near neutral when heavy bass/kicks require protection.
    band = torch.linspace(0, 1, 32).view(1, 1, 32, 1)
    low = torch.exp(-((band - .085) / .13).square())
    presence = torch.exp(-((band - .48) / .20).square())
    air = torch.exp(-((band - .83) / .12).square())
    centre_bass = (.10 * (1 - bass_memory) - .075 * kick_context)
    presence_gain = (.09 * (1 - dialogue_memory) - .07 * dialogue_memory * flux)
    air_gain = .09 * safe_spatial
    common = (low * centre_bass[:, None, None, :]
              + presence * presence_gain[:, None, None, :]
              + air * air_gain[:, None, None, :]).expand(-1, 2, -1, -1)
    stereo_difference = (air * (.035 * safe_spatial * motion)[:, None, None, :])
    stereo = torch.cat([stereo_difference, -stereo_difference], dim=1)
    band_targets = (common + stereo).clamp(-.2, .2)
    return x, targets, band_targets


@torch.no_grad()
def validate(model: SpatialReasoner):
    model.eval()
    generator = torch.Generator().manual_seed(12121984)
    totals = [0., 0., 0., 0.]
    for _ in range(4):
        features, expected, bands_expected = scene(4, 20, generator)
        bands, controls = model(features)
        totals[0] += F.mse_loss(controls, expected).item()
        totals[1] += F.mse_loss(bands, bands_expected).item()
        # Lower is better; rule-based zero-spatial policy is safety baseline.
        totals[2] += F.mse_loss(torch.zeros_like(expected), expected).item()
        totals[3] += float((controls[:, 2][features[:, 3] < -.3] ** 2).mean().item())
    model.train()
    return {"controls_mse":totals[0] / 4,
            "bands_mse":totals[1] / 4,
            "zero_policy_mse":totals[2] / 4,
            "antiphase_width_sq":totals[3] / 4}


def save_state(path: Path, model: SpatialReasoner, opt: torch.optim.Optimizer,
               generator: torch.Generator, seconds: float, steps: int,
               validation: dict, seed: int):
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "format":POLICY_VERSION, "model":model.state_dict(),
        "optimizer":opt.state_dict(), "generator_state":generator.get_state(),
        "training_seconds":float(seconds), "steps":int(steps),
        "validation":validation, "seed":seed, "synthetic_only":True,
        "head_parameters":HEAD_COUNT, "full_architecture_parameters":APEX10M_PARAMETER_COUNT,
        "live_playback_approved":False
    }
    with tempfile.NamedTemporaryFile(dir=path.parent, suffix=".part", delete=False) as tmp:
        tmp_path = Path(tmp.name)
    try:
        torch.save(payload, tmp_path)
        os.replace(tmp_path, path)
    finally:
        tmp_path.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target-total-seconds", type=float, required=True)
    parser.add_argument("--checkpoint", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=20261021)
    parser.add_argument("--max-steps", type=int, default=0,
                        help="Smoke test only: stop after this number of additional updates")
    parser.add_argument("--checkpoint-interval", type=int, default=120)
    args = parser.parse_args()
    if not 0 < args.target_total_seconds <= 54000:
        parser.error("Duration must be in (0,54000] seconds")

    torch.set_num_threads(2)
    torch.manual_seed(args.seed)
    model = SpatialReasoner()
    actual = sum(param.numel() for param in model.parameters())
    if actual != HEAD_COUNT:
        raise AssertionError(f"Spatial model parameters drifted: {actual} != {HEAD_COUNT}")
    opt = torch.optim.AdamW(model.parameters(), lr=.00022, weight_decay=.00008)
    generator = torch.Generator().manual_seed(args.seed)
    seconds = 0.0
    steps = 0
    last_validation = {}
    if args.checkpoint.exists():
        previous = torch.load(args.checkpoint, map_location="cpu", weights_only=True)
        if previous["format"] != POLICY_VERSION or not previous["synthetic_only"]:
            raise RuntimeError("Refusing incompatible, untrusted, or non-synthetic checkpoint")
        if previous["seed"] != args.seed:
            raise RuntimeError("Checkpoint seed mismatch")
        model.load_state_dict(previous["model"], strict=True)
        opt.load_state_dict(previous["optimizer"])
        generator.set_state(previous["generator_state"])
        seconds = float(previous["training_seconds"])
        steps = int(previous["steps"])
        last_validation = previous.get("validation", {})
        print(f"RESUMED seconds={seconds:.1f} steps={steps}", flush=True)

    if not last_validation:
        last_validation = validate(model)
        print("INITIAL VALIDATION "+json.dumps(last_validation), flush=True)
    model.train()
    start_step = steps
    last_saved = time.monotonic()
    last_logged = last_saved
    latest_loss = 0.
    while seconds < args.target_total_seconds:
        started = time.monotonic()
        features, expected, expected_bands = scene(4, 20, generator)
        opt.zero_grad(set_to_none=True)
        bands, controls = model(features)
        control_loss = F.mse_loss(controls, expected)
        bands_loss = F.mse_loss(bands, expected_bands)
        unsafe = controls[:, 2][features[:, 3] < -.3].square().mean()
        loss = control_loss * 4.0 + bands_loss * 2.0 + unsafe * 2.5
        if not torch.isfinite(loss):
            raise RuntimeError(f"Non-finite objective at step {steps}")
        loss.backward()
        nn.utils.clip_grad_norm_(model.parameters(), 1.0)
        opt.step()
        seconds += time.monotonic() - started
        steps += 1
        latest_loss = float(loss.item())
        now = time.monotonic()
        should_stop = args.max_steps > 0 and steps-start_step >= args.max_steps
        if now-last_saved >= args.checkpoint_interval or should_stop or seconds >= args.target_total_seconds:
            last_validation = validate(model)
            save_state(args.checkpoint, model, opt, generator, seconds, steps,
                       last_validation, args.seed)
            report = {
                "model":"APEX 10M spatial policy head",
                "checkpoint_sha256":hashlib.sha256(args.checkpoint.read_bytes()).hexdigest(),
                "real_gradient_updates":steps,
                "measured_training_seconds":round(seconds, 2),
                "requested_total_training_seconds":54000,
                "last_loss":latest_loss, "validation":last_validation,
                "dataset":"procedurally generated synthetic proxy scenes; not licensed music",
                "quality_validated_on_real_audio":False,
                "neural_playback_enabled":False,
                "parameter_count_spatial_head":HEAD_COUNT,
                "parameter_count_total_composite":APEX10M_PARAMETER_COUNT,
                "apex6m_base_not_retrained":True
            }
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
            last_saved = now
        if now-last_logged > 100 or should_stop:
            print(f"steps={steps} train_sec={seconds:.2f} target={args.target_total_seconds:.1f} "
                  f"loss={latest_loss:.5f} val={last_validation}", flush=True)
            last_logged = now
        if should_stop:
            break
    if steps==start_step and seconds<args.target_total_seconds:
        raise RuntimeError("No gradient updates were performed")
    print(f"CHECKPOINT COMPLETE: steps={steps} actual_training_sec={seconds:.2f}",flush=True)


if __name__ == "__main__":
    main()
