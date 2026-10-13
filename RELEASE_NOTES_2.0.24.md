# Cyn 2.0.24 — Theme Studio and Automatic Update Repair

Cyn 2.0.24 replaces the crowded theme decoration introduced in 2.0.23 with immersive theme scenes and introduces a native, sliding theme picker.

## Theme Studio
- Use **<** and **>** or keyboard Left/Right to browse the original 20 built-in themes through large live previews. Changes in the preview are not saved until selecting **Apply theme**.
- The carousel and real application use smoothly eased horizontal slide transitions rather than abrupt switches.
- Animated, audio-responsive artwork extends across the main workspace and gently underneath the navigation rail instead of appearing as a small separate sidebar widget.
- The live 11→12→5 neural-activity network remains on Smart Listening, rather than covering the Sound Studio sidebar.
- Existing theme IDs, saved preferences, audio DSP, Music/Movie AI, and optional AI DJ are preserved.
- **Reduce motion** disables motion while keeping still artwork visible.

## Automatic updater fixes
- Automatic checks are now independent of sound processing, including when the DSP power is off.
- The update checker launches on a recurring approximately 30-minute schedule while Cyn is running; its last successful launch is saved across app restarts.
- The PowerShell console starts without hiding the WPF graphical update prompt.
- Re-enabling auto-updates performs a fresh check; failed checker launches are retried.
- **Later** defers repeat prompts for the same release for six hours. Manual checks remain available.
- Installer integrity continues to be verified with SHA-256; updates still require user approval to install.

## Downloads
- Windows x64 installer: **Cyn-Setup-2.0.24-x64.exe**
- Portable Windows x64 ZIP and installer SHA-256 file included.
- Windows builds are not code-signed and may trigger a security prompt.

Licensed under GNU AGPL-3.0-or-later.
