// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/** Exactly one user-selected primary AI profile. Scene analysis is shared infrastructure. */
final class AiSelection {
    static final String BASIC = "basic";
    static final String APEX_6M = "apex6m";
    static final String APEX_10M = "apex10m";
    static final String AUDONIC = "audonic";
    static final String LIBERTY = "liberty";
    private AiSelection() {}

    static String normalize(String value) {
        if (APEX_6M.equals(value)) return APEX_6M;
        if (APEX_10M.equals(value)) return APEX_10M;
        if (AUDONIC.equals(value)) return AUDONIC;
        if (LIBERTY.equals(value)) return LIBERTY;
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

    /** LIBERTY reuses AUDONIC's verified controls, never its sound-generation DSP. */
    static boolean usesAdaptiveControls(String profile) {
        String selected = normalize(profile);
        return AUDONIC.equals(selected) || LIBERTY.equals(selected);
    }
    static boolean usesCreativeDsp(String profile) {
        return AUDONIC.equals(normalize(profile));
    }
    static boolean usesSessionSafeEq(String profile) {
        return LIBERTY.equals(normalize(profile));
    }

    static boolean usesApex6M(String profile) {
        return APEX_6M.equals(normalize(profile));
    }

    static boolean usesSpatialPreview(String profile) {
        return APEX_10M.equals(normalize(profile));
    }
}
