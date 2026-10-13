# Spotify using your existing EMBER connection

Open **Misc → Spotify / EMBER → Choose EMBER JSON**, or drop the JSON into that
window. The original file is used directly. Alternatively, place `spotify.json`,
`ember-spotify.json`, or `spotify_tokens.json` next to Cyn.exe or in
`%APPDATA%\Cyn`. Cyn discovers it automatically while running.

This reuses EMBER's existing Spotify app and signed-in session. No new developer
app, client secret, redirect registration, or account setup is required. The
provided EMBER panel uses client ID `c82b14bfdfa24024ab295548bd169aae` and PKCE.
Cyn accepts its `access_token`, `refresh_token`, `expires_at` fields at the JSON
root, inside `tokens`, `token_info`, or `spotify`, or an exported
`ember2_spotify_tokens` local-storage value. An explicit `client_id` overrides
the EMBER default. A client-ID-only JSON is not a signed-in session.

The supplied `panel (1).html` keeps the session in browser local storage; the
supplied bot.py delegates integrations to other modules. To export from that
panel, the accompanying `panel-cyn-spotify.html` copy adds **Export session for
Cyn** alongside Spotify's Disconnect button. Use the updated panel in your
existing EMBER installation/origin, sign in there if needed, then export. Merely
opening the HTML at a different address cannot read the existing session.
The source helper is `scripts/add_ember_spotify_export.py`.

Cyn shows Spotify's title, artist, music/podcast type, position, playback device,
and play/pause status, with Previous, Play/Pause, Next, and Open in Spotify.
It never starts playback or changes devices automatically. Spotify may require
Premium and the existing playback scopes for controls. Errors, no playback,
expired sessions, and rate limits clear old track information rather than
pretending a connection is live. Network work stays off the audio/UI threads.

The JSON is a private credential file. Cyn refreshes the existing session through
Spotify's fixed HTTPS token endpoint and updates token fields in that same file,
preserving other EMBER data. Do not run two writers against the same live file;
an exported copy is preferable. Only the selected path is saved separately.
**Disconnect Cyn** stops this connection and suppresses auto-discovery on restart;
it does not sign you out of EMBER or delete your JSON. Select a JSON to reconnect.
Do not put session JSON in releases or source control.

Spotify's API does not supply a dependable prose song description, genre, beat
grid, or upcoming drop. This integration does not invent those fields or use
Spotify metadata to train audio models. The independent local audio path remains
responsible for live recognition. Spotify network audio is not downloaded or
fed into the DJ engine by this integration.

Tests use synthetic credentials and a mocked HTTPS transport to verify real
production parsing, refresh rotation, preservation of unrelated fields,
music/podcast metadata, controls, missing playback, rate limiting and disconnect.
Live account verification requires the user's existing session JSON.
