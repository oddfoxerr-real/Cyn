# Cyn 2.0.28 — AI DJ effects and EMBER Spotify connection

Fixes the zero-effects problem when the large AST model runs slowly. Fresh
YAMNet music recognition now drives the live DJ while AST gathers background
context. Sustained music receives spaced accents even without sharp onsets.
Synthesized sweep and shimmer levels are stronger, bounded by output headroom.
Speech, silence, stale analysis, power-off and exact bypass checks remain active.
The optional model already downloaded in 2.0.22 is reused.

Adds **Misc → Spotify / EMBER**. Choose or drop your existing signed-in EMBER
Spotify session JSON for title, artist, music/podcast type, device, position and
playback controls. No new Spotify developer app is required. Token refresh and
rotation preserve the existing JSON layout and unrelated fields. No private
session JSON is included in the release. The source includes a helper to add a
session-export button to the user's existing EMBER panel.

AI DJ starts disabled after restarting Cyn. Enable it in Misc and temporarily
try 4-second spacing to check the actual accent counter. CLI status now also
reports DJ accents, the last effect and completed background analyses.

Spotify does not provide a dependable song-theme description. Metadata is shown
as supplied; the integration does not invent genre-based bass boosts, download
Spotify network audio, or train a model on Spotify metadata.

Recognition and timing remain experimental. Synthetic audio/transport tests do
not establish subjective listening quality or an individual account's API access.
Live Spotify testing requires the user's session JSON. See docs/SPOTIFY.md and
docs/AI-DJ-VALIDATION.md for details.
