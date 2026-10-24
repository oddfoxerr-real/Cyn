# CYN Windows v2.0.81 — Riff-source native layout

This release significantly redesigns CYN's Windows desktop based on the MIT-licensed Riff project, while keeping CYN's native JUCE audio engine.

## New desktop interface
- Replace the former wide sidebar with an 80px floating navigation rail and individual 48px icon controls.
- Expand the interactive Sound Studio, Smart Listening, Output Devices, AI Hub and AI DJ workspaces into the recovered horizontal space.
- Introduce Riff's dark Material surface-container palette and 28px floating workspace shape; preserve CYN's audio/accent colours.
- Adapt Riff's two-stage navigation animation from app.js: a stretching 260ms phase followed by a 340ms settling phase.
- Keep 31/35/39-band equalizer, APEX/AUDONIC and BrainScan, device routing, and CYN Studio controls available.

## Licensing and compatibility
- Riff source references: https://github.com/rootscripts/riff/blob/main/styles.css and https://github.com/rootscripts/riff/blob/main/app.js
- Copyright (c) 2026 rootscripts. Riff's MIT license is included at third_party/riff-LICENSE.txt and as RIFF-LICENSE.txt in the Windows bundle.
- CYN's JUCE/FxSound audio engine remains subject to its existing licenses; no Riff Electron/streaming or download runtime is bundled.
- Existing user preferences, AI learning, presets and audio driver remain in place. No uninstall is necessary for v2.0.80 users.
- Desktop package, executable and GitHub updater all use v2.0.81.

## Validation
Windows CI performs native MSVC compilation, AI and DSP regression checks, source-layout validation, installer packaging, SHA-256 validation and verified GitHub release publishing. Local hardware and screenshot visual inspection remain independent of CI.

Design PR: https://github.com/oddfoxerr-real/Cyn/pull/79
