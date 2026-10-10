# Cyn AI Hub

A **standalone static web app**, designed to run on GitHub Pages independently of the Cyn Windows launcher.

- `index.html`, `style.css`, `app.js` have no third-party JS dependencies, accounts, trackers, API keys, or server component.
- `community/index.json` is a **reviewed, static** catalogue. Each entry links to a privacy-filtered `*.cynai.json` file.
- `Studio` imports an exported profile or a local Cyn `preferences.json` file. The latter remains **in memory in the local browser**, and the real 11×12×5 network can be trained with the More/Less examples already labelled in Cyn.
- `Export` strips private examples, raw audio, timestamps, system paths, and device info. Shared models contain **only metadata and neural weights/readiness**.
- Use Cyn **Smart listening → Export AI / Import AI** for cross-device/profile interchange.
- Public publishing is by GitHub issue submission and maintainer review, then a community JSON/asset PR. Reviews happen on a dedicated public GitHub issue for each profile. **No star averages are generated or implied** without a review backend.
- Since this is a static web app, a malicious third-party `*.cynai.json` cannot execute as code. Cyn and Hub both validate weight count, numeric finiteness and model architecture.

## Publishing the separate page

This folder is self-contained. It can be copied into its **own GitHub repository** (e.g. `oddfoxerr-real/Cyn-AI-Hub`) without the Windows app code. The currently connected GitHub integration doesn't offer repository creation, so it is initially developed under `Cyn/ai-hub/`.

To enable a GitHub Pages site from the existing Cyn repository, open **Settings → Pages → Build and deployment**, choose **GitHub Actions**, then run the included `Cyn AI Hub Pages` workflow. Its intended site path is `https://oddfoxerr-real.github.io/Cyn/ai-hub/` (subject to GitHub Pages actually being enabled). The C++ AI Hub button currently opens this source directory so it never links to an unconfigured site.

A new standalone repository can publish the contents of this folder at its own GitHub Pages address; adjust community submission/review links when moving.

## Format

```json
{
  "format": "cyn-ai-profile",
  "schema": 1,
  "profile": {
    "name": "My AI",
    "author": "Creator",
    "description": "...",
    "category": "music",
    "license": "CC-BY-4.0"
  },
  "model": {
    "schema": 1,
    "architecture": "cyn-audio-11x12x5",
    "weights": [209, "finite", "numbers"],
    "ready": [true, false, false, false, true]
  }
}
```

The example weights above are **notation only**, not a valid file. The real starter package `community/cyn-starter.cynai.json` is a valid importable example. A trained flag means the model has at least 5 labels in both directions for that effect. No listening metrics are used for rankings.

Audio labels are private; a received profile can be adapted by creating your **own** examples in Cyn. Source AGPL-3.0-or-later; exported public profile metadata defaults to CC BY 4.0 and creators must agree before community publication.
