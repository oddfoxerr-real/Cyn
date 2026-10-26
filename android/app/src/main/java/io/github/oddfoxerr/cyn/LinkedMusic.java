// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import java.util.List;
import java.util.Locale;

/**
 * Provider links are bookmarks, not direct audio streams or OAuth integrations.
 * Never hand these URIs to the CYN audio player or download engine.
 */
final class LinkedMusic {
    private LinkedMusic(){}

    static Uri parse(String input){
        if(input==null)return null;
        String value=input.trim();
        if(value.isEmpty()||value.length()>2048)return null;
        try{
            Uri uri=Uri.parse(value);
            if(!"https".equalsIgnoreCase(uri.getScheme()))return null;
            if(uri.getPort()!=-1&&uri.getPort()!=443)return null;
            String authority=uri.getEncodedAuthority();
            if(authority==null||authority.contains("@"))return null;
            String host=uri.getHost();
            if(host==null)return null;
            host=host.toLowerCase(Locale.ROOT);
            List<String> segments=uri.getPathSegments();
            if(host.equals("music.youtube.com")||host.equals("youtube.com")||
               host.equals("www.youtube.com")){
                if("/search".equals(uri.getPath())&&uri.getQueryParameter("q")!=null)return uri;
                if("/watch".equals(uri.getPath())&&validYouTubeId(uri.getQueryParameter("v")))return uri;
                if("/playlist".equals(uri.getPath())&&validPlaylist(uri.getQueryParameter("list")))return uri;
                if(segments.size()==2&&segments.get(0).equals("shorts")&&validYouTubeId(segments.get(1)))return uri;
                return null;
            }
            if(host.equals("youtu.be")){
                return segments.size()==1&&validYouTubeId(segments.get(0))?uri:null;
            }
            if(host.equals("soundcloud.com")||host.equals("www.soundcloud.com")||
               host.equals("m.soundcloud.com")){
                if(segments.size()>=2&&!segments.get(0).isEmpty())return uri;
                return null;
            }
            if(host.equals("on.soundcloud.com")){
                return segments.size()==1&&!segments.get(0).isEmpty()?uri:null;
            }
        }catch(RuntimeException ignored){}
        return null;
    }

    static boolean isLinked(Uri uri){return uri!=null&&parse(uri.toString())!=null;}

    static String provider(Uri uri){
        if(!isLinked(uri))return "";
        return uri.getHost().toLowerCase(Locale.ROOT).contains("soundcloud")
            ?"SoundCloud":"YouTube Music";
    }

    static Uri searchFromPlayback(ExternalNowPlaying.Track track){
        if(track==null||track.controller==null)return null;
        String source=track.controller.getPackageName();
        String query=(track.title+" "+track.artist).trim();
        if(query.isEmpty()||query.length()>400)return null;
        if("com.google.android.apps.youtube.music".equals(source))
            return Uri.parse("https://music.youtube.com/search?q="+Uri.encode(query));
        if("com.soundcloud.android".equals(source))
            return Uri.parse("https://soundcloud.com/search/sounds?q="+Uri.encode(query));
        return null;
    }

    static boolean isSearchShortcut(Uri uri){
        if(uri==null||!"https".equals(uri.getScheme()))return false;
        String host=uri.getHost();
        String path=uri.getPath();
        return ("music.youtube.com".equals(host)&&"/search".equals(path)||
               "soundcloud.com".equals(host)&&"/search/sounds".equals(path))&&
               uri.getQueryParameter("q")!=null;
    }

    private static boolean validYouTubeId(String id){
        return id!=null&&id.matches("[A-Za-z0-9_-]{11}");
    }
    private static boolean validPlaylist(String id){
        return id!=null&&id.matches("[A-Za-z0-9_-]{10,100}");
    }
}
