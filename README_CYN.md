# Cyn — personal audio

Cyn is a native personal-audio app built on the open-source FxSound DSP, with its own Windows identity, original icons, selectable themes and an offline neural preference model.

Thank you to the [FxSound team and contributors](https://github.com/fxsound2/fxsound-app) for making FxSound open source and providing the audio engine behind Cyn. If you enjoy Cyn, please [visit FxSound](https://www.fxsound.com/) and [support their work](https://www.fxsound.com/contribute) through donations or contributions.

## Install with the setup EXE

Exit Cyn/FxSound from the tray menu, then double-click `Cyn-Setup-2.0.15-x64.exe`. It installs the app and all required files into Program Files, adds Start-menu and optional Desktop shortcuts, and registers an uninstaller. Windows 10/11 Intel/AMD x64 and administrator access are required. An existing FxSound driver is reused; otherwise the bundled Microsoft-signed driver is installed. The original FxSound app does not need to run.

Remove Cyn through Windows Settings > Apps > Installed apps. Your `%APPDATA%\Cyn` preferences and the shared audio driver are retained. Removing the original FxSound installation may remove that shared driver; rerun Cyn setup if needed. The Cyn installer/app are unsigned community builds; the driver retains its original signature. See `Installer/Cyn/README.md` in the source for verification scope.

## Run

Exit any running FxSound or older custom build from its tray menu. Open `Cyn.exe` in this folder, or run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\LAUNCH_CYN.ps1
```

Keep the presets, fonts and `cyn-starter.json` beside the executable. Closing the window hides the app in the tray. Do not run multiple FxSound variants together.

## Optional install

The setup EXE above is recommended. The older script below only copies app files and requires an existing driver.

From the extracted app folder, this installs Cyn to your user Programs folder and adds its own Start-menu shortcut, without admin access:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\INSTALL_CYN.ps1 -Launch
```

Exit the running audio app first. Keep the official FxSound installation for the signed driver. This is a script-based install, not a signed MSI.

## Appearance and motion

Open the top menu or tray menu and choose **Theme**. Cyn has 19 native themes. **ChatGPT Desktop** matches the supplied screenshot's layered charcoal colors: `#181818` content, `#191C1E` panels, `#212121` raised surfaces, `#363636` borders, neutral grey controls and bright legible text. The palette applies to the workspace, effects, EQ, timeline, menus and controls. **Reduce motion** disables transitions and lowers the spectrum refresh rate. Both choices are saved.

Navigation crossfades for 220 ms with a sliding sidebar selection; buttons ease on hover/press, toggles slide and sliders show eased hover halos. The audio values themselves remain immediate while you drag.

Cyn has a distinct taskbar/tray icon, IPC identity, startup entry and `%APPDATA%\Cyn` settings. On first use it copies existing FxSound settings, presets, autosave and earlier Cyn learning data into its own folder without overwriting existing Cyn data or changing the old files. It still uses the official signed FxSound virtual audio driver; Windows will retain that device's official name. Exit other FxSound/Cyn variants before starting it. Cyn also holds the upstream single-host lock so stock FxSound cannot start a second processing engine later while Cyn is active. Upstream FxSound updates remain disabled; Cyn checks its own GitHub Releases for updates instead.

## Teach your preferences

1. Play audio with processing on and open **Smart listening**. The recent-audio timeline shows time; the Sound studio visualizer still shows frequencies.
2. Drag over a section of the last minute. The view freezes while you label it. Select at least half a second.
3. Choose **Bass, Clarity, Ambience, Surround, or Dynamic Boost** and press **More here** or **Less here**. Label both directions, preferably across several songs.
4. Press **Train my model**. Learning happens locally in the background. Enable Smart listening to apply it and use Sensitivity to adjust its strength.

The optional starter model supplies basic Bass and Clarity examples. Other effects stay manual until taught. Your settings remain the baseline; turning Smart listening off restores them. Learned bass reductions also relax positive EQ boosts below 350 Hz so a Bass Boost preset does not keep the same low-end emphasis during sections you label Less. Manual EQ cuts and higher bands are preserved.

The model analyzes source audio **before** effects, avoiding feedback from its own boosts. Changes now happen over seconds, with bounded smoothing. Labels/weights persist under `%APPDATA%\Cyn`; the timeline does not record or upload audio. See `MODEL_CARD.md` (in `models` in the source package) for limitations. This experimental starter has only synthetic validation, not real-song validation.

## Build the full source

Extract to a short path and open PowerShell in the source directory. JUCE 6.1.6 and resources are bundled.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\SETUP_BUILD_TOOLS.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_CYN.ps1
```

Skip setup when VS 2022 C++ Build Tools and Windows SDK are installed. Output: `dist\Cyn\Cyn.exe` and `dist\Cyn-Windows-x64.zip`. Subsequent builds support `-Incremental`. Optional `-WithMcp` requires Go and builds the repository's existing MCP server.

To build the setup EXE after the app, run `powershell -NoProfile -ExecutionPolicy Bypass -File .\BUILD_INSTALLER.ps1`. This downloads a pinned, signature-checked Inno Setup compiler into `.build-tools` when needed, verifies the bundled driver hashes, and writes `dist\Cyn-Setup-2.0.15-x64.exe`. Use `-RebuildApp` to rebuild the app too.

## Local control

```powershell
.\CYN_CONTROL.ps1 -Action Status
.\CYN_CONTROL.ps1 -Action Theme -Value Dusk
.\CYN_CONTROL.ps1 -Action Motion -Value 1
.\CYN_CONTROL.ps1 -Action Smart -Value 1
.\CYN_CONTROL.ps1 -Action Sensitivity -Value 50
.\CYN_CONTROL.ps1 -Action Effects -Value 'bass:6,clarity:3'
```

The bridge also supports Power, Preset, Eq, Output, PinOutput and Leveling. It uses the existing FxSound CLI and checks that the correct Cyn executable is running. Status includes `cyn_build`, model/learning status and effective low-EQ gains. The FxSound process identity, compatible settings and driver integration are retained internally.

## Provenance and license

Official FxSound source commit `d8e7a23d37ed5939c2a3090a1c1756c7f2500b17`, resources `8f0385f015cd6b10026b2db6a0625a6db4de89b0`, and JUCE 6.1.6 `2f980209cc4091a4490bb1bafc5d530f16834e58`. The original uploaded ZIP was unavailable; the earlier recovered theme patch was applied to official source. FxSound DSP and driver functionality is retained. Cyn is an unofficial custom build. AGPL-3.0-or-later; see LICENSE and bundled JUCE notices. Distribute the corresponding source with binaries.

## Update checker fix (2.0.4)

Earlier Cyn 2.0.2 and 2.0.3 installations can falsely show "up to date" because Windows PowerShell 5.1 wraps the GitHub Releases JSON array as a single nested array. The 2.0.4 update checker handles release objects individually and is tested against published releases on Windows. Install 2.0.4 manually once if your prior version cannot find updates. The update checker will work again for future releases.

## Smooth UI animations (v2.0.5)

Turn on **Smooth animations** under Settings > Help > Appearance, or using the main or tray menu. When enabled, native popup menus (including combo box drop-down menus) fade into place, the drop-down arrow rotates while opening, and button, toggle, and slider motion uses a gentler easing curve. Menu geometry stays fixed to preserve clicking and keyboard accessibility. The preference persists in `%APPDATA%\Cyn`. **Reduce motion** overrides these animations.

## Seamless updates (v2.0.6)

GitHub release checks now stage the installer automatically, verify the actual SHA-256 digest, show an animated Cyn window with real download progress, and offer **Install & restart**. The updater requests graceful Cyn shutdown via single-instance IPC, installs silently after Windows' UAC approval, then relaunches Cyn. No downloading EXEs manually or closing the tray app. Automatic checks are quiet when there is no newer version. The installer carries the native update window and separate network worker scripts.

The Smooth animations toggle defaults on unless a user disabled it earlier. Native JUCE popup menus and nested submenus now start their fade at their actual show event, rather than from a too-early LookAndFeel callback. Reduce motion still disables this effect.

## Expanding category controls (v2.0.8)

Click a category in the menu, such as Theme, Save New Preset or Rename Preset. Its text-button options expand **within the same menu**, one after another from underneath the category button, pushing following rows downward. Click again to collapse from bottom to top. The menu uses a single vertical column with scrolling on smaller displays. The Smooth animations option controls the 60Hz cubic-eased stagger; Reduce motion presents the same category hierarchy without animation. Keyboard activation works via Enter and right arrow.

## Cyn AI Hub — native, no website (v2.0.10)

**The AI Hub lives inside Cyn.exe**, under **Smart listening → AI Hub**. No separate website, self-hosted server, GitHub Pages, domain, account, or launcher is required. Cyn bundles an offline community catalogue and starter model inside `CynAiProfiles/`. Open the native AI Hub window to browse, read profile descriptions, and activate an AI model.

**Optional community connectivity:** Refresh community reads a static, public JSON catalogue from the same existing GitHub repository; Cyn only downloads model JSON when you explicitly choose Import. Public profile reviews and new submissions use GitHub Issues through the user's browser when those actions are selected. Without internet, installed profiles, offline training, local import and local export remain available.

The existing 209-parameter neural model is trained locally using the Smart listening timeline and your More/Less feedback. Exported `.cynai.json` packages contain only weights, trained-effect flags and public metadata — **not** sound recordings or private labelled examples. Import backs up your original preferences before replacing the model.

The earlier standalone web prototype and GitHub Pages workflow were removed from v2.0.10. The `ai-hub/community/` folder in the **source repository** is simply the data catalogue used by Cyn, not a website.

## Cyn 2.0.11 — visible AI Hub controls

**AI Hub is now a fourth entry in Cyn's main sidebar**, just under Output devices, so there is a permanent entry point even outside Smart listening. In the Smart listening page footer, Export AI, Import AI and AI Hub buttons are brought above legacy controls and use contrasting theme colours. The new AI PROFILES heading identifies them. A small **Version 2.0.11** label appears below the sidebar brand text to make outdated installations obvious in screenshots. No web hosting is involved, and the bundled profile catalogue still runs offline.

## Cyn 2.0.12 — AI Hub visibility and stable Themes menu

The native AI Hub previously drew the 24-bit theme palette values as alpha-zero colours, making its background and text see-through. All AI Hub palette colours are now explicitly opaque, its native content is marked opaque, and the dialog is centred on Cyn's main window. Community profile list, descriptions, actions, and controls become visible on top of Cyn rather than appearing as a translucent sheet. The native AI Hub and its offline starter profile are retained without any external website.

The experimental category accordion in the low-level JUCE popup code was interrupting theme selection. That experimental mode has been disabled; Theme now opens through JUCE's stable native submenu handling, with the existing optional 215 ms fade animation preserved. The earlier inline accordion effect is deferred until it can be redesigned without breaking theme choice or custom preset items. Verify theme selection under the main menu and tray icon after installing.

## Cyn 2.0.13 — visible AI controls and seamless tooltips

The Smart listening AI actions now use opaque colours for both backgrounds and labels. Tooltips composite with properly rounded transparent corners rather than showing black rectangles. A source regression check runs in the Windows release workflow. All existing models, user labels, effects, and updater behaviour are preserved. This UI release does not claim any additional model training.

## Cyn 2.0.14: Neural Activity & All Audio

The native Smart Listening page includes an animated amber neural-activity network reflecting the 11–12–5 model topology and the application training state. It supports reduced motion and uses a bounded repaint timer. Smart Listening can learn from **any** audio playing through the configured Windows output: music, films, series episodes, podcasts, spoken dialogue, or games. The model is not a source classifier or speech recognizer; the current starter is trained only on synthetic Bass and Clarity examples, with personal More/Less labels for other effects. The longer-running base-model training will be delivered as a **separate subsequent update**, not included in this UI-focused build.

## Cyn 2.0.15 — one-hour AI training
The experimental base model was trained using generated audio-feature data; see models/MODEL_CARD.md and models/starter-evaluation.json for held-out synthetic tests.
