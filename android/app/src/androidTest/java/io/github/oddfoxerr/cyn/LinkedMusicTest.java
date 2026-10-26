// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import java.util.Collections;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public final class LinkedMusicTest {
    @Test public void allowsOnlySupportedSecureProviderLinks(){
        assertNotNull(LinkedMusic.parse("https://music.youtube.com/watch?v=dQw4w9WgXcQ&si=123"));
        assertNotNull(LinkedMusic.parse("https://youtu.be/dQw4w9WgXcQ"));
        assertNotNull(LinkedMusic.parse("https://music.youtube.com/playlist?list=PLabcdefghijklmn"));
        assertNotNull(LinkedMusic.parse("https://soundcloud.com/artist-name/track-name"));
        assertNotNull(LinkedMusic.parse("https://on.soundcloud.com/example"));
        assertNull(LinkedMusic.parse("https://open.spotify.com/track/anything"));
        assertNull(LinkedMusic.parse("https://music.youtube.com.evil.example/watch?v=dQw4w9WgXcQ"));
        assertNull(LinkedMusic.parse("http://music.youtube.com/watch?v=dQw4w9WgXcQ"));
        assertNull(LinkedMusic.parse("https://user@music.youtube.com/watch?v=dQw4w9WgXcQ"));
        assertNull(LinkedMusic.parse("https://soundcloud.com:8080/artist/track"));
        assertNull(LinkedMusic.parse("file:///sdcard/music.mp3"));
        assertNull(LinkedMusic.parse("https://music.youtube.com/watch?v=invalid"));
    }

    @Test public void libraryPersistsLinkAndUnfavoriteKeepsBookmark()throws Exception{
        AudioLibrary library=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
        Uri link=Uri.parse("https://music.youtube.com/watch?v=dQw4w9WgXcQ");
        try{
            library.merge(Collections.singletonList(
                new AudioLibrary.Track(link,"Linked music test","YouTube Music","Linked music",0,false)));
            AudioLibrary restored=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
            AudioLibrary.Track stored=null;
            for(AudioLibrary.Track item:restored.all())if(item.uri.equals(link))stored=item;
            assertNotNull(stored);
            assertFalse(stored.favorite);
            assertFalse(LikedDownloads.canDownload(stored));
            restored.favorite(link);
            restored.favorite(link);
            AudioLibrary after=new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext());
            AudioLibrary.Track retained=null;
            for(AudioLibrary.Track item:after.all())if(item.uri.equals(link))retained=item;
            assertNotNull("Unlike must preserve the saved bookmark",retained);
            assertFalse(retained.favorite);
        }finally{new AudioLibrary(InstrumentationRegistry.getInstrumentation().getTargetContext()).remove(link);}
    }

    @Test public void shortcutsAreSearchLinksNotDirectStreams(){
        Uri music=Uri.parse("https://music.youtube.com/search?q=Test%20song");
        Uri soundcloud=Uri.parse("https://soundcloud.com/search/sounds?q=Test%20song");
        assertTrue(LinkedMusic.isLinked(music));
        assertTrue(LinkedMusic.isLinked(soundcloud));
        assertTrue(LinkedMusic.isSearchShortcut(music));
        assertTrue(LinkedMusic.isSearchShortcut(soundcloud));
        assertEquals("YouTube Music",LinkedMusic.provider(music));
        assertEquals("SoundCloud",LinkedMusic.provider(soundcloud));
        assertFalse(LikedDownloads.isAudiusStream(music));
        assertFalse(LikedDownloads.isAudiusStream(soundcloud));
    }
}
