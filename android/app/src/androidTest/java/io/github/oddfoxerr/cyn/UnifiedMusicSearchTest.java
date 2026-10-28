// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public final class UnifiedMusicSearchTest {
    @Test public void oneSearchPrefersActualPhoneFileOverAudiusWithoutProviderSelection(){
        PublicMusic.Song audius=new PublicMusic.Song("cynSearch123","Faded","Alan Walker",200000);
        AudioLibrary.Track linked=new AudioLibrary.Track(
            Uri.parse("https://music.youtube.com/watch?v=dQw4w9WgXcQ"),
            "Faded","Alan Walker","Linked music",0,false);
        AudioLibrary.Track local=new AudioLibrary.Track(
            Uri.parse("content://media/external/audio/media/888881"),
            "Faded","Alan Walker","Saved local",200000,false);
        List<UnifiedMusicSearch.Result> results=UnifiedMusicSearch.combine(
            "faded",Arrays.asList(linked,local),Collections.singletonList(audius));
        assertEquals(1,results.size());
        UnifiedMusicSearch.Result match=results.get(0);
        assertNotNull(match.audius);
        assertSame(local,match.playableTrack());
        assertSame(local,match.savedPlayableTrack());
        assertTrue(match.savedFrom("Local"));
        assertFalse(match.savedFrom("YouTube Music"));
        assertFalse(match.savedFrom("SoundCloud"));
        assertTrue(match.availableSources().contains("Audius"));
        assertFalse(match.availableSources().contains("YouTube Music"));
    }

    @Test public void linkOnlyResultsAreNotPresentedAsPlayableAudio(){
        AudioLibrary.Track yt=new AudioLibrary.Track(
            Uri.parse("https://music.youtube.com/watch?v=dQw4w9WgXcQ"),
            "Faded","Alan Walker","Linked music",0,false);
        AudioLibrary.Track sc=new AudioLibrary.Track(
            Uri.parse("https://soundcloud.com/artist/track"),
            "Faded","Alan Walker","Linked music",0,false);
        assertTrue(UnifiedMusicSearch.combine("faded",Arrays.asList(yt,sc),
            Collections.emptyList()).isEmpty());
    }

    @Test public void audiusResultPlaysNativelyWhenNoLocalCopyExists(){
        PublicMusic.Song remote=new PublicMusic.Song("cynTest123","Phonk Song","DJ CYN",150000);
        UnifiedMusicSearch.Result result=UnifiedMusicSearch.combine(
            "phonk",Collections.emptyList(),Collections.singletonList(remote)).get(0);
        assertNotNull(result.playableTrack());
        assertTrue(LikedDownloads.isAudiusStream(result.playableTrack().uri));
        assertNull(result.savedPlayableTrack());
    }

    @Test public void filtersOutUnrelatedMusicAndHandlesEmptyQueries(){
        PublicMusic.Song found=new PublicMusic.Song("a","The Midnight","Artist",100000);
        PublicMusic.Song unrelated=new PublicMusic.Song("b","Morning Drive","Elsewhere",100000);
        assertEquals(1,UnifiedMusicSearch.combine("midnight",Collections.emptyList(),
            Arrays.asList(found,unrelated)).size());
        assertTrue(UnifiedMusicSearch.combine(" ",Collections.emptyList(),
            Arrays.asList(found,unrelated)).isEmpty());
    }

    @Test public void noSavedLinkMeansNoVerifiedThirdPartyAvailability(){
        PublicMusic.Song audius=new PublicMusic.Song("c","Phonk Song","DJ CYN",100000);
        UnifiedMusicSearch.Result result=UnifiedMusicSearch.combine(
            "Phonk",Collections.emptyList(),Collections.singletonList(audius)).get(0);
        assertNotNull(result.audius);
        assertFalse(result.savedFrom("YouTube Music"));
        assertFalse(result.savedFrom("SoundCloud"));
        assertEquals("Audius · found",result.availableSources());
    }
}
