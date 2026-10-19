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

public final class MainActivity extends Activity {
    AudioService service;
    final Handler handler=new Handler(Looper.getMainLooper());
    final ExecutorService filesWorker=Executors.newSingleThreadExecutor();
    LinearLayout shell,content,nav,mini,libraryRows;
    LiquidPage pages;
    SharedPreferences ui;
    TextView liveStatus,learningStatus,playLabel,positionLabel,powerButton,connectionTitle,connectionHint,externalStatus,processingDetails,playButton,miniTitle,miniState,miniToggle,updateState;
    SeekBar position;
    Timeline timeline;NeuralView neural;Spectrum spectrum;
    int page=0,feedbackEffect=4,visibleTracks=60;
    int bg=0xff030509,card=0xff0c111c,text=0xffedf3ff,hint=0xffa4b3cc,accent=0xff83b8ff,secondary=0xffaa8fff,outline=0xff1b263b;
    boolean bound,seeking,reduced,libraryBusy,onlyFavorites;
    String query="";
    final ServiceConnection connection=new ServiceConnection(){
        @Override public void onServiceConnected(ComponentName name,IBinder binder){service=((AudioService.LocalBinder)binder).service();build(false);processControlIntent(getIntent());}
        @Override public void onServiceDisconnected(ComponentName name){service=null;build(false);}
    };
    @Override public void onCreate(Bundle state){super.onCreate(state);ui=getSharedPreferences("ui",0);if(state!=null){page=state.getInt("page",0);query=state.getString("query","");onlyFavorites=state.getBoolean("favorites");}if(getIntent().getBooleanExtra("updates",false))page=3;build(false);bound=bindService(new Intent(this,AudioService.class),connection,BIND_AUTO_CREATE);UpdateManager.schedule(this);UpdateManager.check(this,false,null);}
    @Override protected void onSaveInstanceState(Bundle state){super.onSaveInstanceState(state);state.putInt("page",page);state.putString("query",query);state.putBoolean("favorites",onlyFavorites);}
    @Override protected void onNewIntent(Intent i){super.onNewIntent(i);setIntent(i);if(i.getBooleanExtra("updates",false)){page=3;build(false);}processControlIntent(i);}
    void processControlIntent(Intent i){if(service!=null&&AudioEffect.ACTION_DISPLAY_AUDIO_EFFECT_CONTROL_PANEL.equals(i.getAction())){notifications();startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("session",i.getIntExtra(AudioEffect.EXTRA_AUDIO_SESSION,-1)));i.setAction(null);page=0;build(false);offerLiveVisuals();}}
    String asset(String name)throws IOException{try(InputStream in=getAssets().open(name)){return Streams.text(in);}}
    int dp(float v){return Math.round(v*getResources().getDisplayMetrics().density);}
    void palette(){reduced=ui.getBoolean("reduce",false)||!android.animation.ValueAnimator.areAnimatorsEnabled();int t=ui.getInt("studio_theme",0);int[][]colors={{0xff83b8ff,0xffaa8fff,0xff0c111c},{0xff65e4be,0xff7aa7ff,0xff071713},{0xffffad82,0xffea8da9,0xff190f10},{0xffc0a0ff,0xff86d9f3,0xff100d1b}};int[]c=colors[Math.floorMod(t,colors.length)];accent=c[0];secondary=c[1];card=c[2];}
    GradientDrawable shape(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));d.setStroke(dp(1),outline);return d;}
    GradientDrawable gradient(int radius){GradientDrawable d=new GradientDrawable(GradientDrawable.Orientation.TL_BR,new int[]{androidx.core.graphics.ColorUtils.blendARGB(bg,accent,.22f),androidx.core.graphics.ColorUtils.blendARGB(bg,secondary,.22f)});d.setCornerRadius(dp(radius));return d;}
    TextView label(String value,int size,int color){TextView t=new TextView(this);t.setText(value);t.setTextSize(size);t.setTextColor(color);t.setPadding(0,dp(4),0,dp(4));t.setFontFeatureSettings("kern");return t;}
    LinearLayout column(){LinearLayout l=new LinearLayout(this);l.setOrientation(LinearLayout.VERTICAL);return l;}
    LinearLayout row(){LinearLayout l=new LinearLayout(this);l.setGravity(Gravity.CENTER_VERTICAL);return l;}
    void add(LinearLayout parent,View v){parent.addView(v,new LinearLayout.LayoutParams(-1,-2));}
    void gap(LinearLayout l,int height){l.addView(new View(this),new LinearLayout.LayoutParams(1,dp(height)));}
    TextView button(String title,Runnable action){TextView b=label(title,14,text);b.setGravity(Gravity.CENTER);b.setMinHeight(dp(48));b.setPadding(dp(12),dp(10),dp(12),dp(10));b.setBackground(new RippleDrawable(ColorStateList.valueOf(0x3383b8ff),shape(card,16),null));b.setFocusable(true);b.setClickable(true);b.setAccessibilityDelegate(new View.AccessibilityDelegate(){@Override public void onInitializeAccessibilityNodeInfo(View host,android.view.accessibility.AccessibilityNodeInfo info){super.onInitializeAccessibilityNodeInfo(host,info);info.setClassName(Button.class.getName());}});b.setOnClickListener(v->{v.performHapticFeedback(HapticFeedbackConstants.CLOCK_TICK);action.run();});return b;}
    LinearLayout buttons(LinearLayout parent,String[] labels,Runnable[] actions){LinearLayout r=row();for(int i=0;i<labels.length;i++){TextView b=button(labels[i],actions[i]);LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(0,-2,1);lp.setMargins(dp(3),0,dp(3),0);r.addView(b,lp);}add(parent,r);return r;}
    void animateRows(LinearLayout l){if(reduced)return;for(int i=0;i<l.getChildCount();i++){View v=l.getChildAt(i);v.setAlpha(0);v.setTranslationY(dp(8));v.animate().alpha(1).translationY(0).setDuration(220).setStartDelay(Math.min(i,5)*25L).start();}}
    LinearLayout section(String title,String description){LinearLayout c=column();c.setPadding(dp(18),dp(16),dp(18),dp(16));c.setBackground(shape(card,24));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.setMargins(0,dp(8),0,dp(8));content.addView(c,lp);TextView h=label(title,18,text);h.setTypeface(null,Typeface.BOLD);add(c,h);if(!description.isEmpty())add(c,label(description,13,hint));gap(c,8);return c;}
    void build(){build(false);}
    void build(boolean transition){if(isFinishing()||isDestroyed())return;palette();liveStatus=learningStatus=playLabel=positionLabel=powerButton=connectionTitle=connectionHint=externalStatus=processingDetails=playButton=updateState=null;position=null;timeline=null;neural=null;spectrum=null;libraryRows=null;
        if(shell==null){shell=column();shell.setBackgroundColor(bg);setContentView(shell);if(Build.VERSION.SDK_INT>=29)shell.setForceDarkAllowed(false);shell.setOnApplyWindowInsetsListener((v,insets)->{if(Build.VERSION.SDK_INT>=30){Insets e=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());v.setPadding(e.left,e.top,e.right,e.bottom);}else v.setPadding(insets.getSystemWindowInsetLeft(),insets.getSystemWindowInsetTop(),insets.getSystemWindowInsetRight(),insets.getSystemWindowInsetBottom());return insets;});shell.requestApplyInsets();
            LinearLayout header=row();header.setPadding(dp(22),dp(10),dp(22),dp(6));TextView brand=label("CYN",23,text);brand.setTypeface(null,Typeface.BOLD);brand.setLetterSpacing(.15f);header.addView(brand,new LinearLayout.LayoutParams(0,-2,1));TextView badge=label("SOUND STUDIO",10,hint);badge.setLetterSpacing(.12f);header.addView(badge);add(shell,header);
            pages=new LiquidPage(this);shell.addView(pages,new LinearLayout.LayoutParams(-1,0,1));
            mini=column();mini.setPadding(dp(18),dp(10),dp(18),dp(10));mini.setBackground(shape(card,20));LinearLayout.LayoutParams mp=new LinearLayout.LayoutParams(-1,-2);mp.setMargins(dp(12),dp(4),dp(12),0);shell.addView(mini,mp);
            nav=row();nav.setPadding(dp(10),dp(8),dp(10),dp(8));add(shell,nav);
        }
        getWindow().setStatusBarColor(bg);getWindow().setNavigationBarColor(bg);
        ScrollView scroll=new ScrollView(this);scroll.setBackgroundColor(bg);scroll.setFillViewport(true);scroll.setClipToPadding(false);content=column();int side=getResources().getConfiguration().screenWidthDp>=700?60:18;content.setPadding(dp(side),dp(12),dp(side),dp(24));scroll.addView(content);
        if(service==null)add(content,label("Connecting to your sound…",20,text));else if(page==0)music();else if(page==1)learning();else if(page==2)files();else appearance();
        pages.change(scroll,transition&&!reduced);nav.removeAllViews();String[]titles={"Studio","AI","Library","Style"};
        for(int i=0;i<4;i++){final int target=i;TextView b=button(titles[i],()->{if(page!=target){page=target;build(true);}});b.setTextColor(page==i?text:hint);if(page==i)b.setBackground(gradient(18));b.setSelected(page==i);b.setTypeface(null,page==i?Typeface.BOLD:Typeface.NORMAL);LinearLayout.LayoutParams np=new LinearLayout.LayoutParams(0,-2,1);np.setMargins(dp(3),0,dp(3),0);nav.addView(b,np);}
        mini.removeAllViews();LinearLayout m=row();LinearLayout info=column();miniTitle=label("Your library is ready",13,text);miniTitle.setSingleLine();miniTitle.setEllipsize(TextUtils.TruncateAt.END);miniState=label("Choose a track to listen",11,hint);add(info,miniTitle);add(info,miniState);m.addView(info,new LinearLayout.LayoutParams(0,-2,1));TextView toggle=button("▶",()->{if(service==null)return;if(service.player==null){page=2;build(true);}else if(service.playing)service.pause();else startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.TOGGLE));});miniToggle=toggle;toggle.setContentDescription("Play current track");m.addView(toggle,new LinearLayout.LayoutParams(dp(68),-2));add(mini,m);info.setOnClickListener(v->{page=2;build(true);});update();
    }
    void toggle(LinearLayout parent,String title,boolean checked,java.util.function.Consumer<Boolean> change){Switch s=new Switch(this);s.setText(title);s.setTextColor(text);s.setTextSize(14);s.setMinHeight(dp(52));s.setChecked(checked);s.setThumbTintList(ColorStateList.valueOf(accent));s.setOnCheckedChangeListener((b,on)->change.accept(on));add(parent,s);}
    void slider(LinearLayout parent,String title,float value,float min,float max,String suffix,java.util.function.Consumer<Float> change){TextView caption=label(title+"   "+String.format(Locale.ROOT,"%.1f",value)+suffix,13,text);add(parent,caption);SeekBar bar=new SeekBar(this);bar.setMax(120);bar.setProgress(Math.round((value-min)/(max-min)*120));bar.setMinimumHeight(dp(48));bar.setProgressTintList(ColorStateList.valueOf(accent));bar.setThumbTintList(ColorStateList.valueOf(accent));bar.setContentDescription(title);bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onProgressChanged(SeekBar b,int n,boolean user){if(user){float v=min+(max-min)*n/120;caption.setText(title+"   "+String.format(Locale.ROOT,"%.1f",v)+suffix);change.accept(v);}}public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}});add(parent,bar);}
    void category(LinearLayout parent,String title,java.util.function.Consumer<LinearLayout> fill){LinearLayout body=column();body.setVisibility(View.GONE);fill.accept(body);TextView header=button(title+"  ›",()->{boolean open=body.getVisibility()!=View.VISIBLE;body.setVisibility(open?View.VISIBLE:View.GONE);if(open)animateRows(body);});add(parent,header);add(parent,body);gap(parent,8);}
    void music(){
        add(content,label("CYN · Smart Listening",29,text));add(content,label("Your music AI assistant · adaptive sound and live processing.",14,hint));
        LinearLayout hero=section("SOUND STUDIO","");hero.setBackground(gradient(26));connectionTitle=label("",19,text);connectionTitle.setTypeface(null,Typeface.BOLD);add(hero,connectionTitle);spectrum=new Spectrum();hero.addView(spectrum,new LinearLayout.LayoutParams(-1,dp(148)));connectionHint=label("",12,hint);add(hero,connectionHint);processingDetails=label(service.processingStatus(),12,accent);processingDetails.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);add(hero,processingDetails);
        toggle(hero,"CYN enhancement",service.enabled,on->{service.enabled=on;service.changed();});
        LinearLayout c=section("Sound controls","For library playback and compatible music sessions.");slider(c,"Bass",service.bass,0,12," dB",v->{service.bass=v;service.changed();});slider(c,"Vocal clarity",service.clarity,0,8," dB",v->{service.clarity=v;service.changed();});
        category(c,"Space & dynamics",body->{slider(body,"Stereo width",service.width,0,100,"%",v->{service.width=v;service.changed();});slider(body,"Ambience",service.ambience*100,0,100,"%",v->{service.ambience=v/100;service.changed();});slider(body,"Dynamics",service.dynamics*100,0,100,"%",v->{service.dynamics=v/100;service.changed();});slider(body,"Output headroom",service.headroom,.3f,6," dB",v->{service.headroom=v;service.changed();});add(body,label("Ambience, dynamics and the full EQ process CYN library playback. Other apps use the effects their audio session supports.",12,hint));});
        category(c,service.bandCount+"-band equalizer",body->{buttons(body,new String[]{"31","35","39"},new Runnable[]{()->bands(31),()->bands(35),()->bands(39)});gap(body,8);for(int i=0;i<service.bandCount;i++){final int n=i;float hz=(float)(20*Math.pow(1000,i/(double)(service.bandCount-1)));slider(body,hz<1000?Math.round(hz)+" Hz":String.format(Locale.ROOT,"%.1f kHz",hz/1000),service.studioEq[i],-12,12," dB",v->{service.studioEq[n]=v;service.changed();});}add(body,button("Reset equalizer",()->{Arrays.fill(service.studioEq,0);service.changed();build();}));});
        category(c,"Listening presets",body->{buttons(body,new String[]{"Balanced","Impact","Dialogue"},new Runnable[]{()->preset(2,1,.2f),()->preset(5,1,.45f),()->preset(1,3,.15f)});});
        LinearLayout external=section("Connect CYN","Enable real-time sound control for compatible music apps. Android determines which audio sessions accept enhancement.");
        externalStatus=label(service.connectionState()+"\n"+service.connectionHint(),12,hint);externalStatus.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);add(external,externalStatus);
        powerButton=button(service.monitor?"Disconnect CYN":"Connect CYN",()->{
            if(service.monitor) service.stopMonitor();
            else { notifications(); startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR)); offerLiveVisuals(); }
            update();
        });add(external,powerButton);
        add(external,button("Try device-wide EQ (experimental)",()->new AlertDialog.Builder(this)
            .setTitle("Try device-wide compatibility?")
            .setMessage("Android's legacy output-mix EQ works only on some phones. It can affect multiple apps, and obtaining effect control does not prove music is being processed. Compare sound with CYN off/on. Disconnect music apps to stop the attempt.")
            .setPositiveButton("Try",(d,w)->{notifications();startForegroundService(new Intent(this,AudioService.class).setAction(AudioService.MONITOR).putExtra("legacy",true));offerLiveVisuals();})
            .setNegativeButton("Cancel",null).show()));
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED)
            add(external,button("Enable live visuals",this::requestAnalysis));
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
        EditText search=new EditText(this);search.setSingleLine(true);search.setTextColor(text);search.setHintTextColor(hint);search.setHint("Search tracks, artists, albums");search.setContentDescription("Search audio library");search.setText(query);search.setMinHeight(dp(52));search.setPadding(dp(16),dp(8),dp(16),dp(8));search.setBackground(shape(card,16));add(content,search);
        toggle(content,"Favorites only",onlyFavorites,on->{onlyFavorites=on;visibleTracks=60;renderTracks();});
        libraryRows=column();add(content,libraryRows);search.addTextChangedListener(new TextWatcher(){public void beforeTextChanged(CharSequence s,int st,int c,int a){}public void onTextChanged(CharSequence s,int st,int before,int count){query=s.toString();visibleTracks=60;renderTracks();}public void afterTextChanged(Editable e){}});renderTracks();
    }
    List<AudioLibrary.Track> filteredTracks(){ArrayList<AudioLibrary.Track> list=new ArrayList<>();String q=query.toLowerCase(Locale.ROOT);for(AudioLibrary.Track t:service.library.all())if((!onlyFavorites||t.favorite)&&(t.title+" "+t.artist+" "+t.album).toLowerCase(Locale.ROOT).contains(q))list.add(t);return list;}
    void renderTracks(){if(libraryRows==null)return;libraryRows.removeAllViews();List<AudioLibrary.Track> tracks=filteredTracks();if(tracks.isEmpty()){LinearLayout empty=column();empty.setPadding(dp(14),dp(20),dp(14),dp(20));add(empty,label(libraryBusy?"Reading your music…":"Room for your next favorite.",19,text));add(empty,label(query.isEmpty()?"Add files or scan your phone to bring your music into CYN.":"No tracks match this search.",13,hint));add(libraryRows,empty);return;}
        for(int i=0;i<Math.min(visibleTracks,tracks.size());i++){AudioLibrary.Track t=tracks.get(i);final int index=i;LinearLayout r=row();r.setPadding(dp(12),dp(8),dp(8),dp(8));r.setBackground(shape(card,18));TextView art=label(t.title.substring(0,1).toUpperCase(Locale.ROOT),20,accent);art.setGravity(Gravity.CENTER);art.setBackground(gradient(14));r.addView(art,new LinearLayout.LayoutParams(dp(48),dp(48)));LinearLayout words=column();words.setPadding(dp(12),0,dp(4),0);TextView title=label(t.title,14,text);title.setMaxLines(2);title.setEllipsize(TextUtils.TruncateAt.END);add(words,title);add(words,label(t.artist+" · "+time((int)t.duration),11,hint));r.addView(words,new LinearLayout.LayoutParams(0,-2,1));TextView favorite=button(t.favorite?"♥":"♡",()->{try{service.library.favorite(t.uri);renderTracks();}catch(Exception e){toast("Could not save favorite");}});favorite.setContentDescription((t.favorite?"Remove favorite ":"Favorite ")+t.title);r.addView(favorite,new LinearLayout.LayoutParams(dp(48),-2));words.setOnClickListener(v->playTrack(tracks,index));r.setOnClickListener(v->playTrack(tracks,index));r.setFocusable(true);r.setContentDescription("Play "+t.title+" by "+t.artist);r.setOnLongClickListener(v->{new AlertDialog.Builder(this).setTitle(t.title).setItems(new String[]{"Play now","Remove from CYN library"},(d,n)->{if(n==0)playTrack(tracks,index);else try{service.library.remove(t.uri);renderTracks();}catch(Exception e){toast("Could not remove track");}}).show();return true;});add(libraryRows,r);gap(libraryRows,8);}
        if(tracks.size()>visibleTracks)add(libraryRows,button("Show more tracks",()->{visibleTracks+=60;renderTracks();}));
    }
    void playTrack(List<AudioLibrary.Track> queue,int index){notifications();startForegroundService(new Intent(this,AudioService.class).setAction("cyn.QUEUE"));service.playQueue(queue,index);update();}
    void pickTracks(){startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("audio/*").addCategory(Intent.CATEGORY_OPENABLE).putExtra(Intent.EXTRA_ALLOW_MULTIPLE,true).addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION),10);}
    void scanPermission(){String permission=Build.VERSION.SDK_INT>=33?Manifest.permission.READ_MEDIA_AUDIO:Manifest.permission.READ_EXTERNAL_STORAGE;if(checkSelfPermission(permission)!=PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{permission},23);else scan();}
    void scan(){if(libraryBusy)return;libraryBusy=true;renderTracks();AudioLibrary library=service.library;filesWorker.execute(()->{try{List<AudioLibrary.Track> found=library.scan();library.merge(found);handler.post(()->{libraryBusy=false;toast(found.size()+" songs found");if(page==2)build();});}catch(Exception e){handler.post(()->{libraryBusy=false;toast("Music access unavailable. You can still add files.");renderTracks();});}});}
    void learning(){
        add(content,label("Intelligence, in tune.",29,text));add(content,label("Desktop neural models. Private on your phone.",13,hint));
        LinearLayout engine=section("Neural engine","");toggle(engine,"Smart listening",service.smart,on->{service.smart=on;service.changed();});toggle(engine,"Full APEX 6M + YAMNet",service.useApex,on->{service.useApex=on;service.changed();});toggle(engine,"Dialogue / movie mode",service.movieMode,on->{service.movieMode=on;service.changed();});slider(engine,"Sensitivity",service.sensitivity*100,0,100,"%",v->{service.sensitivity=v/100;service.changed();});liveStatus=label("",13,accent);add(engine,liveStatus);add(engine,label("YAMNet identifies the scene. APEX adds the same learned spectral guidance used by PC CYN. Full processing runs on library tracks; external-app sessions use Android's available effects.",12,hint));
        LinearLayout network=section("Personal neural activity","Measured activations from your preference model.");neural=new NeuralView();network.addView(neural,new LinearLayout.LayoutParams(-1,dp(220)));
        LinearLayout c=section("Teach your sound","Select a passage, then tell CYN what you want more or less of.");timeline=new Timeline();c.addView(timeline,new LinearLayout.LayoutParams(-1,dp(130)));buttons(c,new String[]{"Last 5 seconds","Live timeline"},new Runnable[]{()->timeline.selectRecent(),()->timeline.resume()});gap(c,8);
        category(c,"Training target",body->{String[] names={"Clarity","Ambience","Width","Dynamics","Bass"};for(int i=0;i<5;i++){final int n=i;add(body,button(names[i],()->{feedbackEffect=n;toast("Training "+names[n]);timeline.invalidate();}));}});buttons(c,new String[]{"Less here","More here"},new Runnable[]{()->labelFrames(false),()->labelFrames(true)});learningStatus=label("",12,accent);add(c,learningStatus);add(c,button("Train personal model",()->service.train()));
        category(c,"Model tools",body->{add(body,button("Import desktop preference model",()->startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE),30)));add(body,button("Export personal model",()->startActivityForResult(new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/json").putExtra(Intent.EXTRA_TITLE,"CYN-phone-preferences.cynai.json"),31)));add(body,button("Reset personal learning",()->new AlertDialog.Builder(this).setTitle("Reset personal learning?").setMessage("Your saved labels and personal weights will be removed.").setPositiveButton("Reset",(d,w)->{if(service.training){toast("Wait for training to finish");return;}try{service.learning.reset();}catch(Exception e){toast("Could not reset model");}}).setNegativeButton("Cancel",null).show()));});
        add(c,label("Personal training needs at least five More and five Less frames for each selected effect. APEX is the existing 225-step experimental model; its real-music quality is not established.",12,hint));
        add(c,button("Analyze a compatible external app",this::requestAnalysis));
    }
    void labelFrames(boolean more){if(service.training){toast("Wait for training to finish");return;}List<AudioService.Frame> selected=timeline.selection();if(selected.isEmpty()){toast("Play a library track, then select a passage");return;}try{service.learning.label(selected,feedbackEffect,more);toast("Saved "+selected.size()+" frames");timeline.invalidate();}catch(Exception e){toast("Could not save feedback");}}
    void appearance(){add(content,label("Make it yours.",29,text));add(content,label("A quieter interface. A distinct personality.",13,hint));LinearLayout themes=section("Theme studio","");String[]names={"Midnight current","Deep tide","Ember dusk","Violet glass"};for(int i=0;i<names.length;i++){final int n=i;TextView b=button(names[i],()->{ui.edit().putInt("studio_theme",n).apply();build();});if(ui.getInt("studio_theme",0)==i)b.setTextColor(accent);add(themes,b);gap(themes,6);}toggle(themes,"Reduce motion",ui.getBoolean("reduce",false),on->{ui.edit().putBoolean("reduce",on).apply();reduced=on;});
        LinearLayout updates=section("Updates","Version "+BuildConfig.VERSION_NAME+" · build "+BuildConfig.VERSION_CODE);toggle(updates,"Check automatically",UpdateManager.automatic(this),on->UpdateManager.automatic(this,on));toggle(updates,"Download on Wi-Fi",UpdateManager.downloads(this),on->UpdateManager.downloads(this,on));updateState=label(UpdateManager.message,13,accent);updateState.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);add(updates,updateState);add(updates,button("Check and download update",()->{notifications();UpdateManager.check(this,true,null);}));gap(updates,6);add(updates,button("Install downloaded update",()->UpdateManager.install(this)));add(updates,label("CYN verifies the download, package and signing certificate. Android asks you to confirm installation. Audio and learning data stay on your phone.",12,hint));
        LinearLayout about=section("CYN for Android","Built on the FxSound open-source foundation.");add(about,button("Audio diagnostics (local only)",()->showText("Audio diagnostics · privacy-safe",service.diagnostics()+"\nAI engine: "+service.desktopAi.status)));add(about,button("Source & release history",()->browser("https://github.com/oddfoxerr-real/Cyn/tree/codex/android/android")));add(about,button("Licenses & credits",()->{try{showText("Licenses & credits",asset("NOTICE.txt")+"\n\n"+asset("LICENSE.txt"));}catch(Exception e){toast("Could not open licenses");}}));add(about,label("The phone uses the same neural model weights and desktop mixer. Its audio renderer and effects are adapted for Android; identical sound to the Windows FxSound driver is not claimed.",12,hint));add(about,button("Stop CYN audio",()->{service.stopAll();update();}));
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
    void requestAnalysis(){offerLiveVisuals();}
    void notifications(){if(Build.VERSION.SDK_INT>=33&&checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS)!=PackageManager.PERMISSION_GRANTED)requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS},22);}
    @Override public void onRequestPermissionsResult(int code,String[] permissions,int[] results){super.onRequestPermissionsResult(code,permissions,results);if(service==null)return;if(code==23&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED)scan();
        service.changed();
        if(code==21&&results.length>0&&results[0]==PackageManager.PERMISSION_GRANTED){
            service.refreshVisualization();
            toast("CYN live visuals enabled");
        }}
    void update(){if(service==null)return;
        if(miniTitle!=null){setLabel(miniTitle,service.player==null?"Your library is ready":service.title);setLabel(miniState,service.player==null?"Choose a track to listen":service.playing?"Playing · "+time(service.position()):"Paused");}
        if(miniToggle!=null){setLabel(miniToggle,service.playing?"Ⅱ":"▶");miniToggle.setContentDescription(service.playing?"Pause current track":"Play current track");}
        if(playButton!=null)setLabel(playButton,service.playing?"Pause":"Play");
        if(connectionTitle!=null){setLabel(connectionTitle,!service.enabled?"Original sound":service.playing?"Your sound, live":service.monitor?service.connectionState():"Ready when you are");setLabel(connectionHint,service.playing?(service.smart?service.desktopAi.status:"AI off · manual sound controls"):service.connectionHint());}
        if(externalStatus!=null)setLabel(externalStatus,service.connectionState()+"\n"+service.connectionHint()+"\n"+service.visualizationStatus());
        if(processingDetails!=null)setLabel(processingDetails,service.processingStatus()+"\n"+service.visualizationStatus());
        if(powerButton!=null)setLabel(powerButton,service.monitor?"Disconnect CYN":"Connect CYN");
        if(playLabel!=null)setLabel(playLabel,service.title);if(position!=null&&!seeking){position.setMax(Math.max(1,service.duration()));position.setProgress(service.position());}if(positionLabel!=null)setLabel(positionLabel,time(service.position())+" / "+time(service.duration()));
        if(liveStatus!=null){DesktopAi.Result r=service.desktopAi.latest;String state=!service.enabled?"CYN bypassed":!service.smart?"AI off":service.playing?service.desktopAi.status:service.player!=null?"Playback paused":service.monitor?service.analysis:"Play a library track for AI listening";if(r!=null&&service.playing&&service.smart&&service.enabled&&SystemClock.elapsedRealtime()-r.at<2500)state+="\nMusic "+Math.round(r.scene[0]*100)+"% · Speech "+Math.round(r.scene[1]*100)+"% · "+r.milliseconds+" ms inference";setLabel(liveStatus,state);}
        if(learningStatus!=null){int more=0,less=0;for(PreferenceModel.Example e:service.learning.examples)if(e.effect==feedbackEffect){if(e.more)more++;else less++;}setLabel(learningStatus,new String[]{"Clarity","Ambience","Width","Dynamics","Bass"}[feedbackEffect]+" · "+more+" More / "+less+" Less\n"+(service.training?"Training "+service.trainingEpoch+" / 240":service.learning.message));}
        if(updateState!=null)setLabel(updateState,UpdateManager.message);if(timeline!=null)timeline.invalidate();if(neural!=null)neural.invalidate();if(spectrum!=null)spectrum.invalidate();
    }
    void setLabel(TextView view,String value){if(!value.contentEquals(view.getText()))view.setText(value);}
    final Runnable refresh=new Runnable(){public void run(){update();handler.postDelayed(this,250);}};
    @Override protected void onResume(){super.onResume();handler.removeCallbacks(refresh);handler.post(refresh);}
    @Override protected void onPause(){handler.removeCallbacks(refresh);super.onPause();}
    @Override protected void onDestroy(){handler.removeCallbacksAndMessages(null);filesWorker.shutdown();if(bound)unbindService(connection);super.onDestroy();}
    @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(request==71){if(getPackageManager().canRequestPackageInstalls())UpdateManager.install(this);return;}if(result!=RESULT_OK||data==null||service==null)return;
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
            DesktopAi.Result r=service.desktopAi.latest;
            long now=SystemClock.elapsedRealtime();
            boolean library=service.playing&&service.enabled&&r!=null&&now-r.at<2500;
            boolean external=!library&&service.hasSessionSpectrum();
            float[] externalBands=external?service.sessionSpectrum:null;
            float spectralPeak=-12;
            if(library)for(int i=0;i<8;i++)spectralPeak=Math.max(spectralPeak,r.features[i]);
            float volume=library?(float)Math.min(1,Math.sqrt(Math.max(0,r.features[11])*7)):0;
            float spacing=w/31f;
            boolean fading=false;
            paint.setStrokeCap(Paint.Cap.ROUND);
            paint.setStrokeWidth(Math.max(dp(2),spacing*.52f));
            for(int i=0;i<31;i++){
                float goal=0;
                if(library){
                    float position=i*7f/30;
                    int first=(int)position,second=Math.min(7,first+1);
                    float level=r.features[first]+(r.features[second]-r.features[first])*(position-first);
                    goal=(float)Math.sqrt(Math.exp(Math.max(-20,level-spectralPeak)))*volume;
                }else if(externalBands!=null&&i<externalBands.length)goal=externalBands[i];
                goal=Math.max(0,Math.min(1,goal));
                levels[i]+=(goal-levels[i])*(reduced?1:.23f);
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
            if(!reduced&&(library||external||fading)&&isAttachedToWindow())postInvalidateDelayed(40);
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
