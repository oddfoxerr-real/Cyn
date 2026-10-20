"""Conservative, phase-preserving stereo stage for future APEX 10M training.

Widen existing side information only; no invented binaural positioning and no
claim of Dolby Atmos / object metadata decoding. Neural head is not activated.
"""
from __future__ import annotations

import numpy as np
from scipy import signal


def analyze_stereo(samples: np.ndarray, sample_rate: int) -> dict[str, float]:
    """Return interpretably bounded indicators used in training/evaluation."""
    samples = np.asarray(samples, dtype=np.float64)
    _validate(samples, sample_rate)
    left, right = samples[:, 0], samples[:, 1]
    middle = (left + right) * 0.5
    side = (left - right) * 0.5
    eps = 1e-12
    l_rms = float(np.sqrt(np.mean(left * left)))
    r_rms = float(np.sqrt(np.mean(right * right)))
    side_rms = float(np.sqrt(np.mean(side * side)))
    mid_rms = float(np.sqrt(np.mean(middle * middle)))
    correlation = float(np.mean(left * right) / max(l_rms * r_rms, eps))
    return {
        "correlation": float(np.clip(correlation, -1, 1)),
        "side_to_mid": float(side_rms / max(mid_rms, eps)),
        "level_difference_db": float(np.clip(20 * np.log10(max(l_rms, eps) / max(r_rms, eps)), -60, 60)),
    }


def _validate(samples: np.ndarray, sample_rate: int) -> None:
    if samples.ndim != 2 or samples.shape[1] != 2 or not len(samples):
        raise ValueError("Expected a nonempty [frames,2] stereo buffer")
    if sample_rate < 8000 or sample_rate > 192000:
        raise ValueError("Sample rate must be 8-192 kHz")
    if not np.isfinite(samples).all():
        raise ValueError("Audio must have finite samples")


def spatial_preserve(samples: np.ndarray, sample_rate: int, strength: float = 0.4) -> np.ndarray:
    """Enhance already-present stereo ambience, without touching phase timing.

    - Preserves mono and centered material.
    - Doesn't widen low-frequency bass (<~220 Hz).
    - Attenuates widening for strongly anti-correlated input.
    - Peak protects output, preserving input if bypassed.
    This is a reference DSP baseline, NOT 10M neural inference.
    """
    x = np.asarray(samples, dtype=np.float32)
    _validate(x, sample_rate)
    if not np.isfinite(strength) or not 0 <= strength <= 1:
        raise ValueError("Strength must be finite in the range 0..1")
    if strength == 0 or np.array_equal(x[:, 0], x[:, 1]):
        return x.copy()
    cues = analyze_stereo(x, sample_rate)
    if cues["correlation"] < -0.3:
        return x.copy()
    center = 0.5 * (x[:, 0] + x[:, 1])
    side = 0.5 * (x[:, 0] - x[:, 1])
    sos = signal.butter(2, 220, fs=sample_rate, btype="highpass", output="sos")
    high_side = signal.sosfilt(sos, side.astype(np.float64)).astype(np.float32)
    # Max high-frequency side gain: +1.6 dB, scaled down for diffuse material.
    boost = min(0.20 * strength, 0.20)
    if cues["correlation"] < 0.25:
        boost *= max(0.0, (cues["correlation"] + 0.3) / 0.55)
    # Already very wide material must not have its sides further inflated.
    boost *= float(np.clip(1.25 - 0.5 * cues["side_to_mid"], 0.0, 1.0))
    new_side = side + boost * high_side
    out = np.column_stack((center + new_side, center - new_side)).astype(np.float32)
    peak = float(np.max(np.abs(out)))
    if peak > 0.999:
        out *= 0.999 / peak
    return out
