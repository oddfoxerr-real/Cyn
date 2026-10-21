// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/** Maps the shared Studio targets onto the bands an Android session actually exposes. */
final class ConnectedEffects {
    private ConnectedEffects() { }
    static float clamp(float value, float low, float high) {
        return Float.isFinite(value) ? Math.max(low, Math.min(high, value)) : 0;
    }
    static float band(float[] targets, float hz) {
        int count = Math.round(targets[8]);
        if (count != 31 && count != 35 && count != 39) return 0;
        float frequency = clamp(hz, 20, 20000);
        double position = Math.log(frequency / 20.0) / Math.log(1000) * (count - 1);
        int low = Math.min(count - 1, (int) position), high = Math.min(count - 1, low + 1);
        float curve = targets[9 + low] + (targets[9 + high] - targets[9 + low]) * (float) (position - low);
        double octaves = Math.log(frequency / 2200.0) / Math.log(2) / .8;
        double bass = targets[1] / (1 + Math.pow(frequency / 180.0, 2));
        double clarity = targets[2] * Math.exp(-.5 * octaves * octaves);
        return clamp((float) (curve + bass + clarity), -6, 6);
    }
    /** Creative policy mapped only to effects Android sessions can render. */
    static void audonic(float[] targets,float[] policy,float intensity,boolean fresh){
        if(!fresh||policy==null||policy.length!=6)return;
        float strength=clamp(intensity,0,1);
        // No duplicate playback: restrained tonal motion uses the original session EQ.
        targets[1]=clamp(targets[1]+clamp(policy[5],0,1)*strength*1.5f,0,12);
        targets[2]=clamp(targets[2]+(clamp(policy[0],0,1)+clamp(policy[1],0,1))*strength*.75f,0,8);
        targets[3]=clamp(targets[3]+clamp(policy[2],0,1)*strength*.12f,0,1);
        int count=Math.round(targets[8]);
        if(count!=31&&count!=35&&count!=39)return;
        for(int b=0;b<count;b++){
            double hz=20*Math.pow(1000,b/(double)(count-1));
            double octaves=Math.log(hz/800)/Math.log(2);
            float cut=(float)Math.exp(-.5*octaves*octaves)*clamp(policy[3],0,1)*strength;
            targets[9+b]=clamp(targets[9+b]-cut,-12,12);
        }
    }
    static float width(float manual, float neuralGain, float intensity, boolean selected,
                       boolean neuralEnabled, boolean fresh, float sensitivity) {
        // The neural controller may reduce the requested Android virtualization
        // amount. It cannot increase the user's selected spatial maximum.
        float extra = selected && sensitivity > 0
            ? clamp(intensity, 0, 1) * (neuralEnabled ? fresh ? clamp(neuralGain, 0, .7f) : 0 : 1) : 0;
        return clamp(Math.max(manual, extra), 0, 1);
    }
}
