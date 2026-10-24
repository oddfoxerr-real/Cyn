# CYN Windows v2.0.83 — Actual native Riff-workbench redesign

Unlike v2.0.82 (which primarily improved validation), v2.0.83 substantially changes the live Windows desktop interface itself.

## Visual changes
- Replaces the cramped default EQ/spectrum side-by-side layout with a full-width, editable EQ stage. All existing 31, 35 and 39 bands continue to use CYN's real DSP controls.
- Relocates the existing live spectrum to the EQ heading region.
- Replaces three standalone preset/output/AI tiles with one more coherent, native control deck.
- Rebuilds the Sound Studio's section hierarchy and lower effect rack with quieter, layered dark Material tonal surfaces, stronger typography and fewer redundant containers.
- Adds destination labels below native sidebar icons while retaining icon hit targets, keyboard accessibility and tooltips.
- Improves the EQ response stroke, guide lines, and integrated effects rack spacing. The user control callbacks and DSP processing are unchanged.

## Compatibility and attribution
- Upgrade in place over v2.0.82, without uninstalling.
- APEX, AUDONIC, AI DJ, BrainScan, Smart Listening, EQ bands, saved preferences and FxSound driver remain in place.
- Riff is MIT-licensed and its notice remains bundled; FxSound/JUCE licensing is unchanged. Riff Electron, streaming and downloads are not embedded.

## Validation
- The design PR passed native Windows MSVC/JUCE compilation, EQ / AI DJ, AUDONIC / BrainScan, neural animation, motion, source-linked geometry and Power Swallow CI checks.
- The Windows release workflow also compiles and packages the installer, tests the AI/runtime path and verifies the SHA-256 checksum before publication.
- Automated geometry previews do **not** prove final native visual quality. The actual Windows layout still requires on-device visual and usability inspection.

Source PR: https://github.com/oddfoxerr-real/Cyn/pull/81
