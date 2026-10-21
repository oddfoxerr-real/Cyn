# CYN Studio Android 0.2.22 — AUDONIC Creative AI (experimental)

**AUDONIC is a new, fourth primary AI engine in CYN Mobile.** Select AUDONIC in **AI Hub**, then adjust **Creative intensity**. It uses a genuine trained, checksum-verified streaming ONNX model and a bounded native DSP for adaptive harmonic saturation, transient attack, stereo character, colour filtering, musical rhythmic dynamics, and bass harmonics.

AUDONIC inherits useful knowledge by training a new neural student **against the real APEX 10M spatial-policy checkpoint** as a teacher. Existing CYN APEX 6M and YAMNet models remain available under their own AI selections. Selecting AUDONIC disables other primary AI processing; scene analysis infrastructure may still run for shared user feedback.

## Performance and safeguards

- On-device ONNX inference runs in a background AI worker, never the audio callback.
- Real-time DSP uses no memory allocation, model inference, network calls, or file I/O.
- The six FX controls are bounded, fade smoothly, preserve mono compatibility, minimize subbass distortion, and pass through CYN's final output limiter.
- If the neural model fails checksum validation, is stale, or produces non-finite values, AUDONIC's experimental effects bypass safely.
- CYN Mobile can process full stereo PCM only for audio played in **CYN Library**. Connecting CYN to other Android apps still provides compatible session EQ/analysis where granted, not full AUDONIC processing in unrelated apps.
- Existing protected APEX 10M model, updates, 3D Mind Map, themes, FxSound license/credits, and app signing identity remain intact.

## Model transparency

**AUDONIC v0.1 is not a 3-billion-parameter model.** The planned 3B musical planner is a separate, untrained future architecture. This release's model is the much smaller *actually trained* neural streaming student, with exact parameter count, optimizer updates, measured seconds, and model SHA-256 documented in `audonic-manifest.json`.

The current student is trained on an APEX-10M-distilled + procedural synthetic creative FX curriculum. It is *not* claimed to have been trained on Slakh/BabySlakh audio unless the training manifest explicitly says public samples were used. Code for license-filtered public Slakh training is in `audonic/public_data.py`. Please assess real-music performance and level-match comparisons before increasing the intensity.

## License and source material

CYN retains its FxSound attribution and license. The experimental AUDONIC source code and training methods are present in this repository, and Slakh dataset licensing is detailed in `audonic/README.md`. No public audio or music snippets are redistributed in this APK.
