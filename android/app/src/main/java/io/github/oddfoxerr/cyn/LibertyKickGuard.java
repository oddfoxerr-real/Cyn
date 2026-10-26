// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/** Optional conservative bass headroom protection from a real-audio kick classifier.
 * This maps neural kick probability to attenuation only; it never synthesizes sound.
 * External music sessions cannot use these unobserved PCM features.
 */
final class LibertyKickGuard {
    private LibertyKickGuard() { }
    static float attenuationDb(float confidence,float headroom,float intensity,boolean fresh,
                               boolean enabled,boolean inLibrary) {
        if(!enabled||!inLibrary||!fresh||!Float.isFinite(confidence)||
           !Float.isFinite(headroom)||!Float.isFinite(intensity))return 0f;
        if(confidence<.80f||headroom>=.35f||intensity<=0f)return 0f;
        float probability=Math.max(0f,Math.min(1f,(confidence-.80f)/.20f));
        float need=Math.max(0f,Math.min(1f,(.35f-headroom)/.35f));
        return -.6f*probability*need*Math.max(0f,Math.min(1f,intensity));
    }
    static void apply(float[] targets,float db,int count) {
        if(targets==null||targets.length<48||!Float.isFinite(db)||
           db>0||db<-.6001f||(count!=31&&count!=35&&count!=39))return;
        for(int band=0;band<count;band++) {
            double hz=20*Math.pow(1000,band/(double)(count-1));
            float low=(float)(1.0/(1.0+Math.pow(hz/170,4)));
            targets[9+band]=Math.max(-12f,Math.min(12f,targets[9+band]+db*low));
        }
    }
}
