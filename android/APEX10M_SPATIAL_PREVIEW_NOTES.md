# CYN Studio Android 0.2.21 — APEX 10M Trained AI Experimental Release

This version bundles the **95.97%-trained APEX 10M spatial policy head** from the immutable release `apex10m-experimental-rc-v1`. The ONNX model's SHA-256 and provenance manifest are verified at build time and its checksum is verified again before loading on device.

**Inside AI Hub → APEX 10M**, the experimental neural-spatial controller can be enabled or bypassed. The neural head uses actual bass, kick/onset, stereo width, phase, and headroom measurements from music played in CYN's Library, and supplies bounded spatial decisions to CYN's existing conservative DSP. The 3D mind map and connected-audio analysis from v0.2.20 are preserved.

This frozen release's verified training progress is **95.97% of the 15-hour target**; only the new 3,693,992-parameter spatial-policy head was trained. CYN's older APEX 6M base was not retrained. This training was on procedural synthetic proxy scenes, not real music; real-world sound quality and spatial imaging improvements are not yet independently verified. The model's outputs remain subject to phase/headroom safety gates and inference timeout failsafe. The 15-hour GitHub training workflow can continue independently without altering the released checkpoint.

This is an experimental release candidate, not a declaration that the full 10M combined architecture has finished training or that a production-quality AI has been validated.

# CYN Studio Android 0.2.21 — Connected AI 3D Mind Map

- The rotating CYN mind map works when CYN is connected to a compatible external app, not only during CYN Library playback.
- Uses the app's already permission-gated Android Visualizer FFT; as connected music changes, measured bass, transient/kick proxy, and spectrum activity trigger 3D pulses.
- Shows the **real 12 hidden neuron activations and 60 learned weighted paths** from CYN's local Personal AI in the connected-audio mode. Light levels and pathway brightness reflect those measured calculations.
- The map appears in AI Hub for any primary AI profile when CYN is connected.
- Connected mode displays CYN NEURAL; CYN Library APEX 10M mode retains native spatial DSP and the frozen 37.33%-trained ONNX controller.
- Without Android's playback-analysis permission or when the audio app does not expose a usable session, the map rotates at idle and displays Waiting for Audio. It never invents activity.
- All inference and visualization remain on device. No additional privileges, network audio capture, recording storage, or background monitoring are added.
- This release does **not** claim that APEX 10M can process other apps' stereo PCM or run its 10M ONNX policy on external audio. Connected-app effects depend on Android's supported audio session EQ.

# CYN Studio Android 0.2.21 — APEX 10M Experimental Release

The APEX 10M radio choice is now labelled as a released experimental AI version, not as a separate "Try frozen" add-on. Selecting APEX 10M explicitly activates its bundled 37.33%-trained spatial controller. A simple "APEX 10M neural control" switch supports comparisons with classic spatial DSP.

## Live brain-mesh animation fix

- Fixed the frozen animation: the brain now continuously redraws and rotates, even in standby, unless Reduce Motion is enabled.
- 3D rotation visibly speeds up with genuine spatial output and neural decision activity.
- Bass changes illuminate the lower node graph, kick/transient onsets produce sharp bursts, spatial output activates the outer branches, and newly completed neural inference triggers more luminous paths.
- Burst colours vary per event, while triggers, intensity and pacing follow actual CYN PCM/ONNX telemetry.
- APEX 10M's neural inference worker now produces decisions independently of YAMNet scene classification.
- Short, useful labels replace verbose explanation text inside AI Hub and the scanner. Technical qualification remains in release notes.

## Model and audio limits

The bundled ONNX weights are the immutable frozen 37.33% checkpoint of the 3,693,992-parameter spatial head, from a 15-hour synthetic-training target. The original training job is separate. This release does not add trained weights for the original APEX 6M base and does not demonstrate better perceptual quality on real songs. The learned policy can only reduce the conservative stereo effect and is only applied to music played in CYN Library; independent phase/bass controls and fail-closed conditions remain. Android's ordinary verified APK install process still applies.

# CYN Studio Android 0.2.21 — Frozen APEX 10M 37.33% Neural Test

**Experimental preview only. Not validated on real music.**

This APK includes the frozen 37.33%-trained **3,693,992-parameter spatial policy head** exported as ONNX from the synthetic APEX 10M training job. The pre-existing 6,307,477-parameter APEX 6M base is neither retrained nor stacked with this head as a second selected AI; the full architecture adds to 10,001,469 parameters.

In **AI → APEX 10M**, an optional **Try frozen 37.33% neural policy** toggle enables mobile ONNX inference only after the user accepts the research warning. The feature is OFF on first install and can be switched off without changing the other settings.

How it works:
- The native CYN-library PCM processor measures 20 consecutive 100-ms stereo frames: low-frequency bass, onset/kick proxy, side energy, L/R phase correlation, transient flux, midrange proxy, headroom, and directional balance.
- An isolated background ONNX worker runs the frozen temporal policy (no neural inference on the real-time audio thread).
- The learned width, bass, and kick outputs **attenuate the existing conservative spatial DSP only**. The control cannot exceed the user's normal maximum; existing stereo phase, mono, and limiter safety gates still apply.
- Missing model, old inference, unknown format, invalid checksum, or processing errors automatically bypass the experimental effect.
- Only audio played in CYN's **Library** reaches the native PCM stage. Other apps are not processed by this preview.
- The AI Hub shows the neural model's measured inference status. The decorative brain network is not proof of actual individual neuron activations.
- The main 15-hour training run is **not interrupted**; this is a separate frozen test snapshot.
- No audio, preferences, or recordings are uploaded for processing.

Bundled weights are checked against the immutable preview release's manifest and SHA-256. Android installs and verifies the whole APK under the established CYN Studio package identity and signing certificate.

**Limits:** Real-music perceptual quality, large-scale audio reasoning, spatial localization, low-end improvement, and gaming/binaural positioning remain unverified. This does not train or enable the complete combined 10M neural system as a production model. Use the experimental comparison toggle only for listening tests, not as a default claim of improved sound.

# CYN Mobile 0.2.21 — Cinematic APEX network mesh

- Replaced the rotating wire sphere with a brain-shaped **300-node branching 3D network mesh**.
- Always-visible dendrite paths, coloured neuron hubs, corpus-callosum bridges and front/back depth.
- Every activity-gated visual burst chooses a different vivid neon hue and travels over connected node paths, illuminating junctions and branches.
- Pulse emission, intensity and speed follow measured native stereo-spatial DSP energy. The colours and paths are artistic, **not real neural thoughts or a trained 10M inference trace**.
- Lightweight cached geometry, batched primitives, and reduced-motion behaviour; visual stays strictly in AI Hub.
- No Windows/PC changes in this mobile-only release.

## Theme Studio + spatial scan — 0.2.21

- Eleven distinct Material-inspired palettes across CYN Mobile (not just accent changes).
- Five actual page-reveal animations: Liquid ripple, Material fade, Horizontal glide, Iris reveal and Prism sweep, plus Instant.
- Relaxed/Balanced/Snappy page-transition speed presets.
- Material ripple, soft press, floating lift and elastic button interactions.
- Reduce Motion respects user preference and Android system animator settings.
- APEX 10M AI Hub now contains an illuminated projected 3D brain-style scan: static illustrative lights stay visible, while moving pulses respond **only to measured spatial DSP output**.
- The extra APEX 10M neural reasoning head remains **untrained and inactive**. The scan does not claim to show its neural activations.

## Brain scan visibility fix — v0.2.21

- Visible neuron-like nodes, connection paths and small static lights even in standby, rather than an almost empty spinning wireframe.
- 3D perspective layers with foreground and background brightness, color accents and soft node halos.
- Audio-driven highlight pulses travelling along the spatial lattice, a stereo-activity scan line and clearer LIVE/STANDBY labelling.
- The lattice and stationary lights are **illustrative only**. Moving pulses and extra illumination depend on **measured native stereo DSP output**, never simulated AI inference.
- Lightweight Android Canvas renderer with preallocated 3D projection buffers, no cloud calls or extra model weights.
- Keeps existing 11 themes, page transitions, AI-only category placement and separate Updates section.

# CYN Studio Android 0.2.21 preview — APEX 10M Spatial Experiment

This Android prerelease adds an optional APEX 10M experimental stereo spatial
processing mode to the **native PCM audio path for music played inside CYN's
library**. The same experimental spatial concept is available for Windows via
the separate APEX 10M desktop spatial WAV preview release.

## Update Center redesign in 0.2.14
- Material 3-inspired update screen matching the active CYN theme.
- Installed/available versions, step-by-step update status, exact download percentages, clear retry/ready states.
- One primary update action, signed APK verification, and Android-owned install confirmation.
- No fabricated progress: checking and verification are indeterminate; data transfer progress uses real bytes.

## Fixed in 0.2.14
- APEX 10M now appears in **AI Hub** alongside Base AI and APEX 6M, rather than as a misplaced Sound Studio toggle.
- One primary AI can be selected at a time using a radio group. Legacy independent toggles are migrated safely, preferring the experimental spatial profile if both were previously enabled.
- Changing to Base AI or APEX 6M immediately disables APEX 10M processing.
- Shared YAMNet scene analysis and content focus are supporting controls, not extra selected AIs.
- Spatial intensity is now configured inside AI Hub and respects the Smart Listening master switch.

## Using the spatial preview
Open CYN Studio → AI → **APEX 10M · Experimental spatial DSP preview**.
Choose it as the sole primary AI and adjust **Spatial intensity**
(start at 40%). Listen to a track using CYN's **Library** tab.

The effect selectively increases existing high-frequency stereo side detail;
it does not widen low bass and preserves the summed center. The processing
automatically reduces widening when a signal is strongly out of phase or
already especially wide. An off switch stops adding enhancement. Settings
persist locally on-device.

## What this build does NOT do
- **No trained APEX 10M model is active.** APEX 10M's 10,001,469-parameter
  model architecture (6,307,477 existing + 3,693,992 new trainable spatial
  head) is research source only. Its new head has no trained checkpoint.
- This is a **real-time DSP experimental preview**, not binaural HRTF,
  object-audio decoding, Dolby Atmos, DTS:X, multichannel rendering,
  head tracking, or source separation.
- Third-party apps use Android's device-dependent session effects. Their
  audio is **not** routed through this preview's native PCM processor.
- Stereo content with little spatial information cannot be turned reliably
  into real positional audio.
- No user audio is uploaded.

## Install and compatibility
APK uses the existing `io.github.oddfoxerr.cyn.studio` identity,
Android versionCode 25 and versionName 0.2.21-preview. A verified match
to the existing CYN Studio signing certificate is required before publishing.
The APK is released with a SHA-256 checksum and source archive.

APEX 10M is OFF by default on fresh installs, and existing AI, EQ, Library, Themes,
device connection and update settings remain available. This builds on the
existing Android Studio 0.2.11 source branch without changing Windows stable.

## License
CYN source retains the AGPL-3.0-or-later license and applicable original
FxSound notices. The new experimental spatial DSP code is original CYN work.
