# Cyn 2.0.19 — Flagship YAMNet DJ/Movie AI

The main pretrained YAMNet classifier now drives the adaptive Music and Movie audio decisions instead of the earlier three-score shortcut. The worker uses all 521 audio-event scores for scene complexity, extracts percussion, bass drum, electronic/hip-hop, vocals, soundtracks and speech cues, and projects all 1024 pretrained embedding dimensions into 24 compact features for per-user More/Less contextual learning.

Music DJ mode responds to beat and electronic content using low-frequency balance, transients, active dynamic boost, subtle reversible low-EQ punch and pre-effects headroom. The prior 58% Music Bass preference cap and 6.5/10 ceiling are gone. Movie mode remains independently dialogue-oriented. Existing 209-weight personal AI profiles, sound presets and feedback are preserved; the small model still works as fallback. Embedding training stays local and separate per Music/Movie. YAMNet inference runs only on the background CPU worker.

Native Windows unit tests cover phonk-like percussion vs calm music, peak-risk backoff, dialog clarity and contextual feedback. Windows native build, packaging and release checksum are validated before release.

Limitations: this is experimental sound-event-driven mixing, not a separately trained or validated professional mastering model. YAMNet does not predict objectively ideal EQ. The 24D embedding projection is a compression, and input-peak estimation is not a true-peak limiter on post-DSP output. Real listening and CPU-performance tests on users' Windows hardware are still needed.
