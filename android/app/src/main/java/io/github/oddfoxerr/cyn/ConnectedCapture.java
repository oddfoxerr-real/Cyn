// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Handler;
import android.os.Process;
import android.os.SystemClock;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/** Permissioned, local playback analysis. Never records the microphone or replays the mix. */
final class ConnectedCapture {
    final DesktopAi ai;
    final PcmSpectrum spectrum = new PcmSpectrum();
    private final MediaProjection projection;
    private final MediaProjection.Callback callback;
    private final Handler main;
    private final Runnable onStopped;
    private final AudioRecord recorder;
    private volatile boolean running = true;
    volatile boolean analyze;
    volatile long audibleAt;
    volatile String status = "Waiting for shared playback";

    @androidx.annotation.RequiresApi(29)
    ConnectedCapture(Context context, Intent consent, Handler main, Runnable onStopped) {
        if (context.checkSelfPermission(Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED)
            throw new SecurityException("Playback analysis permission required");
        this.main = main;
        this.onStopped = onStopped;
        projection = context.getSystemService(MediaProjectionManager.class)
            .getMediaProjection(Activity.RESULT_OK, consent);
        if (projection == null) throw new IllegalStateException("Playback sharing unavailable");
        callback = new MediaProjection.Callback() {
            @Override public void onStop() { stop(); }
        };
        projection.registerCallback(callback, main);
        AudioRecord created = null;
        try {
            AudioPlaybackCaptureConfiguration capture = new AudioPlaybackCaptureConfiguration.Builder(projection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
                .addMatchingUsage(AudioAttributes.USAGE_GAME)
                .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
                .excludeUid(Process.myUid()).build();
            int minimum = AudioRecord.getMinBufferSize(48000, AudioFormat.CHANNEL_IN_STEREO,
                AudioFormat.ENCODING_PCM_16BIT);
            if (minimum <= 0) throw new IllegalStateException("Stereo playback capture unsupported");
            created = new AudioRecord.Builder().setAudioPlaybackCaptureConfig(capture)
                .setAudioFormat(new AudioFormat.Builder().setSampleRate(48000)
                    .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT).build())
                .setBufferSizeInBytes(Math.max(minimum, 19200)).build();
            if (created.getState() != AudioRecord.STATE_INITIALIZED)
                throw new IllegalStateException("Playback capture failed to initialize");
            created.startRecording();
            if (created.getRecordingState() != AudioRecord.RECORDSTATE_RECORDING)
                throw new IllegalStateException("Playback capture did not start");
            recorder = created;
            ai = new DesktopAi(context);
            NativeEngine.configure(ai.engine, 2);
            spectrum.setActive(true);
        } catch (RuntimeException | LinkageError failure) {
            if (created != null) created.release();
            projection.unregisterCallback(callback);
            projection.stop();
            spectrum.close();
            throw failure;
        }
        Thread worker = new Thread(this::read, "CYN connected playback");
        worker.start();
    }

    boolean fresh() {
        return running && analyze && audibleAt > 0 && SystemClock.elapsedRealtime() - audibleAt < 900;
    }

    private void read() {
        Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
        ByteBuffer pcm = ByteBuffer.allocateDirect(1920 * 4).order(ByteOrder.LITTLE_ENDIAN);
        try {
            while (running) {
                pcm.clear();
                int bytes = recorder.read(pcm, pcm.capacity(), AudioRecord.READ_BLOCKING);
                if (!running) break;
                if (bytes < 0) throw new IllegalStateException("Playback capture ended: " + bytes);
                if (bytes == 0) continue;
                bytes -= bytes % 4;
                pcm.position(0).limit(bytes);
                // Keep the ring current during pause/bypass, so a later AI enable
                // cannot consume old library audio or an old capture window.
                spectrum.capture(pcm, 2);
                double energy = 0;
                for (int i = 0; i < bytes; i += 2) {
                    double sample = pcm.getShort(i) / 32768.0;
                    energy += sample * sample;
                }
                if (bytes > 0 && Math.sqrt(energy / (bytes / 2)) > .0001) {
                    audibleAt = SystemClock.elapsedRealtime();
                    status = "Playback shared with CYN";
                } else status = "Waiting for shareable audio · some apps block capture";
                // This engine is analysis-only (native enhancement stays disabled).
                // Android session effects below alter the original playback once;
                // there is deliberately no AudioTrack or duplicate playback.
                NativeEngine.process(ai.engine, pcm, bytes);
            }
        } catch (RuntimeException | LinkageError failure) {
            status = "Playback analysis stopped · reconnect to retry";
        } finally {
            running = false;
            audibleAt = 0;
            ai.active = false;
            ai.reset();
            try { recorder.stop(); } catch (RuntimeException ignored) { }
            recorder.release();
            spectrum.close();
            main.post(() -> {
                projection.unregisterCallback(callback);
                projection.stop();
                // Clear the service's route on its own thread before scheduling
                // native destruction. A UI tick may still be reading telemetry.
                try { onStopped.run(); }
                finally { ai.closeAfterPlayer(); }
            });
        }
    }

    void stop() {
        running = false;
        audibleAt = 0;
        ai.active = false;
        ai.reset();
        try { recorder.stop(); } catch (RuntimeException ignored) { }
    }
}
