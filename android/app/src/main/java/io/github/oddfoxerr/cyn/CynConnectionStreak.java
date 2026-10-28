// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/**
 * Volatile, UI-only Easter egg detector for the actual Connect CYN button.
 *
 * Four complete Connect CYN -> Disconnect CYN pairs must occur consecutively,
 * with no gap longer than 20 seconds between button actions. No state is saved:
 * a later app session cannot finish a partial streak.
 *
 * SystemClock.elapsedRealtime() is supplied by MainActivity, so time spent
 * asleep counts towards the timeout.
 */
final class CynConnectionStreak {
    static final long MAX_GAP_MS = 20_000L;
    static final int REQUIRED_CYCLES = 4;

    private int completedCycles;
    private boolean awaitingDisconnect;
    private long lastActionMs = -1L;

    void onConnect(long nowMs) {
        if (timedOut(nowMs) || awaitingDisconnect) reset();
        awaitingDisconnect = true;
        lastActionMs = nowMs;
    }

    boolean onDisconnect(long nowMs) {
        if (!awaitingDisconnect || timedOut(nowMs)) {
            reset();
            return false;
        }
        awaitingDisconnect = false;
        lastActionMs = nowMs;
        completedCycles++;
        if (completedCycles == REQUIRED_CYCLES) {
            reset();
            return true;
        }
        return false;
    }

    void reset() {
        completedCycles = 0;
        awaitingDisconnect = false;
        lastActionMs = -1L;
    }

    private boolean timedOut(long nowMs) {
        return lastActionMs >= 0L &&
            (nowMs < lastActionMs || nowMs - lastActionMs > MAX_GAP_MS);
    }
}
