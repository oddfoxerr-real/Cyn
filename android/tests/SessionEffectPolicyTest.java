// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

public final class SessionEffectPolicyTest {
    private static int checks;
    private static Set<String> apps(String... values) {
        Set<String> result = new HashSet<>();
        Collections.addAll(result, values);
        return result;
    }
    private static void expect(boolean wanted, boolean actual, String name) {
        checks++;
        if (wanted != actual) throw new AssertionError(name + ": expected " + wanted + ", found " + actual);
    }
    private static boolean allowed(boolean monitor, boolean enabled, boolean library,
                                   boolean legacy, int id, String pkg, Set<String> playing,
                                   int sessions) {
        return SessionEffectPolicy.allows(monitor, enabled, library, legacy,
            id, 77, pkg, playing, sessions);
    }
    public static void main(String[] args) {
        Set<String> one=apps("app.music");
        Set<String> two=apps("app.music","app.video");
        expect(true,allowed(true,true,false,false,21,"app.music",one,1),"selected app");
        expect(true,allowed(true,true,false,false,21,"app.music",two,3),"multiple sessions matched by package");
        expect(true,allowed(true,true,false,false,22,"app.video",two,3),"second app independently matched");
        expect(false,allowed(true,true,false,false,23,"app.alerts",two,3),"unselected app");
        expect(false,allowed(true,true,false,false,24,"app.music",apps("app.video"),2),"stopped music");
        expect(false,allowed(true,true,false,false,25,null,two,2),"ambiguous unlabelled session");
        expect(false,allowed(true,true,false,false,25,null,one,2),"unlabelled with multiple sessions");
        expect(true,allowed(true,true,false,false,25,null,one,1),"single unlabelled legacy player");
        expect(false,allowed(false,true,false,false,21,"app.music",one,1),"not monitoring");
        expect(false,allowed(true,false,false,false,21,"app.music",one,1),"enhancement off");
        expect(false,allowed(true,true,true,false,21,"app.music",one,1),"library playing");
        expect(false,allowed(true,true,false,false,0,"app.music",one,1),"system mix not selected");
        expect(false,allowed(true,true,false,false,77,"app.music",one,1),"no library feedback");
        expect(false,allowed(true,true,false,false,-1,"app.music",one,1),"invalid ID");
        expect(false,allowed(true,true,false,false,21,"app.music",Collections.emptySet(),1),"no verified media");
        expect(true,allowed(true,true,false,true,0,null,one,0),"explicit legacy mix");
        expect(false,allowed(true,true,false,true,21,"app.music",one,1),"legacy not per session");
        expect(false,allowed(true,true,false,true,0,null,Collections.emptySet(),0),"legacy without playback");
        System.out.println("PASS: " + checks + " CYN Android audio session routing assertions");
    }
}
