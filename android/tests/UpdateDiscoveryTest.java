// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
public final class UpdateDiscoveryTest {
    static void check(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    static String link(String tag){return "<link rel=\"alternate\" href=\"https://github.com/oddfoxerr-real/Cyn/releases/tag/"+tag+"\"/>";}
    public static void main(String[] args)throws Exception{
        String feed="<feed>"+link("v2.0.999")+link("android-studio-v0.2.9-preview")+
            link("android-studio-v0.2.25-preview")+link("android-studio-v0.2.24-preview")+"</feed>";
        UpdateDiscovery.Release found=UpdateDiscovery.find("0.2.23-preview",()->feed,()->{throw new IOException("HTTP 403 rate limit reached");});
        check(found.version.equals("0.2.25-preview"),"Feed bypasses blocked API and compares numerically");
        check(found.size==-1&&found.digest.isEmpty(),"Feed requires release checksum and bounded download");
        check(UpdateDiscovery.parse(feed,"0.2.25-preview")==null,"Current version is not offered");
        check(UpdateDiscovery.parse(feed,"0.3.0-preview")==null,"Never downgrade");
        UpdateDiscovery.Release fallback=new UpdateDiscovery.Release("0.2.26-preview","",123);
        check(UpdateDiscovery.find("0.2.23-preview",()->"<feed>"+link("v2.0.999")+"</feed>",()->fallback)==fallback,"PC-only feed uses paginated API");
        check(UpdateDiscovery.find("0.2.23-preview",()->"bad response",()->fallback)==fallback,"Malformed feed uses API");
        try{UpdateDiscovery.find("0.2.23-preview",()->"bad response",()->{throw new IOException("offline");});throw new AssertionError("Failure reported as current");}
        catch(IOException expected){check(expected.getSuppressed().length==1,"Preserve feed failure");}
        String foreign="<feed>"+link("android-studio-v0.2.99-preview").replace("oddfoxerr-real/Cyn","other/Cyn")+"</feed>";
        try{UpdateDiscovery.parse(foreign,"0.2.23-preview");throw new AssertionError("Foreign release accepted");}catch(IOException expected){}
        if(args.length>0){
            String live=Files.readString(Path.of(args[0]));
            check(UpdateDiscovery.parse(live,"0.2.23-preview")!=null,"Published live feed finds upgrade from screenshot version");
        }
        System.out.println("PASS: API-rate-limit bypass, live release feed, version ordering, channel isolation, fallback and failure states");
    }
}
