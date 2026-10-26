// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.util.Arrays;
public final class LibertyKickGuardTest {
    static void check(boolean ok,String what){if(!ok)throw new AssertionError(what);}
    public static void main(String[] unused){
        float[] clean=new float[48];clean[8]=31;
        for(boolean on:new boolean[]{true,false}){
            float db=LibertyKickGuard.attenuationDb(.95f,.05f,1,true,on,true);
            check(db<=0&&db>=-.6,"negative-only");
            if(!on)check(db==0,"disabled always neutral");
        }
        check(LibertyKickGuard.attenuationDb(1,.01f,1,true,true,false)==0,"external audio cannot get kick guard");
        check(LibertyKickGuard.attenuationDb(1,.01f,1,false,true,true)==0,"stale detector safe bypass");
        check(LibertyKickGuard.attenuationDb(Float.NaN,0,1,true,true,true)==0,"invalid classifier safe bypass");
        check(LibertyKickGuard.attenuationDb(.95f,.6f,1,true,true,true)==0,"headroom sufficient");
        check(LibertyKickGuard.attenuationDb(.95f,.05f,0,true,true,true)==0,"zero intensity bypass");
        check(LibertyKickGuard.attenuationDb(.65f,0,1,true,true,true)==0,"low confidence bypass");
        float attenuation=LibertyKickGuard.attenuationDb(.99f,.01f,1,true,true,true);
        for(int count:new int[]{31,35,39}){
            float[] p=new float[48];p[8]=count;Arrays.fill(p,9,9+count,3f);
            LibertyKickGuard.apply(p,attenuation,count);
            check(p[9]<3f&&p[9]>=2.3999f,"bass bounded attenuation");
            check(Math.abs(p[9+count-1]-3f)<.001f,"treble unchanged");
            for(int n=9;n<9+count;n++)check(Float.isFinite(p[n])&&p[n]<=3f,"no boost or invalid gain");
        }
        System.out.println("PASS: LIBERTY learned-kick guard neutral/off/freshness/headroom, non-generative only");
    }
}
