// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import java.text.Normalizer;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;

/**
 * One query combines confirmed Audius catalog results and saved/local tracks.
 * Never infer third-party availability from a search URL.
 */
final class UnifiedMusicSearch {
    private UnifiedMusicSearch(){}

    static final class Result {
        final String title,artist;
        PublicMusic.Song audius;
        final ArrayList<AudioLibrary.Track> saved=new ArrayList<>();
        Result(String title,String artist){this.title=title;this.artist=artist;}
        boolean savedFrom(String source){
            for(AudioLibrary.Track track:saved){
                if("Audius".equals(source)&&LikedDownloads.isAudiusStream(track.uri))return true;
                if("Local".equals(source)&&"content".equals(track.uri.getScheme()))return true;
                if(LinkedMusic.isLinked(track.uri)&&source.equals(LinkedMusic.provider(track.uri)))return true;
            }
            return false;
        }
        AudioLibrary.Track from(String source){
            for(AudioLibrary.Track track:saved){
                if("Local".equals(source)&&"content".equals(track.uri.getScheme()))return track;
                if("Audius".equals(source)&&LikedDownloads.isAudiusStream(track.uri))return track;
                if(LinkedMusic.isLinked(track.uri)&&source.equals(LinkedMusic.provider(track.uri)))return track;
            }
            return null;
        }
        String availableSources(){
            ArrayList<String> sources=new ArrayList<>();
            if(audius!=null||savedFrom("Audius"))sources.add("Audius · found");
            if(savedFrom("Local"))sources.add("Phone · saved");
            if(savedFrom("YouTube Music"))sources.add("YouTube Music · saved link");
            if(savedFrom("SoundCloud"))sources.add("SoundCloud · saved link");
            return String.join("  •  ",sources);
        }
    }

    static List<Result> combine(String query,List<AudioLibrary.Track> existing,
                                List<PublicMusic.Song> remote){
        String term=normalize(query);
        if(term.isEmpty())return Collections.emptyList();
        LinkedHashMap<String,Result> combined=new LinkedHashMap<>();
        if(existing!=null)for(AudioLibrary.Track track:existing){
            if(track==null||!matches(term,track.title,track.artist))continue;
            String key=key(track.title,track.artist);
            Result item=combined.get(key);
            if(item==null){item=new Result(track.title,track.artist);combined.put(key,item);}
            item.saved.add(track);
        }
        if(remote!=null)for(PublicMusic.Song song:remote){
            if(song==null||!matches(term,song.title,song.artist))continue;
            String key=key(song.title,song.artist);
            Result item=combined.get(key);
            if(item==null){item=new Result(song.title,song.artist);combined.put(key,item);}
            if(item.audius==null)item.audius=song;
        }
        ArrayList<Result> results=new ArrayList<>(combined.values());
        results.sort(Comparator.<Result>comparingInt(
            value->normalize(value.title).equals(term)?0:
                   normalize(value.title).startsWith(term)?1:2)
            .thenComparing(value->value.title.toLowerCase(Locale.ROOT)));
        return results;
    }

    private static String key(String title,String artist){
        return normalize(title)+"\u0001"+normalize(artist);
    }

    private static boolean matches(String term,String title,String artist){
        String name=normalize(title),author=normalize(artist);
        return name.contains(term)||author.contains(term)||
            (name+" "+author).contains(term)||(author+" "+name).contains(term);
    }

    private static String normalize(String text){
        if(text==null)return "";
        return Normalizer.normalize(text,Normalizer.Form.NFKD)
            .replaceAll("\\p{M}+","")
            .toLowerCase(Locale.ROOT)
            .replaceAll("[^\\p{L}\\p{N}]+"," ")
            .trim().replaceAll(" +"," ");
    }
}
