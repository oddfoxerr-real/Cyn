# CYN v2.0.61 — Horizon × Depth Native UI Refresh

This Windows UI update brings the selected sleek, professional Horizon + Depth layout into CYN's **actual JUCE application**. It is not a web mockup or static screenshot.

## New Sound Studio
- Rebuilt around a single labelled left navigation with true-black panels, subtle liquid blue mesh and a wider 1320 × 700 workbench.
- Operational real **31/35/39-band equalizer**, **live visualizer**, active preset and output-device selectors.
- Five **genuine DSP enhancement controls** arranged horizontally in the sound studio.
- Overview / Equalizer / Enhancements buttons provide different working control layouts, not cosmetic or inert tabs.
- AI status is read from CYN's real current model and Smart Listening state, not simulated playback data.

## Consistent pages
- Smart Listening: larger live learning timeline, separate neural-activity visual and model/training/sensitivity/headroom controls.
- Devices: functional output-device and priority controls, matched to the new layout.
- AI Hub: real offline/community profile browser **embedded in the main workspace**. Native activation, trusted model handling and import stay unchanged.
- Misc / AI DJ: existing opt-in effect controls in matching cards.
- CYN Studio and Settings remain native dialogs reached from the new labelled sidebar.

## Dark Theme
- One canonical **Dark Theme** replaces visible colour-theme switching. On first launch previous theme selection is stored locally as `cyn_previous_theme_mode`, while all personal AI profiles, settings, EQ presets, devices, animation, AI Touch and liquid-button preferences are preserved.
- CYN Studio retains the real native Buttons, Animations and AI Touch customization panels.
- The modal now fits 768p displays more comfortably (720px content height).

## Compatibility and release notes
- Windows CI exercises the real JUCE build, AI Hub contracts, ONNX model activation, updater, DSP visual telemetry and EQ/motion regression tests.
- User-facing UI still needs a hands-on check for font scaling, 125%/150% Windows DPI, low-resolution displays, and the intended visual feel before declaring the larger CYN 3.0 stable-design milestone.
- No changes to inference weights, audio processing mathematics, driver installation or device auto-repair permissions.
