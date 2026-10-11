# Cyn 2.0.20 — Neural Core: Real AI Activity Theme

### New: Neural Core
Select **Theme → Neural Core** in Cyn. Deep-indigo glass layers, cyan neural lighting and violet YAMNet event meters provide a distinct visual theme rather than a solid flat color.

- The 11-input / 12-hidden / 5-output network in the sidebar uses actual **personal preference model** activations and weighted signal contributions.
- Header meters show YAMNet's real **music**, **percussion** and **speech** event scores derived from the asynchronous CPU inference worker.
- The sidebar graph stays visible across Sound Studio, Smart Listening and Output Devices while this theme is active.
- Stale playback/neural scores fade toward idle instead of simulating constant activity.
- 24 FPS maximum for the backdrop redraw, separate 24 FPS neural-panel timer, and reduced-motion support; no inference occurs on the UI paint path or the audio callback.
- Properly labels the difference between personal-network activations and YAMNet sound-event predictions (these are NOT visualizations of YAMNet's internal network weights).

### More expressive surfaces for existing themes
- Workspace and navigation gradients with subtle lighting.
- Dimensional cards, menu surfaces, buttons, and native window shells.
- Existing palette IDs and saved themes remain compatible.

### Validation
Native JUCE Windows builds, neural-telemetry/UI regression checks, release installer compilation and installer checksum should pass before publication.

### Limitations
Theme meters reflect the existing classifier's event scores, not calibrated genre confidence. Visual behavior cannot be fully assessed without running the installer on the user's display/Windows machine.
