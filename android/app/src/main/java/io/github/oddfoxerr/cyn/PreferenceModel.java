// SPDX-License-Identifier: AGPL-3.0-or-later
// Java adaptation of Cyn v2.0.16 PersonalAudioModel (11 -> 12 -> 5).
package io.github.oddfoxerr.cyn;

import java.util.*;
import java.util.function.IntConsumer;

public final class PreferenceModel {
    public final double[] weights = new double[209];
    public final boolean[] ready = new boolean[5];
    public static final class Example {
        public final double[] x; public final int effect; public final boolean more; public final long id;
        public Example(double[] x, int effect, boolean more, long id) {
            this.x=x.clone(); this.effect=effect; this.more=more; this.id=id;
        }
    }
    public PreferenceModel() { Random r=new Random(717); for(int i=0;i<209;i++) weights[i]=(r.nextDouble()-.5)*.3; }
    public PreferenceModel copy() { PreferenceModel m=new PreferenceModel(); System.arraycopy(weights,0,m.weights,0,209); System.arraycopy(ready,0,m.ready,0,5); return m; }
    public static double clamp(double v, double lo, double hi) { return Math.max(lo,Math.min(hi,Double.isFinite(v)?v:0)); }
    public static double[] normalize(double[] a) {
        double[] x=new double[11];
        for(int i=0;i<11;i++) x[i]=i<8?(clamp(a[i],-12,0)+6)/6:i==8?clamp(a[i],0,4)/2-1:i==9?clamp(a[i],0,2)-1:(clamp(a[i],-8,0)+4)/4;
        return x;
    }
    public static final class Trace {
        public final double[] inputs, hidden=new double[12], outputs=new double[5];
        public final double[] inputConnections=new double[132], outputConnections=new double[60];
        public final boolean[] ready;
        Trace(double[] a,boolean[] ready) { inputs=normalize(a); this.ready=ready.clone(); }
    }
    public Trace trace(double[] a) {
        Trace t=new Trace(a,ready);
        for(int j=0;j<12;j++) {
            double z=weights[132+j];
            for(int i=0;i<11;i++) { double contribution=t.inputs[i]*weights[j*11+i]; t.inputConnections[j*11+i]=contribution; z+=contribution; }
            t.hidden[j]=Math.tanh(z);
        }
        for(int o=0;o<5;o++) {
            double z=weights[204+o];
            for(int j=0;j<12;j++) { double contribution=t.hidden[j]*weights[144+o*12+j]; t.outputConnections[o*12+j]=contribution; z+=contribution; }
            t.outputs[o]=sigmoid(z);
        }
        return t;
    }
    public double[] hidden(double[] a) {
        double[] x=normalize(a), h=new double[12];
        for(int j=0;j<12;j++) { double z=weights[132+j]; for(int i=0;i<11;i++) z+=x[i]*weights[j*11+i]; h[j]=Math.tanh(z); } return h;
    }
    public double[] predict(double[] a) {
        double[] h=hidden(a), out=new double[5];
        for(int o=0;o<5;o++) { double z=weights[204+o]; for(int j=0;j<12;j++) z+=h[j]*weights[144+o*12+j]; out[o]=sigmoid(z); } return out;
    }
    private static double sigmoid(double z) { return 1/(1+Math.exp(-clamp(z,-30,30))); }
    public boolean fit(List<Example> examples, IntConsumer progress) { return fit(examples,progress,(trace,epoch)->{}); }
    public boolean fit(List<Example> examples, IntConsumer progress, java.util.function.BiConsumer<Trace,Integer> telemetry) {
        int[][] counts=new int[5][2];
        for(Example e:examples) if(e.effect>=0&&e.effect<5) counts[e.effect][e.more?1:0]++;
        boolean[] train=new boolean[5]; boolean any=false;
        for(int o=0;o<5;o++) { train[o]=counts[o][0]>=5&&counts[o][1]>=5; any|=train[o]; }
        if(!any) return false;
        double[] momentum=new double[209], variance=new double[209]; double b1=1,b2=1;
        for(int epoch=1;epoch<=240;epoch++) {
            double[] grad=new double[209];
            for(Example e:examples) {
                int o=e.effect; if(o<0||o>=5||!train[o]) continue;
                double[] x=normalize(e.x), h=hidden(e.x);
                double z=weights[204+o]; for(int j=0;j<12;j++) z+=h[j]*weights[144+o*12+j];
                double d=(sigmoid(z)-(e.more?1:0))/counts[o][e.more?1:0]; grad[204+o]+=d;
                for(int j=0;j<12;j++) { grad[144+o*12+j]+=d*h[j]; double dh=d*weights[144+o*12+j]*(1-h[j]*h[j]); grad[132+j]+=dh; for(int i=0;i<11;i++) grad[j*11+i]+=dh*x[i]; }
            }
            b1*=.9; b2*=.999;
            for(int i=0;i<209;i++) {
                double g=grad[i]+.0001*weights[i]; momentum[i]=.9*momentum[i]+.1*g; variance[i]=.999*variance[i]+.001*g*g;
                weights[i]-=.012*(momentum[i]/(1-b1))/(Math.sqrt(variance[i]/(1-b2))+1e-7);
                if(!Double.isFinite(weights[i])) return false;
            }
            progress.accept(epoch);
            if(epoch==1||epoch%12==0) for(Example e:examples) if(e.effect>=0&&e.effect<5&&train[e.effect]) {
                Trace t=trace(e.x); for(int o=0;o<5;o++) t.ready[o]=train[o]; telemetry.accept(t,epoch); break;
            }
        }
        for(int o=0;o<5;o++) ready[o]|=train[o];
        return true;
    }
}
