// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.Manifest;
import android.app.*;
import android.content.*;
import android.content.pm.*;
import android.media.*;
import android.media.audiofx.*;
import android.media.session.*;
import android.net.Uri;
import android.os.*;
import android.provider.OpenableColumns;
import java.util.*;
import java.util.concurrent.*;

public final class AudioService extends Service {
    static final String PLAY="cyn.PLAY", TOGGLE="cyn.TOGGLE", MONITOR="cyn.MONITOR", STOP="cyn.STOP";
    static final class Frame {
        final double[] x; final long id; final float level;
        Frame(double[] x,float level) { this.x=x.clone(); this.level=level; id=System.nanoTime(); }
    }
    public final class LocalBinder extends Binder { AudioService service() { return AudioService.this; } }
    final Handler handler=new Handler(Looper.getMainLooper());
    final ArrayList<Frame> frames=new ArrayList<>();
    final LinkedHashMap<Integer,Chain> chains=new LinkedHashMap<>();
    final ExecutorService trainer=Executors.newSingleThreadExecutor();
    LearningStore learning;
    SharedPreferences prefs;
    MediaPlayer player;
    MediaSession mediaSession;
    AudioManager audio;
    AudioFocusRequest focus;
    boolean monitor, enabled=true, smart, playing, preparing, training, destroyed, resumeOnFocus;
    int trainingEpoch, activeSession=-1, playerSession=-1;
    float sensitivity=.7f, bass=2, clarity=1, width=0, loudness=0;
    final float[] eq=new float[5];
    String title="Choose a song", status="Ready", analysis="Enable analysis to use Smart listening.";
    double[] predictions=new double[5], neurons=new double[12];
    private long lastFrame;
    private final BroadcastReceiver receiver=new BroadcastReceiver() {
        @Override public void onReceive(Context c,Intent i) {
            String action=i.getAction();
            if(AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(action)) { if(playing) pause(); return; }
            if(!monitor) return;
            int id=i.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION,-1);
            if(id<=0||id==playerSession) return;
            if(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION.equals(action)) attach(id,i.getStringExtra(AudioEffect.EXTRA_PACKAGE_NAME));
            else if(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION.equals(action)) detach(id);
        }
    };
    // The legacy overload is only reached below API 33, before exported flags exist.
    @android.annotation.SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override public void onCreate() {
        super.onCreate(); learning=new LearningStore(this); prefs=getSharedPreferences("audio",0);
        enabled=prefs.getBoolean("enabled",true); smart=prefs.getBoolean("smart",false); sensitivity=prefs.getFloat("sensitivity",.7f);
        bass=prefs.getFloat("bass",2); clarity=prefs.getFloat("clarity",1); width=prefs.getFloat("width",0); loudness=prefs.getFloat("loudness",0);
        for(int i=0;i<5;i++) eq[i]=prefs.getFloat("eq"+i,0);
        audio=getSystemService(AudioManager.class);
        focus=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setAudioAttributes(attributes()).setOnAudioFocusChangeListener(change->{
            if(change==AudioManager.AUDIOFOCUS_LOSS) { resumeOnFocus=false; pause(); }
            else if(change==AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                resumeOnFocus=playing;
                if(player!=null&&!preparing) { try { player.pause(); } catch(IllegalStateException ignored) { } }
                playing=false; updateSession(); updateNotification();
            }
            else if(change==AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) { if(player!=null) player.setVolume(.2f,.2f); }
            else if(change==AudioManager.AUDIOFOCUS_GAIN) { if(player!=null) player.setVolume(.8f,.8f); if(resumeOnFocus) { resumeOnFocus=false; startPlayback(); } }
        },handler).build();
        mediaSession=new MediaSession(this,"Cyn");
        mediaSession.setCallback(new MediaSession.Callback() {
            @Override public void onPlay() { startPlayback(); }
            @Override public void onPause() { pause(); }
            @Override public void onStop() { stopAll(); }
            @Override public void onSeekTo(long pos) { seek((int)pos); }
        });
        IntentFilter f=new IntentFilter(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION);
        f.addAction(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION); f.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        if(Build.VERSION.SDK_INT>=33) registerReceiver(receiver,f,Context.RECEIVER_EXPORTED); else registerReceiver(receiver,f);
        getSystemService(NotificationManager.class).createNotificationChannel(new NotificationChannel("audio","Cyn audio",NotificationManager.IMPORTANCE_LOW));
        handler.post(tick);
    }
    private AudioAttributes attributes() { return new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build(); }
    @Override public IBinder onBind(Intent intent) { return new LocalBinder(); }
    @Override public int onStartCommand(Intent i,int flags,int startId) {
        if(i==null) { stopSelf(); return START_NOT_STICKY; }
        try {
            String action=i.getAction();
            if(STOP.equals(action)) { stopAll(); return START_NOT_STICKY; }
            if(MONITOR.equals(action)) { monitor=true; foreground(); int id=i.getIntExtra("session",-1); if(id>0&&id!=playerSession) attach(id,"Music app"); status="Listening for compatible music apps"; }
            else if(PLAY.equals(action)&&i.getData()!=null) { foreground(); open(i.getData()); }
            else if(TOGGLE.equals(action)) { if(playing) pause(); else { foreground(); startPlayback(); } }
        } catch(Exception e) { status="Audio could not start: "+e.getClass().getSimpleName(); stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); }
        return START_NOT_STICKY;
    }
    void stopMonitor() {
        monitor=false; for(Integer id:new ArrayList<>(chains.keySet())) if(id!=playerSession) detach(id);
        if(playing||preparing) foreground(); else { stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); }
        status="Other-app enhancement stopped";
    }
    private void foreground() {
        int type=0;
        if(Build.VERSION.SDK_INT>=34) {
            type=monitor?ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE:ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK;
            if(monitor&&(playing||preparing)) type|=ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK;
        }
        if(Build.VERSION.SDK_INT>=34) startForeground(41,notification(),type); else startForeground(41,notification());
    }
    private PendingIntent action(String action) {
        return PendingIntent.getService(this,action.hashCode(),new Intent(this,AudioService.class).setAction(action),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
    }
    private Notification notification() {
        PendingIntent open=PendingIntent.getActivity(this,0,new Intent(this,MainActivity.class),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
        Notification.Builder b=new Notification.Builder(this,"audio").setSmallIcon(R.drawable.ic_notification).setContentTitle("Cyn · "+(player!=null?title:"Compatible apps"))
            .setContentText(playing?"Playing with your sound settings":monitor?"Other-app enhancement enabled":"Paused")
            .setContentIntent(open).setOnlyAlertOnce(true).setOngoing(playing||monitor).addAction(new Notification.Action.Builder(android.graphics.drawable.Icon.createWithResource(this,R.drawable.ic_stop),"Stop",action(STOP)).build());
        if(player!=null) b.addAction(new Notification.Action.Builder(android.graphics.drawable.Icon.createWithResource(this,playing?R.drawable.ic_pause:R.drawable.ic_play),playing?"Pause":"Play",action(TOGGLE)).build()).setStyle(new Notification.MediaStyle().setMediaSession(mediaSession.getSessionToken()).setShowActionsInCompactView(1,0));
        return b.build();
    }
    private void updateNotification() { if(playing||monitor||player!=null) getSystemService(NotificationManager.class).notify(41,notification()); }
    private void open(Uri uri) {
        releasePlayer(); frames.clear();
        title="Audio file";
        try(android.database.Cursor c=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)) { if(c!=null&&c.moveToFirst()) title=c.getString(0); } catch(Exception ignored) { }
        try {
            MediaPlayer p=new MediaPlayer(); player=p; preparing=true; p.setAudioAttributes(attributes()); p.setWakeMode(this,PowerManager.PARTIAL_WAKE_LOCK);
            p.setDataSource(this,uri); p.setVolume(.8f,.8f);
            p.setOnPreparedListener(mp->{ if(player!=mp) return; preparing=false; playerSession=mp.getAudioSessionId(); attach(playerSession,"Cyn player"); startPlayback(); });
            p.setOnCompletionListener(mp->{ playing=false; resumeOnFocus=false; audio.abandonAudioFocusRequest(focus); updateSession(); updateNotification(); if(!monitor) stopForeground(STOP_FOREGROUND_DETACH); });
            p.setOnErrorListener((mp,what,extra)->{ status="This audio file could not be played ("+what+"). Try another format."; releasePlayer(); if(!monitor) { stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); } return true; });
            p.prepareAsync(); status="Opening "+title; updateNotification();
        } catch(Exception e) { status="Unable to open this file: "+e.getClass().getSimpleName(); releasePlayer(); if(!monitor) { stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); } }
    }
    void startPlayback() {
        if(player==null||preparing) return;
        try {
            // Android 15 requires visible activity or foreground service before focus.
            foreground();
            if(audio.requestAudioFocus(focus)!=AudioManager.AUDIOFOCUS_REQUEST_GRANTED) { status="Another app has audio focus"; if(!monitor) stopForeground(STOP_FOREGROUND_DETACH); return; }
            player.setVolume(.8f,.8f); player.start(); playing=true; status="Playing"; mediaSession.setActive(true); updateSession(); updateNotification();
        } catch(Exception e) { status="Playback unavailable"; releasePlayer(); if(!monitor) stopForeground(STOP_FOREGROUND_REMOVE); }
    }
    void pause() {
        resumeOnFocus=false;
        if(player!=null&&!preparing) { try { player.pause(); } catch(IllegalStateException ignored) { } }
        playing=false; audio.abandonAudioFocusRequest(focus); updateSession(); updateNotification(); if(!monitor) stopForeground(STOP_FOREGROUND_DETACH);
    }
    int position() { try { return player!=null&&!preparing?player.getCurrentPosition():0; } catch(Exception e) { return 0; } }
    int duration() { try { return player!=null&&!preparing?player.getDuration():0; } catch(Exception e) { return 0; } }
    void seek(int value) { if(player!=null&&!preparing) { player.seekTo(Math.max(0,Math.min(duration(),value))); frames.clear(); updateSession(); } }
    private void updateSession() {
        mediaSession.setPlaybackState(new PlaybackState.Builder().setActions(PlaybackState.ACTION_PLAY|PlaybackState.ACTION_PAUSE|PlaybackState.ACTION_PLAY_PAUSE|PlaybackState.ACTION_STOP|PlaybackState.ACTION_SEEK_TO).setState(playing?PlaybackState.STATE_PLAYING:PlaybackState.STATE_PAUSED,position(),playing?1:0).build());
        mediaSession.setMetadata(new MediaMetadata.Builder().putString(MediaMetadata.METADATA_KEY_TITLE,title).putLong(MediaMetadata.METADATA_KEY_DURATION,duration()).build());
    }
    private void releasePlayer() {
        if(playerSession>0) detach(playerSession); playerSession=-1;
        if(player!=null) { player.release(); player=null; } playing=false; preparing=false; audio.abandonAudioFocusRequest(focus);
    }
    void stopAll() { monitor=false; releasePlayer(); for(Integer id:new ArrayList<>(chains.keySet())) detach(id); mediaSession.setActive(false); stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); status="Stopped"; }
    private void attach(int id,String owner) {
        if(id<=0||chains.containsKey(id)) return;
        if(chains.size()>=8) { status="Too many audio sessions; close an unused player."; return; }
        Chain c=new Chain(id,owner==null?"Music app":owner); chains.put(id,c); activeSession=id; frames.clear(); c.apply(); c.analyze();
    }
    private void detach(int id) {
        Chain c=chains.remove(id); if(c!=null) c.close();
        if(activeSession==id) { activeSession=chains.isEmpty()?-1:chains.keySet().iterator().next(); frames.clear(); }
    }
    void changed() {
        SharedPreferences.Editor e=prefs.edit().putBoolean("enabled",enabled).putBoolean("smart",smart).putFloat("sensitivity",sensitivity).putFloat("bass",bass).putFloat("clarity",clarity).putFloat("width",width).putFloat("loudness",loudness);
        for(int i=0;i<5;i++) e.putFloat("eq"+i,eq[i]); e.apply();
        for(Chain c:chains.values()) { c.apply(); c.analyze(); }
        if(!smart||!enabled) { Arrays.fill(predictions,0); Arrays.fill(neurons,0); analysis="Analysis off · manual settings restored"; }
    }
    void train() {
        if(training) return; training=true; trainingEpoch=0;
        PreferenceModel candidate=learning.model.copy(); ArrayList<PreferenceModel.Example> labels=new ArrayList<>(learning.examples);
        trainer.execute(()->{
            boolean success=candidate.fit(labels,epoch->handler.post(()->trainingEpoch=epoch));
            handler.post(()->{
                if(destroyed) return; training=false;
                if(success) { learning.model=candidate; learning.message="Training complete · 240 real epochs on your feedback."; try { learning.save(); } catch(Exception e) { learning.message="Trained, but could not save the model."; } }
                else learning.message="Each effect needs at least five More and five Less frames.";
            });
        });
    }
    private final Runnable tick=new Runnable() {
        @Override public void run() {
            if(destroyed) return;
            for(Chain c:chains.values()) { if(SystemClock.elapsedRealtime()-c.lastCapture>2000) c.targetBass=c.targetClarity=0; c.bassOffset+=(c.targetBass-c.bassOffset)*.12; c.clarityOffset+=(c.targetClarity-c.clarityOffset)*.12; c.apply(); }
            long now=SystemClock.elapsedRealtime();
            if(now-lastFrame>2500) { Arrays.fill(predictions,0); Arrays.fill(neurons,0); if(smart&&enabled) analysis="Waiting for session analysis · enable permission if needed"; }
            long cutoff=System.nanoTime()-60_000_000_000L; while(!frames.isEmpty()&&frames.get(0).id<cutoff) frames.remove(0);
            handler.postDelayed(this,150);
        }
    };
    final class Chain {
        final int id; final String owner;
        Equalizer equalizer; Virtualizer virtualizer; LoudnessEnhancer enhancer; Visualizer visualizer;
        String failure=""; long lastCapture; double rms,peak,targetBass,targetClarity,bassOffset,clarityOffset;
        String lastSettings="";
        Chain(int id,String owner) {
            this.id=id; this.owner=owner;
            try { equalizer=new Equalizer(0,id); equalizer.setControlStatusListener((effect,control)->lastSettings=""); } catch(RuntimeException e) { failure="Equalizer unavailable on this session"; }
            try { virtualizer=new Virtualizer(0,id); } catch(RuntimeException ignored) { }
            try { enhancer=new LoudnessEnhancer(id); } catch(RuntimeException ignored) { }
        }
        void analyze() {
            if(!smart||!enabled||checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED) {
                if(visualizer!=null) { visualizer.release(); visualizer=null; }
                targetBass=targetClarity=bassOffset=clarityOffset=0; return;
            }
            if(visualizer!=null) return;
            try {
                visualizer=new Visualizer(id); visualizer.setCaptureSize(Math.min(1024,Visualizer.getCaptureSizeRange()[1])); visualizer.setScalingMode(Visualizer.SCALING_MODE_AS_PLAYED);
                visualizer.setDataCaptureListener(new Visualizer.OnDataCaptureListener() {
                    @Override public void onWaveFormDataCapture(Visualizer v,byte[] wave,int rate) { double[] levels=AudioFeatures.levels(wave); rms=levels[0]; peak=levels[1]; }
                    @Override public void onFftDataCapture(Visualizer v,byte[] fft,int rate) {
                        lastCapture=SystemClock.elapsedRealtime();
                        double[] x=AudioFeatures.fromFft(fft,rate/1000,rms,peak);
                        double[] p=learning.model.predict(x);
                        // Do not boost silence. The model does not control Android volume or routing.
                        targetBass=rms>.01&&learning.model.ready[4]?(p[4]*2-1)*bass*sensitivity:0;
                        targetClarity=rms>.01&&learning.model.ready[0]?(p[0]*2-1)*clarity*sensitivity:0;
                        if((playing&&id==playerSession)||(!playing&&id==activeSession)) {
                            predictions=p; neurons=learning.model.hidden(x); analysis=rms>.01?"Live session analysis · features stay on this phone":"Waiting for audible audio";
                            if(rms>.01&&lastCapture-lastFrame>=240) { lastFrame=lastCapture; frames.add(new Frame(x,(float)Math.min(1,rms*4))); while(frames.size()>240) frames.remove(0); }
                        }
                    }
                },Math.min(4000,Visualizer.getMaxCaptureRate()),true,true); visualizer.setEnabled(true);
            } catch(RuntimeException e) { if(visualizer!=null) { visualizer.release(); visualizer=null; } analysis="Analysis unavailable for this session; manual effects still work."; }
        }
        void apply() {
            String signature=enabled+":"+bass+":"+clarity+":"+width+":"+loudness+":"+Arrays.toString(eq)+":"+Math.round(bassOffset*50)+":"+Math.round(clarityOffset*50);
            if(signature.equals(lastSettings)) return;
            try {
                if(equalizer!=null) {
                    equalizer.setEnabled(enabled); short[] range=equalizer.getBandLevelRange();
                    for(short i=0;i<equalizer.getNumberOfBands();i++) {
                        double hz=equalizer.getCenterFreq(i)/1000.0;
                        double pos=PreferenceModel.clamp(Math.log(Math.max(60,hz)/60)/Math.log(14000.0/60)*4,0,4);
                        int lo=(int)pos,hi=Math.min(4,lo+1); double base=eq[lo]+(eq[hi]-eq[lo])*(pos-lo);
                        double b=PreferenceModel.clamp(bass+bassOffset,0,6)/(1+Math.pow(hz/180,2));
                        double c=PreferenceModel.clamp(clarity+clarityOffset,0,6)*(1-1/(1+Math.pow(hz/2200,2)));
                        equalizer.setBandLevel(i,(short)PreferenceModel.clamp((base+b+c)*100,Math.max(-600,range[0]),Math.min(600,range[1])));
                    }
                }
                if(virtualizer!=null) { virtualizer.setEnabled(enabled&&width>0); if(virtualizer.getStrengthSupported()) virtualizer.setStrength((short)(width*10)); }
                if(enhancer!=null) { enhancer.setEnabled(enabled&&loudness>0); enhancer.setTargetGain((int)(loudness*100)); }
                lastSettings=signature;
            } catch(RuntimeException e) { failure="Effect control unavailable or owned by another equalizer"; }
        }
        void close() { if(visualizer!=null) visualizer.release(); if(equalizer!=null) equalizer.release(); if(virtualizer!=null) virtualizer.release(); if(enhancer!=null) enhancer.release(); }
        String description() { return owner+" · session "+id+"\n"+(equalizer!=null?"EQ, bass and clarity":"No equalizer")+(virtualizer!=null?" · width":"")+(enhancer!=null?" · loudness":"")+(failure.isEmpty()?"":"\n"+failure); }
    }
    @Override public void onDestroy() {
        destroyed=true; handler.removeCallbacksAndMessages(null); trainer.shutdownNow(); unregisterReceiver(receiver); releasePlayer();
        for(Chain c:chains.values()) c.close(); chains.clear(); mediaSession.release(); super.onDestroy();
    }
}
