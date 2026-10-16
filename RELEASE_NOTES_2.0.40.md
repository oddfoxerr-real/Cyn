# Cyn v2.0.40 — Version & update controls

See the version that is **actually installed on your PC** in **Settings → Help → Version & updates**.

- **Version** button: shows `Installed: Cyn v2.0.40` (or the real version of that installation). This requires no internet.
- **Check for updates** button: manually opens Cyn's existing signed-checksum-verified GitHub updater to determine whether a newer stable release is available.
- **Automatic updates** toggle: clearly controls background release checks, independently of the manual button.
- The installed version is also displayed directly in the pane without opening a dialog.
- Native application, installer, setup metadata, and footer now all report **2.0.40**, with CI checks to prevent them drifting apart.

**Important:** The Version button reports **what is installed**, not whether it's the newest release online. Use **Check for updates** to compare with GitHub. The latest release is never assumed from a local label.

This update is limited to update/version visibility; it does not alter the audio neural network, sound quality, DSP controls, training data, or themes.
