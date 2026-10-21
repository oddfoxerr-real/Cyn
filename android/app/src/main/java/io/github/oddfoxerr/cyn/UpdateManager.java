// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.app.*;
import android.app.job.*;
import android.content.*;
import android.content.pm.*;
import android.net.*;
import android.os.*;
import android.provider.Settings;
import androidx.core.content.FileProvider;
import org.json.*;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;

final class UpdateManager {
    private static final String API="https://api.github.com/repos/oddfoxerr-real/Cyn/releases?per_page=30&page=";
    private static final String RELEASES_WEB="https://github.com/oddfoxerr-real/Cyn/releases";
    private static final ExecutorService IO=Executors.newSingleThreadExecutor();
    private static final AtomicBoolean BUSY=new AtomicBoolean();
    // Stages are reported by the real updater, not the splash animation.
    static final int IDLE=0, CHECKING=1, DOWNLOADING=2, VERIFYING=3;
    static final int READY=4, UP_TO_DATE=5, AVAILABLE=6, FAILED=7;
    static volatile int stage=IDLE;
    static volatile String message="Ready to check for updates";
    static volatile int progress=-1;
    static boolean busy(){return BUSY.get();}
    static String availableVersion(Context context){
        String candidate=prefs(context).getString("version","");
        return AndroidRelease.number(candidate)>AndroidRelease.number(BuildConfig.VERSION_NAME)
            ?candidate:"";
    }
    private static SharedPreferences prefs(Context c){return c.getSharedPreferences("updates",0);}
    static boolean automatic(Context c){return prefs(c).getBoolean("automatic",true);}
    static boolean downloads(Context c){return prefs(c).getBoolean("download",true);}
    static void schedule(Context c){JobScheduler jobs=c.getSystemService(JobScheduler.class);if(!automatic(c)){jobs.cancel(742);return;}
        jobs.schedule(new JobInfo.Builder(742,new ComponentName(c,UpdateJob.class)).setRequiredNetworkType(JobInfo.NETWORK_TYPE_ANY).setPeriodic(12*60*60*1000L,2*60*60*1000L).setPersisted(true).setBackoffCriteria(30*60*1000L,JobInfo.BACKOFF_POLICY_EXPONENTIAL).build());}
    // A single switch controls both scheduled checks and unmetered background downloads.
    // Installation always remains a user-confirmed Android action.
    static void automatic(Context c,boolean on){
        prefs(c).edit().putBoolean("automatic",on).putBoolean("download",on).apply();
        schedule(c);
    }
    static void downloads(Context c,boolean on){prefs(c).edit().putBoolean("download",on).apply();}
    static void checkOnLaunch(Context context){check(context,false,null,true);}
    static void check(Context context,boolean manual,Runnable complete){check(context,manual,complete,false);}
    private static void check(Context context,boolean manual,Runnable complete,boolean launch){
        Context c=context.getApplicationContext();if(!BUSY.compareAndSet(false,true)){if(complete!=null)complete.run();return;}
        if(!manual&&(!automatic(c)||System.currentTimeMillis()-prefs(c).getLong("checked",0)<6*60*60*1000L)){BUSY.set(false);if(complete!=null)complete.run();return;}
        progress=-1;stage=CHECKING;message="Checking CYN Studio releases…";
        IO.execute(()->{try{JSONObject newest=null;String version=null;
            for(int page=1;page<=2;page++){
                JSONArray releases=new JSONArray(new String(read(API+page,4*1024*1024),StandardCharsets.UTF_8));
                for(int i=0;i<releases.length();i++){JSONObject r=releases.getJSONObject(i);if(r.optBoolean("draft")||r.isNull("published_at"))continue;String v=AndroidRelease.newerVersion(r.optString("tag_name"),BuildConfig.VERSION_NAME);if(v==null)continue;
                    JSONArray assets=r.optJSONArray("assets");if(assets==null)continue;
                    for(int j=0;j<assets.length();j++){JSONObject a=assets.getJSONObject(j);if(AndroidRelease.assetUrl(v).equals(a.optString("browser_download_url"))&&a.optLong("size")>0&&(version==null||AndroidRelease.number(v)>AndroidRelease.number(version))){newest=a;version=v;}}
                }
                if(releases.length()<30)break;
            }
            prefs(c).edit().putLong("checked",System.currentTimeMillis()).apply();
            if(version==null){stage=UP_TO_DATE;message="CYN "+BuildConfig.VERSION_NAME+" · up to date";return;}
            String hash=newest.optString("digest");if(hash.startsWith("sha256:"))hash=hash.substring(7);else hash=new String(read(AndroidRelease.assetUrl(version)+".sha256",4096),StandardCharsets.UTF_8).trim().split("\\s+")[0];
            if(!hash.matches("[0-9a-fA-F]{64}"))throw new IOException("Release has no valid SHA-256");
            String oldVerified=prefs(c).getString("verified_hash","");
            SharedPreferences.Editor store=prefs(c).edit()
                .putString("version",version).putString("hash",hash.toLowerCase(Locale.ROOT));
            if(!hash.equalsIgnoreCase(oldVerified))store.remove("verified_hash");
            store.apply();
            stage=AVAILABLE;message="CYN "+version+" available · waiting for Wi-Fi";
            ConnectivityManager network=c.getSystemService(ConnectivityManager.class);
            if(manual||(downloads(c)&&network!=null&&!network.isActiveNetworkMetered()))download(c,version,hash,newest.getLong("size"));
        }catch(Exception e){
            stage=FAILED;
            message="Update failed · "+e.getMessage();
        }finally{
            BUSY.set(false);
            if(complete!=null)complete.run();
        }});
    }
    private static boolean allowed(URL url){String host=url.getHost();return "https".equals(url.getProtocol())&&(url.getPort()==-1||url.getPort()==443)&&url.getUserInfo()==null&&(host.equals("api.github.com")||host.equals("github.com")||host.equals("release-assets.githubusercontent.com")||host.equals("objects.githubusercontent.com"));}
    private static HttpURLConnection connect(String address)throws Exception{
        URL url=new URL(address);
        for(int redirects=0;redirects<6;redirects++){
            if(!allowed(url))throw new IOException("Untrusted update host");HttpURLConnection c=(HttpURLConnection)url.openConnection();c.setConnectTimeout(15000);c.setReadTimeout(30000);c.setInstanceFollowRedirects(false);c.setRequestProperty("User-Agent","Cyn-Android/"+BuildConfig.VERSION_NAME);c.setRequestProperty("Accept","application/vnd.github+json");c.setRequestProperty("X-GitHub-Api-Version","2022-11-28");int code=c.getResponseCode();
            if(code>=300&&code<400){String location=c.getHeaderField("Location");c.disconnect();if(location==null)throw new IOException("Invalid redirect");url=new URL(url,location);continue;}
            if(code!=200){String remaining=c.getHeaderField("X-RateLimit-Remaining");String retry=c.getHeaderField("Retry-After");c.disconnect();if(code==403||code==429){throw new IOException("GitHub API blocked this check (HTTP "+code+(("0".equals(remaining))?"; rate limit reached":"")+(retry!=null?"; retry after "+retry+"s":"")+"). Open GitHub Releases or retry later.");}throw new IOException("GitHub returned "+code);}return c;
        }throw new IOException("Too many redirects");
    }
    private static byte[] read(String url,int max)throws Exception{HttpURLConnection c=connect(url);try(InputStream in=c.getInputStream();ByteArrayOutputStream out=new ByteArrayOutputStream()){byte[]b=new byte[8192];int n;while((n=in.read(b))!=-1){if(out.size()+n>max)throw new IOException("Response too large");out.write(b,0,n);}return out.toByteArray();}finally{c.disconnect();}}
    private static File folder(Context c)throws IOException{File dir=new File(c.getCacheDir(),"updates");if(!dir.isDirectory()&&!dir.mkdirs())throw new IOException("Update storage unavailable");return dir;}
    private static void download(Context c,String version,String hash,long size)throws Exception{
        if(size<1||size>200*1024*1024)throw new IOException("Unexpected APK size");File destination=new File(folder(c),"Cyn-update.apk");
        if(destination.isFile()&&hash.equalsIgnoreCase(DesktopAi.digest(destination))){
            stage=VERIFYING;message="Verifying previously downloaded APK…";
            verify(c,destination);
            prefs(c).edit().putString("verified_hash",hash.toLowerCase(Locale.ROOT)).apply();
            stage=READY;progress=100;
            message="CYN "+version+" verified · ready to install";notifyReady(c);return;
        }
        stage=DOWNLOADING;progress=0;message="Downloading CYN "+version+" · 0%";
        File part=new File(folder(c),"download.part");HttpURLConnection connection=connect(AndroidRelease.assetUrl(version));
        try{try(InputStream in=connection.getInputStream();FileOutputStream out=new FileOutputStream(part)){byte[]b=new byte[65536];long received=0;int n;while((n=in.read(b))!=-1){received+=n;if(received>size)throw new IOException("APK exceeds release size");out.write(b,0,n);progress=(int)(100*received/size);message="Downloading update · "+progress+"%";}if(received!=size)throw new IOException("Incomplete APK download");out.getFD().sync();}
            stage=VERIFYING;progress=100;message="Verifying download and signing certificate…";
            if(!hash.equalsIgnoreCase(DesktopAi.digest(part)))throw new IOException("APK checksum mismatch");
            verify(c,part);
            if(destination.exists()&&!destination.delete())throw new IOException("Cannot replace update");if(!part.renameTo(destination))throw new IOException("Cannot finish download");
            prefs(c).edit().putString("verified_hash",hash.toLowerCase(Locale.ROOT)).apply();
            stage=READY;message="CYN "+version+" verified · ready to install";notifyReady(c);
        }finally{connection.disconnect();part.delete();}
    }
    private static void verify(Context c,File file)throws Exception{
        PackageManager pm=c.getPackageManager();PackageInfo next=pm.getPackageArchiveInfo(file.getAbsolutePath(),PackageManager.GET_SIGNING_CERTIFICATES),current=pm.getPackageInfo(c.getPackageName(),PackageManager.GET_SIGNING_CERTIFICATES);
        if(next==null||!c.getPackageName().equals(next.packageName)||next.getLongVersionCode()<=current.getLongVersionCode()||next.signingInfo==null||current.signingInfo==null)throw new IOException("APK identity or version rejected");
        android.content.pm.Signature[] a=next.signingInfo.getApkContentsSigners(),b=current.signingInfo.getApkContentsSigners();
        if(a.length==0||!new HashSet<>(Arrays.asList(a)).equals(new HashSet<>(Arrays.asList(b))))throw new IOException("Update signing certificate differs from installed CYN");
    }
    static boolean ready(Context c){
        try {
            String version=prefs(c).getString("version","");
            String hash=prefs(c).getString("hash","");
            return AndroidRelease.number(version)>AndroidRelease.number(BuildConfig.VERSION_NAME)
                &&hash.matches("[0-9a-f]{64}")
                &&hash.equals(prefs(c).getString("verified_hash",""))
                &&new File(folder(c),"Cyn-update.apk").isFile();
        }catch(Exception e){return false;}
    }
    static void install(Activity activity){try{
        File apk=new File(folder(activity),"Cyn-update.apk");String hash=prefs(activity).getString("hash","");if(!apk.isFile()||!hash.equals(DesktopAi.digest(apk)))throw new IOException("Download and verify the update first");verify(activity,apk);
        if(!activity.getPackageManager().canRequestPackageInstalls()){activity.startActivityForResult(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,Uri.parse("package:"+activity.getPackageName())),71);return;}
        Uri uri=FileProvider.getUriForFile(activity,activity.getPackageName()+".updates",apk);
        activity.startActivity(new Intent(Intent.ACTION_VIEW).setDataAndType(uri,"application/vnd.android.package-archive").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION));
    }catch(Exception e){message="Cannot install · "+e.getMessage();android.widget.Toast.makeText(activity,message,android.widget.Toast.LENGTH_LONG).show();}}
    private static void notifyReady(Context c){NotificationManager manager=c.getSystemService(NotificationManager.class);manager.createNotificationChannel(new NotificationChannel("updates","CYN updates",NotificationManager.IMPORTANCE_DEFAULT));if(Build.VERSION.SDK_INT>=33&&c.checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS)!=PackageManager.PERMISSION_GRANTED)return;
        PendingIntent open=PendingIntent.getActivity(c,742,new Intent(c,MainActivity.class).putExtra("updates",true),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
        manager.notify(742,new Notification.Builder(c,"updates").setSmallIcon(R.drawable.ic_notification).setContentTitle("CYN update ready").setContentText("Verified and downloaded. Tap to install.").setAutoCancel(true).setContentIntent(open).build());}
}
