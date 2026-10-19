# CYN v2.0.72 — Direct Android APK QR

- **Add Mobile Version...** in the existing three-line menu now opens a QR that encodes the precise requested static v0.2.3-preview APK URL, including its `?utm_source` suffix, instead of the Android releases page.
- Corrects the former QR payload/matrix mismatch by regenerating the full 41-by-41 matrix and retaining its 4-module white quiet border. Matrix was decoded to verify the destination.
- **Download Android APK** and **Copy link** buttons use that identical literal URL.
- This pins the Android version deliberately. Scanning downloads the v0.2.3 preview even if a newer APK exists.
- No automatic remote pairing is implied; this is a QR download flow.
- Native build and Windows installer version strings synchronize at 2.0.72.
