// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import org.json.*;
import java.net.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

/** Public Audius discovery; streams remain subject to the provider's access rules. */
final class PublicMusic {
    static final class Song {
        final String id,title,artist; final long durationMs;
        Song(String i,String t,String a,long d){id=i;title=t;artist=a;durationMs=d;}
        AudioLibrary.Track asTrack(){
            Uri uri=Uri.parse("https://api.audius.co/v1/tracks/"+Uri.encode(id)+"/stream");
            return new AudioLibrary.Track(uri,title,artist,"Audius",durationMs,false);
        }
    }
    static List<Song> search(String query)throws Exception{
        String endpoint="https://api.audius.co/v1/tracks/search?query="+URLEncoder.encode(query,"UTF-8")+"&limit=25";
        HttpURLConnection conn=(HttpURLConnection)new URL(endpoint).openConnection();
        conn.setConnectTimeout(10000);conn.setReadTimeout(12000);
        conn.setRequestProperty("Accept","application/json");
        conn.setRequestProperty("User-Agent","CYN-Android");
        try {
            if(conn.getResponseCode()!=200)throw new IOException("Music service returned "+conn.getResponseCode());
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();
            try(InputStream in=conn.getInputStream()){
                byte[] buf=new byte[4096];int n;
                while((n=in.read(buf))!=-1){if(bytes.size()+n>1024*1024)throw new IOException("Catalog response too large");bytes.write(buf,0,n);}
            }
            JSONArray items=new JSONObject(new String(bytes.toByteArray(),StandardCharsets.UTF_8)).optJSONArray("data");
            ArrayList<Song> songs=new ArrayList<>();
            if(items==null)return songs;
            for(int i=0;i<items.length();i++){
                JSONObject s=items.optJSONObject(i);if(s==null||s.optBoolean("is_stream_gated",false))continue;
                String id=s.optString("id"),title=s.optString("title");
                if(id.isEmpty()||title.isEmpty())continue;
                JSONObject user=s.optJSONObject("user");
                songs.add(new Song(id,title,user==null?"Unknown artist":user.optString("name","Unknown artist"),s.optLong("duration")*1000L));
            }
            return songs;
        } finally {conn.disconnect();}
    }
}
