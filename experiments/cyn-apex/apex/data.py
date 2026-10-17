"""Deterministic clean synthetic acoustic mixtures with ground-truth noise stems.

These are synthetic tones and envelope textures, not human voices or real songs.
"""
from __future__ import annotations
import numpy as np
import torch
from .model import SAMPLE_RATE


def _add_notes(t: np.ndarray, rng: np.random.Generator, f_range: tuple[float, float], count: int, decay: float = 0.0) -> np.ndarray:
    result = np.zeros_like(t)
    for _ in range(count):
        hz = rng.uniform(*f_range)
        start = rng.uniform(0, 0.75)
        duration = rng.uniform(0.12, 0.55)
        env = np.clip((t-start)*90, 0, 1) * np.clip((start+duration-t)*50, 0, 1)
        if decay:
            env *= np.exp(-decay * np.maximum(t-start, 0))
        partials = sum((np.sin(2*np.pi*hz*(k+1)*t + rng.uniform(-np.pi, np.pi)) / (k+1)**1.2) for k in range(4))
        result += 0.13 * env * partials
    return result


def example(seed: int, *, split: str = "train") -> tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
    rng = np.random.default_rng(seed)
    t = np.arange(SAMPLE_RATE, dtype=np.float64)/SAMPLE_RATE
    factors = rng.uniform(0.4, 1.4, 4)
    bass = _add_notes(t, rng, (45, 150) if split=="train" else (55, 175), rng.integers(1, 4), 0.7)
    melody = _add_notes(t, rng, (180, 900) if split=="train" else (210, 1100), rng.integers(2, 6))
    beat = np.zeros_like(t)
    tempo=rng.uniform(1.6, 5.0)
    for start in np.arange(0, 1, 1/tempo):
        onset=start+rng.uniform(-0.014,0.014)
        envelope=np.exp(-np.maximum(0,t-onset)*rng.uniform(24, 55))*(t>=onset)
        kick=np.sin(2*np.pi*rng.uniform(55,120)*t)*envelope
        snap=rng.standard_normal(t.shape[0])*np.exp(-np.maximum(0,t-onset)*rng.uniform(60,110))*(t>=onset)
        beat += 0.20*kick + 0.025*snap
    # 'speech-like' is a deliberately labeled harmonic proxy, not speech training.
    speech_like=_add_notes(t,rng,(110,250),(rng.integers(2,5)),decay=0.3)
    # Speech-like sequences simulate changing voiced pitch and breath, no phonemes.
    presence = rng.integers(0,2,4).astype(np.float64)
    clean=(factors[0]*bass + factors[1]*melody + factors[2]*beat + factors[3]*speech_like*presence[0])
    clean /= max(np.max(np.abs(clean)),0.5)
    clean *= 0.6
    noise=rng.standard_normal(t.shape[0]); hiss=rng.standard_normal(t.shape[0])
    # Withheld frequency characteristics in testing create an imperfect domain shift.
    if split=="train":
        smoothed=np.convolve(noise,np.ones(17)/17,mode="same")
    else:
        smoothed=np.convolve(noise,np.ones(5)/5,mode="same")
    noise=0.65*noise+0.35*smoothed
    noisy_gain=rng.uniform(0.03,0.28)
    high_gain=rng.uniform(0,0.10)
    interference=noisy_gain*noise + high_gain*(hiss-np.convolve(hiss,np.ones(9)/9,mode="same"))
    mixture=np.clip(clean+interference,-1.0,1.0)
    labels=np.array([factors[0]>0.65, True, factors[1]>0.65, bool(presence[0]), noisy_gain>0.16, high_gain>0.06],dtype=np.float32)
    return (torch.from_numpy(mixture.astype(np.float32)),
            torch.from_numpy(clean.astype(np.float32)),
            torch.from_numpy(labels))


def batch(seed: int, size: int, split: str = "train"):
    items=[example(seed*1000+i,split=split) for i in range(size)]
    return tuple(torch.stack([item[k] for item in items]) for k in range(3))