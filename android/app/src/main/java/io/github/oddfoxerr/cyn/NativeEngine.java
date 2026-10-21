// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;
import java.nio.ByteBuffer;
final class NativeEngine {
    static { System.loadLibrary("cyn_phone"); }
    static native long create();
    static native void destroy(long handle);
    static native void configure(long handle,int channels);
    static native void process(long handle,ByteBuffer buffer,int bytes);
    static native void controls(long handle,float[] values);
    static native void spatialPreview(long handle,float strength);
    static native float spatialActivity(long handle);
    static native float[] spatialPolicyWindow(long handle);
    static native float[] features(long handle);
    static native float[] yamnetWindow(long handle);
    static native float[] apexWindow(long handle);
    static native float[] effectTargets(float[] baseline,float[] scores,float sensitivity,float peak,boolean movie);
    static native float[] decide(float[] features,float[] scene,boolean movie);
    private NativeEngine() {}
}
