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
        Track(Uri u,String t,String a,String b,long d,boolean f){uri=u;title=t;artist=a;album=b;duration=d;favorite=f;}
        JSONObject json()throws JSONException{return new JSONObject().put("uri",uri.toString()).put("title",title).put("artist",artist).put("album",album).put("duration",duration).put("favorite",favorite);}
    }
    private final Context context;
    private final ArrayList<Track> tracks=new ArrayList<>();
    AudioLibrary(Context c){context=c.getApplicationContext();try(InputStream in=new AtomicFile(new File(context.getFilesDir(),"library.json")).openRead()){JSONArray all=new JSONArray(Streams.text(in));for(int i=0;i<all.length();i++){JSONObject t=all.getJSONObject(i);Uri u=Uri.parse(t.getString("uri"));if("content".equals(u.getScheme())||("https".equals(u.getScheme())&&"api.audius.co".equals(u.getHost())&&u.getPath()!=null&&u.getPath().matches("/v1/tracks/[^/]+/stream")))tracks.add(new Track(u,t.getString("title"),t.optString("artist","Unknown artist"),t.optString("album",""),t.optLong("duration"),t.optBoolean("favorite")));}}catch(Exception ignored){}}
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
    synchronized void remove(Uri uri)throws Exception{tracks.removeIf(t->t.uri.equals(uri));save();}
    synchronized void merge(Collection<Track> incoming)throws Exception{LinkedHashMap<String,Track> map=new LinkedHashMap<>();for(Track t:tracks)map.put(t.uri.toString(),t);for(Track t:incoming){Track old=map.get(t.uri.toString());if(old!=null)t.favorite=old.favorite;map.put(t.uri.toString(),t);}tracks.clear();tracks.addAll(map.values());tracks.sort(Comparator.comparing(t->t.title.toLowerCase(Locale.ROOT)));save();}
    private void save()throws Exception{JSONArray a=new JSONArray();for(Track t:tracks)a.put(t.json());AtomicFile file=new AtomicFile(new File(context.getFilesDir(),"library.json"));FileOutputStream out=null;try{out=file.startWrite();out.write(a.toString().getBytes(StandardCharsets.UTF_8));file.finishWrite(out);}catch(Exception e){if(out!=null)file.failWrite(out);throw e;}}
    List<Track> scan(){ArrayList<Track> result=new ArrayList<>();String[] columns={MediaStore.Audio.Media._ID,MediaStore.Audio.Media.TITLE,MediaStore.Audio.Media.ARTIST,MediaStore.Audio.Media.ALBUM,MediaStore.Audio.Media.DURATION};try(Cursor c=context.getContentResolver().query(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,columns,MediaStore.Audio.Media.IS_MUSIC+" != 0",null,MediaStore.Audio.Media.TITLE+" COLLATE NOCASE")){if(c!=null)while(c.moveToNext()){Uri uri=ContentUris.withAppendedId(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,c.getLong(0));result.add(new Track(uri,clean(c.getString(1),"Audio track"),clean(c.getString(2),"Unknown artist"),clean(c.getString(3),""),c.getLong(4),false));}}return result;}
    Track read(Uri uri){String name="Audio track",artist="Unknown artist",album="";long duration=0;try(Cursor c=context.getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)){if(c!=null&&c.moveToFirst())name=clean(c.getString(0),name);}catch(Exception ignored){}
        MediaMetadataRetriever reader=new MediaMetadataRetriever();try{reader.setDataSource(context,uri);name=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_TITLE),name);artist=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ARTIST),artist);album=clean(reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_ALBUM),"");String d=reader.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);if(d!=null)duration=Long.parseLong(d);}catch(Exception ignored){}finally{try{reader.release();}catch(Exception ignored){}}return new Track(uri,name,artist,album,duration,false);}
    private static String clean(String value,String fallback){return value==null||value.trim().isEmpty()||"<unknown>".equals(value)?fallback:value;}
}
