# CYN 30-AI Fusion Research

**Research branch only. Does not update CYN's installer, production DSP, AI Hub, model weights, or personal settings.**

## What is implemented

- 30 verified upstream project references, with code-license notes and a separate checkpoint-license gate.
- One guarded evidence interface in fusion.mjs. It accepts predictions from external calibrated experts and averages *comparable* scene evidence only. This is **not** a trained composite neural network.
- A scalar 11x12x5 CYN prediction implementation matching PersonalAudioModel.h for functional tests against existing CYN profiles.
- Node tests using **mock model predictions**, not actual downloaded or executed 30-model inference.
- All 30 experts disabled by default. No input audio is uploaded, no model weights bundled, no inference happens on CYN's sound callback.

## Run contract checks

From the repository root with Node 22 or newer:

    node --test experiments/fusion30/test_fusion.mjs

## Why full weight merging isn't valid

YAMNet, CLAP, separation transformers, speech enhancers, source-transcription, generation and beat trackers have incompatible architectures, prediction spaces, training targets, sample rates and latency. Their weight tensors cannot be meaningfully averaged or concatenated. The viable design is a single orchestration layer over optional specialized workers, plus a separately trained preference head.

## Actual comparison baseline

CYN's existing experimental comparator is Cyn Real Audio 13h (Experimental), a 209-weight 11->12->5 preference model. Its 13-hour release notes report identity-separated real-voice/instrument evaluations against **heuristic** targets, not verified improvements in music playback.

Source: https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-real-audio-lab-13h-20261016

The present contract tests cannot establish any quality improvement over that model. No conclusion to replace it should be drawn from passing mock tests.

Required before any upgrade claim: independently review checkpoint licenses; download pinned/checksummed weights into isolated environments; implement each adapter; obtain consent/license-cleared real music, cinema and dialogue clips; measure model task scores and CPU/GPU timing, RAM, end-to-end playback latency and glitches; verify clipping and true-peak; conduct level-matched blind A/B listening on exactly the same clips.

## Packaging policy

Source and weights are governed by separate licenses. MusicGen AudioCraft weights are generally restricted to noncommercial use; Open-Unmix UMXL weights are noncommercial; many Essentia weights also have restrictions. All 30 weights remain unbundled. Shipping or automatic preset changes are disabled.
