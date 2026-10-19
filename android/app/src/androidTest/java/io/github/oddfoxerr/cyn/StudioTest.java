// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import android.content.*;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.*;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.rule.ActivityTestRule;
import org.junit.*;
import org.junit.runner.RunWith;
import java.io.*;
import java.nio.*;
import java.util.*;
import static org.junit.Assert.*;
@RunWith(AndroidJUnit4.class)
public class StudioTest {
    @Rule public ActivityTestRule<MainActivity> rule=new ActivityTestRule<>(MainActivity.class);
    private void main(Runnable action){InstrumentationRegistry.getInstrumentation().runOnMainSync(action);}
    private int position(AudioService service){int[] value={0};main(()->value[0]=service.position());return value[0];}
    private MainActivity ready()throws Exception{MainActivity activity=rule.getActivity();long until=SystemClock.elapsedRealtime()+15000;while(activity.service==null&&SystemClock.elapsedRealtime()<until)Thread.sleep(100);assertNotNull(activity.service);return activity;}
    @Test public void captureScreens()throws Exception{
        MainActivity a=ready();String output=InstrumentationRegistry.getArguments().getString("additionalTestOutputDir");File folder=output==null?new File(a.getExternalFilesDir(null),"qa"):new File(output);assertTrue(folder.isDirectory()||folder.mkdirs());
        for(int page=0;page<4;page++){final int p=page;main(()->{a.page=p;a.build(true);});Thread.sleep(800);
            Bitmap screenshot=InstrumentationRegistry.getInstrumentation().getUiAutomation().takeScreenshot();assertNotNull(screenshot);
            try(FileOutputStream out=new FileOutputStream(new File(folder,"screen-"+page+".png"))){assertTrue(screenshot.compress(Bitmap.CompressFormat.PNG,100,out));}finally{screenshot.recycle();}
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
}
