// SPDX-License-Identifier: AGPL-3.0-or-later
#include <jni.h>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include "desktop/SourceAudioAnalyzer.h"
#include "desktop/CynFlagshipMixer.h"
#include "desktop/CynApexFlowHybrid.h"
#include "desktop/CynSmartRange.h"
#include "CynSpatialExperimental.h"

namespace {
constexpr int maxBands=39;
constexpr float pi=3.14159265358979323846f;
float clamp(float v,float lo,float hi) { return std::isfinite(v)?std::clamp(v,lo,hi):0; }
struct Biquad {
    float b0=1,b1=0,b2=0,a1=0,a2=0;
    std::array<float,2> z1{},z2{};
    float tick(float x,int c) { const float y=b0*x+z1[c]; z1[c]=b1*x-a1*y+z2[c];z2[c]=b2*x-a2*y;return y; }
    void peak(float hz,float gain,float q) {
        const float w=2*pi*hz/48000, a=std::pow(10.0f,gain/40),alpha=std::sin(w)/(2*q),den=1+alpha/a;
        b0=(1+alpha*a)/den;b1=-2*std::cos(w)/den;b2=(1-alpha*a)/den;a1=b1;a2=(1-alpha/a)/den;
    }
    void reset() { z1={};z2={}; }
};
struct Engine {
    SourceAudioAnalyzer analyzer;
    uint64_t yamWindow=0,apexWindow=0;
    std::array<std::atomic<float>,48> parameters{};
    std::atomic<float> spatialPreviewStrength{0.0f};
    CynSpatialExperimental spatialPreview;
    std::atomic<float> spatialActivity{0.0f};
    std::atomic<unsigned> serial{0};
    std::array<float,48> p{};
    std::array<Biquad,maxBands> filters{};
    std::array<float,maxBands> gains{};
    std::array<float,2048*2> scratch{};
    std::array<std::array<float,11000>,2> delay{};
    int delayIndex=0,channels=2,countdown=0;
    float wet=0,level=1,ceiling=.94f;
    Engine() { p[0]=0;parameters[0].store(0);p[8]=31;parameters[8].store(31);analyzer.setFormat(32,2,48000); }
    void reset(int ch) {
        channels=ch;analyzer.setFormat(0,0,0);analyzer.setFormat(32,ch,48000);
        for(auto& f:filters) { f.reset(); }
        delay={};delayIndex=0;countdown=0;wet=0;level=1;spatialPreview.reset();spatialActivity.store(0.0f,std::memory_order_relaxed);
    }
    void refresh() {
        for(int retry=0;retry<3;retry++) {
            const unsigned before=serial.load(std::memory_order_acquire);if(before&1)continue;
            std::array<float,48> next{};for(size_t i=0;i<next.size();i++)next[i]=parameters[i].load(std::memory_order_relaxed);
            if(serial.load(std::memory_order_acquire)==before){for(int i=1;i<8;i++)next[i]=p[i]+(next[i]-p[i])*.14f;p=next;break;}
        }
        const int bands=static_cast<int>(p[8]);
        float maxBoost=0;
        for(int b=0;b<bands;b++) {
            gains[b]+=(clamp(p[9+b],-12,12)-gains[b])*.14f;
            const float hz=20*std::pow(1000.0f,static_cast<float>(b)/(bands-1));
            const float bass=p[1]/(1+std::pow(hz/180,2.0f));
            const float oct=std::log2(hz/2200)/.8f;
            const float clarity=p[2]*std::exp(-.5f*oct*oct);
            const float gain=clamp(gains[b]+bass+clarity,-12,12);
            filters[b].peak(hz,gain,4.3f*bands/31);
            maxBoost=std::max(maxBoost,gain);
        }
        level=std::pow(10.0f,(-maxBoost-1+p[6])/20);
        ceiling=std::pow(10.0f,-clamp(p[7],.3f,12)/20);
    }
    void process(int16_t* pcm,int samples) {
        for(int offset=0;offset<samples;) {
            const int n=std::min<int>(samples-offset,scratch.size());
            for(int i=0;i<n;i++)scratch[i]=pcm[offset+i]/32768.0f;
            analyzer.process(scratch.data(),n/channels);
            for(int i=0;i<n;i+=channels) {
                if(countdown--<=0){refresh();countdown=479;}
                const bool active=p[0]>.5f;
                wet+=(static_cast<float>(active)-wet)*.002f;
                if(!active&&wet<.0001f){wet=0;continue;}
                std::array<float,2> out{};
                for(int c=0;c<channels;c++) {
                    float x=scratch[i+c]*level;
                    for(int b=0;b<static_cast<int>(p[8]);b++) x=filters[b].tick(x,c);
                    const int tap=(delayIndex+11000-(c?7691:5651))%11000;
                    const float echo=delay[c][tap];delay[c][delayIndex]=x+echo*.22f;
                    x+=echo*clamp(p[4],0,1)*.20f;
                    out[c]=x;
                }
                if(channels==2){const float mid=(out[0]+out[1])*.5f,side=(out[0]-out[1])*.5f*(1+clamp(p[3],0,1)*.55f);out[0]=mid+side;out[1]=mid-side;
                    // Explicit opt-in: existing stereo ambience only, not trained neural APEX 10M.
                    spatialPreview.process(out[0],out[1],spatialPreviewStrength.load(std::memory_order_relaxed));
                }
                delayIndex=(delayIndex+1)%11000;
                const float peak=std::max(std::abs(out[0]),std::abs(out[channels-1]));
                const float limiting=peak>ceiling?ceiling/std::max(peak,1e-9f):1;
                for(int c=0;c<channels;c++) {
                    float x=out[c]; const float dyn=clamp(p[5],0,1);
                    x=(1-dyn*.2f)*x+dyn*.2f*std::tanh(x*1.4f);
                    x=clamp(x*limiting,-ceiling,ceiling);
                    x=scratch[i+c]+wet*(x-scratch[i+c]);
                    pcm[offset+i+c]=static_cast<int16_t>(std::round(clamp(x,-.999969f,.999969f)*32768));
                }
            }
            spatialActivity.store(spatialPreviewStrength.load(std::memory_order_relaxed)>0.0f
                ? spatialPreview.activity() : 0.0f,std::memory_order_relaxed);
            offset+=n;
        }
    }
};
Engine* engine(jlong p){return reinterpret_cast<Engine*>(p);}
jfloatArray array(JNIEnv* env,const float* data,int n){auto a=env->NewFloatArray(n);env->SetFloatArrayRegion(a,0,n,data);return a;}
}
extern "C" {
JNIEXPORT jlong JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_create(JNIEnv*,jclass){return reinterpret_cast<jlong>(new Engine);}
JNIEXPORT void JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_destroy(JNIEnv*,jclass,jlong p){delete engine(p);}
JNIEXPORT void JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_configure(JNIEnv*,jclass,jlong p,jint ch){if(ch==1||ch==2)engine(p)->reset(ch);}
JNIEXPORT void JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_process(JNIEnv* env,jclass,jlong p,jobject buffer,jint bytes){auto* data=static_cast<int16_t*>(env->GetDirectBufferAddress(buffer));if(data&&bytes>=0&&bytes<=env->GetDirectBufferCapacity(buffer))engine(p)->process(data,bytes/2);}
JNIEXPORT void JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_controls(JNIEnv* env,jclass,jlong p,jfloatArray data){if(env->GetArrayLength(data)!=48)return;std::array<float,48>v{};env->GetFloatArrayRegion(data,0,48,v.data());auto* e=engine(p);e->serial.fetch_add(1,std::memory_order_acq_rel);for(int i=0;i<48;i++)e->parameters[i].store(clamp(v[i],-100,100),std::memory_order_relaxed);const int bands=static_cast<int>(v[8]);e->parameters[8].store(bands==35?35:bands==39?39:31);e->serial.fetch_add(1,std::memory_order_release);}
JNIEXPORT jfloat JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_spatialActivity(JNIEnv*,jclass,jlong handle){
    return handle?engine(handle)->spatialActivity.load(std::memory_order_relaxed):0.0f;
}
JNIEXPORT void JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_spatialPreview(JNIEnv*,jclass,jlong handle,jfloat strength){
    if(handle)engine(handle)->spatialPreviewStrength.store(clamp(strength,0.0f,1.0f),std::memory_order_relaxed);
}
JNIEXPORT jfloatArray JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_features(JNIEnv* env,jclass,jlong p){const auto a=engine(p)->analyzer.snapshot();std::array<float,12>v{};std::copy(a.values.begin(),a.values.end(),v.begin());v[11]=a.rms;return array(env,v.data(),12);}
JNIEXPORT jfloatArray JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_yamnetWindow(JNIEnv* env,jclass,jlong p){std::array<float,15600>w{};auto*e=engine(p);if(!e->analyzer.copyYamNetWindow(w,e->yamWindow))return nullptr;return array(env,w.data(),w.size());}
JNIEXPORT jfloatArray JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_apexWindow(JNIEnv* env,jclass,jlong p){std::array<float,48000>w{};auto*e=engine(p);if(!e->analyzer.copyApexWindow(w,e->apexWindow))return nullptr;return array(env,w.data(),w.size());}
JNIEXPORT jfloatArray JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_effectTargets(JNIEnv*env,jclass,jfloatArray baseline,jfloatArray scores,jfloat sensitivity,jfloat peak,jboolean movie){
    if(env->GetArrayLength(baseline)!=5||env->GetArrayLength(scores)!=5)return nullptr;
    std::array<float,5>b{},s{},out{};env->GetFloatArrayRegion(baseline,0,5,b.data());env->GetFloatArrayRegion(scores,0,5,s.data());
    const float musicUp[5]={3.4f,1.4f,1.4f,4.2f,5.6f},movieUp[5]={2.5f,.8f,1,.8f,.8f},down[5]={.65f,.9f,.9f,.65f,.85f};
    const float headroom=CynSmartRange::headroomGate(clamp((.87f-peak)/.57f,0,1),peak,50,movie);
    for(int i=0;i<5;i++)out[i]=CynSmartRange::effectTarget(b[i],CynSmartRange::expressiveScore(s[i],sensitivity,movie),sensitivity,movie?movieUp[i]:musicUp[i],down[i],headroom,movie,i==3||i==4||(i==0&&sensitivity>55));
    return array(env,out.data(),5);
}
JNIEXPORT jfloatArray JNICALL Java_io_github_oddfoxerr_cyn_NativeEngine_decide(JNIEnv*env,jclass,jfloatArray features,jfloatArray scene,jboolean movie){
    if(env->GetArrayLength(features)!=12||env->GetArrayLength(scene)!=11)return nullptr;
    std::array<float,12>f{};std::array<float,11>s{};env->GetFloatArrayRegion(features,0,12,f.data());env->GetFloatArrayRegion(scene,0,11,s.data());
    InputAudioFeatures a;std::copy_n(f.begin(),11,a.values.begin());a.rms=f[11];
    CynAudioScene c;c.music=s[0];c.speech=s[1];c.percussion=s[2];c.bassDrum=s[3];c.electronic=s[4];c.vocals=s[5];c.impact=s[6];c.soundtrack=s[7];c.silence=s[8];c.complexity=s[9];c.novelty=s[10];c.valid=true;
    const auto d=CynFlagshipMixer::decide(c,a,movie);std::array<float,13>out{};std::copy(d.preference.begin(),d.preference.end(),out.begin());
    constexpr float hz[8]={80,160,320,640,1300,2600,5300,11000};for(int i=0;i<8;i++)out[5+i]=movie?d.eqDelta(hz[i]):CynFlagshipMixer::musicEqCorrection(d,c,hz[i],0);
    return array(env,out.data(),out.size());
}
}
