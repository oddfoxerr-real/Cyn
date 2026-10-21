// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.util.*;

public final class ConnectedMindTraceTest {
    private static void check(boolean condition,String reason){
        if(!condition)throw new AssertionError(reason);
    }
    public static void main(String[] args){
        PreferenceModel model=new PreferenceModel();
        // Audio comes from Android's existing permission-gated Visualizer
        // and is passed through the same actual personal model as the UI.
        byte[] bassFft=new byte[1024],midFft=new byte[1024];
        bassFft[2]=99;bassFft[4]=65;
        midFft[180]=99;midFft[302]=65;
        double[] bass=AudioFeatures.fromFft(bassFft,48000,.24,.71);
        double[] mid=AudioFeatures.fromFft(midFft,48000,.11,.60);
        PreferenceModel.Trace one=model.trace(bass),two=model.trace(mid);
        check(one.hidden.length==12&&one.outputConnections.length==60&&one.outputs.length==5,
            "Wrong shape for the CYN 3D connected neural map");
        boolean changed=false;
        for(int i=0;i<12;i++){
            check(Double.isFinite(one.hidden[i])&&Math.abs(one.hidden[i])<=1,
                "Non-finite or out-of-bounds hidden neuron");
            if(Math.abs(one.hidden[i]-two.hidden[i])>1e-7)changed=true;
        }
        check(changed,"Different audio must produce different measured AI activity");
        for(int o=0;o<5;o++) for(int n=0;n<12;n++){
            double contribution=one.hidden[n]*model.weights[144+o*12+n];
            check(one.outputConnections[o*12+n]==contribution,
                "Rendered neural pathway must equal model's actual weighted contribution");
        }
        check(AudioFeatures.levels(new byte[128])[0]>0,
            "Visualizer wave measurements must remain available");
        System.out.println("PASS connected CYN brain map: 12 real hidden activations, 60 weighted paths, audio sensitivity");
    }
}
