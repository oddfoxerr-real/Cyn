# Cyn AI community — data for the native Windows application

**No separate AI website, GitHub Pages or hosting is needed.**

Cyn 2.0.10+ includes an AI Hub **inside the Windows app** at Smart listening → AI Hub. The built-in catalogue `index.json` and `*.cynai.json` model files in `community/` are copied to `CynAiProfiles/` next to the Cyn executable by `BUILD_CYN.ps1`.

Cyn reads the bundled catalogue offline. Clicking **Refresh community** fetches `index.json` directly from the same existing public GitHub repository. Clicking **Import selected AI** validates an embedded model or downloads the selected community model JSON directly from GitHub and imports it into the running Cyn, backing up the current model first.

User submissions and reviews are managed through GitHub Issues using the existing issue templates; no custom backend is hosted. Cyn links directly to those GitHub Issues only when the user chooses **Share my AI** or **Reviews**. Publishing a profile is a reviewed repository change rather than an anonymous upload endpoint.

**Privacy:** Public model packages have a fixed schema (`cyn-ai-profile`, schema 1, architecture `cyn-audio-11x12x5`), 209 finite weights, five trained flags and optional public metadata. **Do not post private preferences.json, training examples, recorded audio or account credentials.** Cyn's local Import AI and Export AI buttons work without any network connection.

Training uses the same local 209-parameter model already in Cyn. The user labels examples More/Less on the recent-audio timeline and selects Train my model.

This folder contains only the catalogue and model data for the native app, not an HTML website.

## Native ONNX AI models

**CYN APEX 6M** is a normal activatable *Neural Music AI* in Cyn v2.0.51 and later. The AI Hub catalogue record has `kind: "onnx"`, `activation: "apex6m"`, and the official release URL. Cyn validates the release URL and uses the separately bundled SHA-256-verified 6,307,477-parameter `apex6m-225.onnx` model with its existing ONNX runtime.

Selecting **Activate selected AI** switches to Music AI and turns on Smart Listening, preserving the previous trained profile. APEX's genuine neural spectral-mask inference continuously steers bounded DSP EQ alongside CYN's adaptive bass, dynamics, cinematic/music scene recognition, crescendo analysis, flow tracking and optional preset cycling. It does not use or misrepresent the 209-parameter `.cynai.json` format. **Switch to previous AI** restores the original selection; changing Music/Movie modes does not erase private training.

This native model operates on 48 kHz float mono/stereo capture and keeps inference off the live audio thread. Other device formats, missing input, or failed inference must not invent neural activity. The full STFT mask is currently only used for **offline WAV reconstruction** in the standalone APEX program; realtime CYN uses neural EQ derived from that mask, not a full low-latency denoiser. Original training was 225 synthetic steps; a proven listening improvement has not yet been demonstrated. No new training was performed for this release.

APEX release and checkpoint: https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0

Original community submission: https://github.com/oddfoxerr-real/Cyn/issues/32

## External-only AI listings

Other full-size AI contributions without compatible CYN inference support can still be listed with `kind: "research"`. Those have an external download action, no automatic code execution, and cannot masquerade as an activatable model. Standard profile entries continue using `.cynai.json` and retain separate private learning.
