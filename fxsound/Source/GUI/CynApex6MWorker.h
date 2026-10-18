// CYN APEX full ONNX inference (6,307,477 learned parameters).
// SPDX-License-Identifier: AGPL-3.0-or-later
// Worker receives full-band 48k PCM from DSP. The audio thread NEVER runs ONNX.
#pragma once
#include <JuceHeader.h>
#include "DfxDsp.h"
#include "../../../third_party/onnxruntime/onnxruntime_c_api.h"
#include <array>
#include <atomic>
#include <complex>
#include <cmath>
#include <algorithm>
#include <vector>
#include <cstdint>

class CynApex6MWorker final : private juce::Thread {
public:
    explicit CynApex6MWorker(DfxDsp& dsp)
        : juce::Thread("CYN APEX 6M ONNX"), source_(dsp) {}
    ~CynApex6MWorker() override { signalThreadShouldExit(); stopThread(15000); }
    bool installed() const {
        const auto dir=juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory();
        return dir.getChildFile("apex6m-225.onnx").existsAsFile()
            && dir.getChildFile("apex6m-225.onnx").getSize()>20000000
            && dir.getChildFile("onnxruntime.dll").existsAsFile();
    }
    void setProcessingEnabled(bool active) noexcept {
        processing_enabled_.store(active,std::memory_order_release);
        if(!active) {
            stamp_.store(0,std::memory_order_release);
        }
    }
    // Startup is async. A fast PC may become ready immediately, while a slower
    // PC can need several seconds; do not permanently reject its activation.
    void startIfAvailable(bool explicitRetry=false) {
        if(!installed()) return;
        if(isThreadRunning() || ready()) return;
        if(state_.load(std::memory_order_acquire)==Failed && !explicitRetry) return;
        {
            const juce::ScopedLock lock(errorLock_);
            lastError_.clear();
        }
        state_.store(Starting,std::memory_order_release);
        startThread(2);
    }
    bool ready() const noexcept { return ready_.load(std::memory_order_acquire); }
    bool failed() const noexcept { return state_.load(std::memory_order_acquire)==Failed; }
    bool starting() const noexcept { return state_.load(std::memory_order_acquire)==Starting; }
    juce::String diagnostic() const {
        const auto dir=juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory();
        if(!dir.getChildFile("apex6m-225.onnx").existsAsFile() ||
           dir.getChildFile("apex6m-225.onnx").getSize()<=20000000)
            return "APEX model file is missing or incomplete. Repair the Cyn installation.";
        if(!dir.getChildFile("onnxruntime.dll").existsAsFile())
            return "onnxruntime.dll is missing. Repair the Cyn installation.";
        if(ready()) return "APEX neural engine ready.";
        if(failed()) {
            const juce::ScopedLock lock(errorLock_);
            return "APEX neural engine failed: "+lastError_
                 +" Run DIAGNOSE_CYN_APEX.ps1 from the installation folder for details.";
        }
        if(starting()) return "APEX neural engine initializing; activation is queued.";
        return "APEX neural engine has not started; use Activate to retry.";
    }
    bool latest(std::array<float,8>& bands) const noexcept {
        if(!processing_enabled_.load(std::memory_order_acquire)) return false;
        const auto stamp=stamp_.load(std::memory_order_acquire);
        if(!ready() || !stamp || juce::Time::getMillisecondCounter()-stamp>2500)
            return false;
        for(int i=0;i<8;++i)
            bands[(size_t)i]=eq_[(size_t)i].load(std::memory_order_relaxed);
        return stamp_.load(std::memory_order_acquire)==stamp;
    }
    static float interpolate(const std::array<float,8>& bands,float frequency) noexcept {
        static constexpr float centers[8]={80,160,320,640,1300,2600,5300,11000};
        if(frequency<=centers[0]) return bands[0];
        for(int i=1;i<8;++i)
            if(frequency<=centers[i]) {
                const auto t=std::log(frequency/centers[i-1])/
                              std::log(centers[i]/centers[i-1]);
                return bands[(size_t)i-1]+
                    std::clamp(t,0.0f,1.0f)*(bands[(size_t)i]-bands[(size_t)i-1]);
            }
        return bands[7];
    }
private:
    enum State : int { Idle=0, Starting=1, Ready=2, Failed=3 };
    void reportFailure(const juce::String& reason) {
        {
            const juce::ScopedLock lock(errorLock_);
            lastError_=reason;
        }
        ready_.store(false,std::memory_order_release);
        state_.store(Failed,std::memory_order_release);
        juce::Logger::writeToLog("CYN APEX 6M: "+reason);
    }
    using Complex=std::complex<float>;
    static constexpr int fftSize=1024, hop=256, bins=513;
    static constexpr int frames=189; // windows centered every 256 samples, with reflected boundary
    static void fft(std::array<Complex,fftSize>& a) noexcept {
        for(int i=1,j=0;i<fftSize;++i) {
            int bit=fftSize>>1;
            for(;j&bit;bit>>=1) j^=bit;
            j^=bit;
            if(i<j) std::swap(a[(size_t)i],a[(size_t)j]);
        }
        for(int len=2;len<=fftSize;len<<=1) {
            const float angle=-6.283185307179586f/len;
            const Complex inc(std::cos(angle),std::sin(angle));
            for(int i=0;i<fftSize;i+=len) {
                Complex w(1,0);
                for(int j=0;j<len/2;++j) {
                    const auto u=a[(size_t)(i+j)];
                    const auto v=a[(size_t)(i+j+len/2)]*w;
                    a[(size_t)(i+j)]=u+v;
                    a[(size_t)(i+j+len/2)]=u-v;
                    w*=inc;
                }
            }
        }
    }
    static void computeFeatures(const std::array<float,48000>& input,
                                std::vector<float>& magnitudes,
                                std::vector<float>& features) {
        std::array<Complex,fftSize> wave {};
        for(int frame=0;frame<frames;++frame) {
            for(int n=0;n<fftSize;++n) {
                int idx=frame*hop+n-fftSize/2;
                if(idx<0) idx=-idx;
                if(idx>=48000) idx=2*47999-idx;
                if(idx<0 || idx>=48000) idx=0;
                const float hann=0.5f-0.5f*std::cos(
                    6.283185307179586f*n/fftSize);
                wave[(size_t)n]={input[(size_t)idx]*hann,0};
            }
            fft(wave);
            for(int k=0;k<bins;++k) {
                const auto magnitude=std::abs(wave[(size_t)k]);
                const auto index=(size_t)k*frames+frame;
                magnitudes[index]=magnitude;
                features[index]=std::log1p(magnitude);
            }
        }
    }
    static std::array<float,8> bandGoals(const float* mask,
                                          const std::vector<float>& magnitudes) {
        static constexpr float centers[8]={80,160,320,640,1300,2600,5300,11000};
        std::array<double,8> sum {},weight {};
        double total=0, mass=0;
        for(int k=2;k<bins;++k) {
            const float freq=48000.0f*k/fftSize;
            for(int t=0;t<frames;++t) {
                const size_t index=(size_t)k*frames+t;
                const float m=mask[index],a=magnitudes[index];
                if(!std::isfinite(m) || m<0 || m>2 ||
                   !std::isfinite(a)) return {};
                const float w=(std::min)(50.0f,a);
                total+=w*m;mass+=w;
                for(int b=0;b<8;++b) {
                    const float width=std::abs(std::log(
                        (std::max)(freq,1.0f)/centers[b]));
                    if(width<0.65f) {
                        const float cw=1.0f-width/0.65f;
                        sum[(size_t)b]+=cw*w*m;weight[(size_t)b]+=cw*w;
                    }
                }
            }
        }
        const double avg=mass>0.01?total/mass:1.0;
        std::array<float,8> result {};
        for(int b=0;b<8;++b) {
            const double value=weight[(size_t)b]>0.01 ?
                sum[(size_t)b]/weight[(size_t)b]:avg;
            const double ratio=std::clamp(value/(std::max)(avg,0.01),0.2,5.0);
            result[(size_t)b]=(float)std::clamp(
                20.0*std::log10(ratio)*0.32,-1.35,1.15);
        }
        return result;
    }
    void run() override {
        ready_.store(false,std::memory_order_release);
        const auto dir=juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory();
        juce::DynamicLibrary library;
        if(!library.open(dir.getChildFile("onnxruntime.dll").getFullPathName())) {
            const auto code=static_cast<int>(::GetLastError());
            if(code==126)
                reportFailure("Windows could not load ONNX Runtime (error 126). "
                    "Check/install the Microsoft Visual C++ 2015-2022 x64 Runtime.");
            else if(code==193)
                reportFailure("ONNX Runtime is not compatible with this Windows architecture (error 193).");
            else if(code==1114)
                reportFailure("Windows could not initialize ONNX Runtime (error 1114).");
            else
                reportFailure("Windows failed to load onnxruntime.dll (error "
                    +juce::String(code)+").");
            return;
        }
        using ApiFunction=const OrtApiBase*(ORT_API_CALL*)();
        auto* function=library.getFunction("OrtGetApiBase");
        if(!function) { reportFailure("onnxruntime.dll has no OrtGetApiBase export."); return; }
        const auto* base=reinterpret_cast<ApiFunction>(function)();
        if(!base) { reportFailure("ONNX Runtime entry point returned null."); return; }
        const auto* api=base->GetApi(ORT_API_VERSION);
        if(!api) { reportFailure("ONNX Runtime API version does not match this Cyn build."); return; }
        const auto ok=[this,api](OrtStatus* status) {
            if(!status) return true;
            const juce::String message(api->GetErrorMessage(status));
            api->ReleaseStatus(status);
            reportFailure("ONNX Runtime: "+message);
            return false;
        };
        OrtEnv* env=nullptr;OrtSessionOptions* opts=nullptr;
        OrtSession* session=nullptr;OrtMemoryInfo* memory=nullptr;
        OrtAllocator* allocator=nullptr;
        char* inputName=nullptr;char* maskName=nullptr;char* sceneName=nullptr;
        do {
            if(!ok(api->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"CynApex6M",&env)))break;
            if(!ok(api->CreateSessionOptions(&opts)))break;
            if(!ok(api->SetIntraOpNumThreads(opts,2)))break;
            if(!ok(api->SetInterOpNumThreads(opts,1)))break;
            if(!ok(api->CreateSession(env,
                dir.getChildFile("apex6m-225.onnx").getFullPathName().toWideCharPointer(),
                opts,&session)))break;
            if(!ok(api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&memory)))break;
            if(!ok(api->GetAllocatorWithDefaultOptions(&allocator)))break;
            if(!ok(api->SessionGetInputName(session,0,allocator,&inputName)))break;
            if(!ok(api->SessionGetOutputName(session,0,allocator,&maskName)))break;
            if(!ok(api->SessionGetOutputName(session,1,allocator,&sceneName)))break;
            ready_.store(true,std::memory_order_release);
            state_.store(Ready,std::memory_order_release);
            const char* const inputs[]={inputName};
            const char* const outputs[]={maskName,sceneName};
            const int64_t shape[]={1,bins,frames};
            std::array<float,48000> samples {};
            std::vector<float> magnitude((size_t)bins*frames,0.0f);
            std::vector<float> features((size_t)bins*frames,0.0f);
            uint64_t lastWindow=0;
            while(!threadShouldExit()) {
                if(!processing_enabled_.load(std::memory_order_acquire)) {
                    wait(220);
                    continue;
                }
                if(!source_.copyApexWindow(samples,lastWindow)) {
                    wait(170);continue;
                }
                computeFeatures(samples,magnitude,features);
                OrtValue* input=nullptr;OrtValue* result[2]={nullptr,nullptr};
                bool valid=ok(api->CreateTensorWithDataAsOrtValue(
                    memory,features.data(),features.size()*sizeof(float),
                    shape,3,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&input));
                if(valid) {
                    const OrtValue* const params[]={input};
                    valid=ok(api->Run(session,nullptr,inputs,params,1,outputs,2,result));
                }
                float* mask=nullptr;
                if(valid && result[0]) {
                    OrtTensorTypeAndShapeInfo* info=nullptr;
                    valid=ok(api->GetTensorTypeAndShape(result[0],&info));
                    if(valid && info) {
                        size_t dimensions=0;
                        valid=ok(api->GetDimensionsCount(info,&dimensions));
                        if(valid && dimensions==3) {
                            int64_t d[3] {};
                            valid=ok(api->GetDimensions(info,d,3));
                            valid=valid && d[0]==1 && d[1]==bins && d[2]==frames;
                            if(valid) {
                                void* data=nullptr;
                                valid=ok(api->GetTensorMutableData(result[0],&data));
                                if(valid) mask=static_cast<float*>(data);
                            }
                        } else valid=false;
                        api->ReleaseTensorTypeAndShapeInfo(info);
                    }
                } else valid=false;
                if(valid && mask) {
                    auto values=bandGoals(mask,magnitude);
                    for(int b=0;b<8;++b)
                        eq_[(size_t)b].store(values[(size_t)b],std::memory_order_relaxed);
                    stamp_.store(juce::Time::getMillisecondCounter(),
                                 std::memory_order_release);
                }
                for(auto* v:result)if(v)api->ReleaseValue(v);
                if(input)api->ReleaseValue(input);
                if(!valid) {
                    if(!failed()) reportFailure("ONNX returned an invalid output or stopped inference.");
                    break;
                }
                wait(360);
            }
        } while(false);
        if(!threadShouldExit() && !failed())
            reportFailure("ONNX session setup failed before the model became ready.");
        stamp_.store(0,std::memory_order_release);
        ready_.store(false,std::memory_order_release);
        if(allocator) {
            if(inputName)api->AllocatorFree(allocator,inputName);
            if(maskName)api->AllocatorFree(allocator,maskName);
            if(sceneName)api->AllocatorFree(allocator,sceneName);
        }
        if(memory)api->ReleaseMemoryInfo(memory);
        if(session)api->ReleaseSession(session);
        if(opts)api->ReleaseSessionOptions(opts);
        if(env)api->ReleaseEnv(env);
    }
    std::atomic<bool> processing_enabled_ {false};
    DfxDsp& source_;
    std::atomic<bool> ready_ {false};
    std::atomic<int> state_ {Idle};
    mutable juce::CriticalSection errorLock_;
    juce::String lastError_;
    std::array<std::atomic<float>,8> eq_ {};
    std::atomic<uint32_t> stamp_ {0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynApex6MWorker)
};
