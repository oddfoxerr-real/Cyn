# CYN Windows 2.0.85 — Material 3 desktop becomes the normal CYN update

## What's changing
- CYN's Riff-based Electron/Material 3 desktop interface is now installed by the **normal CYN Windows installer**. The main Start Menu, desktop and post-install shortcuts launch **CYN**, not the old JUCE window.
- The original native CYN/FxSound DSP engine remains installed and handles all Windows audio, AI models, ONNX inference, output drivers, and user presets. The new interface launches the native processor minimized when bundled alongside it.
- A **CYN (Classic)** Start Menu shortcut remains available for complete native settings, BrainScan, training tools and any unported features.
- The Material UI includes graphite styling, Riff's position-aware dock indicator, slide-out Presets / AI Hub / Appearance panels, active native EQ/effect controls, the Settings menu, and the radial custom closing animation.
- **Check for Updates** from the Material UI invokes the existing installed CYN updater and its SHA-256 verified Windows installer, instead of redirecting only to GitHub.
- The standard Windows updater sees this as a new **2.0.85** stable release. The legacy app ID, installation directory, native DLLs/models, user profiles and driver setup remain unchanged.
- The installer checks for both the native audio process and the Electron UI before overwriting open executables. It asks the user to exit them; it does not kill running audio without permission.
- The Windows installer and portable archive include the bundled Electron frontend and its upstream Riff MIT attribution, alongside FxSound/JUCE/AGPL license notices.

## Availability and scope
The new interface is **not a replacement DSP engine**. Browser/electron UI and Windows native DSP operate together; there is no standalone fake equalizer, media capture or separate audio server. Some legacy AI editing and neural visualization controls are still accessible through **CYN (Classic)** until equivalent native telemetry is exposed through the secure bridge.

The bundled interface is staged and validated on GitHub Actions before packaging. After the Windows tests, native app compile, model/runtime checks, installer compilation, checksums and release checks pass, publish v2.0.85 as the **latest stable** Windows release with its normal signed/update verification process.

This installer uses the prior Inno Setup AppId to preserve in-place upgrades. No uninstall is required.
 
Riff source/visual foundation © rootscripts (MIT). CYN/FxSound source license: AGPL-3.0-or-later; third-party JUCE and runtime notices remain included.

## Fixes since the first integration candidate
- CYN Audio desktop's Check for Updates now supplies the mandatory current native version and -Interactive arguments to the existing Windows updater.
- Release packaging replaces the old launcher with the verified Material 3 app while retaining the original native engine and classic fallback.
