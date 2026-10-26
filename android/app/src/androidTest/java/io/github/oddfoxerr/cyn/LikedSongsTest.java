// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;
import java.util.Collections;
import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public final class LikedSongsTest {
    @Test public void onlyKnownAudiusStreamUrisAreEligible(){
        Uri valid=Uri.parse("https://api.audius.co/v1/tracks/eQ123/stream?app_name=CYN");
        assertTrue(LikedDownloads.isAudiusStream(valid));
        assertFalse(LikedDownloads.isAudiusStream(Uri.parse("http://api.audius.co/v1/tracks/eQ123/stream")));
        assertFalse(LikedDownloads.isAudiusStream(Uri.parse("https://evil.example/v1/tracks/eQ123/stream")));
        assertFalse(LikedDownloads.isAudiusStream(Uri.parse("https://api.audius.co/v1/tracks/../download")));
        assertFalse(LikedDownloads.isAudiusStream(Uri.parse("https://api.audius.co/v1/tracks/eQ123/download")));
        assertFalse(LikedDownloads.isAudiusStream(Uri.parse("content://media/external/audio/media/17")));
        AudioLibrary.Track track=new AudioLibrary.Track(valid,"My Song","An Artist","Audius",36000,true);
        assertTrue(LikedDownloads.canDownload(track));
        track.favorite=false;
        assertTrue("Unliked tracks remain download-eligible",LikedDownloads.canDownload(track));
        track.favorite=true;
        track.offlineUri=Uri.parse("content://media/external/audio/media/17");
        assertFalse(LikedDownloads.canDownload(track));
        assertEquals(track.offlineUri,track.playbackUri());
    }

    @Test public void unlikingRetainsSavedTrackAndDownloadEligibility()throws Exception{
        AudioLibrary library=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
        Uri source=Uri.parse("https://api.audius.co/v1/tracks/CynLibraryIndependent20261026/stream?app_name=CYN");
        try{
            library.merge(Collections.singletonList(new AudioLibrary.Track(source,"Independent","Artist","Audius",9000,false)));
            AudioLibrary.Track track=null;
            for(AudioLibrary.Track value:library.all())if(value.uri.equals(source))track=value;
            assertNotNull(track);
            assertFalse(track.favorite);
            assertTrue(LikedDownloads.canDownload(track));
            library.favorite(source);
            library.favorite(source);
            AudioLibrary reloaded=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
            AudioLibrary.Track retained=null;
            for(AudioLibrary.Track value:reloaded.all())if(value.uri.equals(source))retained=value;
            assertNotNull("Unfavourite must not remove Library membership",retained);
            assertFalse(retained.favorite);
            assertTrue(LikedDownloads.canDownload(retained));
        }finally{new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext()).remove(source);}
    }

    @Test public void downloadedLikedMusicPersistsAndClearingLikesDoesNotDeleteTracks()throws Exception{
        AudioLibrary library=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
        Uri stream=Uri.parse("https://api.audius.co/v1/tracks/CynLikedTest987654/stream?app_name=CYN");
        Uri offline=Uri.parse("content://media/external/audio/media/17");
        try{
            library.merge(Collections.singletonList(new AudioLibrary.Track(stream,"Test Song","Test Artist","Audius",20000,true)));
            library.downloaded(stream,offline);
            AudioLibrary reloaded=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
            AudioLibrary.Track found=null;
            for(AudioLibrary.Track t:reloaded.all())if(t.uri.equals(stream))found=t;
            assertNotNull(found);
            assertTrue(found.favorite);
            assertEquals(offline,found.offlineUri);
            reloaded.clearFavorites();
            AudioLibrary verified=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
            AudioLibrary.Track remaining=null;
            for(AudioLibrary.Track t:verified.all())if(t.uri.equals(stream))remaining=t;
            assertNotNull(remaining);
            assertFalse(remaining.favorite);
            assertEquals(offline,remaining.offlineUri);
        }finally{new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext()).remove(stream);}
    }
}
