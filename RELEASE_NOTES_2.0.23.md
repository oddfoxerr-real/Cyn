# Cyn 2.0.23 — Dynamic Theme Atmospheres

Cyn 2.0.23 brings distinctive, animated artwork to all 20 built-in themes, rather than changing only the interface palette.

## Theme overhaul
- Each existing theme keeps its saved ID and gains its own visual motif, including Aurora light curtains, Sakura petals, Arctic crystals, a Deep Sea current, a neon city skyline, Synthwave perspective grid, Solar Flare corona, and Neural Core signals.
- The sidebar artwork and subtle workspace lighting react to the measured audio RMS level from Cyn's existing personal-audio telemetry. This is not fabricated YAMNet neural activity.
- Motion and glow run only in the JUCE user interface, with a budget of approximately 24 redraws per second and a still-art fallback when **Reduce motion** is enabled.
- Native navigation, equalizer, audio controls, AI Hub, and live Neural Core topology remain available.

## Existing AI DJ capabilities retained
The experimental, **optional AI DJ** feature from 2.0.22 is included, with its user-controlled activation and separate download. It is off by default, and its model weights are **not** included in the installer. See [AI DJ details](docs/AI-DJ.md).

## Installation and notes
- Download **Cyn-Setup-2.0.23-x64.exe** below to upgrade your existing Cyn installation.
- A portable Windows x64 ZIP and an installer SHA-256 verification file are also provided.
- This version modifies Cyn's UI but introduces no new theme-related drivers, audio-processing algorithms, or AI models.
- Windows builds are unsigned. Windows may prompt before running the installer.

The project remains licensed under the GNU AGPL-3.0-or-later.
