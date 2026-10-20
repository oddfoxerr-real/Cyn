// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/** Exactly one user-selected primary AI profile. Scene analysis is shared infrastructure. */
final class AiSelection {
    static final String BASIC = "basic";
    static final String APEX_6M = "apex6m";
    static final String APEX_10M = "apex10m";
    private AiSelection() {}

    static String normalize(String value) {
        if (APEX_6M.equals(value)) return APEX_6M;
        if (APEX_10M.equals(value)) return APEX_10M;
        return BASIC;
    }

    static String migrate(String saved, boolean legacyApex, boolean legacySpatial) {
        // Legacy independent toggles could both be true; APEX 10M wins once.
        // Afterwards a single canonical profile is persisted.
        if (saved != null) return normalize(saved);
        if (legacySpatial) return APEX_10M;
        if (legacyApex) return APEX_6M;
        return BASIC;
    }

    static boolean usesApex6M(String profile) {
        return APEX_6M.equals(normalize(profile));
    }

    static boolean usesSpatialPreview(String profile) {
        return APEX_10M.equals(normalize(profile));
    }
}
