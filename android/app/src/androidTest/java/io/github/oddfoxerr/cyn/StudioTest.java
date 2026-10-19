// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.*;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.*;
import android.view.PixelCopy;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import org.junit.*;
import org.junit.runner.RunWith;
import java.io.*;
import java.nio.*;
import java.util.*;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import static org.junit.Assert.*;
@RunWith(AndroidJUnit4.class)
public class StudioTest {
    @Rule public ActivityTestRule<MainActivity> rule=new ActivityTestRule<>(MainActivity.class);
    @Before public void usePhoneTouchMode(){InstrumentationRegistry.getInstrumentation().setInTouchMode(true);}
    @Test public void studioIdentityAndUpdateProvider()throws Exception{
        MainActivity a=ready();
        assertEquals("io.github.oddfoxerr.cyn.studio",a.getPackageName());
        android.content.pm.ProviderInfo provider=a.getPackageManager().resolveContentProvider(a.getPackageName()+".updates",0);
        assertNotNull("Studio has its own update FileProvider",provider);
        assertEquals(a.getPackageName(),provider.packageName);
        assertFalse(provider.exported);
        assertTrue(provider.grantUriPermissions);
        assertEquals("CYN Studio",a.getApplicationInfo().loadLabel(a.getPackageManager()).toString());
    }
    private void main(Runnable action){InstrumentationRegistry.getInstrumentation().runOnMainSync(action);}
    private int position(AudioService service){int[] value={0};main(()->value[0]=service.position());return value[0];}
    private MainActivity ready()throws Exception{MainActivity activity=rule.getActivity();long until=SystemClock.elapsedRealtime()+15000;while(activity.service==null&&SystemClock.elapsedRealtime()<until)Thread.sleep(100);assertNotNull(activity.service);return activity;}
    private void waitForTransition(MainActivity activity)throws Exception{
        boolean[] running={true};long until=SystemClock.elapsedRealtime()+10000;
        do{Thread.sleep(50);main(()->running[0]=activity.pages.isTransitioning());}while(running[0]&&SystemClock.elapsedRealtime()<until);
        assertFalse("Page transition did not finish",running[0]);Thread.sleep(150);
    }
    private void assertTextVisible(Bitmap screenshot,View view){
        int[] bounds=new int[4];main(()->{int[] at=new int[2];view.getLocationInWindow(at);bounds[0]=at[0];bounds[1]=at[1];bounds[2]=at[0]+view.getWidth();bounds[3]=at[1]+view.getHeight();});
        int pixels=0;
        for(int y=Math.max(0,bounds[1]);y<Math.min(screenshot.getHeight(),bounds[3]);y++)for(int x=Math.max(0,bounds[0]);x<Math.min(screenshot.getWidth(),bounds[2]);x++){
            int color=screenshot.getPixel(x,y);if(((color>>16)&255)>100&&((color>>8)&255)>100&&(color&255)>100)pixels++;
        }
        assertTrue("Persistent phone control text disappeared",pixels>20);
    }
    private Bitmap captureWindow(MainActivity activity)throws Exception{
        // Capture CYN's rendered surface, excluding unrelated emulator/launcher dialogs.
        InstrumentationRegistry.getInstrumentation().waitForIdleSync();
        Bitmap[] image=new Bitmap[1];int[] result={PixelCopy.ERROR_UNKNOWN};
        CountDownLatch copied=new CountDownLatch(1);
        main(()->{
            View decor=activity.getWindow().getDecorView();
            image[0]=Bitmap.createBitmap(decor.getWidth(),decor.getHeight(),Bitmap.Config.ARGB_8888);
            PixelCopy.request(activity.getWindow(),image[0],status->{result[0]=status;copied.countDown();},new Handler(Looper.getMainLooper()));
        });
        boolean complete=copied.await(10,TimeUnit.SECONDS);
        if(!complete||result[0]!=PixelCopy.SUCCESS){image[0].recycle();fail("CYN window capture failed: "+(complete?result[0]:"timeout"));}
        return image[0];
    }
    @Test public void captureScreens()throws Exception{
        MainActivity a=ready();String output=InstrumentationRegistry.getArguments().getString("additionalTestOutputDir");File folder=output==null?new File(a.getExternalFilesDir(null),"qa"):new File(output);assertTrue(folder.isDirectory()||folder.mkdirs());
        for(int page=0;page<4;page++){final int p=page;main(()->{a.page=p;a.build(true);});waitForTransition(a);
            Bitmap screenshot=captureWindow(a);
            try{
                try(FileOutputStream out=new FileOutputStream(new File(folder,"screen-"+page+".png"))){assertTrue(screenshot.compress(Bitmap.CompressFormat.PNG,100,out));}
                assertTextVisible(screenshot,a.shell.getChildAt(0));assertTextVisible(screenshot,a.miniTitle);assertTextVisible(screenshot,a.miniToggle);
                for(int tab=0;tab<4;tab++)assertTextVisible(screenshot,a.nav.getChildAt(tab));
            }finally{screenshot.recycle();}
        }
    }
    @Test public void screensAndRapidTransitions()throws Exception{MainActivity a=ready();for(int page:new int[]{0,2,1,3,2,0,3,1}){final int p=page;main(()->{a.page=p;a.build(true);});}Thread.sleep(600);main(()->{assertEquals(4,a.nav.getChildCount());assertTrue(a.pages.getChildCount()==1);assertNotNull(a.content);});}
    @Test public void fullModelsAndBypass()throws Exception{
        MainActivity a=ready();DesktopAi ai=new DesktopAi(a);NativeEngine.configure(ai.engine,2);
        ByteBuffer pcm=ByteBuffer.allocateDirect(48000*4).order(ByteOrder.nativeOrder());for(int i=0;i<48000;i++){short value=(short)(8000*Math.sin(2*Math.PI*440*i/48000));pcm.putShort(value).putShort(value);}byte[]expected=new byte[pcm.capacity()];pcm.rewind();pcm.get(expected);pcm.rewind();NativeEngine.process(ai.engine,pcm,pcm.capacity());byte[]actual=new byte[pcm.capacity()];pcm.rewind();pcm.get(actual);assertArrayEquals(expected,actual);
        ai.apexEnabled=true;ai.active=true;long end=SystemClock.elapsedRealtime()+120000;while(ai.latest==null&&SystemClock.elapsedRealtime()<end)Thread.sleep(250);
        try{assertNotNull(ai.status,ai.latest);assertEquals(8,ai.latest.apex.length);assertEquals(13,ai.latest.decision.length);for(float f:ai.latest.apex)assertTrue(Float.isFinite(f));}finally{ai.closeAfterPlayer();}
    }
    @Test public void playbackSeekPauseAndPersistence()throws Exception{
        MainActivity a=ready();File file=new File(a.getFilesDir(),"studio-test.wav");int rate=44100,frames=rate*8;ByteBuffer data=ByteBuffer.allocate(44+frames*4).order(ByteOrder.LITTLE_ENDIAN);
        data.put("RIFF".getBytes("US-ASCII")).putInt(data.capacity()-8).put("WAVEfmt ".getBytes("US-ASCII")).putInt(16).putShort((short)1).putShort((short)2).putInt(rate).putInt(rate*4).putShort((short)4).putShort((short)16).put("data".getBytes("US-ASCII")).putInt(frames*4);
        for(int i=0;i<frames;i++){short sample=(short)(5000*Math.sin(2*Math.PI*220*i/rate));data.putShort(sample).putShort(sample);}try(FileOutputStream out=new FileOutputStream(file)){out.write(data.array());}
        AudioLibrary.Track track=new AudioLibrary.Track(Uri.fromFile(file),"Playback test","CYN tests","QA",8000,false);
        main(()->a.service.playQueue(Collections.singletonList(track),0));long end=SystemClock.elapsedRealtime()+30000;while(position(a.service)<800&&SystemClock.elapsedRealtime()<end)Thread.sleep(100);assertTrue(a.service.status,position(a.service)>=800);
        main(()->a.service.seek(4000));Thread.sleep(800);assertTrue(position(a.service)>=3900);main(()->a.service.pause());int at=position(a.service);Thread.sleep(500);assertTrue(Math.abs(at-position(a.service))<100);main(()->a.service.stopAll());
        AudioLibrary library=new AudioLibrary(a);Uri fake=Uri.parse("content://media/external/audio/media/99999999");library.merge(Collections.singletonList(new AudioLibrary.Track(fake,"Persistent favorite","Artist","Album",1000,false)));library.favorite(fake);assertTrue(new AudioLibrary(a).all().stream().anyMatch(t->t.uri.equals(fake)&&t.favorite));library.remove(fake);
    }
    @Test public void externalMonitoringPreservesPowerAndReportsActualControl()throws Exception {
        MainActivity a=ready();
        main(()->{
            AudioService service=a.service;
            boolean previous=service.enabled;
            service.stopMonitor();
            try {
                service.enabled=false;
                service.changed();
                service.onStartCommand(new Intent(a,AudioService.class).setAction(AudioService.MONITOR),0,1);
                assertTrue("Monitor should start",service.monitor);
                assertFalse("Connecting must not force enhancement on",service.enabled);
                assertEquals("Enhancement off · monitoring only",service.connectionState());
                assertEquals(0,service.controlledExternalEqCount());
                service.stopMonitor();
                service.enabled=true;
                service.changed();
                service.onStartCommand(new Intent(a,AudioService.class).setAction(AudioService.MONITOR),0,2);
                if(service.externalSessionCount()==0)
                    assertEquals("Waiting for a compatible music session",service.connectionState());
                else if(service.controlledExternalEqCount()==0)
                    assertEquals("Music session detected · EQ unavailable",service.connectionState());
            } finally {
                service.stopMonitor();
                service.enabled=previous;
                service.changed();
            }
        });
    }

    @Test public void spectrumUsesOnlyMeasuredBinsAndSilenceDecays()throws Exception {
        byte[] silent=new byte[1024];
        float[] empty=AudioFeatures.spectrum(silent,48000);
        assertEquals(31,empty.length);
        for(float level:empty)assertEquals(0f,level,0.00001f);
        byte[] fft=new byte[1024];
        fft[32]=100;
        fft[128]=30;
        float[] measured=AudioFeatures.spectrum(fft,48000);
        int active=0;float strongest=0;
        for(float level:measured){
            assertTrue(Float.isFinite(level));
            assertTrue(level>=0f&&level<=1f);
            if(level>.01f)active++;
            strongest=Math.max(strongest,level);
        }
        assertEquals("Only occupied spectrum bands should react",2,active);
        assertTrue(strongest>.95f);
    }
    @Test public void connectionDetailsDoNotExposeDeviceIdentity()throws Exception {
        MainActivity a=ready();
        main(()->{
            String details=a.service.diagnostics();
            assertTrue(details.contains("CYN Studio"));
            assertTrue(details.contains("Audio remains on your device"));
            String model=Build.MODEL,manufacturer=Build.MANUFACTURER;
            if(model!=null&&model.length()>2)assertFalse(details.contains(model));
            if(manufacturer!=null&&manufacturer.length()>2)assertFalse(details.contains(manufacturer));
            assertFalse("Connection text should not contain an audio session identifier",details.contains(" · session "));
            assertNotNull(a.service.processingStatus());
        });
    }

    @Test public void connectVisualsExplainPermissionAndNeverInventActivity()throws Exception {
        MainActivity a=ready();
        main(()->{
            AudioService audio=a.service;
            boolean wasEnabled=audio.enabled;
            boolean wasSmart=audio.smart;
            try {
                audio.stopMonitor();
                audio.enabled=true;
                audio.smart=true;
                audio.changed();
                assertFalse(audio.hasSessionSpectrum());
                assertTrue(audio.visualizationStatus().contains("Connect CYN"));
                audio.onStartCommand(new Intent(a,AudioService.class).setAction(AudioService.MONITOR),0,200);
                assertTrue(audio.monitor);
                assertNotNull(audio.visualizationStatus());
                if(a.checkSelfPermission(android.Manifest.permission.RECORD_AUDIO)!=android.content.pm.PackageManager.PERMISSION_GRANTED)
                    assertTrue("Visualizer must request permission rather than simulate measurements",
                        audio.visualizationStatus().contains("permission"));
                audio.stopMonitor();
                assertFalse("Stopping CYN clears external visualization",audio.hasSessionSpectrum());
            }finally{
                audio.stopMonitor();
                audio.enabled=wasEnabled;
                audio.smart=wasSmart;
                audio.changed();
            }
        });
    }

    @Test public void soundControlMarkersOnlyUseAppliedAiTargets()throws Exception {
        MainActivity a=ready();
        main(()->{
            AudioService audio=a.service;
            boolean originalSmart=audio.smart,originalEnabled=audio.enabled;
            boolean originalPlaying=audio.playing;
            try {
                audio.playing=false;
                audio.smart=false;
                audio.changed();
                assertTrue(Float.isNaN(audio.aiControlValue("Bass",-1)));
                assertTrue(Float.isNaN(audio.aiControlValue("EQ",0)));
                audio.smart=true; audio.enabled=false; audio.changed();
                assertTrue(Float.isNaN(audio.aiControlValue("Vocal clarity",-1)));
                assertTrue(Float.isNaN(audio.aiControlValue("Stereo width",-1)));
            }finally {
                audio.playing=originalPlaying;
                audio.smart=originalSmart;audio.enabled=originalEnabled;audio.changed();
            }
        });
    }

    @Test public void aiThumbMovesButBaselineRingNeverMovesAndReturnsWhenOff()throws Exception {
        MainActivity a=ready();
        main(()->{
            AudioService audio=a.service;
            boolean originalEnabled=audio.enabled, originalSmart=audio.smart, originalPlaying=audio.playing;
            int oldBandCount=audio.bandCount;
            try {
                a.page=0;
                a.bands(39);
                audio.enabled=true;audio.smart=true;audio.playing=true;
                audio.appliedAiFresh=true;
                audio.appliedTargetAt=SystemClock.elapsedRealtime();
                audio.appliedTargets[1]=10f;
                audio.appliedTargets[3]=.82f;
                audio.appliedTargets[4]=.73f;
                audio.appliedTargets[5]=.67f;
                audio.appliedTargets[9+38]=7f;
                MainActivity.AiControlMarker bass=null,width=null,ambience=null,dynamics=null,lastEq=null,headroom=null;
                int eqBars=0;
                for(MainActivity.AiControlMarker bar:a.liveControlMarkers){
                    if("Bass".equals(bar.key))bass=bar;
                    if("Stereo width".equals(bar.key))width=bar;
                    if("Ambience".equals(bar.key))ambience=bar;
                    if("Dynamics".equals(bar.key))dynamics=bar;
                    if("Output headroom".equals(bar.key))headroom=bar;
                    if("EQ".equals(bar.key)){
                        eqBars++;
                        if(bar.band==38)lastEq=bar;
                    }
                }
                assertEquals("All 39 EQ bands must expose AI-following thumbs",39,eqBars);
                assertNotNull("Space & Dynamics width uses AI thumb",width);
                assertNotNull("Space & Dynamics ambience uses AI thumb",ambience);
                assertNotNull("Space & Dynamics dynamics uses AI thumb",dynamics);
                assertNotNull("Headroom retains an explicit manual ring",headroom);
                assertNotNull(lastEq);
                assertNotNull(bass);
                float savedBass=bass.manualValue, savedWidth=width.manualValue;
                float savedEq=lastEq.manualValue;
                for(int n=0;n<22;n++){bass.sync();width.sync();ambience.sync();dynamics.sync();lastEq.sync();}
                assertEquals("AI cannot rewrite the saved bass ring",savedBass,bass.manualValue,0.001f);
                assertEquals("AI cannot rewrite the width ring",savedWidth,width.manualValue,0.001f);
                assertEquals("AI cannot rewrite the EQ band ring",savedEq,lastEq.manualValue,0.001f);
                assertEquals(bass.position(10f),bass.getProgress(),2);
                assertEquals(width.position(82f),width.getProgress(),2);
                assertEquals(lastEq.position(7f),lastEq.getProgress(),2);
                audio.smart=false;
                for(int n=0;n<22;n++){bass.sync();width.sync();ambience.sync();dynamics.sync();lastEq.sync();}
                assertEquals("AI-off returns draggable thumb to manual bass ring",bass.position(savedBass),bass.getProgress(),2);
                assertEquals("AI-off returns width thumb to manual ring",width.position(savedWidth),width.getProgress(),2);
                assertEquals("AI-off returns EQ thumb to manual ring",lastEq.position(savedEq),lastEq.getProgress(),2);
            }finally{
                audio.enabled=originalEnabled;audio.smart=originalSmart;audio.playing=originalPlaying;
                audio.bandCount=oldBandCount;audio.changed();a.build();
            }
        });
    }

    @Test public void singleUpdateButtonAndMechanicalSplashContract()throws Exception {
        MainActivity a=ready();
        main(()->{
            a.page=3;
            a.build();
            java.util.ArrayList<String> names=new java.util.ArrayList<>();
            collectLabels(a.content,names);
            assertEquals("One visible entry point controls CYN updates",
                1,java.util.Collections.frequency(names,"UPDATE CYN"));
            assertFalse("Old check and download button must be gone",
                names.contains("Check and download update"));
            assertFalse("Old second install button must be gone",
                names.contains("Install downloaded update"));
            assertTrue("Automatic update option must explain Wi-Fi behavior",
                names.contains("Automatic updates · check and download on Wi-Fi"));
        });
        assertEquals("SEARCHING RELEASES",UpdateSplash.progressText(UpdateManager.CHECKING,-1));
        assertEquals("VERIFYING APK",UpdateSplash.progressText(UpdateManager.VERIFYING,100));
        assertEquals("UP TO DATE",UpdateSplash.progressText(UpdateManager.UP_TO_DATE,-1));
        assertEquals("41%  •  DOWNLOADING",UpdateSplash.progressText(UpdateManager.DOWNLOADING,41));
        assertEquals("INSTALL VERIFIED APK",UpdateSplash.actionText(UpdateManager.READY,true));
        assertEquals("RETRY UPDATE",UpdateSplash.actionText(UpdateManager.FAILED,false));
    }
    private static void collectLabels(View root,java.util.List<String> values) {
        if(root instanceof android.widget.TextView)values.add(
            ((android.widget.TextView)root).getText().toString());
        if(root instanceof android.view.ViewGroup){
            android.view.ViewGroup group=(android.view.ViewGroup)root;
            for(int i=0;i<group.getChildCount();i++)collectLabels(group.getChildAt(i),values);
        }
    }
    @Test public void updateArtUsesMeasuredDownloadPercentageWithoutFakeFill()throws Exception{
        MainActivity a=ready();
        main(()->{
            UpdateSplash.SplashArt art=new UpdateSplash.SplashArt(a,a.accent,true);
            art.layout(0,0,440,780);
            Bitmap bitmap=Bitmap.createBitmap(440,780,Bitmap.Config.ARGB_8888);
            try {
                android.graphics.Canvas canvas=new android.graphics.Canvas(bitmap);
                art.state(UpdateManager.CHECKING,-1);
                art.draw(canvas);
                int color=bitmap.getPixel(220,449);
                assertEquals("Checker stage has no fake filled transfer area",UpdateSplash.INK,color);
                art.state(UpdateManager.DOWNLOADING,70);
                art.draw(canvas);
                // State change must draw without crashing even with reduced motion.
                art.state(UpdateManager.VERIFYING,100);
                art.draw(canvas);
                art.state(UpdateManager.READY,100);
                art.draw(canvas);
                assertNotEquals("The mechanical art must draw its second tone",
                    UpdateSplash.INK,bitmap.getPixel(220,330));
            }finally{bitmap.recycle();}
        });
    }

}
