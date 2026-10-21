# CYN Mobile 0.2.26 preview — AUDONIC Creative BrainScan

- AUDONIC now has a dedicated six-channel BrainScan view in AI Hub on Library playback **and** supported Connect playback, even when Connect monitoring is off for Library.
- The rotating neural mesh gains six independently coloured branches, orbital meters and satellites for Harmonics, Attack, Width, Colour, Rhythm and Overtones. Their levels come from the six actual fresh AUDONIC ONNX effect controls and the selected Creative intensity.
- Signal waves are only emitted following fresh AUDONIC policy decisions or measurable activity changes. Stale inference, bypassed audio and disconnected capture fade to an idle scan rather than fabricating ongoing AI decisions. Reduced-motion mode suppresses animated rotation and pulses.
- Connect retains its existing Android-session capabilities; CYN does not claim that native Library-only harmonic processing is applied to third-party apps. On unsupported players the scan stays in a waiting state.
- Preserves APEX 6M, APEX 10M and Base AI, AUDONIC ONNX/native DSP, Library playback, Connect EQ mapping, saved settings, themes and the API-rate-limit updater fix introduced in 0.2.25.
- Retains package `io.github.oddfoxerr.cyn.studio` and the existing CYN signing certificate. Version code increases from 29 to 30, permitting a normal installation over 0.2.25.

Release gates run static BrainScan/telemetry assertions, existing AUDONIC ONNX inference and native DSP checks, APEX and Connect regressions, Gradle compile and lint, model integrity, signing-certificate validation and upgrade checks through 0.2.25. Physical-device performance and hardware-specific Connect access still require testing.
