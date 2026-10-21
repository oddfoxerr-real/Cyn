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
import android.text.*;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import org.json.*;

public final class MainActivity extends androidx.appcompat.app.AppCompatActivity {
    AudioService service;
    final Handler handler=new Handler(Looper.getMainLooper());
    final ExecutorService filesWorker=Executors.newSingleThreadExecutor();
    LinearLayout shell,content,mini,libraryRows;
    com.google.android.material.bottomnavigation.BottomNavigationView nav;
    LiquidPage pages;
    SharedPreferences ui;
    TextView liveStatus,learningStatus,playLabel,positionLabel,powerButton,connectionTitle,connectionHint,externalStatus,processingDetails,playButton,miniTitle,miniState,miniToggle,updateState;
    SeekBar position;
    final ArrayList<AiControlMarker> liveControlMarkers=new ArrayList<>();
    Timeline timeline;NeuralView neural;Spectrum spectrum;ApexSpatialBrainView brainScan;
    int page=0,feedbackEffect=4,visibleTracks=60;
    int bg=0xff090d14,card=0xff121b2b,text=0xffedf3ff,hint=0xffb9c7da,accent=0xff83b8ff,secondary=0xffaa8fff,outline=0xff2b3a50;
    boolean bound,seeking,reduced,libraryBusy,onlyFavorites;
    String query="";
    final ServiceConnection connection=new ServiceConnection(){
        @Override public void onServiceConnected(ComponentName name,IBinder binder){service=((AudioService.LocalBinder)binder).service();build(false);processControlIntent(getIntent());}
        @Override public void onServiceDisconnected(ComponentName name){service=null;build(false);}
    };
    @Override public void onCreate(Bundle state){super.onCreate(state);ui=getSharedPreferences("ui",0);if(state!=null){page=state.getInt("page",0);query=state.getString("query","");onlyFavorites=state.getBoolean("favorites");}if(getIntent().getBooleanExtra("updates",false))page=4;build(false);bound=bindService(new Intent(this,AudioService.class),connection,BIND_AUTO_CREATE);UpdateManager.schedule(this);UpdateManager.checkOnLaunch(this);}
    @Override protected void onSaveInstanceState(Bundle state){super.onSaveInstanceState(state);state.putInt("page",page);state.putString("query",query);state.putBoolean("favorites",onlyFavorites);}
    @Override protected void onNewIntent(Intent i){super.onNewIntent(i);setIntent(i);if(i.getBooleanExtra("updates",false)){page=4;build(false);}processControlIntent(i);}
    void processControlIntent(Intent i){if(service!=null&&AudioEffect.ACTION_DISPLAY_AUDIO_EFFECT_CONTROL_PANEL.equals(i.getAction())){notifications();startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("session",i.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION,-1)));i.setAction(null);page=0;build(false);offerLiveVisuals();}}
    String asset(String name)throws IOException{try(InputStream in=getAssets().open(name)){return Streams.text(in);}}
    int dp(float v){return Math.round(v*getResources().getDisplayMetrics().density);}
    static final String[] THEME_NAMES={
        "Midnight • electric blue","Deep tide • sea glass","Ember dusk • copper pink",
        "Violet glass • iris","Obsidian ink • silver","Jade pulse • lime",
        "Rose noir • blush","Solar graphite • amber","Arctic mist • frost",
        "Neon city • magenta","Sapphire circuit • cobalt"
    };
    // Accent, secondary, dark background, translucent-surface tone.
    static final int[][] THEME_COLORS={
        {0xff83b8ff,0xffaa8fff,0xff090d14,0xff121b2b},
        {0xff65e4be,0xff7aa7ff,0xff061713,0xff102a24},
        {0xffffad82,0xffea8da9,0xff170e11,0xff302027},
        {0xffc0a0ff,0xff86d9f3,0xff100c1b,0xff241b39},
        {0xffd9e0ea,0xff91b0c5,0xff07090c,0xff171b23},
        {0xff8befae,0xffc5ea7d,0xff07140e,0xff19291c},
        {0xffff91b9,0xffe4a0ed,0xff180b14,0xff32202b},
        {0xffffc577,0xff93b6cc,0xff12110e,0xff292821},
        {0xff99e5ff,0xffa8bdff,0xff07151b,0xff152b38},
        {0xffff74dc,0xff7ea9ff,0xff16091d,0xff301a39},
        {0xff76aaff,0xff63ecd5,0xff080f24,0xff192341}
    };
    void palette(){
        reduced=ui.getBoolean("reduce",false)||!android.animation.ValueAnimator.areAnimatorsEnabled();
        int t=Math.floorMod(ui.getInt("studio_theme",0),THEME_COLORS.length);
        int[] colors=THEME_COLORS[t];
        accent=colors[0];secondary=colors[1];bg=colors[2];card=colors[3];
        outline=androidx.core.graphics.ColorUtils.blendARGB(card,accent,.21f);
    }
    GradientDrawable shape(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));d.setStroke(dp(1),outline);return d;}
    GradientDrawable gradient(int radius){GradientDrawable d=new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{androidx.core.graphics.ColorUtils.blendARGB(bg,accent,.22f),androidx.core.graphics.ColorUtils.blendARGB(bg,secondary,.22f)});d.setCornerRadius(dp(radius));return d;}
    TextView label(String value,int size,int color){TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setPadding(0,dp(4),0,dp(4));t.setFontFeatureSettings("kern");return t;}
    LinearLayout column(){LinearLayout l=new LinearLayout(this);l.setOrientation(LinearLayout.VERTICAL);return l;}
    LinearLayout row(){LinearLayout l=new LinearLayout(this);l.setGravity(Gravity.CENTER_VERTICAL);return l;}
    void add(LinearLayout parent,View v){parent.addView(v,new LinearLayout.LayoutParams(-1,-2));}
    void gap(LinearLayout l,int height){l.addView(new View(this),new LinearLayout.LayoutParams(1,dp(height)));}
    TextView button(String title,Runnable action){
        com.google.android.material.button.MaterialButton b=new com.google.android.material.button.MaterialButton(this);
        b.setText(title);b.setTextSize(13);b.setTextColor(text);b.setAllCaps(false);
        b.setTypeface(null,Typeface.BOLD);b.setGravity(Gravity.CENTER);
        b.setMinHeight(dp(52));b.setMinimumHeight(dp(52));b.setCornerRadius(dp(20));
        b.setInsetTop(0);b.setInsetBottom(0);
        b.setBackgroundTintList(ColorStateList.valueOf(androidx.core.graphics.ColorUtils.blendARGB(card,accent,.07f)));
        b.setStrokeWidth(dp(1));b.setStrokeColor(ColorStateList.valueOf(outline));
        b.setFocusable(true);
        final int motion=ui==null?0:ui.getInt("button_motion",0);
        if(motion>0&&!reduced){
            b.setOnTouchListener((v,event)->{
                switch(event.getActionMasked()){
                    case MotionEvent.ACTION_DOWN:
                        if(motion==1) v.animate().scaleX(.975f).scaleY(.975f).setDuration(100).start();
                        else if(motion==2) v.animate().translationY(-dp(3)).setDuration(130).start();
                        else if(motion==3) v.animate().scaleX(1.035f).scaleY(1.035f).setDuration(135).start();
                        break;
                    case MotionEvent.ACTION_UP:
                    case MotionEvent.ACTION_CANCEL:
                        v.animate().scaleX(1f).scaleY(1f).translationY(0f)
                            .setDuration(motion==3?220:145).start();
                        break;
                }
                return false;
            });
        }
        b.setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.CLOCK_TICK);action.run();});
        return b;
    }
    void primaryAction(TextView view){
        if(view instanceof com.google.android.material.button.MaterialButton){
            com.google.android.material.button.MaterialButton b=(com.google.android.material.button.MaterialButton)view;
            b.setBackgroundTintList(ColorStateList.valueOf(accent));b.setTextColor(bg);
            b.setStrokeWidth(0);b.setCornerRadius(dp(23));
        }
    }
    LinearLayout buttons(LinearLayout parent,String[] labels,Runnable[] actions){LinearLayout r=row();for(int i=0;i<labels.length;i++){TextView b=button(labels[i],actions[i]);LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-2,1);lp.setMargins(dp(3),0,dp(3),0);r.addView(b,lp);}add(parent,r);return r;}
    void animateRows(LinearLayout l){if(reduced)return;for(int i=0;i<l.getChildCount();i++){View v=l.getChildAt(i);v.setAlpha(0);v.setTranslationY(dp(8));v.animate().alpha(1).translationY(0).setDuration(220).setStartDelay(Math.min(i,5)*25L).start();}}
    LinearLayout section(String title,String description){
        com.google.android.material.card.MaterialCardView surface=new com.google.android.material.card.MaterialCardView(this);
        surface.setCardBackgroundColor(card);surface.setRadius(dp(28));surface.setCardElevation(0);
        surface.setStrokeWidth(dp(1));surface.setStrokeColor(outline);surface.setUseCompatPadding(false);
        surface.setClipToOutline(true);
        LinearLayout c=column();c.setPadding(dp(20),dp(18),dp(20),dp(20));
        surface.addView(c,new android.widget.FrameLayout.LayoutParams(-1,-2));
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(9),0,dp(9));
        content.addView(surface,lp);
        TextView h=label(title,19,text);h.setTypeface(null,Typeface.BOLD);add(c,h);
        if(!description.isEmpty())add(c,label(description,13,hint));gap(c,10);return c;
    }
    void build(){build(false);}
    void build(boolean transition){if(isFinishing()||isDestroyed())return;palette();liveControlMarkers.clear();liveStatus=learningStatus=playLabel=positionLabel=powerButton=connectionTitle=connectionHint=externalStatus=processingDetails=playButton=updateState=null;position=null;timeline=null;neural=null;spectrum=null;brainScan=null;libraryRows=null;
        if(shell==null){shell=column();shell.setBackgroundColor(bg);setContentView(shell);if(Build.VERSION.SDK_INT>=29)shell.setForceDarkAllowed(false);shell.setOnApplyWindowInsetsListener((v,insets)->{if(Build.VERSION.SDK_INT>=30){Insets e=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());v.setPadding(e.left,e.top,e.right,e.bottom);}else v.setPadding(insets.getSystemWindowInsetLeft(),insets.getSystemWindowInsetTop(),insets.getSystemWindowInsetRight(),insets.getSystemWindowInsetBottom());return insets;});shell.requestApplyInsets();
            LinearLayout header=row();header.setPadding(dp(22),dp(10),dp(22),dp(6));TextView brand=label("CYN",27,text);brand.setTypeface(null,Typeface.BOLD);brand.setLetterSpacing(.09f);header.addView(brand,new LinearLayout.LayoutParams(0,-2,1));TextView badge=label("SOUND STUDIO",10,hint);badge.setLetterSpacing(.12f);header.addView(badge);add(shell,header);
            pages=new LiquidPage(this);shell.addView(pages,new LinearLayout.LayoutParams(-1,0,1));
            mini=column();mini.setPadding(dp(18),dp(10),dp(18),dp(10));mini.setBackground(shape(androidx.core.graphics.ColorUtils.blendARGB(card,accent,.09f),26));LinearLayout.LayoutParams mp=new LinearLayout.LayoutParams(-1,-2);mp.setMargins(dp(12),dp(4),dp(12),0);shell.addView(mini,mp);
            nav=new com.google.android.material.bottomnavigation.BottomNavigationView(this);
            nav.setBackgroundColor(bg);
            nav.setElevation(0);
            nav.setItemIconSize(dp(24));
            nav.setItemHorizontalTranslationEnabled(false);
            nav.setLabelVisibilityMode(com.google.android.material.navigation.NavigationBarView.LABEL_VISIBILITY_LABELED);
            nav.setItemActiveIndicatorEnabled(true);
            nav.setItemActiveIndicatorWidth(dp(65));
            nav.setItemActiveIndicatorHeight(dp(32));
            // Build the menu once: clearing a live menu during a selection callback
            // can uncheck the destination before Material finishes dispatching it.
            String[] navigationTitles={"Studio","AI","Library","Style","Updates"};
            int[] navigationIcons={R.drawable.ic_cyn_studio,R.drawable.ic_cyn_ai,
                    R.drawable.ic_cyn_library,R.drawable.ic_cyn_style,R.drawable.ic_cyn_updates};
            for(int i=0;i<navigationTitles.length;i++)
                nav.getMenu().add(0,i,i,navigationTitles[i]).setIcon(navigationIcons[i]);
            nav.setOnItemSelectedListener(item->{
                int target=item.getItemId();
                if(target<0||target>=5)return false;
                if(page!=target){
                    page=target;
                    // Let BottomNavigationView finish selecting the item before
                    // rebuilding the content. A newer tap supersedes a queued one.
                    nav.post(()->{
                        if(!isFinishing()&&!isDestroyed()&&page==target)build(true);
                    });
                }
                return true;
            });
            add(shell,nav);
        }
        getWindow().setStatusBarColor(bg);getWindow().setNavigationBarColor(bg);
        ScrollView scroll=new ScrollView(this);scroll.setBackgroundColor(bg);scroll.setFillViewport(true);scroll.setClipToPadding(false);content=column();int side=getResources().getConfiguration().screenWidthDp>=700?60:18;content.setPadding(dp(side),dp(12),dp(side),dp(24));scroll.addView(content);
        if(service==null)add(content,label("Connecting to your sound…",20,text));else if(page==0)music();else if(page==1)learning();else if(page==2)files();else if(page==3)appearance();else updatesPage();
        pages.change(scroll,transition&&!reduced,
            ui.getInt("page_transition",4),accent,ui.getInt("transition_speed",100));
        // Reuse the stable Material menu: just update palette and selection.
        // Reconstructing menu views mid-tap caused intermittent missed selections.
        ColorStateList tabColors=new ColorStateList(
            new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},
            new int[]{accent,hint});
        nav.setItemIconTintList(tabColors);
        nav.setItemTextColor(tabColors);
        nav.setItemActiveIndicatorColor(ColorStateList.valueOf(
            androidx.core.graphics.ColorUtils.blendARGB(card,accent,.2f)));
        if(nav.getSelectedItemId()!=page)nav.setSelectedItemId(page);
        mini.removeAllViews();LinearLayout m=row();LinearLayout info=column();miniTitle=label("Your library is ready",13,text);miniTitle.setSingleLine();miniTitle.setEllipsize(TextUtils.TruncateAt.END);miniState=label("Choose a track to listen",11,hint);add(info,miniTitle);add(info,miniState);m.addView(info,new LinearLayout.LayoutParams(0,-2,1));TextView toggle=button("▶",()->{ExternalNowPlaying.Track external=ExternalNowPlaying.current(this);if(external!=null&&!service.playing){ExternalNowPlaying.toggle(external);return;}if(service==null)return;if(service.player==null){if(ExternalNowPlaying.instance==null){startActivity(new Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS"));}else{page=2;build(true);}return;}if(service.playing)service.pause();else startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.TOGGLE));});miniToggle=toggle;toggle.setContentDescription("Play current track");m.addView(toggle,new LinearLayout.LayoutParams(dp(68),-2));add(mini,m);info.setOnClickListener(v->{ExternalNowPlaying.Track external=ExternalNowPlaying.current(this);if(external!=null&&!service.playing){try{startActivity(getPackageManager().getLaunchIntentForPackage(external.controller.getPackageName()));}catch(RuntimeException ignored){}return;}if(service.player==null&&ExternalNowPlaying.instance==null){startActivity(new Intent("android.settings.ACTION_NOTIFICATION_LISTENER_SETTINGS"));return;}page=2;build(true);});update();
    }
    void toggle(LinearLayout parent,String title,boolean checked,java.util.function.Consumer<Boolean> change){
        com.google.android.material.materialswitch.MaterialSwitch s=new com.google.android.material.materialswitch.MaterialSwitch(this);
        s.setText(title);s.setTextColor(text);s.setTextSize(14);s.setMinHeight(dp(56));
        s.setPadding(dp(2),dp(5),dp(3),dp(5));
        s.setThumbTintList(new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},
            new int[]{bg,hint}));
        s.setTrackTintList(new ColorStateList(new int[][]{new int[]{android.R.attr.state_checked},new int[]{}},
            new int[]{accent,outline}));
        s.setChecked(checked);s.setOnCheckedChangeListener((b,on)->change.accept(on));add(parent,s);
    }
    void slider(LinearLayout parent,String title,float value,float min,float max,String suffix,java.util.function.Consumer<Float> change){TextView caption=label(title+"   "+String.format(Locale.ROOT,"%.1f",value)+suffix,13,text);add(parent,caption);FoxSlider bar=new FoxSlider(this,reduced);bar.setMax(120);bar.setProgress(Math.round((value-min)/(max-min)*120));bar.setMinimumHeight(dp(62));bar.setProgressTintList(ColorStateList.valueOf(accent));bar.setThumbTintList(ColorStateList.valueOf(accent));bar.setContentDescription(title);bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){if(user){float v=min+(max-min)*n/120;caption.setText(title+"   "+String.format(Locale.ROOT,"%.1f",v)+suffix);change.accept(v);}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});add(parent,bar);}
    /** The outlined circle is the user's baseline; the native seek thumb follows real AI DSP. */
    void smartSlider(LinearLayout parent,String key,int band,String labelText,float baseline,float min,float max,
                     String suffix,java.util.function.Consumer<Float> change){
        TextView caption=label(labelText+"   "+String.format(Locale.ROOT,"%.1f",baseline)+suffix,13,text);
        add(parent,caption);
        AiControlMarker bar=new AiControlMarker(key,band,baseline,min,max,suffix);
        bar.setMax(120);
        bar.setProgress(bar.position(baseline));
        bar.setMinimumHeight(dp(62));
        bar.setProgressTintList(ColorStateList.valueOf(accent));
        bar.setThumbTintList(ColorStateList.valueOf(accent));
        bar.setContentDescription(labelText+" · draggable AI position; ring is your manual setting");
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            @Override public void onProgressChanged(SeekBar slider,int progress,boolean user){
                if(user){
                    float newValue=min+(max-min)*progress/120f;
                    bar.manualValue=newValue;
                    bar.visualProgress=progress;
                    caption.setText(labelText+"   "+String.format(Locale.ROOT,"%.1f",newValue)+suffix);
                    change.accept(newValue);
                    bar.invalidate();
                }
            }
            @Override public void onStartTrackingTouch(SeekBar slider){bar.touching=true;}
            @Override public void onStopTrackingTouch(SeekBar slider){bar.touching=false;bar.sync();}
        });
        add(parent,bar);
        TextView aiLabel=label("Ring · saved choice   |   Thumb · live AI",11,hint);
        add(parent,aiLabel);
        bar.readout=aiLabel;
        liveControlMarkers.add(bar);
    }
    final class AiControlMarker extends FoxSlider {
        final String key;
        final int band;
        final float minimum,maximum;
        final String suffix;
        float manualValue, visualProgress;
        TextView readout;
        boolean touching;
        final Paint markerPaint=new Paint(Paint.ANTI_ALIAS_FLAG);
        AiControlMarker(String key,int band,float baseline,float min,float max,String suffix){
            super(MainActivity.this,reduced);
            this.key=key;this.band=band;minimum=min;maximum=max;
            manualValue=baseline;this.suffix=suffix;
            visualProgress=position(baseline);
            // The baseline ring must remain visible, including while the AI is off.
            setMinimumHeight(dp(62));
        }
        int position(float value){
            return Math.max(0,Math.min(120,Math.round((value-minimum)/(maximum-minimum)*120)));
        }
        void sync(){
            if(service==null)return;
            final float applied=service.aiControlValue(key,band);
            final boolean hasAi=Float.isFinite(applied);
            final float wanted=hasAi?position(Math.max(minimum,Math.min(maximum,applied))):position(manualValue);
            if(!touching){
                // Reversible: the physical SeekBar thumb travels to AI, then back to manual.
                // Only user touch calls the change listener; animation never saves AI as preset.
                if(reduced||Math.abs(wanted-visualProgress)<.6f)visualProgress=wanted;
                else visualProgress+=(wanted-visualProgress)*.36f;
                int next=Math.round(visualProgress);
                if(getProgress()!=next)setProgress(next);
            }
            if(readout!=null){
                if(hasAi)setLabel(readout,"AI thumb · "+String.format(Locale.ROOT,"%.1f",applied)+suffix+
                        "  |  Ring · your setting");
                else if(!service.enabled||!service.smart)setLabel(readout,"AI off · thumb returning to your saved setting");
                else if("Output headroom".equals(key))setLabel(readout,"Manual headroom · AI does not modify this control");
                else setLabel(readout,"No applied AI adjustment · manual setting");
            }
            invalidate();
        }
        @Override protected void onDraw(Canvas canvas){
            super.onDraw(canvas);
            // Always draw the ring at the saved manual choice, never at the AI target.
            float ratio=(manualValue-minimum)/(maximum-minimum);
            float centerX=getPaddingLeft()+(getWidth()-getPaddingLeft()-getPaddingRight())*ratio;
            float centerY=getHeight()*.5f;
            markerPaint.setColor(secondary);
            markerPaint.setStyle(Paint.Style.STROKE);
            markerPaint.setStrokeWidth(dp(2.25f));
            markerPaint.setAlpha(touching?165:255);
            canvas.drawCircle(centerX,centerY,dp(11),markerPaint);
            markerPaint.setStyle(Paint.Style.FILL);
            markerPaint.setAlpha(255);
            canvas.drawCircle(centerX,centerY,dp(2.5f),markerPaint);
        }
    }
    void category(LinearLayout parent,String title,java.util.function.Consumer<LinearLayout> fill){
        LinearLayout body=column();body.setVisibility(View.GONE);fill.accept(body);
        final TextView[] header=new TextView[1];
        header[0]=button(title+"  ⌄",()->{
            boolean open=body.getVisibility()!=View.VISIBLE;
            body.setVisibility(open?View.VISIBLE:View.GONE);
            header[0].setText(title+(open?"  ⌃":"  ⌄"));
            header[0].setContentDescription((open?"Collapse ":"Expand ")+title);
            if(open)animateRows(body);
        });
        header[0].setContentDescription("Expand "+title);
        add(parent,header[0]);add(parent,body);gap(parent,9);
    }
    void music(){
        add(content,label("CYN · Smart Listening",29,text));add(content,label("Your music AI assistant · adaptive sound and live processing.",14,hint));
        LinearLayout hero=section("SOUND STUDIO","");hero.setBackground(gradient(26));connectionTitle=label("",19,text);connectionTitle.setTypeface(null,Typeface.BOLD);add(hero,connectionTitle);spectrum=new Spectrum();hero.addView(spectrum,new LinearLayout.LayoutParams(-1,dp(service.playing||service.monitor?148:92)));connectionHint=label("",12,hint);add(hero,connectionHint);processingDetails=label(service.processingStatus(),12,accent);processingDetails.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);add(hero,processingDetails);
        toggle(hero,"CYN enhancement",service.enabled,on->{service.enabled=on;service.changed();});
        LinearLayout c=section("Sound controls","For library playback and compatible music sessions.");smartSlider(c,"Bass",-1,"Bass",service.bass,0,12," dB",v->{service.bass=v;service.changed();});smartSlider(c,"Vocal clarity",-1,"Vocal clarity",service.clarity,0,8," dB",v->{service.clarity=v;service.changed();});
        category(c,"Space & dynamics · live AI sliders",body->{smartSlider(body,"Stereo width",-1,"Stereo width",service.width,0,100,"%",v->{service.width=v;service.changed();});smartSlider(body,"Ambience",-1,"Ambience",service.ambience*100,0,100,"%",v->{service.ambience=v/100;service.changed();});smartSlider(body,"Dynamics",-1,"Dynamics",service.dynamics*100,0,100,"%",v->{service.dynamics=v/100;service.changed();});smartSlider(body,"Output headroom",-1,"Output headroom",service.headroom,.3f,6," dB",v->{service.headroom=v;service.changed();});add(body,label("Ambience, dynamics and the full EQ process CYN library playback. Other apps use the effects their audio session supports.",12,hint));});
        category(c,service.bandCount+"-band equalizer · live AI sliders",body->{buttons(body,new String[]{"31","35","39"},new Runnable[]{()->bands(31),()->bands(35),()->bands(39)});gap(body,8);for(int i=0;i<service.bandCount;i++){final int n=i;float hz=(float)(20*Math.pow(1000,i/(double)(service.bandCount-1)));smartSlider(body,"EQ",n,hz<1000?Math.round(hz)+" Hz":String.format(Locale.ROOT,"%.1f kHz",hz/1000),service.studioEq[i],-12,12," dB",v->{service.studioEq[n]=v;service.changed();});}add(body,button("Reset equalizer",()->{Arrays.fill(service.studioEq,0);service.changed();build();}));});
        category(c,"Listening presets",body->{buttons(body,new String[]{"Balanced","Impact","Dialogue"},new Runnable[]{()->preset(2,1,.2f),()->preset(5,1,.45f),()->preset(1,3,.15f)});});
        LinearLayout external=section("Connect CYN","Enable real-time sound control for compatible music apps. Android determines which audio sessions accept enhancement.");
        externalStatus=label(service.connectionState()+"\n"+service.connectionHint(),12,hint);externalStatus.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);add(external,externalStatus);
        powerButton=button(service.monitor?"Disconnect CYN":"Connect CYN",()->{
            if(service.monitor) service.stopMonitor();
            else { startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR)); enableConnectAi(); }
            update();
        });primaryAction(powerButton);add(external,powerButton);
        add(external,button("Try device-wide EQ (experimental)",()->new com.google.android.material.dialog.MaterialAlertDialogBuilder(this)
            .setTitle("Try device-wide compatibility?")
            .setMessage("Android's legacy output-mix EQ works only on some phones. It can affect multiple apps, and obtaining effect control does not prove music is being processed. Compare sound with CYN off/on. Disconnect music apps to stop the attempt.")
            .setPositiveButton("Try",(d,w)->{startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("legacy",true));enableConnectAi();})
            .setNegativeButton("Cancel",null).show()));
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED)
            add(external,button("Enable live visuals",this::offerLiveVisuals));
        add(external,button("Enable Connect AI",this::enableConnectAi));
        add(external,button("Connection details",this::connectionHelp));
    }
    void bands(int count){float[]old=service.studioEq.clone();int n=service.bandCount;for(int i=0;i<count;i++){float position=i*(n-1f)/(count-1);int lo=(int)position;service.studioEq[i]=old[lo]+(old[Math.min(n-1,lo+1)]-old[lo])*(position-lo);}service.bandCount=count;service.changed();build();}
    void preset(float b,float c,float d){service.bass=b;service.clarity=c;service.dynamics=d;service.changed();build();}
    void connectionHelp(){
        String message="Status: "+service.connectionState()+
            "\n\nProcessing: "+service.processingStatus()+
            "\n\n"+service.connectionHint()+
            "\n\nCYN listens to and enhances compatible playback through available Android audio effects. Some players do not expose a controllable session. The optional CYN Library has its own native DSP path."+
            "\n\nThis panel does not expose your phone identity, app names or audio session identifiers. For optional local troubleshooting, open Style → Audio diagnostics.";
        new AlertDialog.Builder(this).setTitle("CYN connection").setMessage(message).setPositiveButton("Done",null).show();
    }
    void files(){
        add(content,label("Optional audio library",29,text));add(content,label(service.library.all().size()+" local tracks · an extra way to enjoy CYN",13,hint));
        LinearLayout player=section("NOW PLAYING","");playLabel=label(service.title,21,text);playLabel.setTypeface(null,Typeface.BOLD);add(player,playLabel);positionLabel=label("",12,hint);add(player,positionLabel);position=new SeekBar(this);position.setContentDescription("Track position");position.setMinimumHeight(dp(48));position.setProgressTintList(ColorStateList.valueOf(accent));position.setThumbTintList(ColorStateList.valueOf(accent));position.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){}public void onStartTrackingTouch(SeekBar b){seeking=true;}public void onStopTrackingTouch(SeekBar b){seeking=false;service.seek(b.getProgress());}});add(player,position);
        LinearLayout transport=buttons(player,new String[]{"Previous",service.playing?"Pause":"Play","Next"},new Runnable[]{()->service.previous(),()->{if(service.player==null)toast("Choose a track below");else if(service.playing)service.pause();else startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.TOGGLE));build();},()->service.next()});playButton=(TextView)transport.getChildAt(1);
        toggle(player,"Shuffle",service.prefs.getBoolean("shuffle",false),service::shuffle);category(player,"Repeat",body->buttons(body,new String[]{"Off","All","One"},new Runnable[]{()->{service.repeat(0);toast("Repeat off");},()->{service.repeat(2);toast("Repeat all");},()->{service.repeat(1);toast("Repeat track");}}));liveStatus=label("",12,accent);add(player,liveStatus);
        buttons(content,new String[]{"Add files","Scan phone"},new Runnable[]{this::pickTracks,this::scanPermission});gap(content,12);
        LinearLayout online=section("PUBLIC MUSIC · AUDIUS","Search the Audius open music catalog and play available tracks through CYN's own audio engine.");
        EditText publicSearch=new EditText(this);publicSearch.setSingleLine(true);publicSearch.setTextColor(text);publicSearch.setHintTextColor(hint);
        publicSearch.setHint("Search online music");publicSearch.setMinHeight(dp(52));publicSearch.setPadding(dp(16),dp(8),dp(16),dp(8));
        publicSearch.setBackground(shape(card,16));add(online,publicSearch);
        LinearLayout publicResults=column();add(online,publicResults);
        add(online,button("Search Audius",()->{
            String term=publicSearch.getText().toString().trim();
            if(term.isEmpty()){toast("Enter a song or artist");return;}
            publicResults.removeAllViews();add(publicResults,label("Searching Audius…",13,hint));
            filesWorker.execute(()->{
                try{List<PublicMusic.Song> found=PublicMusic.search(term);
                    handler.post(()->{
                        if(page!=2||!publicResults.isAttachedToWindow())return;
                        publicResults.removeAllViews();
                        if(found.isEmpty()){add(publicResults,label("No public tracks found.",13,hint));return;}
                        for(PublicMusic.Song song:found){
                            LinearLayout trackRow=row();
                            TextView play=button("▶  "+song.title+" · "+song.artist,()->{
                                if(service==null){toast("Audio engine connecting");return;}
                                try{playTrack(java.util.Collections.singletonList(song.asTrack()),0);}
                                catch(RuntimeException e){toast("Unable to start stream");}
                            });
                            trackRow.addView(play,new LinearLayout.LayoutParams(0,-2,1));
                            add(trackRow,button("＋",()->{
                                if(service==null){toast("Audio engine connecting");return;}
                                filesWorker.execute(()->{
                                    try{service.library.merge(java.util.Collections.singletonList(song.asTrack()));
                                        handler.post(()->{toast("Added to your Library");if(page==2)renderTracks();});
                                    }catch(Exception e){handler.post(()->toast("Could not save track"));}
                                });
                            }));
                            add(publicResults,trackRow);
                        }
                    });
                }catch(Exception e){handler.post(()->{if(page==2&&publicResults.isAttachedToWindow()){publicResults.removeAllViews();add(publicResults,label("Online search unavailable. Check your connection.",13,hint));}});}
            });
        }));

        EditText search=new EditText(this);search.setSingleLine(true);search.setTextColor(text);search.setHintTextColor(hint);search.setHint("Search tracks, artists, albums");search.setContentDescription("Search audio library");search.setText(query);search.setMinHeight(dp(52));search.setPadding(dp(16),dp(8),dp(16),dp(8));search.setBackground(shape(card,16));add(content,search);
        toggle(content,"Favorites only",onlyFavorites,on->{onlyFavorites=on;visibleTracks=60;renderTracks();});
        libraryRows=column();add(content,libraryRows);search.addTextChangedListener(new TextWatcher(){public void beforeTextChanged(CharSequence s,int st,int c,int a){}public void onTextChanged(CharSequence s,int st,int before,int count){query=s.toString();visibleTracks=60;renderTracks();}public void afterTextChanged(Editable e){}});renderTracks();
    }
    List<AudioLibrary.Track> filteredTracks(){ArrayList<AudioLibrary.Track> list=new ArrayList<>();String q=query.toLowerCase(Locale.ROOT);for(AudioLibrary.Track t:service.library.all())if((!onlyFavorites||t.favorite)&&(t.title+" "+t.artist+" "+t.album).toLowerCase(Locale.ROOT).contains(q))list.add(t);return list;}
    void renderTracks(){if(libraryRows==null)return;libraryRows.removeAllViews();List<AudioLibrary.Track> tracks=filteredTracks();if(tracks.isEmpty()){LinearLayout empty=column();empty.setPadding(dp(14),dp(20),dp(14),dp(20));add(empty,label(libraryBusy?"Reading your music…":"Room for your next favorite.",19,text));add(empty,label(query.isEmpty()?"Add files or scan your phone to bring your music into CYN.":"No tracks match this search.",13,hint));add(libraryRows,empty);return;}
        for(int i=0;i<Math.min(visibleTracks,tracks.size());i++){AudioLibrary.Track t=tracks.get(i);final int index=i;LinearLayout r=row();r.setPadding(dp(12),dp(8),dp(8),dp(8));r.setBackground(shape(card,18));TextView art=label(t.title.substring(0,1).toUpperCase(Locale.ROOT),20,accent);art.setGravity(Gravity.CENTER);art.setBackground(gradient(14));r.addView(art,new LinearLayout.LayoutParams(dp(48),dp(48)));LinearLayout words=column();words.setPadding(dp(12),0,dp(4),0);TextView title=label(t.title,14,text);title.setMaxLines(2);title.setEllipsize(TextUtils.TruncateAt.END);add(words,title);add(words,label(t.artist+" · "+time((int)t.duration),11,hint));r.addView(words,new LinearLayout.LayoutParams(0,-2,1));androidx.appcompat.widget.AppCompatImageButton favorite=new androidx.appcompat.widget.AppCompatImageButton(this);
            favorite.setImageResource(t.favorite?R.drawable.ic_favorite_filled:R.drawable.ic_favorite_outline);
            favorite.setImageTintList(ColorStateList.valueOf(t.favorite?accent:hint));
            favorite.setBackground(new RippleDrawable(ColorStateList.valueOf(androidx.core.graphics.ColorUtils.setAlphaComponent(accent,50)),shape(card,16),null));
            favorite.setPadding(dp(11),dp(11),dp(11),dp(11));
            favorite.setScaleType(ImageView.ScaleType.FIT_CENTER);
            favorite.setSelected(t.favorite);
            favorite.setTag(t.uri.toString());
            favorite.setContentDescription((t.favorite?"Remove favorite ":"Favorite ")+t.title);
            favorite.setOnClickListener(v->{try{service.library.favorite(t.uri);renderTracks();}catch(Exception e){toast("Could not save favorite");}});
            r.addView(favorite,new LinearLayout.LayoutParams(dp(48),dp(48)));words.setOnClickListener(v->playTrack(tracks,index));r.setOnClickListener(v->playTrack(tracks,index));r.setFocusable(true);r.setContentDescription("Play "+t.title+" by "+t.artist);r.setOnLongClickListener(v->{new AlertDialog.Builder(this).setTitle(t.title).setItems(new String[]{"Play now","Remove from CYN library"},(d,n)->{if(n==0)playTrack(tracks,index);else try{service.library.remove(t.uri);renderTracks();}catch(Exception e){toast("Could not remove track");}}).show();return true;});add(libraryRows,r);gap(libraryRows,8);}
        if(tracks.size()>visibleTracks)add(libraryRows,button("Show more tracks",()->{visibleTracks+=60;renderTracks();}));
    }
    void playTrack(List<AudioLibrary.Track> queue,int index){notifications();startForegroundService(new Intent(this,AudioService.class).setAction("cyn.QUEUE"));service.playQueue(queue,index);update();}
    void pickTracks(){startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("audio/*").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION),10);}
    void scanPermission(){String permission=Build.VERSION.SDK_INT>=33?Manifest.permission.READ_MEDIA_AUDIO:Manifest.permission.READ_EXTERNAL_STORAGE;if(checkSelfPermission(permission)!=PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{permission},23);else scan();}
    void scan(){if(libraryBusy)return;libraryBusy=true;renderTracks();AudioLibrary library=service.library;filesWorker.execute(()->{try{List<AudioLibrary.Track> found=library.scan();library.merge(found);handler.post(()->{libraryBusy=false;toast(found.size()+" songs found");if(page==2)build();});}catch(Exception e){handler.post(()->{libraryBusy=false;toast("Music access unavailable. You can still add files.");renderTracks();});}});}
    void learning(){
        add(content,label("Intelligence, in tune.",29,text));add(content,label("Desktop neural models. Private on your phone.",13,hint));
        LinearLayout engine=section("AI Hub · choose one engine","Selecting a different AI replaces the previous primary engine. Shared audio analysis is not a second selectable AI.");
        toggle(engine,"Smart Listening (master switch)",service.smart,on->{service.smart=on;service.changed();});
        android.widget.RadioGroup aiChoices=new android.widget.RadioGroup(this);
        aiChoices.setOrientation(android.widget.RadioGroup.VERTICAL);
        String[] profileIds={AiSelection.BASIC,AiSelection.APEX_6M,AiSelection.APEX_10M};
        String[] profileLabels={"CYN Base AI · YAMNet + personal preferences","APEX 6M · existing neural spectral guidance","APEX 10M · Experimental AI Release"};
        int selectedViewId=View.NO_ID;
        for(int i=0;i<profileIds.length;i++){
            com.google.android.material.radiobutton.MaterialRadioButton option=new com.google.android.material.radiobutton.MaterialRadioButton(this);
            int viewId=View.generateViewId();option.setId(viewId);
            option.setTag(profileIds[i]);option.setText(profileLabels[i]);
            option.setTextColor(text);option.setTextSize(14);option.setMinHeight(dp(54));
            option.setContentDescription(profileLabels[i]);
            option.setButtonTintList(ColorStateList.valueOf(accent));
            aiChoices.addView(option,new android.widget.RadioGroup.LayoutParams(-1,-2));
            if(service.aiProfile.equals(profileIds[i]))selectedViewId=viewId;
        }
        if(selectedViewId!=View.NO_ID)aiChoices.check(selectedViewId);
        aiChoices.setOnCheckedChangeListener((group,id)->{
            android.view.View selected=group.findViewById(id);
            if(selected==null)return;
            Object tag=selected.getTag();
            if(tag instanceof String && !service.aiProfile.equals(tag)){
                service.selectAiProfile((String)tag);
                build();
            }
        });
        add(engine,aiChoices);
        add(engine,label("One primary AI at a time. APEX 10M is the current experimental neural-spatial release.",12,hint));
        if(AiSelection.APEX_10M.equals(service.aiProfile)){
            slider(engine,"Spatial intensity",service.spatialPreviewStrength*100,0,100,"%",v->{service.spatialPreviewStrength=v/100;service.changed();});
            add(engine,label("APEX 10M  ·  95.97% TRAINED",12,accent));
            toggle(engine,"APEX 10M neural control",service.partialNeuralPreview,on->{
                service.partialNeuralPreview=on;
                service.currentAi().partialPolicy=null;
                service.currentAi().partialStatus=on?"Starting neural engine…":"Classic spatial DSP";
                service.changed();
                build();
            });
            add(engine,label(service.partialNeuralPreview
                ? "Neural engine · "+service.currentAi().partialStatus
                : "Classic spatial DSP mode",12,hint));
            add(engine,label("Works with songs played in CYN Library.",12,hint));

        }
        category(engine,"Content focus (not another AI)",body->{
            toggle(body,"Dialogue / movie tuning",service.movieMode,on->{service.movieMode=on;service.changed();});
        });
        slider(engine,"Sensitivity",service.sensitivity*100,0,100,"%",v->{service.sensitivity=v/100;service.changed();});
        liveStatus=label("",13,accent);add(engine,liveStatus);
        if(AiSelection.APEX_10M.equals(service.aiProfile)||service.monitor){
            LinearLayout network=section("CYN • 3D mind map",
                "Live AI activity across CYN Library and connected audio");
            brainScan=new ApexSpatialBrainView(this,new ApexSpatialBrainView.Signal(){
                @Override public float activity(){
                    if(service==null)return 0f;
                    float[] values=service.apexVisualTelemetry();
                    return service.mindExternal()&&values!=null&&values.length>=4
                        ?values[2]:service.apexSpatialActivity();
                }
                @Override public float[] channels(){return service==null?null:service.apexVisualTelemetry();}
                @Override public long decisionAt(){return service==null?0:service.apexDecisionTimestamp();}
                @Override public boolean enabled(){return service!=null&&service.mindLive();}
                @Override public boolean connected(){return service!=null&&service.mindConnected();}
                @Override public boolean external(){return service!=null&&service.mindExternal();}
                @Override public float[] hidden(){return service==null?null:service.mindHidden();}
                @Override public float[] connections(){return service==null?null:service.mindConnections();}
                @Override public float[] outputs(){return service==null?null:service.mindOutputs();}
                @Override public boolean reduced(){return reduced;}
            },accent,secondary);
            network.addView(brainScan,new LinearLayout.LayoutParams(-1,dp(334)));

        }else{
            LinearLayout network=section("Personal neural activity","Measured activations from your preference model.");
            neural=new NeuralView();
            network.addView(neural,new LinearLayout.LayoutParams(-1,dp(220)));
        }
        LinearLayout c=section("Teach your sound","Select a passage, then tell CYN what you want more or less of.");timeline=new Timeline();c.addView(timeline,new LinearLayout.LayoutParams(-1,dp(130)));buttons(c,new String[]{"Last 5 seconds","Live timeline"},new Runnable[]{()->timeline.selectRecent(),()->timeline.resume()});gap(c,8);
        category(c,"Training target",body->{String[] names={"Clarity","Ambience","Width","Dynamics","Bass"};for(int i=0;i<5;i++){final int n=i;add(body,button(names[i],()->{feedbackEffect=n;toast("Training "+names[n]);timeline.invalidate();}));}});buttons(c,new String[]{"Less here","More here"},new Runnable[]{()->labelFrames(false),()->labelFrames(true)});learningStatus=label("",12,accent);add(c,learningStatus);add(c,button("Train personal model",()->service.train()));
        category(c,"Model tools",body->{add(body,button("Import desktop preference model",()->startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE),30)));add(body,button("Export personal model",()->startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/json").putExtra(Intent.EXTRA_TITLE,"CYN-phone-preferences.cynai.json"),31)));add(body,button("Reset personal learning",()->new AlertDialog.Builder(this).setTitle("Reset personal learning?").setMessage("Your saved labels and personal weights will be removed.").setPositiveButton("Reset",(d,w)->{if(service.training){toast("Wait for training to finish");return;}try{service.learning.reset();}catch(Exception e){toast("Could not reset model");}}).setNegativeButton("Cancel",null).show()));});
        add(c,label("Personal training uses your More/Less feedback. APEX 10M's release model has separate training.",12,hint));
        add(c,button("Analyze a compatible external app",this::requestAnalysis));
    }
    void labelFrames(boolean more){if(service.training){toast("Wait for training to finish");return;}List<AudioService.Frame> selected=timeline.selection();if(selected.isEmpty()){toast("Play a library track, then select a passage");return;}try{service.learning.label(selected,feedbackEffect,more);toast("Saved "+selected.size()+" frames");timeline.invalidate();}catch(Exception e){toast("Could not save feedback");}}
    void appearance(){
        add(content,label("Make it yours.",29,text));
        add(content,label("Material surfaces, characterful palettes and your choice of motion.",13,hint));
        LinearLayout themes=section("Theme Studio","11 distinct palettes. Tap a theme to apply it throughout CYN.");
        for(int i=0;i<THEME_NAMES.length;i++){
            final int selected=i;
            int[] palette=THEME_COLORS[i];
            LinearLayout swatch=row();
            View accentChip=new View(this);
            GradientDrawable chip=new GradientDrawable(GradientDrawable.Orientation.TL_BR,
                new int[]{palette[0],palette[1]});
            chip.setCornerRadius(dp(13));accentChip.setBackground(chip);
            LinearLayout.LayoutParams cp=new LinearLayout.LayoutParams(dp(34),dp(34));
            cp.setMargins(0,0,dp(10),0);
            swatch.addView(accentChip,cp);
            TextView select=button(THEME_NAMES[i]+(ui.getInt("studio_theme",0)==i?"  ✓":""),()->{
                ui.edit().putInt("studio_theme",selected).apply();
                build(true);
            });
            if(ui.getInt("studio_theme",0)==i)primaryAction(select);
            swatch.addView(select,new LinearLayout.LayoutParams(0,dp(52),1));
            add(themes,swatch);
            gap(themes,7);
        }
        LinearLayout motions=section("Page transitions","The actual outgoing page animates as the next page appears.");
        for(int i=0;i<LiquidPage.MODES.length;i++){
            final int selected=i;
            TextView option=button(LiquidPage.MODES[i]+(ui.getInt("page_transition",4)==i?"  ✓":""),()->{
                ui.edit().putInt("page_transition",selected).apply();build(true);
            });
            if(ui.getInt("page_transition",4)==i)primaryAction(option);
            add(motions,option);gap(motions,6);
        }
        LinearLayout speed=section("Motion timing","Used for page transitions. Disable motion to skip all page animations.");
        String[] speedLabels={"Relaxed","Balanced","Snappy"};
        int[] speeds={70,100,145};
        for(int i=0;i<3;i++){
            int target=speeds[i];
            TextView choice=button(speedLabels[i]+(ui.getInt("transition_speed",100)==target?"  ✓":""),()->{
                ui.edit().putInt("transition_speed",target).apply();build();
            });
            if(ui.getInt("transition_speed",100)==target)primaryAction(choice);
            add(speed,choice);gap(speed,6);
        }
        LinearLayout controls=section("Button interactions","Pick a consistent button response. All controls remain fully clickable.");
        String[] modes={"Material ripple","Soft press","Floating lift","Elastic pop"};
        for(int i=0;i<modes.length;i++){
            int selected=i;
            TextView choice=button(modes[i]+(ui.getInt("button_motion",0)==i?"  ✓":""),()->{
                ui.edit().putInt("button_motion",selected).apply();build();
            });
            if(ui.getInt("button_motion",0)==i)primaryAction(choice);
            add(controls,choice);gap(controls,6);
        }
        toggle(controls,"Reduce motion",ui.getBoolean("reduce",false),on->{
            ui.edit().putBoolean("reduce",on).apply();reduced=on;build();
        });
    }
    void updatesPage(){
        add(content,label("Updates & about",29,text));
        add(content,label("Version management stays in Updates, separate from Theme Studio.",13,hint));
        LinearLayout updates=section("Updates","Version "+BuildConfig.VERSION_NAME+" · build "+BuildConfig.VERSION_CODE);
        toggle(updates,"Automatic updates · check and download on Wi-Fi",UpdateManager.automatic(this),on->{
            UpdateManager.automatic(this,on);
            toast(on?"Automatic Wi-Fi updates enabled":"Automatic updates disabled");
        });
        updateState=label(UpdateManager.message,13,accent);
        updateState.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        add(updates,updateState);
        TextView updateButton=button("Open update center",()->{
            notifications();
            UpdateSplash.show(this);
        });
        primaryAction(updateButton);add(updates,updateButton);
        add(updates,label("Material-style updater · official release checks, live progress and verified installs. Automatic updates download on unmetered networks; installation always needs your approval.",12,hint));
        LinearLayout about=section("CYN for Android","Built on the FxSound open-source foundation.");add(about,button("Audio diagnostics (local only)",()->showText("Audio diagnostics · privacy-safe",service.diagnostics()+"\nAI engine: "+service.currentAi().status)));add(about,button("Source & release history",()->browser("https://github.com/oddfoxerr-real/Cyn/tree/codex/android/android")));add(about,button("FxSound credits, licenses & no warranty",()->{try{showText("FxSound credits & licenses",asset("NOTICE.txt")+"\n\n"+asset("LICENSE.txt"));}catch(Exception e){toast("Could not open licenses");}}));add(about,label("The phone uses the same neural model weights and desktop mixer. Its audio renderer and effects are adapted for Android; identical sound to the Windows FxSound driver is not claimed.",12,hint));add(about,button("Stop CYN audio",()->{service.stopAll();update();}));
    }
    void enableConnectAi(){
        if(Build.VERSION.SDK_INT<29){offerLiveVisuals();return;}
        if(service!=null&&service.connectedCapture!=null){toast("Connect AI is already sharing playback");return;}
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED){
            new AlertDialog.Builder(this).setTitle("Enable Connect AI")
                .setMessage("CYN analyzes shared playback on this phone to adjust supported audio effects. Android calls the first permission microphone access; CYN only captures playback. Audio is not saved or uploaded. Next, approve Android's sharing prompt.")
                .setPositiveButton("Continue",(d,w)->requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO},24))
                .setNegativeButton("Not now",null).show();
            return;
        }
        try {
            android.media.projection.MediaProjectionManager manager=getSystemService(android.media.projection.MediaProjectionManager.class);
            Intent consent=Build.VERSION.SDK_INT>=34
                ?manager.createScreenCaptureIntent(android.media.projection.MediaProjectionConfig.createConfigForDefaultDisplay())
                :manager.createScreenCaptureIntent();
            startActivityForResult(consent,72);
        }catch(RuntimeException error){toast("Playback sharing is unavailable on this device");}
    }
    void offerLiveVisuals(){
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)==PackageManager.PERMISSION_GRANTED){
            if(service!=null)service.refreshVisualization();
            return;
        }
        new AlertDialog.Builder(this).setTitle("Enable CYN live visuals?")
            .setMessage("To show a real-time spectrum and neural activity for music outside CYN, Android requires its microphone-labelled audio permission. CYN uses Android's playback Visualizer; it does not open the microphone, save music or upload audio. Some apps may still block analysis.")
            .setPositiveButton("Enable",(d,w)->requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO},21))
            .setNegativeButton("Not now",null).show();
    }
    void requestAnalysis(){service.smart=true;service.changed();offerLiveVisuals();}
    void notifications(){if(Build.VERSION.SDK_INT>=33&&checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)!=PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS},22);}
    @Override public void onRequestPermissionsResult(int code,String[] permissions,int[] results){super.onRequestPermissionsResult(code,permissions,results);if(service==null)return;if(code==24&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED){enableConnectAi();return;}if(code==23&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED)scan();
        service.changed();
        if(code==21&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED){
            service.refreshVisualization();
            toast("CYN live visuals enabled");
        }}
    void update(){if(service==null)return;
        ExternalNowPlaying.Track external=ExternalNowPlaying.current(this);
        boolean externalShown=external!=null&&!service.playing;
        if(miniTitle!=null){setLabel(miniTitle,externalShown?external.title:service.player==null?"Enable media access for Spotify, SoundCloud & more":service.title);setLabel(miniState,externalShown?external.app+" · "+(external.playing?"Playing":"Paused")+(external.artist.isEmpty()?"":" · "+external.artist):service.player==null?"Tap to allow media controls":service.playing?"Playing · "+time(service.position()):"Paused");}
        if(miniToggle!=null){setLabel(miniToggle,externalShown?(external.playing?"Ⅱ":"▶"):service.playing?"Ⅱ":"▶");miniToggle.setContentDescription(externalShown?(external.playing?"Pause "+external.app:"Play "+external.app):service.playing?"Pause current track":"Play current track");}
        if(playButton!=null)setLabel(playButton,service.playing?"Pause":"Play");
        if(connectionTitle!=null){setLabel(connectionTitle,!service.enabled?"Original sound":service.playing?"Your sound, live":service.monitor?service.connectionState():"Ready when you are");setLabel(connectionHint,service.playing?(service.smart?service.currentAi().status:"AI off · manual sound controls"):service.connectionHint());}
        if(externalStatus!=null)setLabel(externalStatus,service.connectionState()+"\n"+service.connectionHint()+"\n"+service.visualizationStatus());
        if(processingDetails!=null)setLabel(processingDetails,service.processingStatus()+"\n"+service.visualizationStatus());
        if(powerButton!=null)setLabel(powerButton,service.monitor?"Disconnect CYN":"Connect CYN");
        if(playLabel!=null)setLabel(playLabel,service.title);if(position!=null&&!seeking){position.setMax(Math.max(1,service.duration()));position.setProgress(service.position());}if(positionLabel!=null)setLabel(positionLabel,time(service.position())+" / "+time(service.duration()));
        if(liveStatus!=null){DesktopAi.Result r=service.currentAi().latest;String state=!service.enabled?"CYN bypassed":!service.smart?"AI off":(service.playing||service.capturedAudioFresh())?service.currentAi().status:service.player!=null?"Playback paused":service.monitor?service.analysis:"Connect your music apps for AI listening";if(r!=null&&(service.playing||service.capturedAudioFresh())&&service.smart&&service.enabled&&SystemClock.elapsedRealtime()-r.at<2500)state+="\nMusic "+Math.round(r.scene[0]*100)+"% · Speech "+Math.round(r.scene[1]*100)+"% · "+r.milliseconds+" ms inference";setLabel(liveStatus,state);}
        if(learningStatus!=null){int more=0,less=0;for(PreferenceModel.Example e:service.learning.examples)if(e.effect==feedbackEffect){if(e.more)more++;else less++;}setLabel(learningStatus,new String[]{"Clarity","Ambience","Width","Dynamics","Bass"}[feedbackEffect]+" · "+more+" More / "+less+" Less\n"+(service.training?"Training "+service.trainingEpoch+" / 240":service.learning.message));}
        if(updateState!=null)setLabel(updateState,UpdateManager.message);for(AiControlMarker marker:liveControlMarkers)marker.sync();if(timeline!=null)timeline.invalidate();if(neural!=null)neural.invalidate();if(brainScan!=null)brainScan.invalidate();if(spectrum!=null)spectrum.invalidate();
    }
    void setLabel(TextView view,String value){if(!value.contentEquals(view.getText()))view.setText(value);}
    final Runnable refresh=new Runnable(){public void run(){update();handler.postDelayed(this,250);}};
    @Override protected void onResume(){super.onResume();handler.removeCallbacks(refresh);handler.post(refresh);}
    @Override protected void onPause(){handler.removeCallbacks(refresh);super.onPause();}
    @Override protected void onDestroy(){handler.removeCallbacksAndMessages(null);filesWorker.shutdown();if(bound)unbindService(connection);super.onDestroy();}
    @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request==72){if(result==RESULT_OK&&data!=null){startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.CAPTURE).putExtra("consent",data));}else toast("Playback sharing cancelled · basic EQ remains available");return;}if(request==71){if(getPackageManager().canRequestPackageInstalls())UpdateManager.install(this);return;}if(result!=RESULT_OK||data==null||service==null)return;
        if(request==10){ArrayList<Uri>uris=new ArrayList<>();if(data.getClipData()!=null)for(int i=0;i<data.getClipData().getItemCount();i++)uris.add(data.getClipData().getItemAt(i).getUri());else if(data.getData()!=null)uris.add(data.getData());for(Uri uri:uris)try{getContentResolver().takePersistableUriPermission(uri,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException ignored){}AudioLibrary library=service.library;libraryBusy=true;filesWorker.execute(()->{try{ArrayList<AudioLibrary.Track> tracks=new ArrayList<>();for(Uri uri:uris)tracks.add(library.read(uri));library.merge(tracks);handler.post(()->{libraryBusy=false;page=2;build();toast(tracks.size()+" tracks added");});}catch(Exception e){handler.post(()->{libraryBusy=false;toast("Could not add audio files");});}});}
        else if(request==30&&data.getData()!=null){if(service.training){toast("Wait for training to finish");return;}try(InputStream in=getContentResolver().openInputStream(data.getData())){service.learning.importModel(in);toast("Desktop preference weights imported");}catch(Exception e){toast("Model import failed: "+e.getMessage());}}
        else if(request==31&&data.getData()!=null){try(OutputStream out=getContentResolver().openOutputStream(data.getData(),"wt")){out.write(service.learning.exportModel().getBytes(java.nio.charset.StandardCharsets.UTF_8));toast("Personal model exported");}catch(Exception e){toast("Could not export model");}}
    }
    String time(int ms){int seconds=Math.max(0,ms)/1000;return String.format(Locale.ROOT,"%d:%02d",seconds/60,seconds%60);}
    void toast(String value){Toast.makeText(this,value,Toast.LENGTH_SHORT).show();}
    void browser(String url){try{startActivity(new Intent(Intent.ACTION_VIEW,Uri.parse(url)));}catch(Exception e){toast("No browser available");}}
    void showText(String title,String value){TextView t=label(value,13,text);t.setTextIsSelectable(true);t.setPadding(dp(20),dp(12),dp(20),dp(20));ScrollView scroll=new ScrollView(this);scroll.setBackgroundColor(card);scroll.addView(t);new AlertDialog.Builder(this).setTitle(title).setView(scroll).setPositiveButton("Done",null).show();}
    final class Spectrum extends View {
        final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
        final Path curve=new Path();
        final float[] levels=new float[31];
        Spectrum(){
            super(MainActivity.this);
            setContentDescription("Live measured audio spectrum and equalizer contour");
        }
        @Override protected void onDraw(Canvas canvas){
            super.onDraw(canvas);
            if(service==null)return;
            float w=getWidth(),h=getHeight();
            PcmSpectrum.Frame pcm=service.pcmSpectrum.latest();
            boolean library=service.playing&&pcm!=null&&System.nanoTime()-pcm.atNanos<350_000_000L;
            boolean external=!library&&service.hasSessionSpectrum();
            float[] audioBands=library?pcm.bands:external?service.sessionSpectrum:null;
            float spacing=w/31f;
            boolean fading=false;
            paint.setStrokeCap(Paint.Cap.ROUND);
            paint.setStrokeWidth(Math.max(dp(2),spacing*.52f));
            for(int i=0;i<31;i++){
                float goal=0;
                if(audioBands!=null&&i<audioBands.length)goal=audioBands[i];
                goal=Math.max(0,Math.min(1,goal));
                levels[i]+=(goal-levels[i])*(reduced?1:goal>levels[i]?.34f:.17f);
                if(levels[i]>.015f)fading=true;
                float height=Math.max(dp(1),levels[i]*h*.72f);
                paint.setColor(androidx.core.graphics.ColorUtils.blendARGB(accent,secondary,i/30f));
                paint.setAlpha(Math.round(65+190*Math.min(1,levels[i])));
                canvas.drawLine((i+.5f)*spacing,h*.5f-height*.5f,(i+.5f)*spacing,h*.5f+height*.5f,paint);
            }
            paint.setAlpha(185);
            paint.setStrokeWidth(dp(1.5f));
            paint.setColor(text);
            paint.setStyle(Paint.Style.STROKE);
            curve.reset();
            for(int i=0;i<service.bandCount;i++){
                float x=w*i/(service.bandCount-1f),y=h*.5f-service.studioEq[i]*h/32;
                if(i==0)curve.moveTo(x,y);else curve.lineTo(x,y);
            }
            canvas.drawPath(curve,paint);
            paint.setStyle(Paint.Style.FILL);
            paint.setAlpha(255);
            if(!library&&!external&&!fading){
                paint.setColor(hint);
                paint.setTextSize(dp(11));
                paint.setTextAlign(Paint.Align.CENTER);
                canvas.drawText("Waiting for measured audio",w*.5f,h*.5f+dp(28),paint);
                paint.setTextAlign(Paint.Align.LEFT);
            }
            if(!reduced&&(library||external||fading)&&isAttachedToWindow())postInvalidateOnAnimation();
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
            if(frozen!=null) { p.setColor((new int[]{0xff79caff,0xffb899ff,0xff70e1c2,0xffffa47c,0xffee8bbb}[feedbackEffect]&0xffffff)|0x33000000); c.drawRoundRect(Math.min(start,end)*w,dp(5),Math.max(start,end)*w,h-dp(5),dp(10),dp(10),p); }
            int count=Math.min(80,frames.size()); float step=w/count; p.setStrokeWidth(Math.max(dp(2),step*.45f)); p.setStrokeCap(Paint.Cap.ROUND); p.setColor(accent);
            for(int i=0;i<count;i++) { AudioService.Frame f=frames.get(i*frames.size()/count); p.setColor(accent);for(PreferenceModel.Example e:service.learning.examples)if(e.id==f.id){p.setColor(new int[]{0xff79caff,0xffb899ff,0xff70e1c2,0xffffa47c,0xffee8bbb}[e.effect]);break;} float height=Math.max(dp(3),f.level*(h-dp(28))); c.drawLine((i+.5f)*step,(h-height)/2,(i+.5f)*step,(h+height)/2,p); }
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
        final String[] names={"Clarity","Ambience","Surround","Dynamic","Bass"};
        final double[] previousInput=new double[11],previousHidden=new double[12],previousOutput=new double[5];
        final double[] previousEdgesIn=new double[132],previousEdgesOut=new double[60];
        final double[] visibleInput=new double[11],visibleHidden=new double[12],visibleOutput=new double[5];
        final double[] visibleEdgesIn=new double[132],visibleEdgesOut=new double[60];
        PreferenceModel.Trace lastTrace;
        long transitionAt;
        NeuralView(){
            super(MainActivity.this);
            setContentDescription("Measured neural activations: glowing nodes and signal impulses follow live model values");
        }
        float x(int i,int count){return dp(12)+(getWidth()-dp(24))*i/(count-1f);}
        void transition(double[] from,double[] target,double[] output,float t){
            for(int i=0;i<output.length;i++)output[i]=from[i]+(target[i]-from[i])*t;
        }
        void copyVisibleToPrevious(){
            System.arraycopy(visibleInput,0,previousInput,0,11);
            System.arraycopy(visibleHidden,0,previousHidden,0,12);
            System.arraycopy(visibleOutput,0,previousOutput,0,5);
            System.arraycopy(visibleEdgesIn,0,previousEdgesIn,0,132);
            System.arraycopy(visibleEdgesOut,0,previousEdgesOut,0,60);
        }
        void edges(Canvas c,double[] values,int from,int to,float y1,float y2,long now,boolean pulse){
            p.setStrokeWidth(dp(.75f));
            for(int j=0;j<to;j++)for(int i=0;i<from;i++){
                double v=values[j*from+i],strength=Math.min(1,Math.abs(v)*5);
                if(strength<.008)continue;
                float x1=x(i,from),x2=x(j,to);
                p.setColor(v<0?0xffffa76c:accent);
                p.setAlpha(Math.min(165,12+(int)(145*strength)));
                c.drawLine(x1,y1,x2,y2,p);
                if(pulse&&strength>.18){
                    // The number, brightness and speed of impulses depend on measured connection strength.
                    float phase=(float)(((now*(.00030+.00048*strength)+(i*7+j*11)*.19)%1+1)%1);
                    float px=x1+(x2-x1)*phase,py=y1+(y2-y1)*phase;
                    p.setAlpha((int)(50+145*strength));
                    c.drawCircle(px,py,dp(1.1f)+(float)(dp(1.5f)*strength),p);
                }
            }
        }
        void nodes(Canvas c,double[] values,float y,boolean[] trained){
            for(int i=0;i<values.length;i++){
                double value=trained!=null&&!trained[i]?0:values[i];
                float strength=(float)Math.min(1,Math.abs(value));
                p.setColor(value<0?0xffffa76c:accent);
                p.setAlpha((int)(24+85*strength));
                c.drawCircle(x(i,values.length),y,dp(3.8f)+dp(4)*strength,p);
                p.setAlpha((int)(60+195*strength));
                c.drawCircle(x(i,values.length),y,dp(2.8f)+dp(1.5f)*strength,p);
            }
        }
        @Override protected void onDraw(Canvas c){
            super.onDraw(c);
            if(service==null)return;
            PreferenceModel.Trace t=service.neuralTrace();
            long now=SystemClock.elapsedRealtime();
            p.setColor(hint);p.setAlpha(255);p.setTextSize(dp(12));
            if(t==null){
                lastTrace=null;
                c.drawText(service.training?"Waiting for a measured training epoch":"Waiting for fresh neural activity",dp(8),getHeight()*.5f,p);
                return;
            }
            if(lastTrace!=t){
                copyVisibleToPrevious();
                lastTrace=t;transitionAt=now;
            }
            float blend=reduced?1:Math.min(1,(now-transitionAt)/270f);
            transition(previousInput,t.inputs,visibleInput,blend);
            transition(previousHidden,t.hidden,visibleHidden,blend);
            transition(previousOutput,t.outputs,visibleOutput,blend);
            transition(previousEdgesIn,t.inputConnections,visibleEdgesIn,blend);
            transition(previousEdgesOut,t.outputConnections,visibleEdgesOut,blend);
            float y1=dp(20),y2=getHeight()*.43f,y3=getHeight()-dp(53);
            boolean live=!reduced&&service.enabled&&service.smart&&!service.training;
            edges(c,visibleEdgesIn,11,12,y1,y2,now,live);
            edges(c,visibleEdgesOut,12,5,y2,y3,now,live);
            nodes(c,visibleInput,y1,null);
            nodes(c,visibleHidden,y2,null);
            double[] signedOutput=new double[5];
            for(int i=0;i<5;i++)signedOutput[i]=t.ready[i]?(visibleOutput[i]*2-1):0;
            nodes(c,signedOutput,y3,t.ready);
            p.setTextAlign(Paint.Align.CENTER);p.setAlpha(255);p.setColor(hint);p.setTextSize(dp(9));
            for(int i=0;i<5;i++){
                float labelX=x(i,5);
                c.drawText(names[i],labelX,y3+dp(19),p);
                c.drawText(t.ready[i]?Math.round(visibleOutput[i]*100)+"%":"--",labelX,y3+dp(34),p);
            }
            p.setTextAlign(Paint.Align.LEFT);
            if(!reduced&&(live||blend<1)&&isAttachedToWindow())postInvalidateDelayed(40);
        }
    }
}
