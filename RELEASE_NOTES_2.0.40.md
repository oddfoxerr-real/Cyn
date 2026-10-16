# Cyn v2.0.40 — Gemini Night, neural identity and Version controls

See the version that is **actually installed on your PC** in **Settings → Help → Version & updates**.

- **Version** button: shows `Installed: Cyn v2.0.40` (or the real version of that installation). This requires no internet.
- **Check for updates** button: manually opens Cyn's existing signed-checksum-verified GitHub updater to determine whether a newer stable release is available.
- **Automatic updates** toggle: clearly controls background release checks, independently of the manual button.
- The installed version is also displayed directly in the pane without opening a dialog.
- Native application, installer, setup metadata, and footer now all report **2.0.40**, with CI checks to prevent them drifting apart.

**Important:** The Version button reports **what is installed**, not whether it's the newest release online. Use **Check for updates** to compare with GitHub. The latest release is never assumed from a local label.

## New theme and visual neural identity

- **Gemini Night** theme in the existing Theme Studio: dark charcoal workspace inspired by Google's dark UI, with subtle blue/violet and multicolour assistant-like glows. No Google branding or assets are copied.
- Neural activity card now recolors when switching the **actual** active AI profile: cyan Starter, violet Cinema, amber Real Audio 13h, teal personal, and deterministic colors for custom imports.
- The visible connection density changes by model profile and the **actual live weighted activation**. The display still represents Cyn's true 11-input / 12-hidden / 5-output model; hiding more or fewer lines does not add neurons or suggest one model has an objectively higher intelligence.
- Fixed the collision between the Smart Listening toggle / YAMNet status bar and the Listening Mode / Active AI Model labels.
- All neural colors, connections, pulses and backdrops respect the existing reduced-motion option. No DSP, recorded audio, or trained model weights are modified by the visuals.

This update does not alter audio DSP, training data, or device repair.
