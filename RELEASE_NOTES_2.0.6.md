# Cyn v2.0.6 — Seamless update flow + real dropdown animations

### Actual dropdown animations
- Fixes the lifecycle race: the animation now begins at JUCE's popup **setVisible** point, not from an early LookAndFeel callback.
- Covers the main menu, combo-box dropdowns, and nested submenus with a 215 ms fade.
- Leaves popup bounds fixed, preserving mouse and keyboard target accuracy.
- Smooth animations defaults ON for users without a saved preference. **Reduce motion** still overrides it.

### Material-style update animation
- A dedicated Cyn WPF updater window with a glowing core, orbit rings, inward moving particles and finishing sparkle.
- Real-time byte progress during verified GitHub downloads, followed by checking, verifying, installing and complete stages.
- Automatically checks for releases in the background; does not show a window when already current.
- Automatically downloads and verifies available updates without requiring you to download an installer.
- One click **Install & restart**: Cyn gracefully shuts down through JUCE's existing single-instance IPC, starts the Inno Setup installer silently after Windows UAC approval, and reopens.
- Cancel before installation; no forced termination of the audio host.
- The existing GitHub Releases SHA-256 checks stay enforced.

**Windows may still ask for administrator approval during installation.** App audio settings and all 19 themes are retained.
