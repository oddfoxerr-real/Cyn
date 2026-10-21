// SPDX-License-Identifier: AGPL-3.0-or-later
// Matches CynApex6MWorker at desktop commit 74704e8c.
package io.github.oddfoxerr.cyn;
final class ApexFeatures {
    static final int FFT=1024, BINS=513, FRAMES=189;
    static final float[] CENTERS={80,160,320,640,1300,2600,5300,11000};
    final float[] magnitude=new float[BINS*FRAMES], features=new float[BINS*FRAMES];
    private final double[] real=new double[FFT],imag=new double[FFT];
    void compute(float[] samples) {
        if(samples.length!=48000)throw new IllegalArgumentException("APEX needs one second of 48 kHz audio");
        for(int frame=0;frame<FRAMES;frame++) {
            for(int n=0;n<FFT;n++) {
                int index=frame*256+n-512;
                if(index<0)index=-index;if(index>=48000)index=2*47999-index;if(index<0||index>=48000)index=0;
                real[n]=samples[index]*(.5-.5*Math.cos(2*Math.PI*n/FFT));imag[n]=0;
            }
            for(int i=1,j=0;i<FFT;i++){int bit=FFT>>1;for(;(j&bit)!=0;bit>>=1)j^=bit;j^=bit;if(i<j){double v=real[i];real[i]=real[j];real[j]=v;}}
            for(int len=2;len<=FFT;len<<=1){double angle=-2*Math.PI/len,cr=Math.cos(angle),ci=Math.sin(angle);
                for(int i=0;i<FFT;i+=len){double wr=1,wi=0;for(int j=0;j<len/2;j++){int a=i+j,b=a+len/2;double vr=real[b]*wr-imag[b]*wi,vi=real[b]*wi+imag[b]*wr,ur=real[a],ui=imag[a];real[a]=ur+vr;imag[a]=ui+vi;real[b]=ur-vr;imag[b]=ui-vi;double nr=wr*cr-wi*ci;wi=wr*ci+wi*cr;wr=nr;}}}
            for(int k=0;k<BINS;k++){int index=k*FRAMES+frame;magnitude[index]=(float)Math.hypot(real[k],imag[k]);features[index]=(float)Math.log1p(magnitude[index]);}
        }
    }
    float[] goals(float[] mask) {
        if(mask.length!=magnitude.length)throw new IllegalArgumentException("APEX mask shape mismatch");
        double[] sum=new double[8],weight=new double[8];double total=0,mass=0;
        for(int k=2;k<BINS;k++)for(int t=0;t<FRAMES;t++){
            int index=k*FRAMES+t;float m=mask[index],a=magnitude[index];
            if(!Float.isFinite(m)||m<0||m>2||!Float.isFinite(a))throw new IllegalArgumentException("Invalid APEX inference");
            double w=Math.min(50,a);total+=w*m;mass+=w;
            for(int b=0;b<8;b++){double width=Math.abs(Math.log(48000.0*k/FFT/CENTERS[b]));if(width<.65){double cw=1-width/.65;sum[b]+=cw*w*m;weight[b]+=cw*w;}}
        }
        double avg=mass>.01?total/mass:1;float[] result=new float[8];
        for(int b=0;b<8;b++){double value=weight[b]>.01?sum[b]/weight[b]:avg;result[b]=(float)PreferenceModel.clamp(20*Math.log10(PreferenceModel.clamp(value/Math.max(avg,.01),.2,5))*.32,-1.35,1.15);}
        return result;
    }
    static float interpolate(float[] bands,float hz){if(hz<=CENTERS[0])return bands[0];for(int i=1;i<8;i++)if(hz<=CENTERS[i])return bands[i-1]+(bands[i]-bands[i-1])*(float)(Math.log(hz/CENTERS[i-1])/Math.log(CENTERS[i]/CENTERS[i-1]));return bands[7];}
}
