// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.Manifest;
import android.app.*;
import android.content.*;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.graphics.*;
import android.graphics.drawable.*;
import android.media.audiofx.AudioEffect;
import android.net.Uri;
import android.os.*;
import android.view.*;
import android.widget.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;
import org.json.*;

public final class MainActivity extends Activity {
    AudioService service;
    final Handler handler=new Handler(Looper.getMainLooper());
    LinearLayout shell,content,nav;
    android.content.SharedPreferences ui;
    TextView liveStatus,learningStatus,playLabel,positionLabel,powerButton,connectionTitle,connectionHint,playButton;
    SeekBar position;
    Timeline timeline;
    NeuralView neural;
    int page=0, feedbackEffect=4;
    int bg=0xff171a20, card=0xff1e222a, text=0xfff0f3f7, hint=0xffabb6c5, accent=0xff6ad4e5, outline=0xff39424f;
    boolean bound, seeking, reduced;
    boolean checkingUpdate;
    JSONArray themes;
    final ServiceConnection connection=new ServiceConnection() {
        @Override public void onServiceConnected(ComponentName name,IBinder binder) { service=((AudioService.LocalBinder)binder).service(); build(); processControlIntent(getIntent()); }
        @Override public void onServiceDisconnected(ComponentName name) { service=null; build(); }
    };
    @Override public void onCreate(Bundle state) {
        super.onCreate(state); ui=getSharedPreferences("ui",0); if(state!=null) page=state.getInt("page",0);
        try { themes=new JSONArray(asset("themes.json")); } catch(Exception e) { themes=new JSONArray(); }
        palette(); build();
        bound=bindService(new Intent(this,AudioService.class),connection,BIND_AUTO_CREATE);
    }
    @Override protected void onSaveInstanceState(Bundle state) { super.onSaveInstanceState(state); state.putInt("page",page); }
    @Override protected void onNewIntent(Intent i) { super.onNewIntent(i); setIntent(i); processControlIntent(i); }
    void processControlIntent(Intent i) {
        if(service!=null&&AudioEffect.ACTION_DISPLAY_AUDIO_EFFECT_CONTROL_PANEL.equals(i.getAction())) {
            int session=i.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION,-1);
            startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("session",session)); i.setAction(null); page=0; build();
        }
    }
    String asset(String name) throws IOException { try(InputStream in=getAssets().open(name)) { return Streams.text(in); } }
    int dp(float v) { return Math.round(v*getResources().getDisplayMetrics().density); }
    void palette() {
        reduced=ui.getBoolean("reduce",false);
        JSONObject p=themes.optJSONObject(ui.getInt("theme",0)); if(p==null) return;
        bg=p.optInt("bg",bg); card=p.optInt("card",card); text=p.optInt("text",text); hint=p.optInt("hint",hint); accent=p.optInt("accent",accent); outline=p.optInt("outline",outline);
    }
    GradientDrawable shape(int color,int radius) { GradientDrawable d=new GradientDrawable(); d.setColor(color); d.setCornerRadius(dp(radius)); d.setStroke(dp(1),outline); return d; }
    TextView label(String value,int size,int color) {
        TextView t=new TextView(this); t.setText(value); t.setTextSize(size); t.setTextColor(color); t.setPadding(0,dp(4),0,dp(4)); t.setFontFeatureSettings("kern"); return t;
    }
    LinearLayout column() { LinearLayout l=new LinearLayout(this); l.setOrientation(LinearLayout.VERTICAL); return l; }
    void add(LinearLayout parent,View v) { parent.addView(v,new LinearLayout.LayoutParams(-1,-2)); }
    void gap(LinearLayout l,int height) { View v=new View(this); l.addView(v,new LinearLayout.LayoutParams(1,dp(height))); }
    TextView button(String title,Runnable run) {
        TextView b=label(title,14,text); b.setGravity(Gravity.CENTER); b.setMinHeight(dp(48)); b.setPadding(dp(12),dp(10),dp(12),dp(10)); b.setAccessibilityDelegate(new View.AccessibilityDelegate() {
            @Override public void onInitializeAccessibilityNodeInfo(View host,android.view.accessibility.AccessibilityNodeInfo info) { super.onInitializeAccessibilityNodeInfo(host,info); info.setClassName(Button.class.getName()); }
        });
        b.setBackground(new RippleDrawable(ColorStateList.valueOf((accent&0xffffff)|0x33000000),shape(card,16),null)); b.setClickable(true); b.setFocusable(true); b.setOnClickListener(v->run.run()); return b;
    }
    void animateRows(LinearLayout l) {
        if(reduced) return;
        for(int i=0;i<l.getChildCount();i++) { View v=l.getChildAt(i); v.setAlpha(0); v.setTranslationY(dp(10)); v.animate().alpha(1).translationY(0).setStartDelay(Math.min(i,8)*24L).setDuration(210).start(); }
    }
    LinearLayout section(String title,String description) {
        LinearLayout c=column(); c.setPadding(dp(18),dp(16),dp(18),dp(16)); c.setBackground(shape(card,24));
        LinearLayout.LayoutParams p=new LinearLayout.LayoutParams(-1,-2); p.setMargins(0,dp(8),0,dp(8)); content.addView(c,p);
        TextView h=label(title,18,text); h.setTypeface(null,Typeface.BOLD); add(c,h);
        if(!description.isEmpty()) add(c,label(description,12,hint)); gap(c,8); return c;
    }
    void build() {
        palette(); liveStatus=learningStatus=playLabel=positionLabel=powerButton=connectionTitle=connectionHint=playButton=null; position=null; timeline=null; neural=null;
        getWindow().setStatusBarColor(bg); getWindow().setNavigationBarColor(bg);
        shell=column(); shell.setBackgroundColor(bg); setContentView(shell);
        shell.setOnApplyWindowInsetsListener((v,insets)->{
            if(Build.VERSION.SDK_INT>=30) { android.graphics.Insets edges=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout()); v.setPadding(edges.left,edges.top,edges.right,edges.bottom); }
            else v.setPadding(insets.getSystemWindowInsetLeft(),insets.getSystemWindowInsetTop(),insets.getSystemWindowInsetRight(),insets.getSystemWindowInsetBottom());
            return Build.VERSION.SDK_INT>=30?WindowInsets.CONSUMED:insets.consumeSystemWindowInsets();
        }); shell.requestApplyInsets();
        LinearLayout header=new LinearLayout(this); header.setGravity(Gravity.CENTER_VERTICAL); header.setPadding(dp(20),dp(12),dp(20),dp(6));
        ImageView icon=new ImageView(this); icon.setImageResource(R.drawable.cyn_icon); icon.setContentDescription("Cyn"); header.addView(icon,new LinearLayout.LayoutParams(dp(44),dp(44)));
        LinearLayout wordmark=column(); add(wordmark,label("CYN / AUDIO",15,text)); add(wordmark,label("YOUR SOUND. YOUR WAY.",9,hint)); header.addView(wordmark,new LinearLayout.LayoutParams(0,-2,1));
        TextView preview=label("ANDROID\nPREVIEW",9,accent); preview.setGravity(Gravity.END); header.addView(preview); add(shell,header);
        ScrollView scroll=new ScrollView(this); scroll.setFillViewport(true); scroll.setClipToPadding(false); shell.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        content=column(); content.setPadding(dp(18),dp(8),dp(18),dp(20)); scroll.addView(content);
        if(service==null) add(content,label("Connecting to Cyn…",18,text));
        else if(page==0) music(); else if(page==1) learning(); else if(page==2) files(); else appearance();
        nav=new LinearLayout(this); nav.setPadding(dp(12),dp(8),dp(12),dp(8));
        String[] titles={"Music","Learn","Files","More"};
        for(int i=0;i<titles.length;i++) { final int target=i; TextView b=button(titles[i],()->{ page=target; build(); }); b.setTextSize(13); b.setPadding(dp(4),dp(12),dp(4),dp(12)); b.setMinHeight(dp(52)); b.setTextColor(page==i?accent:hint); b.setSelected(page==i); b.setTypeface(null,page==i?Typeface.BOLD:Typeface.NORMAL); LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-1,1); lp.setMargins(dp(3),0,dp(3),0); nav.addView(b,lp); }
        add(shell,nav); animateRows(content); update();
    }
    void toggle(LinearLayout parent,String title,boolean checked,java.util.function.Consumer<Boolean> change) {
        Switch s=new Switch(this); s.setText(title); s.setTextColor(text); s.setTextSize(14); s.setMinHeight(dp(48)); s.setChecked(checked); s.setThumbTintList(ColorStateList.valueOf(accent)); s.setOnCheckedChangeListener((b,on)->change.accept(on)); add(parent,s);
    }
    void slider(LinearLayout parent,String title,float value,float min,float max,String suffix,java.util.function.Consumer<Float> change) {
        TextView caption=label(title+"   "+String.format(Locale.ROOT,"%.1f",value)+suffix,13,text); add(parent,caption);
        SeekBar bar=new SeekBar(this); bar.setMax(120); bar.setProgress(Math.round((value-min)/(max-min)*120)); bar.setProgressTintList(ColorStateList.valueOf(accent)); bar.setThumbTintList(ColorStateList.valueOf(accent)); bar.setContentDescription(title); bar.setMinimumHeight(dp(48));
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar b,int n,boolean user) { if(user) { float v=min+(max-min)*n/120f; caption.setText(title+"   "+String.format(Locale.ROOT,"%.1f",v)+suffix); change.accept(v); } }
            public void onStartTrackingTouch(SeekBar b) {} public void onStopTrackingTouch(SeekBar b) {}
        }); add(parent,bar);
    }
    void category(LinearLayout parent,String name,java.util.function.Consumer<LinearLayout> fill) {
        LinearLayout body=column(); body.setVisibility(View.GONE); fill.accept(body);
        TextView header=button(name+"  ›",()->{
            boolean open=body.getVisibility()!=View.VISIBLE; body.setVisibility(open?View.VISIBLE:View.GONE);
            if(open) animateRows(body);
        }); add(parent,header); add(parent,body); gap(parent,8);
    }
    void files() {
        add(content,label("Audio files",30,text)); add(content,label("An extra player for songs saved on your phone.",13,hint));
        LinearLayout source=section("File player","Choose one audio file to play inside Cyn.");
        playLabel=label(service.title,16,text); add(source,playLabel);
        positionLabel=label("0:00 / 0:00",12,hint); add(source,positionLabel);
        position=new SeekBar(this); position.setContentDescription("Song position"); position.setMinimumHeight(dp(48)); position.setProgressTintList(ColorStateList.valueOf(accent)); position.setThumbTintList(ColorStateList.valueOf(accent));
        position.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() { public void onProgressChanged(SeekBar b,int n,boolean u) {} public void onStartTrackingTouch(SeekBar b) { seeking=true; } public void onStopTrackingTouch(SeekBar b) { seeking=false; service.seek(b.getProgress()); } }); add(source,position);
        add(source,button("Open audio file",()->startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("audio/*").addCategory(Intent.CATEGORY_OPENABLE).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION),10)));
        gap(source,8); playButton=button("Play",()->{ if(service.player==null) toast("Open an audio file first."); else if(service.playing) service.pause(); else startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.TOGGLE)); }); add(source,playButton);
        liveStatus=label("",12,accent); add(source,liveStatus);
        toggle(source,"Apply sound settings",service.enabled,on->{ service.enabled=on; service.changed(); });
        add(source,button("Adjust sound",()->{ page=0; build(); }));
    }
    void enableMusic(boolean legacy) {
        notifications();
        startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("legacy",legacy));
    }
    void music() {
        add(content,label("Your music",30,text));
        LinearLayout source=section("Cyn for your music apps","");
        connectionTitle=label("",18,accent); connectionTitle.setTypeface(null,Typeface.BOLD); connectionTitle.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE); add(source,connectionTitle);
        powerButton=button("Enable Cyn",()->{
            if(service.monitor&&service.enabled) { service.enabled=false; service.changed(); service.stopMonitor(); update(); }
            else enableMusic(false);
        });
        powerButton.setTextSize(18); powerButton.setTypeface(null,Typeface.BOLD); powerButton.setMinHeight(dp(60));
        powerButton.setTextColor(Color.luminance(accent)>.179?Color.BLACK:Color.WHITE);
        powerButton.setBackground(new RippleDrawable(ColorStateList.valueOf(0x33000000),shape(accent,18),null)); add(source,powerButton);
        gap(source,8); connectionHint=label("",14,hint); add(source,connectionHint);
        add(source,button("Connection help",this::connectionHelp));
        LinearLayout sound=section("Your sound","Bass, clarity and EQ for your music.");
        category(sound,"Presets",body->{
            String[] names={"Balanced","Bass focus","Clear vocals","Soft listening"};
            float[][] values={{2,1,0,0},{5,1,0,0},{1,4,0,0},{0,1,0,0}};
            for(int i=0;i<names.length;i++) { final int n=i; add(body,button(names[i],()->{ service.bass=values[n][0]; service.clarity=values[n][1]; service.width=values[n][2]; service.loudness=values[n][3]; Arrays.fill(service.eq,0); service.changed(); build(); })); }
        });
        slider(sound,"Bass",service.bass,0,6," dB",v->{ service.bass=v; service.changed(); });
        slider(sound,"Clarity",service.clarity,0,6," dB",v->{ service.clarity=v; service.changed(); });
        category(sound,"More effects",body->{
            slider(body,"Stereo width",service.width,0,100,"%",v->{ service.width=v; service.changed(); });
            slider(body,"Loudness",service.loudness,0,3," dB",v->{ service.loudness=v; service.changed(); });
            add(body,label("Width uses Android's virtualizer where available. Loudness is a manual gain control, not desktop Dynamic Boost or automatic volume leveling.",12,hint));
        });
        category(sound,"Equalizer",body->{
            String[] names={"60 Hz","235 Hz","917 Hz","3.58 kHz","14 kHz"};
            for(int i=0;i<5;i++) { final int n=i; slider(body,names[i],service.eq[i],-6,6," dB",v->{ service.eq[n]=v; service.changed(); }); }
            add(body,label("The curve is mapped onto your device's available equalizer bands. Combined boosts are limited to 6 dB.",12,hint));
        });
        LinearLayout smart=section("Smart listening","Learn when you prefer more or less bass and clarity.");
        toggle(smart,"Personal adjustments",service.smart,on->{ service.smart=on; service.changed(); if(on) requestAnalysis(); });
        slider(smart,"Sensitivity",service.sensitivity*100,0,100,"%",v->{ service.sensitivity=v/100; service.changed(); });
        add(smart,button("Open learning timeline",()->{ page=1; build(); }));
        add(content,button("Stop all Cyn audio",()->service.stopAll()));
    }
    void connectionHelp() {
        LinearLayout help=column(); help.setPadding(dp(20),dp(12),dp(20),dp(12)); help.setBackgroundColor(card);
        add(help,label("1. Tap Enable Cyn on the Music screen.\n\n2. Start or restart a song in your music app.\n\n3. In that app's audio or equalizer settings, choose Cyn if it is offered.",15,text));
        add(help,label("Android must share the player's audio session with Cyn. Support varies by app and phone; Spotify is not guaranteed. Remote Spotify Connect playback cannot be processed on this phone.",13,hint));
        add(help,button("Connection diagnostics",()->showText("Audio connection",service.diagnostics()))); gap(help,8);
        add(help,button("Try legacy compatibility",()->new AlertDialog.Builder(this).setTitle("Try output-mix effects?").setMessage("For players that do not share a session, this requests Android's legacy output-mix equalizer. It can affect all music on the phone and may not work on Samsung firmware. Turn off other equalizers first. Disable Cyn stops these effects; Cyn never changes your device volume.").setPositiveButton("Try",(d,w)->enableMusic(true)).setNegativeButton("Cancel",null).show()));
        ScrollView scroll=new ScrollView(this); scroll.addView(help); new AlertDialog.Builder(this).setTitle("Connect your music app").setView(scroll).setPositiveButton("Done",null).show();
    }
    void requestAnalysis() {
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED) new AlertDialog.Builder(this).setTitle("Allow audio analysis?").setMessage("Android requires microphone permission for its session visualizer. Cyn uses it to read the playing audio session, never the microphone. Only numerical features and your labels are stored locally; no recordings or uploads.").setPositiveButton("Continue",(d,w)->requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO},21)).setNegativeButton("Later",null).show();
        else service.changed();
    }
    void notifications() { if(Build.VERSION.SDK_INT>=33&&checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)!=PackageManager.PERMISSION_GRANTED) requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS},22); }
    @Override public void onRequestPermissionsResult(int code,String[] permissions,int[] results) { super.onRequestPermissionsResult(code,permissions,results); if(service!=null) service.changed(); }
    void learning() {
        add(content,label("Make it personal",30,text)); add(content,label("Teach Cyn how you like a moment to feel.",13,hint));
        LinearLayout c=section("Listening timeline","Drag across the live history to freeze and select a section. Only features are kept, for up to 60 seconds.");
        timeline=new Timeline(); c.addView(timeline,new LinearLayout.LayoutParams(-1,dp(130)));
        add(c,button("Select the last 5 seconds",()->timeline.selectRecent())); gap(c,8);
        add(c,button("Resume live timeline",()->timeline.resume()));
        category(c,"Feedback effect",body->{
            add(body,button("Bass",()->{ feedbackEffect=4; toast("Feedback target: Bass"); }));
            add(body,button("Clarity",()->{ feedbackEffect=0; toast("Feedback target: Clarity"); }));
        });
        add(c,button("More here",()->labelFrames(true))); gap(c,8); add(c,button("Less here",()->labelFrames(false)));
        learningStatus=label("",12,accent); add(c,learningStatus);
        add(c,button("Train my model",()->{ if(service.training) toast("Training is already running."); else service.train(); }));
        LinearLayout network=section("Local neural activity","Actual inputs → hidden activations → outputs from one forward pass. Cyan is positive; amber is negative. No audio goes to a server.");
        neural=new NeuralView(); network.addView(neural,new LinearLayout.LayoutParams(-1,dp(220))); liveStatus=label("",12,hint); add(network,liveStatus);
        add(network,button("Enable audio analysis",()->{ service.smart=true; service.changed(); requestAnalysis(); }));
        LinearLayout starter=section("Your starting point","Personal learning starts neutral. Bass and clarity adapt after you train; width and loudness remain manual in this Android preview.");
        add(starter,label("For each effect, label at least five frames More and five Less across varied songs. Android captures a mono, processed spectrum; this differs from desktop Cyn's input analysis.",12,hint));
        add(starter,button("Try the experimental desktop starter",()->new AlertDialog.Builder(this).setTitle("Try starter weights?").setMessage("This is the v2.0.16 neural model trained on synthetic audio, not real songs. Its accuracy on Android has not been validated. This replaces current weights but keeps your feedback.").setPositiveButton("Load starter",(d,w)->{ if(service.training) { toast("Wait for training to finish."); return; } try { service.learning.starter(); } catch(Exception e) { toast("Could not load starter: "+e.getMessage()); } }).setNegativeButton("Cancel",null).show()));
        gap(starter,8); add(starter,button("Reset personal learning",()->new AlertDialog.Builder(this).setTitle("Clear local learning?").setMessage("This removes your feedback and learned weights on this phone.").setPositiveButton("Reset",(d,w)->{ if(service.training) { toast("Wait for training to finish."); return; } try { service.learning.reset(); } catch(Exception e) { toast("Could not save reset."); } }).setNegativeButton("Cancel",null).show()));
    }
    void labelFrames(boolean more) {
        if(service.training) { toast("Wait for training to finish before adding feedback."); return; }
        List<AudioService.Frame> selected=timeline.selection(); if(selected.isEmpty()) { toast("Play music, enable analysis, then select a section."); return; }
        try { service.learning.label(selected,feedbackEffect,more); toast((more?"More ":"Less ")+(feedbackEffect==4?"bass":"clarity")+" · "+selected.size()+" frames saved"); } catch(Exception e) { toast("Could not save feedback."); }
    }
    void appearance() {
        add(content,label("More",30,text)); add(content,label("Themes, updates and app information.",13,hint));
        LinearLayout c=section("Appearance","Themes adapted from your current desktop Cyn.");
        String current=themes.optJSONObject(ui.getInt("theme",0))==null?"Graphite":themes.optJSONObject(ui.getInt("theme",0)).optString("name");
        category(c,"Theme · "+current,body->{
            for(int i=0;i<themes.length();i++) { final int id=i; JSONObject theme=themes.optJSONObject(i); TextView b=button(theme.optString("name"),()->{ ui.edit().putInt("theme",id).apply(); build(); }); b.setTextColor(theme.optInt("accent",accent)); add(body,b); }
        });
        toggle(c,"Reduce motion",reduced,on->{ ui.edit().putBoolean("reduce",on).apply(); reduced=on; });
        LinearLayout about=section("Cyn for Android",BuildConfig.VERSION_NAME+" · build "+BuildConfig.VERSION_CODE);
        add(about,label("Thank you, FxSound, for the open-source foundation. Android uses Android audio effects; it does not contain the Windows FxSound driver or claim identical DSP. Windows features such as AI Hub sharing and driver routing are not included in this preview.",13,hint));
        add(about,button("Check for Android updates",this::checkUpdates)); gap(about,8);
        add(about,label("Install newer APKs over this app to retain your feedback. Android verifies the app identity, signing key and version. Windows releases are separate.",12,hint));
        add(about,button("Support FxSound",()->browser("https://www.fxsound.com/donate"))); gap(about,8);
        add(about,button("Cyn source on GitHub",()->browser("https://github.com/oddfoxerr-real/Cyn"))); gap(about,8);
        add(about,button("Licenses and credits",()->{ try { showText("Licenses and credits",asset("NOTICE.txt")+"\n\n"+asset("LICENSE.txt")); } catch(Exception e) { toast("License file unavailable."); } }));
        add(about,label("Only update checks contact GitHub when you tap Check for Android updates. Audio and feedback are never uploaded. Files are opened through Android's file picker. Personal feedback stays in app-private storage and is removed when the app is uninstalled.",12,hint));
    }
    void checkUpdates() {
        if(checkingUpdate) { toast("An update check is already running."); return; }
        checkingUpdate=true; toast("Checking Android releases…");
        new Thread(()->{
            String found=null,problem=null;
            try {
                for(int page=1;page<=3;page++) {
                    javax.net.ssl.HttpsURLConnection connection=(javax.net.ssl.HttpsURLConnection)new java.net.URL("https://api.github.com/repos/oddfoxerr-real/Cyn/releases?per_page=50&page="+page).openConnection();
                    connection.setConnectTimeout(12000); connection.setReadTimeout(12000);
                    connection.setRequestProperty("User-Agent","Cyn-Android/"+BuildConfig.VERSION_NAME);
                    connection.setRequestProperty("Accept","application/vnd.github+json");
                    JSONArray releases;
                    try {
                        if(connection.getResponseCode()!=200) throw new IOException("GitHub returned "+connection.getResponseCode());
                        try(InputStream input=connection.getInputStream(); ByteArrayOutputStream output=new ByteArrayOutputStream()) {
                            byte[] buffer=new byte[8192]; int count;
                            while((count=input.read(buffer))!=-1) { if(output.size()+count>4*1024*1024) throw new IOException("Release response too large"); output.write(buffer,0,count); }
                            releases=new JSONArray(output.toString("UTF-8"));
                        }
                    } finally { connection.disconnect(); }
                    for(int i=0;i<releases.length();i++) {
                        JSONObject release=releases.getJSONObject(i);
                        if(release.optBoolean("draft")||release.isNull("published_at")) continue;
                        String v=AndroidRelease.newerVersion(release.optString("tag_name"),BuildConfig.VERSION_NAME);
                        if(v==null) continue;
                        JSONArray assets=release.optJSONArray("assets"); if(assets==null) continue;
                        for(int a=0;a<assets.length();a++) {
                            JSONObject asset=assets.getJSONObject(a);
                            if("uploaded".equals(asset.optString("state"))&&AndroidRelease.assetUrl(v).equals(asset.optString("browser_download_url"))&&
                                (found==null||AndroidRelease.number(v)>AndroidRelease.number(found))) found=v;
                        }
                    }
                    if(releases.length()<50) break;
                }
            } catch(Exception e) { problem=e.getMessage(); }
            final String version=found,error=problem;
            handler.post(()->{
                checkingUpdate=false; if(isFinishing()||isDestroyed()) return;
                if(error!=null) { showText("Update check failed",error+"\nTry again later. Your installed app has not changed."); return; }
                if(version==null) { toast("Cyn "+BuildConfig.VERSION_NAME+" is the newest available Android preview."); return; }
                new AlertDialog.Builder(this).setTitle("Cyn "+version+" is available").setMessage("Download the APK, then open it and choose Update. Keep this app installed so your settings and personal model are preserved. Android will ask you to confirm installation.")
                    .setPositiveButton("Download update",(d,w)->browser(AndroidRelease.assetUrl(version))).setNegativeButton("Later",null).show();
            });
        },"Cyn-update-check").start();
    }
    void browser(String url) { try { startActivity(new Intent(Intent.ACTION_VIEW,Uri.parse(url))); } catch(ActivityNotFoundException e) { toast("No browser is installed."); } }
    void showText(String title,String value) { ScrollView s=new ScrollView(this); s.setBackgroundColor(card); TextView t=label(value,13,text); t.setPadding(dp(20),dp(12),dp(20),dp(12)); t.setTextIsSelectable(true); s.addView(t); new AlertDialog.Builder(this).setTitle(title).setView(s).setPositiveButton("Close",null).show(); }
    void toast(String s) { Toast.makeText(this,s,Toast.LENGTH_LONG).show(); }
    static String time(int ms) { return String.format(Locale.ROOT,"%d:%02d",ms/60000,(ms/1000)%60); }
    void update() {
        if(service==null) return;
        if(powerButton!=null) {
            boolean on=service.monitor&&service.enabled, connected=false, active=false;
            for(AudioService.Chain c:service.chains.values()) if(c.id!=service.playerSession) { connected=true; if(c.controlsEq()) active=true; }
            setLabel(powerButton,on?"Disable Cyn":"Enable Cyn");
            setLabel(connectionTitle,!on?"Cyn is off":active?"Sound settings connected":connected?"Connected · effects unavailable":"Waiting for your music app");
            setLabel(connectionHint,!on?"Enable Cyn, then play music in your usual app.":active?"Cyn has control of the equalizer. Adjust your sound below.":connected?"Another equalizer or this phone may be blocking effects. Open Connection help.":"Start or restart a song in your music app. Choose Cyn in its equalizer settings if offered.");
        }
        if(playLabel!=null) playLabel.setText(service.title+(service.playing?"  ·  Playing":service.preparing?"  ·  Opening":""));
        if(playButton!=null) { setLabel(playButton,service.preparing?"Opening…":service.playing?"Pause":"Play"); playButton.setEnabled(service.player!=null&&!service.preparing); playButton.setAlpha(playButton.isEnabled()?1f:.5f); }
        if(position!=null&&!seeking) { position.setMax(Math.max(1,service.duration())); position.setProgress(service.position()); }
        if(positionLabel!=null) positionLabel.setText(time(service.position())+" / "+time(service.duration()));
        if(liveStatus!=null) {
            StringBuilder state=new StringBuilder(page==1?service.analysis:service.status);
            if(page==2) { AudioService.Chain c=service.chains.get(service.playerSession); if(c!=null) state.append("\n").append(c.description()); }
            if(page==1) state.append("\n").append(service.neuralStatus());
            liveStatus.setText(state.toString());
        }
        if(learningStatus!=null) {
            int more=0,less=0; for(PreferenceModel.Example e:service.learning.examples) if(e.effect==feedbackEffect) { if(e.more) more++; else less++; }
            learningStatus.setText((feedbackEffect==4?"Bass":"Clarity")+" · "+more+" More / "+less+" Less\n"+(service.training?"Training: "+service.trainingEpoch+" / 240 epochs":service.learning.message));
        }
        if(timeline!=null) timeline.invalidate(); if(neural!=null) neural.invalidate();
    }
    void setLabel(TextView view,String value) { if(!value.contentEquals(view.getText())) view.setText(value); }
    final Runnable refresh=new Runnable() { public void run() { update(); handler.postDelayed(this,250); } };
    @Override protected void onResume() { super.onResume(); handler.post(refresh); }
    @Override protected void onPause() { handler.removeCallbacks(refresh); super.onPause(); }
    @Override protected void onDestroy() { handler.removeCallbacksAndMessages(null); if(bound) unbindService(connection); super.onDestroy(); }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        super.onActivityResult(request,result,data);
        if(request==10&&result==RESULT_OK&&data!=null&&data.getData()!=null) {
            Uri uri=data.getData(); try { getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION); } catch(SecurityException ignored) { }
            notifications(); startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.PLAY).setData(uri));
        }
    }
    final class Timeline extends View {
        final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG); ArrayList<AudioService.Frame> frozen; float start=0,end=1;
        Timeline() { super(MainActivity.this); setContentDescription("Audio feature timeline. Drag to select a section, or use Select the last five seconds."); setFocusable(true); }
        ArrayList<AudioService.Frame> displayed() { return frozen==null?service.frames:frozen; }
        void resume() { frozen=null; invalidate(); }
        void selectRecent() { frozen=new ArrayList<>(service.frames); long cutoff=System.nanoTime()-5_000_000_000L; frozen.removeIf(f->f.id<cutoff); start=0; end=1; invalidate(); }
        List<AudioService.Frame> selection() {
            if(frozen==null||frozen.isEmpty()) return Collections.emptyList(); int a=Math.max(0,(int)(Math.min(start,end)*frozen.size())); int b=Math.min(frozen.size(),Math.max(a+1,(int)Math.ceil(Math.max(start,end)*frozen.size()))); return new ArrayList<>(frozen.subList(Math.min(a,frozen.size()-1),b));
        }
        @Override protected void onDraw(Canvas c) {
            super.onDraw(c); ArrayList<AudioService.Frame> frames=displayed(); float w=getWidth(),h=getHeight();
            if(frames.isEmpty()) { p.setColor(hint); p.setTextSize(dp(12)); c.drawText("Play music and enable audio analysis",dp(8),h/2,p); return; }
            if(frozen!=null) { p.setColor((accent&0xffffff)|0x33000000); c.drawRoundRect(Math.min(start,end)*w,dp(5),Math.max(start,end)*w,h-dp(5),dp(10),dp(10),p); }
            int count=Math.min(80,frames.size()); float step=w/count; p.setStrokeWidth(Math.max(dp(2),step*.45f)); p.setStrokeCap(Paint.Cap.ROUND); p.setColor(accent);
            for(int i=0;i<count;i++) { AudioService.Frame f=frames.get(i*frames.size()/count); float height=Math.max(dp(3),f.level*(h-dp(28))); c.drawLine((i+.5f)*step,(h-height)/2,(i+.5f)*step,(h+height)/2,p); }
        }
        @Override public boolean onTouchEvent(android.view.MotionEvent event) {
            float x=Math.max(0,Math.min(.9999f,event.getX()/Math.max(1,getWidth())));
            if(event.getAction()==MotionEvent.ACTION_DOWN) { if(frozen==null) frozen=new ArrayList<>(service.frames); start=end=x; getParent().requestDisallowInterceptTouchEvent(true); }
            else if(event.getAction()==MotionEvent.ACTION_MOVE) end=x;
            else if(event.getAction()==MotionEvent.ACTION_UP) { end=x; getParent().requestDisallowInterceptTouchEvent(false); performClick(); }
            else if(event.getAction()==MotionEvent.ACTION_CANCEL) getParent().requestDisallowInterceptTouchEvent(false);
            invalidate(); return true;
        }
        @Override public boolean performClick() { super.performClick(); return true; }
    }
    final class NeuralView extends View {
        final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
        final double[] direction=new double[5];
        final String[] names={"Clarity","Ambience","Surround","Dynamic","Bass"};
        NeuralView() { super(MainActivity.this); setContentDescription("Measured 11 input, 12 hidden and 5 output neural activations; no decorative animation."); }
        float x(int i,int count) { return dp(10)+(getWidth()-dp(20))*i/(count-1f); }
        void edges(Canvas c,double[] values,int from,int to,float y1,float y2) {
            p.setStrokeWidth(dp(.8f));
            for(int j=0;j<to;j++) for(int i=0;i<from;i++) {
                double v=values[j*from+i]; p.setColor(v<0?0xffeeb060:accent); p.setAlpha(10+(int)(170*(1-Math.exp(-Math.abs(v)))));
                c.drawLine(x(i,from),y1,x(j,to),y2,p);
            }
        }
        void nodes(Canvas c,double[] values,float y) {
            for(int i=0;i<values.length;i++) { double v=values[i]; p.setColor(v<0?0xffeeb060:accent); p.setAlpha(35+(int)(220*Math.min(1,Math.abs(v)))); c.drawCircle(x(i,values.length),y,dp(3.5f),p); }
        }
        @Override protected void onDraw(Canvas c) {
            super.onDraw(c); PreferenceModel.Trace t=service.neuralTrace();
            p.setAlpha(255); p.setColor(hint); p.setTextSize(dp(12));
            if(t==null) { c.drawText(service.training?"Waiting for a training epoch":"Waiting for fresh audio analysis",dp(4),getHeight()/2f,p); return; }
            float y1=dp(20),y2=getHeight()*.42f,y3=getHeight()-dp(55);
            edges(c,t.inputConnections,11,12,y1,y2); edges(c,t.outputConnections,12,5,y2,y3);
            nodes(c,t.inputs,y1); nodes(c,t.hidden,y2);
            for(int i=0;i<5;i++) direction[i]=t.ready[i]?t.outputs[i]*2-1:0; nodes(c,direction,y3);
            p.setTextAlign(Paint.Align.CENTER); p.setAlpha(255); p.setColor(hint); p.setTextSize(dp(9));
            for(int i=0;i<5;i++) {
                float labelX=dp(23)+(getWidth()-dp(46))*i/4f;
                c.drawText(names[i],labelX,y3+dp(19),p);
                c.drawText(t.ready[i]?Math.round(t.outputs[i]*100)+"%":"--",labelX,y3+dp(34),p);
            }
            p.setTextAlign(Paint.Align.LEFT);
        }
    }
}
