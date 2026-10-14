# Cyn 2.0.29 — Remix the playing song

AI DJ now uses the song itself for beat repeats, rhythmic echoes and filter
sweeps. This replaces the short noise sweeps and sine shimmers that did not
deliver the intended live remix experience. Effects last two estimated beats,
blend more clearly at normal intensity and retain stereo source content.
Timing is approximate, using detected onsets with a 120 BPM starting estimate.
This is a live effects feature, not a trained generative remix model.

Spotify linking is removed entirely: no button, connection window, session
discovery, token refresh or playback-control worker. AI DJ operates on the audio
already passing through Cyn, without a JSON file or account connection.

Speech/stale-analysis suppression, minimum spacing, power bypass and output
headroom limits remain active. Enable AI DJ under Misc after restarting; the
existing downloaded model is reused. Try a 4-second minimum spacing to hear
the remix transitions more frequently.
