// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.util.*;
public final class AudioModelTest {
    static int assertions;
    static void check(boolean ok,String message) { assertions++; if(!ok) throw new AssertionError(message); }
    static double[] feature(boolean bass) { return new double[]{bass?-.1:-10,bass?-.5:-8,-3,-2,-3,-3,-4,-5,1,0,-2}; }
    public static void main(String[] args) {
        PreferenceModel m=new PreferenceModel();
        double[] silence=AudioFeatures.fromFft(new byte[1024],48000,0,0);
        for(double x:silence) check(Double.isFinite(x),"Silence must remain finite");
        for(double p:m.predict(silence)) check(p>=0&&p<=1,"Probabilities bounded");
        double[] hostile=new double[11]; Arrays.fill(hostile,Double.NaN);
        for(double p:m.predict(hostile)) check(Double.isFinite(p),"Invalid samples sanitized");
        byte[] low=new byte[1024]; low[2]=100; byte[] high=new byte[1024]; high[400]=100;
        check(AudioFeatures.fromFft(low,48000,.2,.6)[0]>AudioFeatures.fromFft(high,48000,.2,.6)[0],"Bass band distinguishes low and high tones");
        byte[] wave=new byte[100]; Arrays.fill(wave,(byte)128); check(AudioFeatures.levels(wave)[0]==0,"Unsigned waveform silence");
        check(AudioFeatures.offset(1,0)==0,"Sensitivity zero has no adjustment");
        check(AudioFeatures.offset(1,1)==3&&AudioFeatures.offset(0,1)==-3,"Signed bounded changes");
        List<PreferenceModel.Example> labels=new ArrayList<>();
        for(int i=0;i<8;i++) { labels.add(new PreferenceModel.Example(feature(true),4,true,i)); labels.add(new PreferenceModel.Example(feature(false),4,false,i+20)); }
        final int[] epoch={0}; check(m.fit(labels,e->epoch[0]=e),"Balanced data trains"); check(epoch[0]==240,"Real 240 epochs completed");
        check(m.ready[4]&&!m.ready[0],"Only trained head ready");
        check(m.predict(feature(true))[4]>.85,"Learns More bass examples");
        check(m.predict(feature(false))[4]<.15,"Learns Less bass examples");
        PreferenceModel copy=m.copy(); copy.weights[0]+=1; check(copy.weights[0]!=m.weights[0],"Training copy isolated from live inference");
        PreferenceModel insufficient=new PreferenceModel(); check(!insufficient.fit(labels.subList(0,6),e->{}),"Insufficient labels rejected");
        System.out.println("PASS: "+assertions+" audio feature and learning assertions");
    }
}
