# CYN Mobile 0.2.22 preview — Connect AI playback analysis

Connect now offers Android playback sharing consent and analyzes permitted media/game audio through an isolated copy of the existing YAMNet/APEX inference engine. The shared Studio EQ curve (31/35/39 bands) maps to the bands exposed by each Android effect session. The selected APEX 10M spatial policy can reduce the requested Android Virtualizer strength when that effect is supported. Library playback remains optional.

## Fixed
- Connect previously ran only FFT balancing and the personal preference model. The selected YAMNet/APEX engines were activated only by Library playback.
- Connect ignored Studio's EQ curve and used a separate legacy five-band array.
- A failed YAMNet inference no longer erases an independently successful APEX spatial decision.
- Capture and Library have separate native/inference state. Disconnect, revoked consent, paused/blocked input, disabled enhancement, and profile changes invalidate neural decisions.
- Optional stereo/loudness effect errors no longer prevent a working EQ from being updated.

## Use
Tap Connect CYN, grant audio permission, and approve Android's playback-sharing prompt. No microphone source, screen frames, files, or uploads are used. Play music in a compatible app; if its EQ session was already open, restart playback or select CYN in its EQ settings. Enable Connect AI retries a declined or stopped sharing session. Device-wide EQ remains an explicit experimental option.

## Actual scope
This is not a universal Android audio driver. Apps can block capture or expose no usable audio-effect session. Capture alone cannot replace or mute another app's original output; CYN does not play a duplicate captured stream. Neural adjustments use Android's EQ/Virtualizer, not Library's native stereo/ambience/dynamics renderer. Android may decline virtualization, especially on particular outputs. Multiple unrelated external sessions retain manual/per-session fallback adjustments; mix-derived neural decisions are applied only to a single external session or an explicitly selected system-mix route. The UI distinguishes analysis from available effect control. Samsung/device routing and audible outcomes require on-phone verification.

The existing experimental APEX checkpoint and its synthetic-training limitations are unchanged. Android 10+ is required for playback capture; Android 9 retains compatible-session effects and Visualizer analysis. Package identity and registered signing certificate are unchanged; versionCode is 26.
