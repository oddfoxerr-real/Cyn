// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

public final class ConnectedEffectsTest {
    private static void check(boolean value,String message) {
        if(!value)throw new AssertionError(message);
    }
    private static void near(float actual,float expected,String message) {
        check(Math.abs(actual-expected)<.001,message+": "+actual);
    }
    public static void main(String[] args) {
        for(int count:new int[]{31,35,39}) {
            float[] targets=new float[48];targets[8]=count;
            near(ConnectedEffects.band(targets,1000),0,"Neutral Studio stays neutral");
            targets[9]=-4;targets[9+count-1]=3;
            near(ConnectedEffects.band(targets,20),-4,"Low endpoint reaches connected EQ");
            near(ConnectedEffects.band(targets,20000),3,"High endpoint reaches connected EQ");
            int middle=(count-1)/2;
            targets[9+middle]=2;
            float hz=(float)(20*Math.pow(1000,middle/(double)(count-1)));
            near(ConnectedEffects.band(targets,hz),2,"Selected Studio band reaches connected EQ");
            targets[1]=12;targets[2]=8;
            for(int f=20;f<=20000;f+=31)check(Math.abs(ConnectedEffects.band(targets,f))<=6,"Platform gain cap");
            targets[1]=Float.NaN;
            check(Float.isFinite(ConnectedEffects.band(targets,80)),"Invalid model input cannot reach Android");
        }
        near(ConnectedEffects.width(.2f,.6f,1,true,true,true,.7f),.6f,"Fresh neural policy changes connected width");
        near(ConnectedEffects.width(0,.7f,1,true,true,false,.7f),0,"Stale policy does not widen audio");
        near(ConnectedEffects.width(.2f,.7f,1,false,true,true,.7f),.2f,"Other profiles restore manual width");
        near(ConnectedEffects.width(0,.7f,1,true,true,true,0),0,"Zero sensitivity bypasses neural width");
        near(ConnectedEffects.width(0,1,1,true,true,true,1),.7f,"Neural width stays capped");
        near(ConnectedEffects.width(.2f,0,1,true,true,false,1),.2f,"Bypass preserves manual width");
        float[] neutral=new float[48];neutral[8]=31;
        float[] policy={.4f,.3f,.2f,.5f,.1f,.6f};
        float[] stale=neutral.clone();ConnectedEffects.audonic(stale,policy,1,false);
        check(java.util.Arrays.equals(stale,neutral),"Stale AUDONIC is exact bypass");
        float[] zero=neutral.clone();ConnectedEffects.audonic(zero,policy,0,true);
        check(java.util.Arrays.equals(zero,neutral),"Zero intensity is exact bypass");
        ConnectedEffects.audonic(neutral,policy,1,true);
        check(ConnectedEffects.band(neutral,80)>0,"AUDONIC reaches connected bass EQ");
        check(neutral[3]>0,"AUDONIC reaches connected width");
        for(float v:neutral)check(Float.isFinite(v),"Finite creative targets");
        float[] invalid={Float.NaN,Float.POSITIVE_INFINITY,Float.NaN,Float.NaN,0,Float.NaN};
        ConnectedEffects.audonic(neutral,invalid,1,true);
        for(float v:neutral)check(Float.isFinite(v),"Invalid creative controls rejected");
        float[] liberty=new float[48];liberty[8]=31;
        float[] zeroIntensity=liberty.clone();
        ConnectedEffects.libertySession(zeroIntensity,.3f,.6f,.6f,0);
        check(java.util.Arrays.equals(zeroIntensity,liberty),"LIBERTY zero intensity preserves original EQ");
        float[] silent=liberty.clone();
        ConnectedEffects.libertySession(silent,0,.6f,.6f,1);
        check(java.util.Arrays.equals(silent,liberty),"LIBERTY does not react to silence");
        float[] nonFinite=liberty.clone();
        ConnectedEffects.libertySession(nonFinite,.3f,Float.NaN,.6f,1);
        check(java.util.Arrays.equals(nonFinite,liberty),"LIBERTY rejects invalid FFT offsets");
        ConnectedEffects.libertySession(liberty,.3f,.6f,.4f,1);
        check(liberty[9]>0&&liberty[9+20]>0,"LIBERTY maps measured audio only to EQ");
        for(float gain:liberty)check(Float.isFinite(gain),"LIBERTY never creates invalid EQ targets");
        System.out.println("PASS Connect EQ interpolation, model effect mapping, stale policy and off transitions");
    }
}
