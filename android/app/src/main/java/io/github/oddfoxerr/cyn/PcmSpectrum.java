// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * Real-time display-only spectrum from CYN's actual decoded 48 kHz PCM.
 * Capture on the audio callback copies samples only. FFT runs on its own low-priority
 * worker, never in ONNX inference or on the real-time audio processing thread.
 */
final class PcmSpectrum implements AutoCloseable {
    static final int SIZE=4096, BANDS=31;
    static final class Frame {
        final float[] bands;
        final long atNanos;
        Frame(float[] bands,long atNanos){this.bands=bands;this.atNanos=atNanos;}
    }
    private final short[] ring=new short[SIZE],samples=new short[SIZE];
    private final double[] real=new double[SIZE],imag=new double[SIZE],hann=new double[SIZE];
    private final int[] bandForBin=new int[SIZE/2];
    private final ScheduledExecutorService worker=Executors.newSingleThreadScheduledExecutor(r->{
        Thread t=new Thread(r,"CYN visual spectrum");t.setDaemon(true);t.setPriority(Thread.MIN_PRIORITY);return t;
    });
    private int write,count;
    private long capturedAt;
    private volatile boolean active,closed;
    private volatile Frame result=new Frame(new float[BANDS],0);
    PcmSpectrum(){
        for(int i=0;i<SIZE;i++)hann[i]=.5-.5*Math.cos(2*Math.PI*i/(SIZE-1));
        Arrays.fill(bandForBin,-1);
        for(int k=1;k<SIZE/2;k++){
            double hz=k*48000.0/SIZE;
            if(hz>=30&&hz<=19000){
                int band=(int)Math.floor((BANDS-1)*Math.log(hz/30)/Math.log(19000.0/30));
                bandForBin[k]=Math.max(0,Math.min(BANDS-1,band));
            }
        }
        worker.scheduleAtFixedRate(this::analyze,40,40,TimeUnit.MILLISECONDS);
    }
    synchronized void setActive(boolean enabled){
        active=enabled&&!closed;
        if(!active)clearLocked();
    }
    synchronized void clear(){clearLocked();}
    private void clearLocked(){
        write=0;count=0;capturedAt=0;
        Arrays.fill(ring,(short)0);
        result=new Frame(new float[BANDS],0);
    }
    void capture(ByteBuffer input,int channels){
        if(!active||closed||channels<1||channels>2)return;
        ByteBuffer pcm=input.duplicate().order(ByteOrder.LITTLE_ENDIAN);
        int bytesPerFrame=2*channels;
        synchronized(this){
            if(!active||closed)return;
            while(pcm.remaining()>=bytesPerFrame){
                int left=pcm.getShort();
                int sample=channels==2?(left+pcm.getShort())/2:left;
                ring[write]=(short)sample;
                write=(write+1)&(SIZE-1);
                if(count<SIZE)count++;
            }
            capturedAt=System.nanoTime();
        }
    }
    Frame latest(){return result;}
    private void analyze(){
        if(!active||closed)return;
        long timestamp;
        synchronized(this){
            timestamp=capturedAt;
            if(!active||count<SIZE/4||timestamp==0||System.nanoTime()-timestamp>250_000_000L){
                result=new Frame(new float[BANDS],0);
                return;
            }
            Arrays.fill(samples,(short)0);
            int start=(write-count+SIZE)&(SIZE-1);
            int offset=SIZE-count;
            for(int i=0;i<count;i++)samples[offset+i]=ring[(start+i)&(SIZE-1)];
        }
        double energy=0;
        for(int i=0;i<SIZE;i++){
            double v=samples[i]/32768.0;
            energy+=v*v;
            int reversed=Integer.reverse(i) >>> 20;
            real[reversed]=v*hann[i];
            imag[reversed]=0;
        }
        float[] bands=new float[BANDS];
        double rms=Math.sqrt(energy/SIZE);
        if(rms>.003){
            for(int len=2;len<=SIZE;len<<=1){
                double a=-2*Math.PI/len,wrStep=Math.cos(a),wiStep=Math.sin(a);
                for(int offset=0;offset<SIZE;offset+=len){
                    double wr=1,wi=0;
                    for(int k=0;k<len/2;k++){
                        int even=offset+k,odd=even+len/2;
                        double tr=wr*real[odd]-wi*imag[odd],ti=wr*imag[odd]+wi*real[odd];
                        real[odd]=real[even]-tr;imag[odd]=imag[even]-ti;
                        real[even]+=tr;imag[even]+=ti;
                        double nextWr=wr*wrStep-wi*wiStep;
                        wi=wr*wiStep+wi*wrStep;wr=nextWr;
                    }
                }
            }
            double[] powers=new double[BANDS];double peak=0;
            for(int bin=1;bin<SIZE/2;bin++){
                int band=bandForBin[bin];
                if(band>=0)powers[band]+=real[bin]*real[bin]+imag[bin]*imag[bin];
            }
            for(double power:powers)peak=Math.max(peak,power);
            if(peak>1e-12){
                double gain=Math.min(1,Math.sqrt(rms)*2.7);
                for(int i=0;i<BANDS;i++)
                    bands[i]=(float)Math.min(1,Math.sqrt(powers[i]/peak)*gain);
            }
        }
        if(active&&!closed)result=new Frame(bands,timestamp);
    }
    @Override public void close(){
        closed=true;setActive(false);worker.shutdownNow();
    }
}
