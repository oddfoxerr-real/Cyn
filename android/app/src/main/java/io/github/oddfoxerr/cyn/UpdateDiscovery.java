// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Public release feed first; API is optional. URLs are constructed from validated versions. */
final class UpdateDiscovery {
    static final class Release {
        final String version,digest;
        final long size;
        Release(String version,String digest,long size){this.version=version;this.digest=digest;this.size=size;}
    }
    interface Feed { String read() throws Exception; }
    interface Api { Release read() throws Exception; }
    private static final Pattern LINK=Pattern.compile(
        "<link\\b[^>]*\\bhref=[\"']https://github\\.com/oddfoxerr-real/Cyn/releases/tag/(android-studio-v[0-9.]+-preview)[\"'][^>]*>");
    static Release parse(String feed,String installed)throws IOException{
        if(feed==null||!feed.contains("<feed")||!feed.contains("</feed>"))
            throw new IOException("Invalid release feed");
        Matcher links=LINK.matcher(feed);
        String latest=null;
        boolean channel=false;
        while(links.find()){
            String tag=links.group(1),version=tag.substring("android-studio-v".length());
            if(AndroidRelease.number(version)<0)continue;
            channel=true;
            if(AndroidRelease.newerVersion(tag,installed)!=null&&
                    (latest==null||AndroidRelease.number(version)>AndroidRelease.number(latest)))latest=version;
        }
        // A feed containing only PC/model releases cannot prove Android is current.
        if(!channel)throw new IOException("Android releases absent from recent feed");
        return latest==null?null:new Release(latest,"",-1);
    }
    static Release find(String installed,Feed feed,Api api)throws Exception{
        try{return parse(feed.read(),installed);}
        catch(Exception unavailable){
            try{return api.read();}
            catch(Exception failure){failure.addSuppressed(unavailable);throw failure;}
        }
    }
}
