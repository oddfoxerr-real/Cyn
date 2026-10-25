// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import java.util.Set;

/**
 * Fail-closed Android audio-effect eligibility, separate from EQ implementation.
 * A MediaSession does not reveal its audio session ID. The optional package name
 * carried by Android's audio-effect OPEN broadcast is the only association we
 * can use between a supported audio-effect session and a playing media app.
 */
final class SessionEffectPolicy {
    private SessionEffectPolicy() { }

    static boolean allows(
            boolean monitoring, boolean enabled, boolean libraryPlaying,
            boolean legacyMix, int sessionId, int librarySessionId,
            String sessionPackage, Set<String> playingPackages,
            int externalSessions) {
        if (!monitoring || !enabled || libraryPlaying ||
                sessionId < 0 || sessionId == librarySessionId ||
                playingPackages == null || playingPackages.isEmpty()) {
            return false;
        }
        // Legacy mix is always a separately requested, device-dependent mode.
        if (legacyMix) return sessionId == 0;
        if (sessionId == 0) return false;
        if (sessionPackage != null && !sessionPackage.trim().isEmpty()) {
            return playingPackages.contains(sessionPackage);
        }
        // Some older players omit the advertised mandatory package extra.
        // Do not attribute an unlabelled session when several are open or playing.
        return externalSessions == 1 && playingPackages.size() == 1;
    }
}
