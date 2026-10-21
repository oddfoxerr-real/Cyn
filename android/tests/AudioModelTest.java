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
        final int[] epoch={0}, sampleEpoch={0}, samples={0};
        check(m.fit(labels,e->epoch[0]=e,(t,e)->{
            check(e>sampleEpoch[0],"Training snapshots advance with completed epochs"); sampleEpoch[0]=e; samples[0]++;
            check(Arrays.equals(t.outputs,m.trace(labels.get(0).x).outputs),"Training telemetry uses the candidate weights");
        }),"Balanced data trains"); check(epoch[0]==240&&sampleEpoch[0]==240&&samples[0]==21,"Real epochs sampled, including final weights");
        PreferenceModel.Trace trace=m.trace(feature(true));
        check(Arrays.equals(trace.outputs,m.predict(feature(true))),"Telemetry matches inference outputs");
        for(int j=0;j<12;j++) {
            double sum=m.weights[132+j];
            for(int i=0;i<11;i++) { check(trace.inputConnections[j*11+i]==trace.inputs[i]*m.weights[j*11+i],"Actual weighted input contribution"); sum+=trace.inputConnections[j*11+i]; }
            check(Math.abs(Math.tanh(sum)-trace.hidden[j])<1e-12,"Hidden activation includes bias");
        }
        for(int o=0;o<5;o++) for(int j=0;j<12;j++) check(trace.outputConnections[o*12+j]==trace.hidden[j]*m.weights[144+o*12+j],"Actual weighted output contribution");
        check(AndroidRelease.newerVersion("v2.0.17","0.1.0-preview")==null,"Windows releases excluded");
        check(AndroidRelease.newerVersion("android-studio-v0.1.0-preview","0.1.1-preview")==null,"Older APK rejected");
        check(AndroidRelease.newerVersion("android-studio-v0.1.1-preview","0.1.1-preview")==null,"Same APK rejected");
        check("0.1.10-preview".equals(AndroidRelease.newerVersion("android-studio-v0.1.10-preview","0.1.9-preview")),"Numeric update ordering");
        check(AndroidRelease.newerVersion("android-studio-vinvalid","0.1.0-preview")==null,"Malformed version rejected");
        check(AndroidRelease.assetUrl("0.2.1-preview").endsWith("/android-studio-v0.2.1-preview/Cyn-Studio-Android-0.2.1-preview.apk"),"Studio release link matches version");
        check(AndroidRelease.newerVersion("android-v99.0.0-preview","0.2.0-preview")==null,"Legacy Android channel excluded even with higher version");
        check(AndroidRelease.newerVersion("android-studio-v0.2.1-preview","invalid")==null,"Invalid installed version rejected");
        check(AndroidRelease.newerVersion(null,"0.2.0-preview")==null,"Missing release tag rejected");
        check(m.ready[4]&&!m.ready[0],"Only trained head ready");
        check(m.predict(feature(true))[4]>.85,"Learns More bass examples");
        check(m.predict(feature(false))[4]<.15,"Learns Less bass examples");
        PreferenceModel copy=m.copy(); copy.weights[0]+=1; check(copy.weights[0]!=m.weights[0],"Training copy isolated from live inference");
        PreferenceModel insufficient=new PreferenceModel(); check(!insufficient.fit(labels.subList(0,6),e->{}),"Insufficient labels rejected");
        System.out.println("PASS: "+assertions+" audio feature and learning assertions");
    }
}
