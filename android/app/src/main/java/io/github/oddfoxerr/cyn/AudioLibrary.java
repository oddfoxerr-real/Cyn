// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.*;
import android.database.Cursor;
import android.media.MediaMetadataRetriever;
import android.net.Uri;
import android.provider.*;
import android.util.AtomicFile;
import org.json.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

final class AudioLibrary {
    static final class Track {
        final Uri uri;final String title,artist,album;final long duration;boolean favorite;
        Uri offlineUri;
        Track(Uri u,String t,String a,String b,long d,boolean f){uri=u;title=t;artist=a;album=b;duration=d;favorite=f;}
        Uri playbackUri(){return offlineUri==null?uri:offlineUri;}
        JSONObject json()throws JSONException{
            JSONObject result=new JSONObject().put("uri",uri.toString()).put("title",title)
                .put("artist",artist).put("album",album).put("duration",duration).put("favorite",favorite);
            if(offlineUri!=null)result.put("offline_uri",offlineUri.toString());
            return result;
        }
    }
    private final Context context;
    private final ArrayList<Track> tracks=new ArrayList<>();
    AudioLibrary(Context c){
        context=c.getApplicationContext();
        try(InputStream in=new AtomicFile(new File(context.getFilesDir(),"library.json")).openRead()){
            JSONArray all=new JSONArray(Streams.text(in));
            for(int i=0;i<all.length();i++){
                JSONObject entry=all.getJSONObject(i);
                Uri uri=Uri.parse(entry.getString("uri"));
                if(!"content".equals(uri.getScheme())&&!LikedDownloads.isAudiusStream(uri))continue;
                Track track=new Track(uri,entry.getString("title"),entry.optString("artist","Unknown artist"),
                    entry.optString("album",""),entry.optLong("duration"),entry.optBoolean("favorite"));
                String offline=entry.optString("offline_uri","");
                if(!offline.isEmpty()){
                    Uri downloaded=Uri.parse(offline);
                    if("content".equals(downloaded.getScheme())||
                        ("file".equals(downloaded.getScheme())&&downloaded.getPath()!=null&&
                        downloaded.getPath().startsWith(new File(context.getFilesDir(),"offline-music").getAbsolutePath()+File.separator)))
                        track.offlineUri=downloaded;
                }
                tracks.add(track);
            }
        }catch(Exception ignored){}
    }
    synchronized List<Track> all(){return new ArrayList<>(tracks);}
    synchronized void favorite(Uri uri)throws Exception{
        for(Track t:tracks)if(t.uri.equals(uri)){
            boolean previous=t.favorite;
            t.favorite=!previous;
            try{save();}catch(Exception e){t.favorite=previous;throw e;}
            return;
        }
        throw new FileNotFoundException("Track no longer exists in the library");
    }
    synchronized void remove(Uri uri)throws Exception{
        ArrayList<Track> previous=new ArrayList<>(tracks);
        tracks.removeIf(t->t.uri.equals(uri));
        try{save();}catch(Exception e){tracks.clear();tracks.addAll(previous);throw e;}
    }
    synchronized void clearFavorites()throws Exception{
        ArrayList<Track> favorites=new ArrayList<>();
        for(Track t:tracks)if(t.favorite){favorites.add(t);t.favorite=false;}
        try{save();}catch(Exception e){for(Track t:favorites)t.favorite=true;throw e;}
    }
    synchronized void downloaded(Uri source,Uri local)throws Exception{
        if(local==null||(!"content".equals(local.getScheme())&&!"file".equals(local.getScheme())))
            throw new IOException("Invalid downloaded track URI");
        for(Track t:tracks)if(t.uri.equals(source)&&LikedDownloads.isAudiusStream(source)){
            Uri previous=t.offlineUri;
            t.offlineUri=local;
            try{save();}catch(Exception e){t.offlineUri=previous;throw e;}
            return;
        }
        throw new FileNotFoundException("Song has been removed from CYN Library");
    }
    synchronized void merge(Collection<Track> incoming)throws Exception{
        LinkedHashMap<String,Track> map=new LinkedHashMap<>();
        HashSet<String> downloadedUris=new HashSet<>();
        for(Track t:tracks){map.put(t.uri.toString(),t);if(t.offlineUri!=null)downloadedUris.add(t.offlineUri.toString());}
        for(Track t:incoming){
            if(downloadedUris.contains(t.uri.toString())&&!map.containsKey(t.uri.toString()))continue;
            Track old=map.get(t.uri.toString());
            if(old!=null){t.favorite=old.favorite;t.offlineUri=old.offlineUri;}
            map.put(t.uri.toString(),t);
        }
        ArrayList<Track> previous=new ArrayList<>(tracks);
        tracks.clear();tracks.addAll(map.values());
        tracks.sort(Comparator.comparing(t->t.title.toLowerCase(Locale.ROOT)));
        try{save();}catch(Exception e){tracks.clear();tracks.addAll(previous);throw e;}
    }
    private void save()throws Exception{JSONArray a=new JSONArray();for(Track t:tracks)a.put(t.json());AtomicFile file=new AtomicFile(new File(context.getFilesDir(),"library.json"));FileOutputStream out=null;try{out=file.startWrite();out.write(a.toString().getBytes(StandardCharsets.UTF_8));file.finishWrite(out);}catch(Exception e){if(out!=null)file.failWrite(out);throw e;}}
    List<Track> scan(){ArrayList<Track> result=new ArrayList<>();String[] columns={MediaStore.Audio.Media._ID,MediaStore.Audio.Media.TITLE,MediaStore.Audio.Media.ARTIST,MediaStore.Audio.Media.ALBUM,MediaStore.Audio.Media.DURATION};try(Cursor c=context.getContentResolver().query(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,columns,MediaStore.Audio.Media.IS_MUSIC+" != 0",null,MediaStore.Audio.Media.TITLE+" COLLATE NOCASE")){if(c!=null)while(c.moveToNext()){Uri uri=ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,c.getLong(0));result.add(new Track(uri,clean(c.getString(1),"Audio track"),clean(c.getString(2),"Unknown artist"),clean(c.getString(3),""),c.getLong(4),false));}}return result;}
    Track read(Uri uri){String name="Audio track",artist="Unknown artist",album="";long duration=0;try(Cursor c=context.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst())name=clean(c.getString(0),name);}catch(Exception ignored){}
        MediaMetadataRetriever reader=new MediaMetadataRetriever();try{reader.setDataSource(context,uri);name=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE),name);artist=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST),artist);album=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM),"");String d=reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);if(d!=null)duration=Long.parseLong(d);}catch(Exception ignored){}finally{try{reader.release();}catch(Exception ignored){}}return new Track(uri,name,artist,album,duration,false);}
    private static String clean(String value,String fallback){return value==null||value.trim().isEmpty()||"<unknown>".equals(value)?fallback:value;}
}
