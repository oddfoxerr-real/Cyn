# Cyn AI community — data for the native Windows application

**No separate AI website, GitHub Pages or hosting is needed.**

Cyn 2.0.10+ includes an AI Hub **inside the Windows app** at Smart listening → AI Hub. The built-in catalogue `index.json` and `*.cynai.json` model files in `community/` are copied to `CynAiProfiles/` next to the Cyn executable by `BUILD_CYN.ps1`.

Cyn reads the bundled catalogue offline. Clicking **Refresh community** fetches `index.json` directly from the same existing public GitHub repository. Clicking **Import selected AI** validates an embedded model or downloads the selected community model JSON directly from GitHub and imports it into the running Cyn, backing up the current model first.

User submissions and reviews are managed through GitHub Issues using the existing issue templates; no custom backend is hosted. Cyn links directly to those GitHub Issues only when the user chooses **Share my AI** or **Reviews**. Publishing a profile is a reviewed repository change rather than an anonymous upload endpoint.

**Privacy:** Public model packages have a fixed schema (`cyn-ai-profile`, schema 1, architecture `cyn-audio-11x12x5`), 209 finite weights, five trained flags and optional public metadata. **Do not post private preferences.json, training examples, recorded audio or account credentials.** Cyn's local Import AI and Export AI buttons work without any network connection.

Training uses the same local 209-parameter model already in Cyn. The user labels examples More/Less on the recent-audio timeline and selects Train my model.

This folder contains only the catalogue and model data for the native app, not an HTML website.

## Research-only AI listings

The catalogue also supports a restricted `kind: "research"` record with an official repository release URL. This is **not a Cyn smart-listening profile**. The native AI Hub labels the action **View research files**, opening the GitHub research prerelease. It never calls `importSmartProfile`, replaces local preferences, or installs an executable. Non-research entries continue using the existing `.cynai.json` schema and activation flow.

**CYN APEX-6M FlowSense** is a distinct 6.3-million-parameter offline denoiser, trained on synthetic mixtures, plus separate signal-based timing/energy analysis. FlowSense computes objective intensity arcs, onset activity and build/transition/breakdown *candidates*; it is not neural emotion recognition and is not trained to interpret personal listening preferences. There is no native Windows real-time audio integration.

Official prerelease (research only): https://github.com/oddfoxerr-real/Cyn/releases/tag/cyn-apex6m-flowsense-research-20261017

The local model files and Python runtime must be downloaded and run separately; this optional research listing does not ship a working APEX plug-in.
