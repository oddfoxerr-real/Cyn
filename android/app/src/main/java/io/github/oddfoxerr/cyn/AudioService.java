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

@androidx.annotation.OptIn(markerClass = androidx.media3.common.util.UnstableApi.class)
public final class AudioService extends Service {
    static final String CAPTURE="cyn.CAPTURE";
    static final String PLAY="cyn.PLAY", TOGGLE="cyn.TOGGLE", MONITOR="cyn.MONITOR", STOP="cyn.STOP";
    static final class Frame {
        final double[] x; final long id; final float level;
        Frame(double[] x,float level) { this.x=x.clone(); this.level=level; id=System.nanoTime(); }
    }
    public final class LocalBinder extends Binder { AudioService service() { return AudioService.this; } }
    final Handler handler=new Handler(Looper.getMainLooper());
    final ArrayList<Frame> frames=new ArrayList<>();
    final LinkedHashMap<Integer,Chain> chains=new LinkedHashMap<>();
    private Set<String> activePlaybackPackages=Collections.emptySet();
    private long lastPlaybackRefresh;
    private final AudioDeviceCallback deviceCallback=new AudioDeviceCallback() {
        @Override public void onAudioDevicesAdded(AudioDeviceInfo[] devices) { scheduleRouteRecovery(); }
        @Override public void onAudioDevicesRemoved(AudioDeviceInfo[] devices) { scheduleRouteRecovery(); }
    };
    private void scheduleRouteRecovery(){
        handler.removeCallbacks(routeRecovery);
        handler.postDelayed(routeRecovery,700);
    }
    final ExecutorService trainer=Executors.newSingleThreadExecutor();
    LearningStore learning;
    SharedPreferences prefs;
    androidx.media3.exoplayer.ExoPlayer player;
    DesktopAi desktopAi;
    ConnectedCapture connectedCapture;
    private boolean projectionForeground;
    private long captureGeneration;
    private String captureStatus="Enable Connect AI to share playback";
    DesktopAi currentAi() { return !playing&&connectedCapture!=null?connectedCapture.ai:desktopAi; }
    boolean capturedAudioFresh() { return monitor&&!playing&&enabled&&connectedCapture!=null&&connectedCapture.fresh(); }
    private void stopCapture() {
        captureGeneration++;
        ConnectedCapture old=connectedCapture;
        connectedCapture=null;
        projectionForeground=false;
        if(old!=null)old.stop();
        appliedAiFresh=false;
        connectedMind=null;
        clearTrace();
        // Revert external sessions to their manual Studio settings immediately.
        // A disconnected Connect AI must never leave stale neural EQ applied.
        if(!destroyed) {
            updateNativeControls();
            for(Chain chain:chains.values()) chain.apply();
        }
    }
    private void startCapture(Intent consent) {
        stopCapture();
        if(Build.VERSION.SDK_INT<29||consent==null)return;
        monitor=true;
        projectionForeground=true;
        foreground(); // Android requires this before getMediaProjection.
        final long generation=captureGeneration;
        try {
            connectedCapture=new ConnectedCapture(this,consent,handler,()->{
                if(destroyed||generation!=captureGeneration)return;
                connectedCapture=null;
                projectionForeground=false;
                captureStatus="Playback sharing stopped · enable Connect AI to restart";
                // Revoked consent invalidates all AI decisions immediately.
                appliedAiFresh=false;
                connectedMind=null;
                clearTrace();
                updateNativeControls();
                for(Chain chain:chains.values())chain.apply();
                if(monitor||playing)foreground();
            });
            captureStatus="Playback sharing started";
        } catch(RuntimeException|LinkageError error) {
            projectionForeground=false;
            captureStatus="Playback sharing unavailable · reconnect to retry";
            foreground();
        }
    }
    AudioLibrary library;
    float ambience=0, dynamics=.2f, headroom=1;
    int bandCount=31;
    final float[] studioEq=new float[39];
    boolean useApex, movieMode, spatialPreviewEnabled, partialNeuralPreview;
    String aiProfile=AiSelection.BASIC;
    float spatialPreviewStrength=0.4f;
    float audonicIntensity=0.45f;
    private long libraryFrame, libraryTrace;
    // UI telemetry is captured from PCM and inference in a 150ms service tick.
    // Indexes: bass, kick/transient, spatial DSP, neural decision intensity.
    volatile float[] apexVisual=new float[]{0,0,0,0};
    volatile long apexDecisionAt;
    MediaSession mediaSession;
    AudioManager audio;
    AudioFocusRequest focus;
    boolean monitor, enabled=true, smart, playing, preparing, training, destroyed, resumeOnFocus;
    int trainingEpoch, activeSession=-1, playerSession=-1;
    private final Runnable routeRecovery=()->{
        if(destroyed||!monitor)return;
        // Bluetooth/headphone changes can invalidate effect control without a
        // corresponding CLOSE broadcast. Recreate existing effects, not audio.
        for(Chain chain:new ArrayList<>(chains.values()))
            if(chain.id!=playerSession)chain.reopen();
        refreshVisualization();
        updateNotification();
    };
    float sensitivity=.7f, bass=2, clarity=1, width=0, loudness=0;
    final float[] eq=new float[5];
    String title="Choose a song", status="Ready", analysis="Enable analysis to use Smart listening.";
    double[] predictions=new double[5], neurons=new double[12];
    private long lastFrame;
    private volatile long traceAt;
    private volatile PreferenceModel.Trace liveTrace, trainingTrace;
    int traceSequence;
    final PcmSpectrum pcmSpectrum=new PcmSpectrum();
    volatile float[] sessionSpectrum=new float[31];
    volatile long sessionSpectrumAt;
    // Visualizer(0) may provide permitted output-mix measurements even if the
    // music player never exposes an EQ session. It does not route audio.
    private Visualizer outputVisuals;
    private long visualRetryAt;
    private volatile double measuredRms, measuredPeak;
    private String visualError="Not started";
    float aiDeltaDb;
    int aiEqBands;
    // Read on the UI thread: actual targets delivered to the native CYN engine.
    final float[] appliedTargets=new float[48];
    long appliedTargetAt;
    boolean appliedAiFresh;
    boolean legacy;
    PreferenceModel.Trace neuralTrace() { return training?trainingTrace:enabled&&smart&&SystemClock.elapsedRealtime()-traceAt<900?liveTrace:null; }
    Chain displayedChain() { return chains.get(legacy?0:playing?playerSession:activeSession); }
    int externalSessionCount() {
        int count=0;
        for(Chain c:chains.values()) if(c.id>0&&c.id!=playerSession) count++;
        return count;
    }
    int controlledExternalEqCount() {
        int count=0;
        for(Chain c:chains.values()) if(c.id>0&&c.id!=playerSession&&c.controlsEq()) count++;
        return count;
    }
    String connectionState() {
        if(!monitor) return "Other apps disconnected";
        if(!enabled) return "Enhancement off · monitoring only";
        if(legacy) {
            Chain c=chains.get(0);
            return c!=null&&c.controlsEq()?"System-mix EQ available · routing unverified":"System-mix EQ unavailable on this device";
        }
        int controlled=controlledExternalEqCount();
        if(controlled>0) return controlled==1?"1 music session EQ controlled":controlled+" music session EQs controlled";
        if(externalSessionCount()>0) return "Music session detected · EQ unavailable";
        return "Waiting for a compatible music session";
    }
    String connectionHint() {
        if(!monitor) return "Connect your music apps for adaptive sound. Library playback is optional.";
        if(!enabled) return "Monitoring is on, but enhancement is switched off.";
        if(connectedCapture!=null)return connectedCapture.status+" · "+(smart?currentAi().status:"AI off");
        if(ExternalNowPlaying.instance==null)
            return "Enable CYN media controls in Android Notification Access to identify playing music apps. This does not grant system-wide DSP.";
        if(legacy) return "Experimental output-mix mode: even EQ control does not prove that other apps use it. Compare sound on/off.";
        if(controlledExternalEqCount()>0) return "CYN controls Android's session EQ. Enable Connect AI for neural playback analysis. Effect support depends on the player and device.";
        if(externalSessionCount()>0) {
            for(Chain c:chains.values()) if(c.id>0&&c.id!=playerSession&&!c.failure.isEmpty()) return c.failure;
            return "Android exposed a session but did not grant usable effect control.";
        }
        return "Open a music app and restart its playback. Select CYN in that app's EQ settings if available. Some apps never share an audio session.";
    }
    String neuralStatus() {
        if(training) return "Training candidate · epoch "+trainingEpoch+" / 240 · example input";
        PreferenceModel.Trace t=neuralTrace(); Chain c=displayedChain();
        if(playing)return enabled?(smart?desktopAi.status:"AI off · manual library sound"):"CYN bypassed · original audio";
        if(capturedAudioFresh())return currentAi().status+" · "+processingStatus();
        if(t==null) return "No fresh audio analysis · no live model values";
        String outputs="Frame #"+traceSequence+" · Bass "+(t.ready[4]?Math.round(t.outputs[4]*100)+"%":"untrained")+" · Clarity "+(t.ready[0]?Math.round(t.outputs[0]*100)+"%":"untrained");
        boolean applied=c!=null&&c.controlsEq()&&sensitivity>0&&(t.ready[4]||t.ready[0]);
        return outputs+"\n"+(applied?"Adjustments sent to Android EQ":"Preview only · adjustments not applied")+
            (c==null?"":String.format(Locale.ROOT,"\nEQ offsets requested: bass %+.2f dB · clarity %+.2f dB",c.controlsEq()?PreferenceModel.clamp(bass+c.bassOffset,0,6)-bass:0,c.controlsEq()?PreferenceModel.clamp(clarity+c.clarityOffset,0,6)-clarity:0));
    }
    void clearTrace() { liveTrace=null; traceAt=0; Arrays.fill(predictions,0); Arrays.fill(neurons,0); }
    /** Privacy-minimized local diagnostics: no manufacturer, model, app identities or session IDs. */
    String diagnostics() {
        return "CYN Studio "+BuildConfig.VERSION_NAME+
            "\nConnection: "+connectionState()+
            "\nPlayback mode: "+processingStatus()+
            "\nConnect AI: "+(connectedCapture!=null?connectedCapture.status:captureStatus)+
            "\nExternal effect sessions detected: "+externalSessionCount()+
            "\nExternal EQ controls available: "+controlledExternalEqCount()+
            "\nSystem mix compatibility: "+(legacy?"requested":"not requested")+
            "\nLive session spectrum: "+(hasSessionSpectrum()?"receiving samples":"no recent samples")+
            "\nPersonal neural activity: "+(neuralTrace()!=null?"measured locally":"no recent activation data")+
            "\nVisualization: "+visualizationStatus()+
            "\nAudio remains on your device. These details are shown only in the app.";
    }
    /** Reports a measured control change, never an invented AI animation. */
    float aiControlValue(String control,int band) {
        if(!enabled||!smart)return Float.NaN;
        if(playing||capturedAudioFresh()) {
            if(!playing&&(displayedChain()==null||!displayedChain().controlsEq()||(!legacy&&externalSessionCount()!=1)))return Float.NaN;
            if(!appliedAiFresh||SystemClock.elapsedRealtime()-appliedTargetAt>1800)return Float.NaN;
            switch(control) {
                case "Bass":return appliedTargets[1];
                case "Vocal clarity":return appliedTargets[2];
                case "Stereo width":return playing?appliedTargets[3]*100f:displayedChain().appliedWidth*100f;
                case "Ambience":return playing?appliedTargets[4]*100f:Float.NaN;
                case "Dynamics":return playing?appliedTargets[5]*100f:Float.NaN;
                case "EQ":return band>=0&&band<bandCount?appliedTargets[9+band]:Float.NaN;
                default:return Float.NaN;
            }
        }
        if(!monitor||neuralTrace()==null)return Float.NaN;
        Chain chain=chains.get(legacy?0:activeSession);
        if(chain==null||!chain.controlsEq()||SystemClock.elapsedRealtime()-chain.lastCapture>1400)return Float.NaN;
        if("Bass".equals(control))return (float)PreferenceModel.clamp(bass+chain.bassOffset,0,6);
        if("Vocal clarity".equals(control))return (float)PreferenceModel.clamp(clarity+chain.clarityOffset,0,6);
        return Float.NaN;
    }
    boolean hasSessionSpectrum() {
        return monitor&&enabled&&sessionSpectrumAt>0&&SystemClock.elapsedRealtime()-sessionSpectrumAt<1200;
    }
    String visualizationStatus(){
        if(!monitor&&!playing)return "Connect CYN to activate live audio visuals";
        if(!enabled)return "Visuals paused · enhancement disabled";
        if(playing)return "CYN internal player visualization";
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED)
            return "Live visuals need playback analysis permission";
        if(capturedAudioFresh())return smart?"Live shared playback spectrum + neural analysis":"Live shared playback spectrum · AI off";
        if(hasSessionSpectrum())return neuralTrace()!=null
            ?"Live audio spectrum + neural activity"
            :"Live audio spectrum · enable Smart Listening for neural activity";
        return "Waiting for playback measurements · "+visualError;
    }
    // Connected-app visual telemetry comes from the already permission-gated
    // AudioVisualizer FFT and the real 11->12->5 CYN personal neural model.
    // Permissioned playback sharing additionally supplies full PCM to currentAi().
    // Visualizer-only fallback remains the personal model, never fabricated APEX inference.
    static final class MindSample {
        final float[] channels,hidden,connections,outputs;
        final long at;
        MindSample(float[] channels,float[] hidden,float[] connections,float[] outputs,long at) {
            this.channels=channels;
            this.hidden=hidden;
            this.connections=connections;
            this.outputs=outputs;
            this.at=at;
        }
    }
    private volatile MindSample connectedMind;
    private double previousMindRms,previousMindBass;
    private synchronized void publishConnectedMind(double[] fftFeatures,double rms,
            PreferenceModel.Trace trace,long at) {
        if(!monitor||!enabled||!smart||playing||rms<=.01||trace==null)return;
        double low= Math.exp(fftFeatures[0])+Math.exp(fftFeatures[1]);
        double mid= Math.exp(fftFeatures[3])+Math.exp(fftFeatures[4])+
            Math.exp(fftFeatures[5]);
        float bass=(float)PreferenceModel.clamp(low*2.2,0,1);
        float kick=(float)PreferenceModel.clamp(
            Math.max(0,rms-previousMindRms)*7.0+
            Math.max(0,low-previousMindBass)*4.0,0,1);
        float spectrum=(float)PreferenceModel.clamp(mid*2,0,1);
        float neural=0;
        float[] hidden=new float[12],connections=new float[60],outputs=new float[5];
        for(int i=0;i<12;i++) {
            hidden[i]=(float)PreferenceModel.clamp(trace.hidden[i],-1,1);
            neural+=Math.abs(hidden[i])/12.0f;
        }
        for(int i=0;i<60;i++)
            connections[i]=(float)PreferenceModel.clamp(trace.outputConnections[i],-1,1);
        for(int i=0;i<5;i++)
            outputs[i]=(float)PreferenceModel.clamp(trace.outputs[i],0,1);
        // A single volatile immutable snapshot keeps the 12 hidden activations
        // and 60 real weighted paths in step with the displayed audio frame.
        connectedMind=new MindSample(
            new float[]{bass,kick,spectrum,neural},hidden,connections,outputs,at);
        previousMindRms=rms; previousMindBass=low;
    }
    boolean mindConnected(){return playing||monitor;}
    boolean mindExternal(){
        return !playing&&monitor&&enabled&&smart&&hasSessionSpectrum()
            &&connectedMind!=null
            &&SystemClock.elapsedRealtime()-connectedMind.at<1200;
    }
    boolean mindLive(){
        return (playing||capturedAudioFresh())&&enabled&&smart&&(spatialPreviewEnabled||AiSelection.AUDONIC.equals(aiProfile))||mindExternal();
    }
    float[] mindHidden(){return mindExternal()?connectedMind.hidden:null;}
    float[] mindConnections(){return mindExternal()?connectedMind.connections:null;}
    float[] mindOutputs(){return mindExternal()?connectedMind.outputs:null;}
    /** Immutable six-effect ONNX decision, only while the selected engine is live. */
    float[] audonicVisualTelemetry(){
        if(!enabled||!smart||!AiSelection.AUDONIC.equals(aiProfile)
                ||(!playing&&!capturedAudioFresh()))return null;
        DesktopAi active=currentAi();
        DesktopAi.AudonicPolicy policy=active==null?null:active.audonicPolicy;
        return policy!=null&&SystemClock.elapsedRealtime()-policy.at<2500
            ?policy.effects:null;
    }
    float[] apexVisualTelemetry(){
        return capturedAudioFresh()?apexVisual:mindExternal()?connectedMind.channels:playing?apexVisual:null;
    }
    long apexDecisionTimestamp(){
        return capturedAudioFresh()?apexDecisionAt:mindExternal()?connectedMind.at:playing?apexDecisionAt:0;
    }
    private void updateApexVisualTelemetry(){
        DesktopAi desktopAi=currentAi();
        if((!playing&&!capturedAudioFresh())||!enabled||!smart||!(spatialPreviewEnabled||AiSelection.AUDONIC.equals(aiProfile))||desktopAi==null){
            apexVisual=new float[]{0,0,0,0};
            apexDecisionAt=0;
            return;
        }
        float bassValue=0,kickValue=0;
        float[] window=NativeEngine.spatialPolicyWindow(desktopAi.engine);
        if(window!=null&&window.length==160){
            bassValue=Math.max(0f,Math.min(1f,window[19]));
            kickValue=Math.max(0f,Math.min(1f,window[20+19]));
        }
        Chain chain=displayedChain();
        float spatial=playing?apexSpatialActivity():chain!=null&&chain.controlsEq()?chain.appliedWidth:0;
        DesktopAi.AudonicPolicy creative=desktopAi.audonicPolicy;
        if(AiSelection.AUDONIC.equals(aiProfile)&&creative!=null&&SystemClock.elapsedRealtime()-creative.at<2500){
            apexDecisionAt=creative.at;
            apexVisual=new float[]{bassValue,kickValue,spatial,Math.max(0f,Math.min(1f,creative.effects[0]+creative.effects[1]))};
            return;
        }
        DesktopAi.PartialPolicy partial=desktopAi.partialPolicy;
        if(partialNeuralPreview&&partial!=null&&SystemClock.elapsedRealtime()-partial.at<2500){
            apexDecisionAt=partial.at;
            apexVisual=new float[]{bassValue,kickValue,spatial,
                Math.max(0f,Math.min(1f,partial.spatial))};
        }else{
            DesktopAi.Result decision=desktopAi.latest;
            apexDecisionAt=decision!=null&&SystemClock.elapsedRealtime()-decision.at<2500
                ?decision.at:0;
            apexVisual=new float[]{bassValue,kickValue,spatial,0};
        }
    }
    float apexSpatialActivity(){
        if(!playing||!enabled||!smart||desktopAi==null||desktopAi.engine==0)return 0f;
        if(AiSelection.AUDONIC.equals(aiProfile))return Math.max(0f,Math.min(1f,NativeEngine.audonicActivity(desktopAi.engine)));
        if(!spatialPreviewEnabled)return 0f;
        return Math.max(0f,Math.min(1f,NativeEngine.spatialActivity(desktopAi.engine)));
    }
    String processingStatus() {
        if(!enabled)return "Enhancement off · original sound";
        if(playing) {
            if(AiSelection.AUDONIC.equals(aiProfile))return "AUDONIC Experimental AI · "+desktopAi.audonicStatus;
            if(spatialPreviewEnabled)return partialNeuralPreview
                ? "APEX 10M Experimental Release · "+desktopAi.partialStatus
                : "APEX 10M · Classic Spatial DSP";
            if(!smart)return "CYN native DSP active · manual tuning";
            DesktopAi.Result r=desktopAi.latest;
            if(r==null||SystemClock.elapsedRealtime()-r.at>2500)
                return "CYN native DSP active · waiting for AI analysis";
            return String.format(Locale.ROOT,
                "Neural decisions sent to CYN DSP · adjustment peak %.1f dB · %d EQ bands",
                aiDeltaDb,aiEqBands);
        }
        if(monitor) {
            if(connectedCapture!=null) {
                if(!smart)return "Connected manual sound · AI off";
                if(!capturedAudioFresh())return connectedCapture.status;
                if(!legacy&&externalSessionCount()>1)return currentAi().status+" · multiple sessions, per-session EQ fallback";
                String route=legacy?"system-mix effects (device dependent)":"Android session effects";
                boolean control=displayedChain()!=null&&displayedChain().controlsEq();
                if(!control)return currentAi().status+" · analysis only, no effect control";
                return currentAi().status+" → "+route+
                    (spatialPreviewEnabled?" · "+currentAi().partialStatus:"");
            }
            if(legacy) return chains.containsKey(0)&&chains.get(0).controlsEq()
                ?(hasSessionSpectrum()?"System-mix EQ + live adaptive frequency control · routing varies by device":"System-mix EQ controlled · live analysis unavailable or disabled")
                :"Android system-mix EQ not controlled";
            if(controlledExternalEqCount()>0)return hasSessionSpectrum()
                ?"Music-session EQ + live adaptive frequency control · native YAMNet unavailable in this route"
                :"Android music-session EQ controlled · analysis unavailable or disabled";
            if(externalSessionCount()>0)return "Music session detected · EQ control unavailable";
            return "Monitoring · waiting for compatible playback";
        }
        return "CYN idle · choose music or connect compatible audio";
    }
    void startLegacy() {
        for(Integer id:new ArrayList<>(chains.keySet())) detach(id);
        legacy=true; monitor=true; attach(0,"Legacy output mix (device dependent)");
        status=connectionState();
        updateNotification();
    }
    private final BroadcastReceiver receiver=new BroadcastReceiver() {
        @Override public void onReceive(Context c,Intent i) {
            String action=i.getAction();
            if(AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(action)) { if(playing) pause(); return; }
            if(!monitor||legacy) return;
            int id=i.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION,-1);
            if(id<=0||id==playerSession) return;
            String sourcePackage=i.getStringExtra(AudioEffect.EXTRA_PACKAGE_NAME);
            if(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION.equals(action)) {
                attach(id,"External playback",sourcePackage);
            } else if(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION.equals(action)) {
                Chain chain=chains.get(id);
                if(chain!=null&&(chain.packageName==null||
                        chain.packageName.equals(sourcePackage)))detach(id);
            }
        }
    };
    // The legacy overload is only reached below API 33, before exported flags exist.
    @android.annotation.SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override public void onCreate() {
        super.onCreate(); learning=new LearningStore(this); prefs=getSharedPreferences("audio",0);
        desktopAi=new DesktopAi(this);library=new AudioLibrary(this);
        ambience=prefs.getFloat("ambience",0);dynamics=prefs.getFloat("dynamics",.2f);headroom=prefs.getFloat("headroom",1);
        bandCount=prefs.getInt("bands",31);if(bandCount!=31&&bandCount!=35&&bandCount!=39)bandCount=31;
        movieMode=prefs.getBoolean("movie",false);
        aiProfile=AiSelection.migrate(prefs.getString("ai_profile",null),prefs.getBoolean("apex",false),prefs.getBoolean("spatial10m_preview",false));
        useApex=AiSelection.usesApex6M(aiProfile);
        spatialPreviewEnabled=AiSelection.usesSpatialPreview(aiProfile);
        spatialPreviewStrength=Math.max(0f,Math.min(1f,prefs.getFloat("spatial10m_strength",.4f)));
        audonicIntensity=Math.max(0f,Math.min(1f,prefs.getFloat("audonic_intensity",.45f)));
        partialNeuralPreview=prefs.getBoolean("apex10m_partial_opt_in",false);
        for(int i=0;i<39;i++)studioEq[i]=prefs.getFloat("studioEq"+i,0);
        enabled=prefs.getBoolean("enabled",true); smart=prefs.getBoolean("smart",true); sensitivity=prefs.getFloat("sensitivity",.7f);
        bass=prefs.getFloat("bass",2); clarity=prefs.getFloat("clarity",1); width=prefs.getFloat("width",0); loudness=prefs.getFloat("loudness",0);
        for(int i=0;i<5;i++) eq[i]=prefs.getFloat("eq"+i,0);
        audio=getSystemService(AudioManager.class);
        audio.registerAudioDeviceCallback(deviceCallback,handler);
        focus=new AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN).setAudioAttributes(attributes()).setOnAudioFocusChangeListener(change->{
            if(change==AudioManager.AUDIOFOCUS_LOSS) { resumeOnFocus=false; pause(); }
            else if(change==AudioManager.AUDIOFOCUS_LOSS_TRANSIENT) {
                resumeOnFocus=playing;
                if(player!=null) { try { player.pause(); } catch(IllegalStateException ignored) { } }
                playing=false; updateSession(); updateNotification();
            }
            else if(change==AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK) { if(player!=null) player.setVolume(.25f); }
            else if(change==AudioManager.AUDIOFOCUS_GAIN) { if(player!=null) player.setVolume(1f); if(resumeOnFocus) { resumeOnFocus=false; startPlayback(); } }
        },handler).build();
        mediaSession=new MediaSession(this,"Cyn");
        mediaSession.setCallback(new MediaSession.Callback() {
            @Override public void onPlay() { startPlayback(); }
            @Override public void onPause() { pause(); }
            @Override public void onStop() { stopAll(); }
            @Override public void onSeekTo(long pos) { seek((int)pos); }
            @Override public void onSkipToNext(){ next(); }
            @Override public void onSkipToPrevious(){ previous(); }
        });
        IntentFilter f=new IntentFilter(AudioEffect.ACTION_OPEN_AUDIO_EFFECT_CONTROL_SESSION);
        f.addAction(AudioEffect.ACTION_CLOSE_AUDIO_EFFECT_CONTROL_SESSION); f.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        if(Build.VERSION.SDK_INT>=33) registerReceiver(receiver,f,Context.RECEIVER_EXPORTED); else registerReceiver(receiver,f);
        getSystemService(NotificationManager.class).createNotificationChannel(new NotificationChannel("audio","Cyn audio",NotificationManager.IMPORTANCE_LOW));
        handler.post(tick);
    }
    private AudioAttributes attributes() { return new AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build(); }
    @Override public IBinder onBind(Intent intent) { return new LocalBinder(); }
    /** Start the Android playback visualizer only with the user's permission. */
    void refreshVisualization() {
        if(!monitor||!enabled||playing||
           checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED) {
            releaseOutputVisuals();
            return;
        }
        if(outputVisuals!=null)return;
        long now=SystemClock.elapsedRealtime();
        if(now-visualRetryAt<7000)return;
        visualRetryAt=now;
        Visualizer visual=null;
        try {
            visual=new Visualizer(0);
            visual.setCaptureSize(Math.min(1024,Visualizer.getCaptureSizeRange()[1]));
            visual.setScalingMode(Visualizer.SCALING_MODE_AS_PLAYED);
            visual.setDataCaptureListener(new Visualizer.OnDataCaptureListener() {
                @Override public void onWaveFormDataCapture(Visualizer effect,byte[] wave,int samplingRate){
                    if(effect!=outputVisuals)return;
                    double[] levels=AudioFeatures.levels(wave);
                    measuredRms=levels[0];
                    measuredPeak=levels[1];
                }
                @Override public void onFftDataCapture(Visualizer effect,byte[] fft,int samplingRate){
                    if(effect!=outputVisuals||!monitor||!enabled||playing||destroyed)return;
                    float[] spectrum=AudioFeatures.spectrum(fft,samplingRate/1000);
                    double rms=measuredRms,peak=measuredPeak;
                    if(rms<.005)return;
                    double[] features=AudioFeatures.fromFft(fft,samplingRate/1000,rms,peak);
                    handler.post(()->{
                        if(destroyed||effect!=outputVisuals||!monitor||!enabled||playing)return;
                        long t=SystemClock.elapsedRealtime();
                        Chain c=chains.get(legacy?0:activeSession);
                        if(c!=null&&c.lastCapture>0&&t-c.lastCapture<500&&
                           sessionSpectrumAt>0&&t-sessionSpectrumAt<500)return;
                        sessionSpectrum=spectrum;
                        sessionSpectrumAt=t;
                        if(capturedAudioFresh())return;
                        if(smart){
                            PreferenceModel.Trace trace=learning.model.trace(features);
                            liveTrace=trace;traceAt=t;traceSequence++;
                            publishConnectedMind(features,rms,trace,t);
                            predictions=trace.outputs;
                            neurons=trace.hidden;
                            analysis="Live Android output-mix analysis · on-device";
                            if(t-lastFrame>=240){
                                lastFrame=t;
                                frames.add(new Frame(features,(float)Math.min(1,rms*4)));
                                while(frames.size()>240)frames.remove(0);
                            }
                        }else analysis="Live audio spectrum · AI disabled";
                    });
                }
            },Math.min(20000,Visualizer.getMaxCaptureRate()),true,true);
            outputVisuals=visual;
            visual.setEnabled(true);
            visualError="Waiting for audible Android output";
        }catch(RuntimeException|LinkageError e){
            if(visual!=null)try{visual.release();}catch(RuntimeException ignored){}
            outputVisuals=null;
            visualError="Android output-mix visualization unavailable";
        }
    }
    private void releaseOutputVisuals(){
        Visualizer old=outputVisuals;
        outputVisuals=null;
        measuredRms=0;measuredPeak=0;
        if(old!=null)try{old.setEnabled(false);old.release();}catch(RuntimeException ignored){}
    }
    @Override public int onStartCommand(Intent i,int flags,int startId) {
        if(i==null) { stopSelf(); return START_NOT_STICKY; }
        try {
            String action=i.getAction();
            // Screen recording / MediaProjection has been retired from CYN Android.
            // Reject even stale intents from an older APK instead of starting playback capture.
            if(CAPTURE.equals(action))return START_NOT_STICKY;
            if(STOP.equals(action)) { stopAll(); return START_NOT_STICKY; }
            if(MONITOR.equals(action)) {
                monitor=true; lastPlaybackRefresh=0; foreground();
                if(i.getBooleanExtra("legacy",false)) startLegacy();
                else {
                    if(legacy) { detach(0); legacy=false; }
                    int id=i.getIntExtra("session",-1);
                    if(id>0&&id!=playerSession) attach(id,"External playback");
                    status=connectionState();
                }
                refreshVisualization();
                updateNotification();
            }
            else if("cyn.QUEUE".equals(action)) { foreground(); }
            else if(PLAY.equals(action)&&i.getData()!=null) { foreground(); open(i.getData()); }
            else if(TOGGLE.equals(action)) { if(playing) pause(); else { foreground(); startPlayback(); } }
        } catch(Exception e) { status="Audio could not start: "+e.getClass().getSimpleName(); stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); }
        return START_NOT_STICKY;
    }
    void stopMonitor() {
        stopCapture(); monitor=false; legacy=false; activePlaybackPackages=Collections.emptySet(); handler.removeCallbacks(routeRecovery); releaseOutputVisuals(); connectedMind=null; sessionSpectrum=new float[31]; sessionSpectrumAt=0; clearTrace();
        for(Integer id:new ArrayList<>(chains.keySet())) if(id!=playerSession) detach(id);
        if(playing||preparing) foreground(); else { stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); }

        status=connectionState();
        updateNotification();
    }
    private void foreground() {
        int type=0;
        if(Build.VERSION.SDK_INT>=34) {
            type=monitor?ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE:ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK;
            if(monitor&&(playing||preparing)) type|=ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK;
        }

        if(Build.VERSION.SDK_INT>=29) startForeground(41,notification(),type); else startForeground(41,notification());
    }
    private PendingIntent action(String action) {
        return PendingIntent.getService(this,action.hashCode(),new Intent(this,AudioService.class).setAction(action),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
    }
    private Notification notification() {
        PendingIntent open=PendingIntent.getActivity(this,0,new Intent(this,MainActivity.class),PendingIntent.FLAG_UPDATE_CURRENT|PendingIntent.FLAG_IMMUTABLE);
        Notification.Builder b=new Notification.Builder(this,"audio").setSmallIcon(R.drawable.ic_notification).setContentTitle("Cyn · "+(player!=null?title:"Compatible apps"))
            .setContentText(playing?"Playing with your sound settings":monitor?connectionState():"Paused")
            .setContentIntent(open).setOnlyAlertOnce(true).setOngoing(playing||monitor).addAction(new Notification.Action.Builder(android.graphics.drawable.Icon.createWithResource(this,R.drawable.ic_stop),"Stop",action(STOP)).build());
        if(player!=null) b.addAction(new Notification.Action.Builder(android.graphics.drawable.Icon.createWithResource(this,playing?R.drawable.ic_pause:R.drawable.ic_play),playing?"Pause":"Play",action(TOGGLE)).build()).setStyle(new Notification.MediaStyle().setMediaSession(mediaSession.getSessionToken()).setShowActionsInCompactView(1,0));
        return b.build();
    }
    private void updateNotification() { if(playing||monitor||player!=null) getSystemService(NotificationManager.class).notify(41,notification()); }
    private void open(Uri uri) {
        List<AudioLibrary.Track> queue=library.all();int index=-1;
        for(int i=0;i<queue.size();i++)if(queue.get(i).uri.equals(uri))index=i;
        if(index<0){queue=Collections.singletonList(new AudioLibrary.Track(uri,"Audio file","","",0,false));index=0;}
        playQueue(queue,index);
    }
    void playQueue(List<AudioLibrary.Track> queue,int index) {
        if(queue.isEmpty()||index<0||index>=queue.size())return;
        releasePlayer();frames.clear();clearTrace();
        // Never stack the native library DSP with the optional output-mix EQ.
        if(legacy){detach(0);legacy=false;monitor=false;}
        try {
            androidx.media3.exoplayer.DefaultRenderersFactory factory=new androidx.media3.exoplayer.DefaultRenderersFactory(this){
                @Override protected androidx.media3.exoplayer.audio.AudioSink buildAudioSink(Context context,boolean floatOutput,boolean playbackParameters){
                    androidx.media3.common.audio.SonicAudioProcessor resampler=new androidx.media3.common.audio.SonicAudioProcessor();resampler.setOutputSampleRateHz(48000);
                    return new androidx.media3.exoplayer.audio.DefaultAudioSink.Builder(context).setEnableFloatOutput(false).setAudioProcessors(new androidx.media3.common.audio.AudioProcessor[]{resampler,new CynAudioProcessor(desktopAi.engine,pcmSpectrum)}).build();
                }
            };
            player=new androidx.media3.exoplayer.ExoPlayer.Builder(this,factory).build();
            player.setAudioAttributes(new androidx.media3.common.AudioAttributes.Builder().setUsage(androidx.media3.common.C.USAGE_MEDIA).setContentType(androidx.media3.common.C.AUDIO_CONTENT_TYPE_MUSIC).build(),false);
            player.setWakeMode(androidx.media3.common.C.WAKE_MODE_LOCAL);player.setHandleAudioBecomingNoisy(true);
            player.setTrackSelectionParameters(player.getTrackSelectionParameters().buildUpon().setMaxAudioChannelCount(2).build());
            player.setShuffleModeEnabled(prefs.getBoolean("shuffle",false));player.setRepeatMode(prefs.getInt("repeat",0));
            final androidx.media3.exoplayer.ExoPlayer current=player;
            player.addListener(new androidx.media3.common.Player.Listener(){
                @Override public void onPlaybackStateChanged(int state){
                    if(player!=current)return;preparing=state==androidx.media3.common.Player.STATE_BUFFERING;
                    if(state==androidx.media3.common.Player.STATE_READY){preparing=false;updateSession();}
                    if(state==androidx.media3.common.Player.STATE_ENDED){playing=false;pcmSpectrum.setActive(false);desktopAi.active=false;audio.abandonAudioFocusRequest(focus);updateSession();updateNotification();if(!monitor)stopForeground(STOP_FOREGROUND_DETACH);}
                }
                @Override public void onIsPlayingChanged(boolean value){if(player!=current)return;playing=value;pcmSpectrum.setActive(value);updateNativeControls();updateSession();updateNotification();}
                @Override public void onMediaItemTransition(androidx.media3.common.MediaItem item,int reason){if(item!=null&&item.mediaMetadata.title!=null)title=item.mediaMetadata.title.toString();frames.clear();clearTrace();desktopAi.reset();updateSession();updateNotification();}
                @Override public void onPlayerError(androidx.media3.common.PlaybackException error){status="Cannot play this track · "+error.getErrorCodeName();playing=false;preparing=false;desktopAi.active=false;audio.abandonAudioFocusRequest(focus);updateSession();updateNotification();if(!monitor)stopForeground(STOP_FOREGROUND_DETACH);}
            });
            ArrayList<androidx.media3.common.MediaItem> items=new ArrayList<>();
            for(AudioLibrary.Track track:queue)items.add(new androidx.media3.common.MediaItem.Builder().setUri(track.playbackUri()).setMediaId(track.uri.toString()).setMediaMetadata(new androidx.media3.common.MediaMetadata.Builder().setTitle(track.title).setArtist(track.artist).setAlbumTitle(track.album).build()).build());
            title=queue.get(index).title;player.setMediaItems(items,index,0);preparing=true;foreground();player.prepare();startPlayback();
        }catch(Exception e){status="Unable to play: "+e.getClass().getSimpleName();releasePlayer();if(!monitor)stopForeground(STOP_FOREGROUND_REMOVE);}
    }
    void next(){if(player!=null){player.seekToNextMediaItem();updateSession();}}
    void previous(){if(player!=null){if(player.getCurrentPosition()>3000)player.seekTo(0);else player.seekToPreviousMediaItem();updateSession();}}
    void shuffle(boolean on){prefs.edit().putBoolean("shuffle",on).apply();if(player!=null)player.setShuffleModeEnabled(on);}
    void repeat(int mode){prefs.edit().putInt("repeat",mode).apply();if(player!=null)player.setRepeatMode(mode);}
    String currentUri(){return player!=null&&player.getCurrentMediaItem()!=null?player.getCurrentMediaItem().mediaId:"";}
    void startPlayback() {
        if(player==null) return;
        try {
            // Android 15 requires visible activity or foreground service before focus.
            foreground();
            if(audio.requestAudioFocus(focus)!=AudioManager.AUDIOFOCUS_REQUEST_GRANTED) { status="Another app has audio focus"; if(!monitor) stopForeground(STOP_FOREGROUND_DETACH); return; }
            player.setVolume(1f); player.play(); playing=player.isPlaying(); status="Playing"; mediaSession.setActive(true); updateSession(); updateNotification();
        } catch(Exception e) { status="Playback unavailable"; releasePlayer(); if(!monitor) stopForeground(STOP_FOREGROUND_REMOVE); }
    }
    void pause() {
        resumeOnFocus=false;
        if(player!=null) { try { player.pause(); } catch(IllegalStateException ignored) { } }
        playing=false;pcmSpectrum.setActive(false); audio.abandonAudioFocusRequest(focus); updateSession(); updateNotification(); if(!monitor) stopForeground(STOP_FOREGROUND_DETACH);desktopAi.active=false;desktopAi.reset();
    }
    int position() { try { return player!=null?(int)Math.max(0,player.getCurrentPosition()):0; } catch(Exception e) { return 0; } }
    int duration() { try { return player!=null?(int)Math.max(0,player.getDuration()):0; } catch(Exception e) { return 0; } }
    void seek(int value) { if(player!=null&&!preparing) { player.seekTo(Math.max(0,Math.min(duration(),value))); desktopAi.reset();frames.clear(); updateSession(); } }
    private void updateSession() {
        mediaSession.setPlaybackState(new PlaybackState.Builder().setActions(PlaybackState.ACTION_PLAY|PlaybackState.ACTION_PAUSE|PlaybackState.ACTION_PLAY_PAUSE|PlaybackState.ACTION_STOP|PlaybackState.ACTION_SEEK_TO|PlaybackState.ACTION_SKIP_TO_NEXT|PlaybackState.ACTION_SKIP_TO_PREVIOUS).setState(playing?PlaybackState.STATE_PLAYING:PlaybackState.STATE_PAUSED,position(),playing?1:0).build());
        mediaSession.setMetadata(new MediaMetadata.Builder().putString(MediaMetadata.METADATA_KEY_TITLE,title).putLong(MediaMetadata.METADATA_KEY_DURATION,duration()).build());
    }
    private void releasePlayer() {
        if(playerSession>0) detach(playerSession); playerSession=-1;
        if(player!=null) { player.release(); player=null; } pcmSpectrum.setActive(false);desktopAi.active=false;desktopAi.reset();playing=false; preparing=false; audio.abandonAudioFocusRequest(focus);
    }
    void stopAll() { stopCapture(); monitor=false; legacy=false; activePlaybackPackages=Collections.emptySet(); handler.removeCallbacks(routeRecovery); connectedMind=null; releaseOutputVisuals(); sessionSpectrumAt=0; sessionSpectrum=new float[31]; clearTrace(); releasePlayer(); for(Integer id:new ArrayList<>(chains.keySet())) detach(id); mediaSession.setActive(false); stopForeground(STOP_FOREGROUND_REMOVE); stopSelf(); status="Stopped"; }
    private void attach(int id,String owner) { attach(id,owner,null); }
    private void attach(int id,String owner,String packageName) {
        if(id<0||(!legacy&&id==0)||(legacy&&id!=0)||chains.containsKey(id)) return;
        if(chains.size()>=8) { status="Too many audio sessions; close an unused player."; return; }
        clearTrace(); Chain c=new Chain(id,owner==null?"Music app":owner,packageName);
        chains.put(id,c); activeSession=id; frames.clear(); c.apply(); c.analyze();
        if(monitor) { status=connectionState(); updateNotification(); }
    }
    private void detach(int id) {
        clearTrace(); Chain c=chains.remove(id); if(c!=null) c.close();
        if(activeSession==id) {
            activeSession=chains.isEmpty()?-1:chains.keySet().iterator().next(); frames.clear();
            sessionSpectrum=new float[31]; sessionSpectrumAt=0;
        }
        if(monitor) { status=connectionState(); updateNotification(); }
    }
    void selectAiProfile(String selected) {
        aiProfile=AiSelection.normalize(selected);
        // Choosing the experimental APEX 10M release intentionally enables
        // its current neural controller as part of that primary engine.
        // Existing saved selections are left untouched until reselected.
        if(AiSelection.APEX_10M.equals(aiProfile))partialNeuralPreview=true;
        if(AiSelection.AUDONIC.equals(aiProfile))partialNeuralPreview=false;
        useApex=AiSelection.usesApex6M(aiProfile);
        spatialPreviewEnabled=AiSelection.usesSpatialPreview(aiProfile);
        if (desktopAi!=null) desktopAi.reset();
        if(connectedCapture!=null)connectedCapture.ai.reset();
        clearTrace();
        changed();
    }
    void changed() {
        // Derived from one canonical radio selection. Never permit two main AI
        // engines to be user-selected simultaneously after migration.
        aiProfile=AiSelection.normalize(aiProfile);
        useApex=AiSelection.usesApex6M(aiProfile);
        spatialPreviewEnabled=AiSelection.usesSpatialPreview(aiProfile);
        SharedPreferences.Editor e=prefs.edit().putString("ai_profile",aiProfile).putBoolean("apex10m_partial_opt_in",partialNeuralPreview).putBoolean("spatial10m_preview",spatialPreviewEnabled).putFloat("spatial10m_strength",spatialPreviewStrength).putFloat("audonic_intensity",audonicIntensity).putBoolean("enabled",enabled).putBoolean("smart",smart).putFloat("sensitivity",sensitivity).putFloat("bass",bass).putFloat("clarity",clarity).putFloat("width",width).putFloat("loudness",loudness);
        for(int i=0;i<5;i++) e.putFloat("eq"+i,eq[i]);
        e.putFloat("ambience",ambience).putFloat("dynamics",dynamics).putFloat("headroom",headroom).putInt("bands",bandCount).putBoolean("apex",useApex).putBoolean("movie",movieMode);
        for(int i=0;i<39;i++)e.putFloat("studioEq"+i,studioEq[i]);e.apply();updateNativeControls();
        for(Chain c:chains.values()) { c.apply(); c.analyze(); }
        if(monitor) { refreshVisualization(); status=connectionState(); updateNotification(); }
        if(!smart||!enabled) { clearTrace(); analysis="Analysis off · manual settings restored"; }
    }
    void updateNativeControls() {
        this.desktopAi.active=playing&&enabled&&smart;
        if(connectedCapture!=null) {
            connectedCapture.analyze=monitor&&!playing&&enabled;
            connectedCapture.ai.active=capturedAudioFresh()&&smart;
            if(!connectedCapture.ai.active)connectedCapture.ai.reset();
            PcmSpectrum.Frame spectrum=connectedCapture.spectrum.latest();
            if(capturedAudioFresh()&&spectrum.atNanos>0&&System.nanoTime()-spectrum.atNanos<350_000_000L){
                sessionSpectrum=spectrum.bands;sessionSpectrumAt=SystemClock.elapsedRealtime();
            }
        }
        DesktopAi desktopAi=currentAi();
        desktopAi.active=(playing||capturedAudioFresh())&&enabled&&smart;
        desktopAi.apexEnabled=useApex;
        desktopAi.audonicEnabled=AiSelection.AUDONIC.equals(aiProfile)&&desktopAi.active;
        desktopAi.movie=movieMode;
        boolean wantsPartial=spatialPreviewEnabled&&partialNeuralPreview&&desktopAi.active;
        if(desktopAi.partialEnabled!=wantsPartial) {
            desktopAi.partialEnabled=wantsPartial;
            desktopAi.partialPolicy=null;
        }
        DesktopAi.Result r=desktopAi.latest;
        boolean fresh=desktopAi.active&&r!=null&&SystemClock.elapsedRealtime()-r.at<2500;
        float[] p=new float[48];p[0]=enabled?1:0;p[1]=bass;p[2]=clarity;p[3]=width/100;p[4]=ambience;p[5]=dynamics;p[6]=loudness;p[7]=headroom;p[8]=bandCount;
        for(int b=0;b<bandCount;b++)p[9+b]=studioEq[b];
        if(fresh){
            double[] x=new double[11];for(int i=0;i<11;i++)x[i]=r.features[i];
            PreferenceModel.Trace trace=learning.model.trace(x);
            float[] preferences=Arrays.copyOf(r.decision,5);
            for(int o=0;o<5;o++)if(trace.ready[o])preferences[o]=.78f*preferences[o]+.22f*(float)trace.outputs[o];
            float peak=r.features[11]*(float)Math.exp(Math.min(3.5,r.features[8]));
            float[] targets=NativeEngine.effectTargets(new float[]{clarity/8*10,ambience*10,width/10,dynamics*10,bass/12*10},preferences,sensitivity*100,peak,movieMode);
            p[1]=targets[4]/10*12;p[2]=targets[0]/10*8;p[3]=targets[2]/10;p[4]=targets[1]/10;p[5]=targets[3]/10;
            float[] curve=Arrays.copyOfRange(r.decision,5,13);for(int i=0;i<8;i++)curve[i]+=r.apex[i];
            for(int b=0;b<bandCount;b++)p[9+b]+=sensitivity*ApexFeatures.interpolate(curve,(float)(20*Math.pow(1000,b/(double)(bandCount-1))));
            if(r.at!=libraryTrace){libraryTrace=r.at;liveTrace=trace;traceAt=SystemClock.elapsedRealtime();traceSequence++;predictions=trace.outputs;neurons=trace.hidden;
                if(!playing)publishConnectedMind(x,r.features[11],trace,traceAt);
            }
            if(r.at!=libraryFrame){libraryFrame=r.at;frames.add(new Frame(x,Math.min(1,r.features[11]*4)));while(frames.size()>240)frames.remove(0);}
        }
        if(!playing){
            DesktopAi.AudonicPolicy creative=desktopAi.audonicPolicy;
            ConnectedEffects.audonic(p,creative==null?null:creative.effects,audonicIntensity,
                desktopAi.audonicEnabled&&creative!=null&&SystemClock.elapsedRealtime()-creative.at<2500);
        }
        float changeDb=Math.max(Math.abs(p[1]-bass),Math.abs(p[2]-clarity));
        int changedBands=0;
        for(int b=0;b<bandCount;b++) {
            float difference=Math.abs(p[9+b]-studioEq[b]);
            changeDb=Math.max(changeDb,difference);
            if(difference>.05f)changedBands++;
        }
        aiDeltaDb=fresh?changeDb:0;
        aiEqBands=fresh?changedBands:0;
        System.arraycopy(p,0,appliedTargets,0,p.length);
        appliedTargetAt=SystemClock.elapsedRealtime();
        appliedAiFresh=fresh;
        if(playing||capturedAudioFresh())analysis=processingStatus();
        if(!playing)return; // Capture is analysis-only; Chain applies these targets to original playback.
        NativeEngine.controls(desktopAi.engine,p);
        // Only PCM from CYN library playback passes through this native processor.
        // Android external-app sessions cannot use this effect through the EQ API.
        float spatialStrength=enabled&&smart&&spatialPreviewEnabled?spatialPreviewStrength:0f;
        if(partialNeuralPreview&&spatialPreviewEnabled) {
            DesktopAi.PartialPolicy partial=desktopAi.partialPolicy;
            boolean verified=partial!=null&&SystemClock.elapsedRealtime()-partial.at<2500;
            // Fail closed: never fall back to unguarded widening while the user
            // has selected the experimental neural-guided mode.
            spatialStrength*=verified?partial.gain:0f;
        }
        NativeEngine.spatialPreview(desktopAi.engine,spatialStrength);
        float[] creative=new float[6];
        boolean audonicReady=false;
        if(desktopAi.audonicEnabled){
            DesktopAi.AudonicPolicy decision=desktopAi.audonicPolicy;
            audonicReady=decision!=null&&SystemClock.elapsedRealtime()-decision.at<2500;
            if(audonicReady){
                final float[] max={.55f,.65f,.45f,.55f,.25f,.50f};
                for(int j=0;j<6;j++){
                    float value=decision.effects[j];
                    creative[j]=Float.isFinite(value)
                        ?Math.max(0f,Math.min(max[j],value*audonicIntensity)):0f;
                }
            }
        }
        // Fail closed if weights, inference or audio features are unavailable.
        NativeEngine.audonicControls(desktopAi.engine,creative,audonicReady);
    }
    void train() {
        if(training) return; training=true; trainingEpoch=0; trainingTrace=null;
        PreferenceModel candidate=learning.model.copy(); ArrayList<PreferenceModel.Example> labels=new ArrayList<>(learning.examples);
        trainer.execute(()->{
            boolean success=candidate.fit(labels,epoch->{},(trace,epoch)->handler.post(()->{
                if(!destroyed) { trainingEpoch=epoch; trainingTrace=trace; }
            }));
            handler.post(()->{
                if(destroyed) return; training=false; trainingTrace=null; clearTrace();
                if(success) { learning.model=candidate; learning.message="Training complete · 240 real epochs on your feedback."; try { learning.save(); } catch(Exception e) { learning.message="Trained, but could not save the model."; } }
                else learning.message="Each effect needs at least five More and five Less frames.";
            });
        });
    }
    private final Runnable tick=new Runnable() {
        @Override public void run() {
            if(destroyed) return;
            long now=SystemClock.elapsedRealtime();
            if(monitor&&(lastPlaybackRefresh==0||now-lastPlaybackRefresh>=750)){
                Set<String> current=ExternalNowPlaying.playingPackages(AudioService.this);
                if(!activePlaybackPackages.equals(current)){
                    activePlaybackPackages=current;
                    for(Chain c:chains.values())c.lastSettings="";
                    updateNotification();
                }
                lastPlaybackRefresh=now;
            }
            updateNativeControls();
            for(Chain c:chains.values()) { if(SystemClock.elapsedRealtime()-c.lastCapture>900) c.targetBass=c.targetClarity=0; c.bassOffset+=(c.targetBass-c.bassOffset)*.12; c.clarityOffset+=(c.targetClarity-c.clarityOffset)*.12; c.apply(); }
            updateApexVisualTelemetry();
            if(monitor)refreshVisualization();
            if(!playing&&now-traceAt>900) { clearTrace(); if(smart&&enabled) analysis="Waiting for session analysis · enable permission if needed"; }
            long cutoff=System.nanoTime()-60_000_000_000L; while(!frames.isEmpty()&&frames.get(0).id<cutoff) frames.remove(0);
            handler.postDelayed(this,150);
        }
    };
    final class Chain {
        final int id; final String owner; final String packageName;
        Equalizer equalizer; Virtualizer virtualizer; LoudnessEnhancer enhancer; Visualizer visualizer;
        String failure=""; long lastCapture; double rms,peak,targetBass,targetClarity,bassOffset,clarityOffset;
        String lastSettings=""; float appliedWidth;
        Chain(int id,String owner,String packageName) {
            this.id=id; this.owner=owner; this.packageName=packageName;
            initializeEffects();
        }
        private void initializeEffects(){
            try { equalizer=new Equalizer(0,id); equalizer.setControlStatusListener((effect,control)->lastSettings=""); } catch(RuntimeException e) { failure="Equalizer unavailable on this session"; }
            try { virtualizer=new Virtualizer(0,id); } catch(RuntimeException ignored) { }
            try { enhancer=new LoudnessEnhancer(id); } catch(RuntimeException ignored) { }
        }
        void reopen(){
            close(); equalizer=null; virtualizer=null; enhancer=null; visualizer=null;
            failure=""; lastSettings=""; lastCapture=0; targetBass=targetClarity=bassOffset=clarityOffset=0;
            initializeEffects(); apply(); analyze();
        }
        void analyze() {
            if(!monitor||!enabled||checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED) {
                if(visualizer!=null) { visualizer.release(); visualizer=null; }
                targetBass=targetClarity=bassOffset=clarityOffset=0; return;
            }
            if(visualizer!=null) return;
            try {
                visualizer=new Visualizer(id); visualizer.setCaptureSize(Math.min(1024,Visualizer.getCaptureSizeRange()[1])); visualizer.setScalingMode(Visualizer.SCALING_MODE_AS_PLAYED);
                visualizer.setDataCaptureListener(new Visualizer.OnDataCaptureListener() {
                    @Override public void onWaveFormDataCapture(Visualizer v,byte[] wave,int rate) { double[] levels=AudioFeatures.levels(wave); rms=levels[0]; peak=levels[1]; }
                    @Override public void onFftDataCapture(Visualizer v,byte[] fft,int rate) {
                        if(destroyed||v!=visualizer||!monitor||!enabled||chains.get(id)!=Chain.this) return;
                        lastCapture=SystemClock.elapsedRealtime();
                        if(capturedAudioFresh())return;
                        if(!playing&&(id==0||id==activeSession)) {
                            if(rms>.01) {
                                sessionSpectrum=AudioFeatures.spectrum(fft,rate/1000);
                                sessionSpectrumAt=lastCapture;
                            } else {
                                sessionSpectrumAt=0;
                            }
                        }
                        if(!smart)return;
                        double[] x=AudioFeatures.fromFft(fft,rate/1000,rms,peak);
                        PreferenceModel.Trace trace=learning.model.trace(x); double[] p=trace.outputs;
                        // Do not boost silence. The model does not control Android volume or routing.
                        // External-player fallback: measured spectral balancing, not YAMNet.
                        // Personal model decisions take precedence once the user has trained them.
                        // Conservative bounded offsets avoid amplifying already strong frequency ranges.
                        double low=Math.exp(x[0])+Math.exp(x[1]);
                        double presence=Math.exp(x[4])+Math.exp(x[5]);
                        double spectralBass=PreferenceModel.clamp((.20-low)*5.5,-.65,1.45)*sensitivity;
                        double spectralClarity=PreferenceModel.clamp((.18-presence)*5,-.55,1.25)*sensitivity;
                        targetBass=rms>.01?(learning.model.ready[4]?
                            (p[4]*2-1)*Math.min(3,bass)*sensitivity:spectralBass):0;
                        targetClarity=rms>.01?(learning.model.ready[0]?
                            (p[0]*2-1)*Math.min(3,clarity)*sensitivity:spectralClarity):0;
                        if(!playing&&(id==0||id==activeSession)) {
                            if(rms>.01) {
                                liveTrace=trace; traceAt=lastCapture; traceSequence++;
                                publishConnectedMind(x,rms,trace,lastCapture);
                            } else clearTrace();
                            predictions=p; neurons=trace.hidden; analysis=rms>.01?"Live session analysis · features stay on this phone":"Waiting for audible audio";
                            if(rms>.01&&lastCapture-lastFrame>=240) { lastFrame=lastCapture; frames.add(new Frame(x,(float)Math.min(1,rms*4))); while(frames.size()>240) frames.remove(0); }
                        }
                    }
                },Math.min(20000,Visualizer.getMaxCaptureRate()),true,true); visualizer.setEnabled(true);
            } catch(RuntimeException e) { if(visualizer!=null) { visualizer.release(); visualizer=null; } analysis="Analysis unavailable for this session; manual effects still work."; }
        }
        void apply() {
            // The same Studio EQ and selected neural model now drive both routes.
            // Only use mix-wide AI on the explicitly selected system mix or the
            // sole external session. Never apply a mixed signal's AI to multiple unrelated players.
            // A captured external mix must never drive Library's own session.
            // Connect applies AI only to an eligible external effect session.
            // Never send mix-wide neural decisions to unrelated audio sessions.
            // A system-mix route is eligible only when explicitly selected;
            // otherwise exactly one active external session must be targeted.
            // MediaSession metadata alone cannot identify an AudioEffect session.
            // Prefer the explicit package extra of the player's OPEN broadcast.
            // A single unlabelled session retains the old compatibility fallback.
            boolean externalRoute=SessionEffectPolicy.allows(
                monitor,enabled,playing,legacy,id,playerSession,packageName,
                activePlaybackPackages,externalSessionCount());
            boolean spatialShared=externalRoute&&capturedAudioFresh()&&smart;
            boolean shared=spatialShared&&appliedAiFresh;
            float[] targets=appliedTargets.clone();
            if(!shared) {
                targets[1]=bass+(float)(smart?bassOffset:0);
                targets[2]=clarity+(float)(smart?clarityOffset:0);
                targets[3]=width/100;
                targets[8]=bandCount;
                for(int b=0;b<bandCount;b++)targets[9+b]=studioEq[b];
            }
            DesktopAi.PartialPolicy policy=currentAi().partialPolicy;
            boolean policyFresh=spatialShared&&policy!=null&&SystemClock.elapsedRealtime()-policy.at<2500;
            float spatial=ConnectedEffects.width(targets[3],policyFresh?policy.gain:0,
                spatialPreviewStrength,spatialShared&&spatialPreviewEnabled,partialNeuralPreview,policyFresh,sensitivity);
            // Fail closed for unselected sessions. A music metadata session does
            // not prove an effect session belongs to that player, so refuse
            // ambiguous routes rather than process every phone sound.
            boolean effectEnabled=enabled&&(id==playerSession||externalRoute);
            String signature=effectEnabled+":"+Arrays.toString(targets)+":"+spatial+":"+loudness;
            if(signature.equals(lastSettings)&&(!effectEnabled||controlsEq())) return;
            boolean effectsApplied=true;
            try {
                if(equalizer!=null) {
                    if(!equalizer.hasControl()) { failure="Another equalizer owns this session. Disable competing equalizers and reconnect."; effectsApplied=false; }
                    else if(equalizer.setEnabled(effectEnabled)!=AudioEffect.SUCCESS || equalizer.getEnabled()!=effectEnabled) { failure="Android rejected the requested EQ state"; effectsApplied=false; }
                    else {
                        short[] range=equalizer.getBandLevelRange();
                        for(short i=0;i<equalizer.getNumberOfBands();i++) {
                            float gain=ConnectedEffects.band(targets,equalizer.getCenterFreq(i)/1000f);
                            equalizer.setBandLevel(i,(short)ConnectedEffects.clamp(gain*100,Math.max(-600,range[0]),Math.min(600,range[1])));
                        }
                        failure="";
                    }
                } else {failure="Equalizer unavailable on this session";effectsApplied=false;}
            } catch(RuntimeException error) {failure="Effect control unavailable or owned by another equalizer";effectsApplied=false;}
            // Optional effect failure must not disable a functioning equalizer.
            appliedWidth=0;
            try {
                if(virtualizer!=null&&virtualizer.hasControl()) {
                    if(virtualizer.getStrengthSupported())virtualizer.setStrength((short)(spatial*1000));
                    if(virtualizer.setEnabled(effectEnabled&&spatial>0)==AudioEffect.SUCCESS&&virtualizer.getEnabled())
                        appliedWidth=virtualizer.getRoundedStrength()/1000f;
                }
            } catch(RuntimeException ignored) {effectsApplied=false;}
            try {
                if(enhancer!=null&&enhancer.hasControl()) {
                    enhancer.setEnabled(effectEnabled&&loudness>0);
                    enhancer.setTargetGain((int)(loudness*100));
                }
            } catch(RuntimeException ignored) {effectsApplied=false;}
            lastSettings=effectsApplied?signature:"";
        }
        void close() { if(visualizer!=null) visualizer.release(); if(equalizer!=null) equalizer.release(); if(virtualizer!=null) virtualizer.release(); if(enhancer!=null) enhancer.release(); }
        boolean controlsEq() { try { return enabled&&equalizer!=null&&equalizer.hasControl()&&equalizer.getEnabled()&&failure.isEmpty(); } catch(RuntimeException e) { return false; } }
        String description() { return owner+" · session "+id+"\n"+(controlsEq()?"EQ enabled and controlled":enabled?"EQ not active":"Enhancement disabled")+(virtualizer!=null?" · width":"")+(enhancer!=null?" · loudness":"")+(failure.isEmpty()?"":"\n"+failure); }
    }
    @Override public void onDestroy() {
        destroyed=true; stopCapture(); handler.removeCallbacksAndMessages(null); trainer.shutdownNow(); unregisterReceiver(receiver); audio.unregisterAudioDeviceCallback(deviceCallback); releaseOutputVisuals(); releasePlayer();
        for(Chain c:chains.values()) c.close(); chains.clear(); pcmSpectrum.close();mediaSession.release();desktopAi.closeAfterPlayer(); super.onDestroy();
    }
}
