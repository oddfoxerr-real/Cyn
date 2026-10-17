# Cyn v2.0.49 — Community model visibility and Publish AI fix

**Fix:** Official full-size APEX model listing can disappear after a successful AI Hub refresh.

## Changes

- Replace brittle APEX tag allowlisting with a native-tested official-repository-release URL validator. Only `https://github.com/oddfoxerr-real/Cyn/releases/tag/<safe-tag>` is accepted; query strings, redirects, unrelated hosts, user-info and path traversal remain rejected.
- Show concrete **accepted / total** counts and number of **external** (not native-importable) models after refreshing the community catalogue. No more silent "refreshed" when an entry was filtered.
- Official CYN APEX 6M Full Neural Preview retains its community submission at [issue #32](https://github.com/oddfoxerr-real/Cyn/issues/32) and points at [the runnable 6.3M-model release](https://github.com/oddfoxerr-real/Cyn/releases/tag/apex-6m-v1.0.0).
- **Publish AI** now accepts arbitrary contributed trained AI formats (standard Cyn profiles, ONNX, PyTorch, model ZIPs). Contributors declare actual checkpoint rights and dependencies; publishing no longer requires making misleading claims that every model is a small `.cynai.json`.
- CI compiles and runs C++ test cases for the real URL validator, validates catalogue membership and builds CYN Windows x64.

## How to verify

1. Install Cyn v2.0.49 and fully exit any previous Cyn instance from the system tray before restarting.
2. Open **Smart Listening → AI Hub → Refresh community**.
3. Expect **five** entries including *CYN APEX 6M (Full Neural Preview)*. The status should read `Community refreshed: 5/5 listings (1 external).`
4. Select APEX; use **View research files** to open its actual full-model WAV processor release.

APEX's existing 6,307,477-parameter ONNX audio processor runs in a standalone executable. It is not currently activatable as a real-time Cyn DSP profile. The app never silently downloads or runs executable submissions. This fix changes community visibility and publication, not audio processing, model weights or neural training. All training remains on hold.
