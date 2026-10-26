// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.net.Uri;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public final class ProviderBrowserTest {
    @Test public void searchesStayInsideTheCorrectProvider(){
        Uri youtube=ProviderBrowserActivity.searchUrl("youtube","Alan Walker Faded");
        Uri soundcloud=ProviderBrowserActivity.searchUrl("soundcloud","phonk");
        assertNotNull(youtube);assertNotNull(soundcloud);
        assertEquals("music.youtube.com",youtube.getHost());
        assertEquals("/search",youtube.getPath());
        assertEquals("Alan Walker Faded",youtube.getQueryParameter("q"));
        assertEquals("soundcloud.com",soundcloud.getHost());
        assertEquals("/search/sounds",soundcloud.getPath());
        assertEquals("phonk",soundcloud.getQueryParameter("q"));
        assertNull(ProviderBrowserActivity.searchUrl("other","music"));
        assertNull(ProviderBrowserActivity.searchUrl("youtube"," "));
        assertTrue(ProviderBrowserActivity.isProviderPage("youtube",youtube));
        assertTrue(ProviderBrowserActivity.isProviderPage("soundcloud",soundcloud));
        assertFalse(ProviderBrowserActivity.isProviderPage("youtube",soundcloud));
        assertFalse(ProviderBrowserActivity.isProviderPage("soundcloud",youtube));
    }

    @Test public void browserRejectsCredentialsInUrlsAndCrossSiteMainNavigation(){
        assertFalse(ProviderBrowserActivity.isProviderPage("youtube",
            Uri.parse("https://music.youtube.com.evil.example/search?q=song")));
        assertFalse(ProviderBrowserActivity.isProviderPage("soundcloud",
            Uri.parse("https://user@soundcloud.com/artist/track")));
        assertFalse(ProviderBrowserActivity.isProviderPage("soundcloud",
            Uri.parse("http://soundcloud.com/artist/track")));
        assertFalse(ProviderBrowserActivity.isProviderPage("youtube",
            Uri.parse("https://accounts.google.com/ServiceLogin")));
    }

    @Test public void onlySelectedItemsAreSavedNotSearchPages(){
        assertFalse(LinkedMusic.isExactItem(Uri.parse("https://music.youtube.com/search?q=artist")));
        assertFalse(LinkedMusic.isExactItem(Uri.parse("https://soundcloud.com/search/sounds?q=artist")));
        assertTrue(LinkedMusic.isExactItem(Uri.parse(
            "https://music.youtube.com/watch?v=dQw4w9WgXcQ")));
        assertTrue(LinkedMusic.isExactItem(Uri.parse(
            "https://soundcloud.com/example-artist/example-song")));
        assertFalse(LinkedMusic.isExactItem(Uri.parse(
            "https://open.spotify.com/track/some-id")));
    }
}
