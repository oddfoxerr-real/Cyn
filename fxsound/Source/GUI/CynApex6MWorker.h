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
    void startIfAvailable() {
        // May be called again after a failed initialization. JUCE owns the
        // thread lifecycle; never start a second worker while one is running.
        if(!installed() || isThreadRunning()) return;
        {
            const juce::ScopedLock lock(diagnosticLock_);
            failureDetail_.clear();
        }
        loadState_.store(1,std::memory_order_release);
        startThread(2);
    }
    bool ready() const noexcept { return ready_.load(std::memory_order_acquire); }
    bool failed() const noexcept { return loadState_.load(std::memory_order_acquire)==3; }
    juce::String diagnostic() const {
        const auto dir=juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory();
        const auto model=dir.getChildFile("apex6m-225.onnx");
        if(!model.existsAsFile())
            return "APEX model is missing from " + dir.getFullPathName()
                 + ". Reinstall the complete CYN release.";
        if(model.getSize()<=20000000)
            return "APEX model file is incomplete or damaged. Reinstall CYN.";
        if(!dir.getChildFile("onnxruntime.dll").existsAsFile())
            return "ONNX Runtime DLL is missing beside Cyn.exe. Reinstall CYN.";
        const int state=loadState_.load(std::memory_order_acquire);
        if(state==2) return "Neural engine ready.";
        if(state==1) return "Initializing APEX neural engine...";
        if(state==3) {
            const juce::ScopedLock lock(diagnosticLock_);
            return failureDetail_.isEmpty() ? "APEX engine initialization failed."
                                            : failureDetail_;
        }
        return "Neural engine not started. Select APEX to start it.";
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
    void recordFailure(const juce::String& message) {
        {
            const juce::ScopedLock lock(diagnosticLock_);
            failureDetail_=message;
        }
        loadState_.store(3,std::memory_order_release);
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
           #if JUCE_WINDOWS
            const auto winError=juce::String((int)::GetLastError());
            recordFailure("Cannot load onnxruntime.dll (Windows error "+winError+
                "). Check the Microsoft Visual C++ x64 runtime; if missing, install it from Microsoft.");
           #else
            recordFailure("Cannot load onnxruntime.dll. Check required native dependencies.");
           #endif
            return;
        }
        using ApiFunction=const OrtApiBase*(ORT_API_CALL*)();
        auto* function=library.getFunction("OrtGetApiBase");
        if(!function) {
            recordFailure("onnxruntime.dll does not export OrtGetApiBase. Reinstall CYN.");
            return;
        }
        const auto* base=reinterpret_cast<ApiFunction>(function)();
        if(!base) {
            recordFailure("ONNX Runtime returned an invalid API base.");
            return;
        }
        const auto* api=base->GetApi(ORT_API_VERSION);
        if(!api) {
            recordFailure("ONNX Runtime API version is incompatible with this CYN build.");
            return;
        }
        const auto ok=[api,this](OrtStatus* status) {
            if(!status) return true;
            const auto* text=api->GetErrorMessage(status);
            const juce::String detail=text ? juce::String::fromUTF8(text)
                                           : "unspecified ONNX Runtime error";
            recordFailure("APEX ONNX initialization/inference failed: "+detail);
            api->ReleaseStatus(status);
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
            loadState_.store(2,std::memory_order_release);
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
                            if(!valid) recordFailure("APEX model output shape does not match this CYN version.");
                            if(valid) {
                                void* data=nullptr;
                                valid=ok(api->GetTensorMutableData(result[0],&data));
                                if(valid) mask=static_cast<float*>(data);
                            }
                        } else {
                            valid=false;
                            recordFailure("APEX model has unexpected output tensor dimensions.");
                        }
                        api->ReleaseTensorTypeAndShapeInfo(info);
                    }
                } else {
                    valid=false;
                    if(loadState_.load(std::memory_order_acquire)!=3)
                        recordFailure("APEX inference did not return a valid output tensor.");
                }
                if(valid && mask) {
                    auto values=bandGoals(mask,magnitude);
                    for(int b=0;b<8;++b)
                        eq_[(size_t)b].store(values[(size_t)b],std::memory_order_relaxed);
                    stamp_.store(juce::Time::getMillisecondCounter(),
                                 std::memory_order_release);
                }
                for(auto* v:result)if(v)api->ReleaseValue(v);
                if(input)api->ReleaseValue(input);
                if(!valid)break;
                wait(360);
            }
        } while(false);
        stamp_.store(0,std::memory_order_release);
        ready_.store(false,std::memory_order_release);
        if(!threadShouldExit() && !failed())
            recordFailure("APEX neural engine stopped unexpectedly; retry initialization.");
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
    // 0: not started, 1: loading, 2: ready, 3: failed.
    std::atomic<int> loadState_ {0};
    mutable juce::CriticalSection diagnosticLock_;
    juce::String failureDetail_;
    std::array<std::atomic<float>,8> eq_ {};
    std::atomic<uint32_t> stamp_ {0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynApex6MWorker)
};
