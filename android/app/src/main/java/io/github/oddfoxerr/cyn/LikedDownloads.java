// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.provider.MediaStore;
import org.json.JSONObject;
import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Locale;
import java.util.UUID;

/**
 * Explicit user-triggered local downloads from Audius's artist-permitted download endpoint.
 * Never stores a streaming URL as a downloaded file, bypasses an Audius gate, or downloads
 * content from arbitrary hosts supplied by a catalog entry.
 */
final class LikedDownloads {
    private static final int MAX_METADATA=512*1024;
    private static final long MAX_AUDIO=250L*1024*1024;
    private LikedDownloads(){}

    static boolean isAudiusStream(Uri uri){
        if(uri==null||!"https".equalsIgnoreCase(uri.getScheme())||
            !"api.audius.co".equalsIgnoreCase(uri.getHost())||(uri.getPort()!=-1&&uri.getPort()!=443))return false;
        List<String> parts=uri.getPathSegments();
        return parts.size()==4&&"v1".equals(parts.get(0))&&"tracks".equals(parts.get(1))&&
            "stream".equals(parts.get(3))&&parts.get(2).matches("[A-Za-z0-9_-]{1,128}");
    }

    static boolean canDownload(AudioLibrary.Track track){
        return track!=null&&track.favorite&&isAudiusStream(track.uri)&&track.offlineUri==null;
    }

    static Uri download(Context context,AudioLibrary.Track song)throws Exception{
        if(!canDownload(song))throw new IOException("Only liked, not-yet-downloaded Audius songs can be downloaded.");
        String id=song.uri.getPathSegments().get(2);
        String prefix="https://api.audius.co/v1/tracks/"+Uri.encode(id);
        JSONObject data=fetchMetadata(prefix+"?app_name=CYN");
        if(!data.optBoolean("is_downloadable",false)||
            data.optBoolean("is_download_gated",false)||
            !data.optBoolean("is_streamable",true)||
            data.optBoolean("is_stream_gated",false))
            throw new IOException("The artist has not enabled a public download for this song.");

        HttpURLConnection connection=openAudio(prefix+"/download?app_name=CYN&type=mp3");
        try{
            int code=connection.getResponseCode();
            if(code==401||code==402||code==403)throw new IOException("This song requires permission from Audius to download.");
            if(code!=200)throw new IOException("Audius download unavailable (HTTP "+code+").");
            String contentType=connection.getContentType();
            if(contentType!=null&&!contentType.toLowerCase(Locale.ROOT).startsWith("audio/")&&
                !contentType.toLowerCase(Locale.ROOT).startsWith("application/octet-stream"))
                throw new IOException("Audius did not return a downloadable audio file.");
            long length=connection.getContentLengthLong();
            if(length>MAX_AUDIO)throw new IOException("The track exceeds CYN's 250 MB download limit.");
            if(Build.VERSION.SDK_INT>=29)return saveToMusic(context,song,id,connection.getInputStream());
            return savePrivately(context,id,connection.getInputStream());
        }finally{connection.disconnect();}
    }

    private static JSONObject fetchMetadata(String endpoint)throws Exception{
        HttpURLConnection conn=open(endpoint);
        try{
            if(conn.getResponseCode()!=200)throw new IOException("Could not verify download permission with Audius.");
            try(InputStream in=conn.getInputStream();ByteArrayOutputStream out=new ByteArrayOutputStream()){
                byte[] buf=new byte[8192];int n;
                while((n=in.read(buf))!=-1){
                    if(out.size()+n>MAX_METADATA)throw new IOException("Audius metadata response too large");
                    out.write(buf,0,n);
                }
                JSONObject response=new JSONObject(new String(out.toByteArray(),StandardCharsets.UTF_8));
                JSONObject data=response.optJSONObject("data");
                if(data==null)throw new IOException("Missing Audius track permission metadata");
                return data;
            }
        }finally{conn.disconnect();}
    }

    private static HttpURLConnection open(String endpoint)throws IOException{
        URL url=new URL(endpoint);
        if(!"https".equalsIgnoreCase(url.getProtocol())||url.getUserInfo()!=null)
            throw new IOException("Only secure Audius download links are allowed");
        HttpURLConnection conn=(HttpURLConnection)url.openConnection();
        conn.setConnectTimeout(12000);
        conn.setReadTimeout(25000);
        conn.setInstanceFollowRedirects(false);
        conn.setRequestProperty("Accept","audio/mpeg, audio/*, application/octet-stream, application/json");
        conn.setRequestProperty("User-Agent","CYN-Android");
        return conn;
    }

    private static HttpURLConnection openAudio(String endpoint)throws IOException{
        URL next=new URL(endpoint);
        for(int redirect=0;redirect<=5;redirect++){
            HttpURLConnection conn=open(next.toString());
            int code;
            try{code=conn.getResponseCode();}
            catch(IOException failure){conn.disconnect();throw failure;}
            if(code!=301&&code!=302&&code!=303&&code!=307&&code!=308)return conn;
            String location=conn.getHeaderField("Location");
            conn.disconnect();
            if(location==null)throw new IOException("Download redirect had no destination");
            next=new URL(next,location);
            if(!"https".equalsIgnoreCase(next.getProtocol())||next.getUserInfo()!=null)
                throw new IOException("Audius redirected to an insecure download");
        }
        throw new IOException("Too many download redirects");
    }

    private static String filename(AudioLibrary.Track song){
        String candidate=song.artist+" - "+song.title;
        candidate=candidate.replaceAll("[\\\\/\\:*?\"<>|\\p{Cntrl}]"," ").trim().replaceAll("\\s+"," ");
        if(candidate.length()>96)candidate=candidate.substring(0,96).trim();
        return (candidate.isEmpty()?"CYN track":candidate)+".mp3";
    }

    private static Uri saveToMusic(Context context,AudioLibrary.Track song,String id,InputStream input)throws IOException{
        ContentResolver resolver=context.getContentResolver();
        ContentValues fields=new ContentValues();
        fields.put(MediaStore.Audio.Media.DISPLAY_NAME,filename(song));
        fields.put(MediaStore.Audio.Media.MIME_TYPE,"audio/mpeg");
        fields.put(MediaStore.Audio.Media.RELATIVE_PATH,Environment.DIRECTORY_MUSIC+"/CYN");
        fields.put(MediaStore.Audio.Media.IS_PENDING,1);
        Uri destination=resolver.insert(MediaStore.Audio.Media.EXTERNAL_CONTENT_URI,fields);
        if(destination==null)throw new IOException("Could not create Music/CYN file");
        boolean finished=false;
        try{
            try(InputStream in=input;OutputStream out=resolver.openOutputStream(destination,"w")){
                if(out==null)throw new IOException("Could not open downloaded audio file");
                transfer(in,out);
            }
            ContentValues published=new ContentValues();
            published.put(MediaStore.Audio.Media.IS_PENDING,0);
            if(resolver.update(destination,published,null,null)!=1)
                throw new IOException("Could not publish downloaded song");
            finished=true;
            return destination;
        }finally{if(!finished)resolver.delete(destination,null,null);}
    }

    private static Uri savePrivately(Context context,String id,InputStream input)throws IOException{
        File directory=new File(context.getFilesDir(),"offline-music");
        if(!directory.isDirectory()&&!directory.mkdirs())throw new IOException("Could not create offline music folder");
        File target=new File(directory,id+".mp3");
        File temp=new File(directory,id+"-"+UUID.randomUUID()+".part");
        try{
            try(InputStream in=input;OutputStream out=new FileOutputStream(temp)){transfer(in,out);}
            if(target.exists()&&!target.delete())throw new IOException("Could not replace existing downloaded file");
            if(!temp.renameTo(target))throw new IOException("Could not finish downloaded song");
            return Uri.fromFile(target);
        }finally{if(temp.exists())temp.delete();}
    }

    static void discard(Context context,Uri downloaded){
        if(downloaded==null)return;
        try{
            if("content".equals(downloaded.getScheme()))context.getContentResolver().delete(downloaded,null,null);
            else if("file".equals(downloaded.getScheme())){
                String path=downloaded.getPath();
                File folder=new File(context.getFilesDir(),"offline-music");
                if(path!=null&&path.startsWith(folder.getCanonicalPath()+File.separator))new File(path).delete();
            }
        }catch(Exception ignored){}
    }

    private static void transfer(InputStream in,OutputStream out)throws IOException{
        byte[] bytes=new byte[32768];long total=0;int n;
        while((n=in.read(bytes))!=-1){
            total+=n;
            if(total>MAX_AUDIO)throw new IOException("Audio file exceeds 250 MB limit");
            out.write(bytes,0,n);
        }
        if(total==0)throw new IOException("Audius returned an empty audio file");
        out.flush();
    }
}
