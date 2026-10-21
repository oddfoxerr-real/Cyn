# CYN Mobile 0.2.24 preview

- Restores AUDONIC selection, saved creative intensity, verified model assets and native creative DSP. Base AI, APEX 6M and APEX 10M remain available.
- AUDONIC now handles mono Library audio as well as stereo. Connect maps fresh creative model decisions to compatible Android session EQ and virtualization; custom waveshaping remains native Library DSP.
- Repairs Audius result sizing, keeps the search action above results, adds keyboard search, progress, empty and retry states, and rejects stale results after navigation or another search.
- Applies all 11 CYN palettes to Material 3 components, fields, dialogs, navigation and player surfaces. Keeps saved themes and audio settings. No wallpaper color extraction.
- Keeps package io.github.oddfoxerr.cyn.studio and the existing release certificate; version code increases to 28. Uses the existing Android release discovery and installer flow.

Release gates include real ONNX inference, trained AUDONIC controls through native stereo/mono PCM, bypass and gain bounds, Connect mapping, preference migration, Android compilation/lint, APK signatures and alignment, packaged model integrity, and matching upgrade identity against 0.2.22 and 0.2.23.

Hardware playback, live Audius availability and Android installation on individual devices require device confirmation. No emulator or uninstall is required by this release workflow. Connect still depends on the external player exposing supported Android audio effects and permitting capture for neural analysis.
