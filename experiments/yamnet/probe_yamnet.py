#!/usr/bin/env python3
"""Run Google's actual pretrained YAMNet on a WAV file or a synthetic probe.

Experimental isolated compatibility test for Cyn. This DOES NOT run inside
Cyn.exe or read background Windows audio; it is for selecting the right runtime
before modifying any real-time DSP code. The weights load from TF Hub.

YAMNet input: 16000 Hz mono float32 waveform. Outputs: 521 AudioSet event
scores, 1024-dimensional per-patch embeddings, and log mel spectrograms.
"""
import argparse
import csv
import json
import math
from pathlib import Path
import time

import numpy as np
import scipy.signal
from scipy.io import wavfile


MODEL_URL = "https://tfhub.dev/google/yamnet/1"
TARGET_RATE = 16000
CLASSES = 521
EMBEDDING_DIM = 1024


def load_waveform(path):
    if path is None:
        t = np.arange(TARGET_RATE * 3, dtype=np.float32) / TARGET_RATE
        # This is deliberately NOT a movie/music detection benchmark.
        wave = 0.08 * np.sin(2 * np.pi * 440 * t)
        return wave.astype(np.float32), "synthetic 440Hz/3s"

    rate, samples = wavfile.read(str(path))
    if samples.ndim > 2:
        raise ValueError("WAV audio must be mono or interleaved multichannel")
    if np.issubdtype(samples.dtype, np.integer):
        info = np.iinfo(samples.dtype)
        if np.issubdtype(samples.dtype, np.unsignedinteger):
            center = (float(info.max) + 1.0) / 2.0
            audio = (samples.astype(np.float32) - center) / center
        else:
            audio = samples.astype(np.float32) / max(abs(info.min), info.max)
    elif np.issubdtype(samples.dtype, np.floating):
        audio = samples.astype(np.float32)
    else:
        raise ValueError("Unsupported WAV sample encoding")
    if audio.ndim == 2:
        audio = audio.mean(axis=1)
    if rate != TARGET_RATE:
        factor = math.gcd(int(rate), TARGET_RATE)
        audio = scipy.signal.resample_poly(audio, TARGET_RATE // factor, int(rate) // factor)
    if audio.size > TARGET_RATE * 180:
        raise ValueError("Only files up to 3 minutes are supported by this test")
    return np.clip(audio, -1, 1).astype(np.float32), path.name


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--wav", type=Path,
                        help="Local WAV file to classify; default is generated test audio")
    parser.add_argument("--json", type=Path,
                        help="Write a small JSON report with labels, not raw audio")
    options = parser.parse_args()

    import tensorflow as tf
    import tensorflow_hub as hub

    waveform, source = load_waveform(options.wav)
    if not np.isfinite(waveform).all() or waveform.size < TARGET_RATE:
        raise ValueError("YAMNet requires valid waveform with at least 1 second of audio")

    start = time.monotonic()
    model = hub.load(MODEL_URL)
    load_seconds = time.monotonic() - start

    start = time.monotonic()
    scores, embeddings, log_mel_spectrogram = model(tf.convert_to_tensor(waveform))
    inference_seconds = time.monotonic() - start

    raw_scores = scores.numpy()
    raw_embeddings = embeddings.numpy()
    raw_spectrogram = log_mel_spectrogram.numpy()
    assert raw_scores.ndim == 2 and raw_scores.shape[1] == CLASSES
    assert raw_embeddings.shape == (raw_scores.shape[0], EMBEDDING_DIM)
    assert raw_spectrogram.ndim == 2 and raw_spectrogram.shape[1] == 64
    assert raw_scores.shape[0] > 0
    assert np.isfinite(raw_scores).all() and np.isfinite(raw_embeddings).all()
    assert np.all((0 <= raw_scores) & (raw_scores <= 1))

    class_map_path = Path(model.class_map_path().numpy().decode("utf-8"))
    with class_map_path.open(newline="", encoding="utf-8") as stream:
        labels = [row["display_name"] for row in csv.DictReader(stream)]
    assert len(labels) == CLASSES

    aggregated = raw_scores.mean(axis=0)
    idx = np.argsort(aggregated)[-8:][::-1]
    top = [{"label": labels[int(k)], "score": float(aggregated[k])} for k in idx]
    report = {
        "model": MODEL_URL,
        "source": source,
        "rate_hz": TARGET_RATE,
        "mono_float32_samples": int(waveform.size),
        "patches": int(raw_scores.shape[0]),
        "class_scores": CLASSES,
        "embedding_dimensions": EMBEDDING_DIM,
        "spectrogram_bands": 64,
        "model_load_seconds": round(load_seconds, 3),
        "inference_seconds": round(inference_seconds, 3),
        "top_events": top,
        "integrated_into_cyn": False,
        "limitations": (
            "YAMNet recognizes acoustic event classes, NOT EQ or bass preferences. "
            "Predicted event scores are not calibrated genre probabilities; a "
            "synthetic tone is not a movie/dialogue quality evaluation."
        ),
    }
    print(json.dumps(report, indent=2))
    if options.json:
        options.json.parent.mkdir(parents=True, exist_ok=True)
        options.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
