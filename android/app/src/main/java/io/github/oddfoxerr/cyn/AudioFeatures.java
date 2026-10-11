// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

/** Android Visualizer FFT features. NOT equivalent to desktop pre-DSP biquads.
 * Mono capture has no stereo width. This domain gap is disclosed in the UI. */
public final class AudioFeatures {
    public static final double[] EDGES={20,90,180,360,720,1440,2880,5760,20000};
    public static double[] fromFft(byte[] fft, int sampleRate, double rms, double peak) {
        double[] x=new double[11], energies=new double[8]; double total=0;
        for(int k=1;k<fft.length/2;k++) {
            double hz=(double)k*sampleRate/fft.length;
            double e=(double)fft[k*2]*fft[k*2]+(double)fft[k*2+1]*fft[k*2+1]; total+=e;
            for(int b=0;b<8;b++) if(hz>=EDGES[b]&&hz<EDGES[b+1]) { energies[b]+=e; break; }
        }
        for(int b=0;b<8;b++) x[b]=Math.log(Math.max(1e-6,energies[b]/Math.max(1,total)));
        x[8]=Math.log(Math.max(1,peak/Math.max(1e-6,rms)));
        x[9]=0; x[10]=Math.log(Math.max(1e-6,rms)); return x;
    }
    public static double[] levels(byte[] wave) {
        double energy=0,peak=0;
        for(byte b:wave) { double v=((b&255)-128)/128.0; energy+=v*v; peak=Math.max(peak,Math.abs(v)); }
        return new double[]{Math.sqrt(energy/Math.max(1,wave.length)),peak};
    }
    /** Bounded signed offset: neutral probability leaves the manual baseline intact. */
    public static double offset(double prediction, double sensitivity) {
        return PreferenceModel.clamp((prediction-.5)*2,-1,1)*PreferenceModel.clamp(sensitivity,0,1)*3;
    }
}
