// Cyn pretrained YAMNet async scene inference. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "DfxDsp.h"
#include "CynFlagshipMixer.h"
#include "../../../third_party/onnxruntime/onnxruntime_c_api.h"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>

// All 521 AudioSet classes contribute to scene complexity, and every one of the
// 1024 pretrained embedding dimensions contributes to the 24-feature personal
// sound-preference learner. ONNX inference NEVER runs on the audio callback.
class CynYamNetWorker final : private juce::Thread {
public:
    explicit CynYamNetWorker(DfxDsp& source)
        : juce::Thread("Cyn YAMNet flagship"), source_(source) {}
    ~CynYamNetWorker() override { signalThreadShouldExit(); stopThread(15000); }

    void setProcessingEnabled(bool active) noexcept {
        processing_enabled_.store(active,std::memory_order_release);
        if(!active) {
            lastResultMs_.store(0,std::memory_order_release);
        }
    }
    void startIfAvailable() {
        const auto dir=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        model_=dir.getChildFile("yamnet-16k-0p975s.onnx");
        runtime_=dir.getChildFile("onnxruntime.dll");
        if(model_.existsAsFile() && model_.getSize()>1000000 &&
           runtime_.existsAsFile() && runtime_.getSize()>100000)
            startThread(2);
    }

    bool latest(CynAudioScene& scene) const noexcept {
        if(!processing_enabled_.load(std::memory_order_acquire)) return false;
        const auto now=juce::Time::getMillisecondCounter();
        for(int retry=0;retry<3;++retry) {
            const auto version=sequence_.load(std::memory_order_acquire);
            if(version&1) continue;
            scene.music=metrics_[0].load(std::memory_order_relaxed);
            scene.speech=metrics_[1].load(std::memory_order_relaxed);
            scene.percussion=metrics_[2].load(std::memory_order_relaxed);
            scene.bassDrum=metrics_[3].load(std::memory_order_relaxed);
            scene.electronic=metrics_[4].load(std::memory_order_relaxed);
            scene.vocals=metrics_[5].load(std::memory_order_relaxed);
            scene.impact=metrics_[6].load(std::memory_order_relaxed);
            scene.soundtrack=metrics_[7].load(std::memory_order_relaxed);
            scene.silence=metrics_[8].load(std::memory_order_relaxed);
            scene.complexity=metrics_[9].load(std::memory_order_relaxed);
            scene.novelty=metrics_[10].load(std::memory_order_relaxed);
            for(int i=0;i<CynAudioScene::embeddingSize;++i)
                scene.embedding[(size_t)i]=embedding_[(size_t)i].load(std::memory_order_relaxed);
            const auto stamp=lastResultMs_.load(std::memory_order_relaxed);
            if(sequence_.load(std::memory_order_acquire)!=version) continue;
            if(!stamp || (uint32_t)(now-stamp)>2500) return false;
            scene.timestamp=stamp;
            scene.valid=true;
            return true;
        }
        return false;
    }

    // Legacy status API remains usable by other components.
    bool latest(float& speech,float& music,float& impacts) const noexcept {
        CynAudioScene scene;
        if(!latest(scene)) return false;
        speech=scene.speech; music=scene.music; impacts=scene.impact;
        return true;
    }
private:
    void publish(CynAudioScene& s) noexcept {
        // One writer; atomic seqlock gives consistent inference snapshots.
        sequence_.fetch_add(1,std::memory_order_acq_rel);
        const float values[11]={s.music,s.speech,s.percussion,s.bassDrum,
            s.electronic,s.vocals,s.impact,s.soundtrack,s.silence,
            s.complexity,s.novelty};
        for(int i=0;i<11;++i)
            metrics_[(size_t)i].store(values[i],std::memory_order_relaxed);
        for(int i=0;i<CynAudioScene::embeddingSize;++i)
            embedding_[(size_t)i].store(s.embedding[(size_t)i],std::memory_order_relaxed);
        lastResultMs_.store(juce::Time::getMillisecondCounter(),std::memory_order_relaxed);
        sequence_.fetch_add(1,std::memory_order_release);
    }
    void run() override {
        juce::DynamicLibrary library;
        if(!library.open(runtime_.getFullPathName())) return;
        using GetApiBaseFn=const OrtApiBase*(ORT_API_CALL*)();
        const auto address=library.getFunction("OrtGetApiBase");
        if(address==nullptr) return;
        const OrtApiBase* apiBase=reinterpret_cast<GetApiBaseFn>(address)();
        if(!apiBase) return;
        const OrtApi* api=apiBase->GetApi(ORT_API_VERSION);
        if(!api) return;
        const auto ok=[api](OrtStatus* error) {
            if(!error) return true;
            api->ReleaseStatus(error); return false;
        };
        OrtEnv* env=nullptr; OrtSessionOptions* options=nullptr;
        OrtSession* session=nullptr; OrtMemoryInfo* memory=nullptr;
        OrtAllocator* allocator=nullptr; char* inName=nullptr;
        char* outName0=nullptr; char* outName1=nullptr;
        do {
            if(!ok(api->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"CynYAMNet",&env))) break;
            if(!ok(api->CreateSessionOptions(&options))) break;
            if(!ok(api->SetIntraOpNumThreads(options,2))) break;
            if(!ok(api->SetInterOpNumThreads(options,1))) break;
            if(!ok(api->CreateSession(env,model_.getFullPathName().toWideCharPointer(),options,&session))) break;
            if(!ok(api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&memory))) break;
            if(!ok(api->GetAllocatorWithDefaultOptions(&allocator))) break;
            if(!ok(api->SessionGetInputName(session,0,allocator,&inName))) break;
            if(!ok(api->SessionGetOutputName(session,0,allocator,&outName0))) break;
            if(!ok(api->SessionGetOutputName(session,1,allocator,&outName1))) break;
            const char* const inputNames[]={inName};
            const char* const outputNames[]={outName0,outName1};
            const int64_t shape[]={15600};
            std::array<float,15600> waveform {};
            uint64_t lastWindow=0;
            CynAudioScene previous {};
            while(!threadShouldExit()) {
                if(!processing_enabled_.load(std::memory_order_acquire)) {
                    wait(220);
                    continue;
                }
                if(!source_.copyYamNetWindow(waveform,lastWindow)) {
                    wait(220); continue;
                }
                OrtValue* input=nullptr;
                OrtValue* output[2]={nullptr,nullptr};
                bool valid=ok(api->CreateTensorWithDataAsOrtValue(memory,
                    waveform.data(),waveform.size()*sizeof(float),
                    shape,1,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&input));
                if(valid) {
                    const OrtValue* const inputs[]={input};
                    valid=ok(api->Run(session,nullptr,inputNames,inputs,1,outputNames,2,output));
                }
                const float* scores=nullptr;
                const float* representation=nullptr;
                if(valid) for(auto* value:output) {
                    if(!value) { valid=false; break; }
                    OrtTensorTypeAndShapeInfo* info=nullptr;
                    valid=ok(api->GetTensorTypeAndShape(value,&info));
                    if(valid) {
                        size_t count=0;
                        valid=ok(api->GetDimensionsCount(info,&count));
                        if(valid && count==2) {
                            int64_t dims[2] {};
                            valid=ok(api->GetDimensions(info,dims,2));
                            if(valid && dims[0]==1 && (dims[1]==521 || dims[1]==1024)) {
                                void* data=nullptr;
                                valid=ok(api->GetTensorMutableData(value,&data));
                                if(valid) {
                                    if(dims[1]==521) scores=static_cast<const float*>(data);
                                    else representation=static_cast<const float*>(data);
                                }
                            } else valid=false;
                        } else valid=false;
                        api->ReleaseTensorTypeAndShapeInfo(info);
                    }
                    if(!valid) break;
                }
                if(valid && scores && representation) {
                    CynAudioScene s;
                    const auto clamp=[](float x) {return std::isfinite(x) ? std::clamp(x,0.0f,1.0f) : 0.0f;};
                    const auto top=[&](int a,int b) {return (std::max)(clamp(scores[a]),clamp(scores[b]));};
                    s.music=top(132,133);
                    s.speech=(std::max)(top(0,2),clamp(scores[3]));
                    s.percussion=(std::max)(top(156,157),top(158,159));
                    s.bassDrum=(std::max)(clamp(scores[163]),clamp(scores[238]));
                    s.electronic=(std::max)(top(234,235),top(236,240));
                    s.electronic=(std::max)(s.electronic,top(212,237));
                    s.vocals=top(24,249);
                    s.impact=(std::max)(top(420,421),top(430,487));
                    s.soundtrack=top(265,267);
                    s.silence=clamp(scores[494]);
                    float sum=0,active=0;
                    for(int i=0;i<521;++i) {
                        const float p=clamp(scores[i]);
                        sum+=p; active+=p>0.15f?1.0f:0.0f;
                    }
                    s.complexity=std::clamp(0.5f*sum/20.0f+
                                            0.5f*active/40.0f,0.0f,1.0f);
                    // A fixed signed projection of ALL 1024 YAMNet embedding
                    // components; equal-dimensional embedding compatibility
                    // across Music/Movie learning, without claiming it is a
                    // pretrained ideal-EQ or volume-prediction head.
                    float norm=0.0f;
                    for(int i=0;i<1024;++i) {
                        const float v=std::isfinite(representation[i])
                            ? std::clamp(representation[i],-100.0f,100.0f):0.0f;
                        const uint32_t code=(uint32_t)i*2654435761u+1013904223u;
                        const int bucket=(int)((code>>12)%CynAudioScene::embeddingSize);
                        s.embedding[(size_t)bucket]+=(code&0x80000000u)?v:-v;
                    }
                    for(auto& x:s.embedding) norm+=x*x;
                    if(norm>1e-12f) for(auto& x:s.embedding) x/=std::sqrt(norm);
                    if(previous.valid) {
                        float similarity=0;
                        for(int i=0;i<CynAudioScene::embeddingSize;++i)
                            similarity+=s.embedding[(size_t)i]*previous.embedding[(size_t)i];
                        s.novelty=std::clamp((1.0f-similarity)*0.8f,0.0f,1.0f);
                        // Smooth classification by scene window; do not smooth
                        // embedding itself or it will erase learnable timbre.
                        const auto smooth=[&](float old,float value){return 0.55f*old+0.45f*value;};
                        s.music=smooth(previous.music,s.music);
                        s.speech=smooth(previous.speech,s.speech);
                        s.percussion=smooth(previous.percussion,s.percussion);
                        s.bassDrum=smooth(previous.bassDrum,s.bassDrum);
                        s.electronic=smooth(previous.electronic,s.electronic);
                        s.vocals=smooth(previous.vocals,s.vocals);
                        s.impact=smooth(previous.impact,s.impact);
                        s.soundtrack=smooth(previous.soundtrack,s.soundtrack);
                        s.silence=smooth(previous.silence,s.silence);
                        s.complexity=smooth(previous.complexity,s.complexity);
                        s.novelty=smooth(previous.novelty,s.novelty);
                    }
                    s.valid=true; previous=s; publish(s);
                } else valid=false;
                for(auto* value:output) if(value) api->ReleaseValue(value);
                if(input) api->ReleaseValue(input);
                if(!valid) break;  // fallback: keep original audio DSP
                wait(380);
            }
        } while(false);
        lastResultMs_.store(0,std::memory_order_release);
        if(allocator) {
            if(inName) api->AllocatorFree(allocator,inName);
            if(outName0) api->AllocatorFree(allocator,outName0);
            if(outName1) api->AllocatorFree(allocator,outName1);
        }
        if(memory) api->ReleaseMemoryInfo(memory);
        if(session) api->ReleaseSession(session);
        if(options) api->ReleaseSessionOptions(options);
        if(env) api->ReleaseEnv(env);
    }
    std::atomic<bool> processing_enabled_ {false};
    DfxDsp& source_;
    juce::File model_,runtime_;
    std::array<std::atomic<float>,11> metrics_ {};
    std::array<std::atomic<float>,CynAudioScene::embeddingSize> embedding_ {};
    std::atomic<uint32_t> sequence_ {0},lastResultMs_ {0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynYamNetWorker)
};
