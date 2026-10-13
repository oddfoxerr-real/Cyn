// AST / Kaldi-compatible frontend, evaluated on the worker only.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

class CynAstFeatures {
public:
    static constexpr int samples=128000, rows=1024, bins=128;
    CynAstFeatures() {
        for(int i=0;i<400;++i) window_[i]=0.5-0.5*std::cos(6.283185307179586*i/399);
        const double low=mel(20),step=(mel(8000)-low)/129;
        for(int m=0;m<bins;++m) for(int k=0;k<256;++k) {
            const double f=mel(k*16000.0/512);
            weights_[m][k]=(std::max)(0.0,(std::min)((f-low-m*step)/step,(low+(m+2)*step-f)/step));
        }
    }
    void compute(const float* audio,size_t count,std::vector<float>& output) const {
        output.assign(rows*bins,float(4.2677393/(2*4.5689974)));
        const size_t frames=count>=400?(std::min)(size_t(rows),1+(count-400)/160):0;
        for(size_t frame=0;frame<frames;++frame) {
            std::array<std::complex<double>,512> spectrum{};
            double mean=0;
            for(int j=0;j<400;++j) mean+=audio[frame*160+j];
            mean/=400;
            for(int j=0;j<400;++j) {
                double x=audio[frame*160+j]-mean;
                double prev=audio[frame*160+(j?j-1:0)]-mean;
                spectrum[j]=(x-0.97*prev)*window_[j];
            }
            for(unsigned i=1,j=0;i<512;++i) {
                unsigned bit=256; for(;j&bit;bit>>=1) j^=bit; j^=bit;
                if(i<j) std::swap(spectrum[i],spectrum[j]);
            }
            for(int length=2;length<=512;length*=2) {
                const auto root=std::polar(1.0,-6.283185307179586/length);
                for(int i=0;i<512;i+=length) {
                    std::complex<double> w=1;
                    for(int j=0;j<length/2;++j) {
                        auto u=spectrum[i+j],v=spectrum[i+j+length/2]*w;
                        spectrum[i+j]=u+v; spectrum[i+j+length/2]=u-v; w*=root;
                    }
                }
            }
            for(int m=0;m<bins;++m) {
                double power=0;
                for(int k=0;k<256;++k) power+=std::norm(spectrum[k])*weights_[m][k];
                output[frame*bins+m]=float((std::log((std::max)(power,1.1920928955078125e-7))+4.2677393)/(2*4.5689974));
            }
        }
    }
private:
    static double mel(double hz) { return 1127.0*std::log1p(hz/700); }
    std::array<double,400> window_{};
    std::array<std::array<double,256>,128> weights_{};
};
