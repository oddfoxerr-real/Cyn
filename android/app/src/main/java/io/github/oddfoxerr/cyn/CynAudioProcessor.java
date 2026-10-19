// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import androidx.media3.common.C;
import androidx.media3.common.audio.BaseAudioProcessor;
import androidx.media3.common.audio.AudioProcessor;
import java.nio.ByteBuffer;
@androidx.annotation.OptIn(markerClass = androidx.media3.common.util.UnstableApi.class)
final class CynAudioProcessor extends BaseAudioProcessor {
    private final long engine;
    CynAudioProcessor(long engine) { this.engine=engine; }
    @Override protected AudioFormat onConfigure(AudioFormat format) throws AudioProcessor.UnhandledAudioFormatException {
        if(format.encoding!=C.ENCODING_PCM_16BIT||format.channelCount<1||format.channelCount>2||format.sampleRate!=48000) throw new AudioProcessor.UnhandledAudioFormatException(format);
        return format;
    }
    @Override protected void onFlush() { if(inputAudioFormat.channelCount>0) NativeEngine.configure(engine,inputAudioFormat.channelCount); }
    @Override public void queueInput(ByteBuffer input) {
        int bytes=input.remaining(); if(bytes==0)return;
        ByteBuffer output=replaceOutputBuffer(bytes);output.put(input);output.flip();
        NativeEngine.process(engine,output,bytes);
    }
}
