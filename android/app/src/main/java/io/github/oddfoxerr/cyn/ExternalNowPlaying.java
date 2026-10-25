// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.app.Notification;
import android.content.ComponentName;
import android.content.Context;
import android.media.MediaMetadata;
import android.media.session.MediaController;
import android.media.session.MediaSessionManager;
import android.media.session.PlaybackState;
import android.service.notification.NotificationListenerService;
import java.util.List;
import java.util.HashSet;
import java.util.Set;

/** Reads only user-authorized media-session metadata, never notification text. */
public final class ExternalNowPlaying extends NotificationListenerService {
    static volatile ExternalNowPlaying instance;
    static final class Track {
        final String title, artist, app;
        final boolean playing;
        final MediaController controller;
        Track(String title,String artist,String app,boolean playing,MediaController controller) {
            this.title=title;this.artist=artist;this.app=app;this.playing=playing;this.controller=controller;
        }
    }
    @Override public void onListenerConnected(){super.onListenerConnected();instance=this;}
    @Override public void onListenerDisconnected(){instance=null;super.onListenerDisconnected();}
    @Override public void onDestroy(){instance=null;super.onDestroy();}
    static Track current(Context context) {
        if(instance==null)return null;
        try {
            MediaSessionManager manager=(MediaSessionManager)context.getSystemService(Context.MEDIA_SESSION_SERVICE);
            List<MediaController> sessions=manager.getActiveSessions(
                new ComponentName(context,ExternalNowPlaying.class));
            Track paused=null;
            for(MediaController controller:sessions) {
                if(context.getPackageName().equals(controller.getPackageName()))continue;
                MediaMetadata metadata=controller.getMetadata();
                PlaybackState state=controller.getPlaybackState();
                if(metadata==null||state==null)continue;
                String title=metadata.getString(MediaMetadata.METADATA_KEY_TITLE);
                if(title==null||title.trim().isEmpty())continue;
                String artist=metadata.getString(MediaMetadata.METADATA_KEY_ARTIST);
                String app=controller.getPackageName();
                try { app=context.getPackageManager().getApplicationLabel(
                    context.getPackageManager().getApplicationInfo(app,0)).toString(); }
                catch(Exception ignored){}
                boolean playing=state.getState()==PlaybackState.STATE_PLAYING||
                    state.getState()==PlaybackState.STATE_BUFFERING;
                Track track=new Track(title,artist==null?"":artist,app,playing,controller);
                if(playing)return track;
                if(paused==null&&state.getState()==PlaybackState.STATE_PAUSED)paused=track;
            }
            return paused;
        } catch(RuntimeException ignored) { return null; }
    }
    /**
     * Only package identities of user-authorized, currently playing MediaSessions.
     * Kept in memory for per-session DSP ownership; never shown in diagnostics.
     * MediaSession metadata alone cannot expose an effect session identifier.
     */
    static Set<String> playingPackages(Context context) {
        Set<String> playing=new HashSet<>();
        if(instance==null)return playing;
        try {
            MediaSessionManager manager=(MediaSessionManager)
                context.getSystemService(Context.MEDIA_SESSION_SERVICE);
            List<MediaController> sessions=manager.getActiveSessions(
                new ComponentName(context,ExternalNowPlaying.class));
            for(MediaController controller:sessions) {
                String pkg=controller.getPackageName();
                if(pkg==null||pkg.isEmpty()||context.getPackageName().equals(pkg))continue;
                PlaybackState state=controller.getPlaybackState();
                if(state!=null&&state.getState()==PlaybackState.STATE_PLAYING)
                    playing.add(pkg);
            }
        } catch(RuntimeException ignored) { playing.clear(); }
        return playing;
    }
    static void toggle(Track track) {
        if(track==null)return;
        try {
            if(track.playing)track.controller.getTransportControls().pause();
            else track.controller.getTransportControls().play();
        }catch(RuntimeException ignored){}
    }
}
