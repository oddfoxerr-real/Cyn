# CYN v2.0.71 — Add Mobile Version

- Adds **Add Mobile Version...** to the existing three-line (hamburger) desktop menu.
- Opens a native CYN-styled QR window that can be scanned using the phone's camera.
- QR opens the Android Studio releases listing, so the destination continues to include newer mobile versions without encoding a particular APK or device detail.
- QR is precomputed and rendered locally without remote QR-image services, app telemetry or permissions. Includes Open mobile releases and Copy link actions.
- Transparently distinguishes installation from pairing: scanning the QR installs CYN Mobile; it does not yet link PC and Android audio processing. Actual device pairing is a separate feature that requires mutual authorization and transport.
- Does not change audio DSP, AI models, local settings, or the Android app.
- Validate the native dialog and QR scanning on a Windows desktop and Android phone after the automated build.
