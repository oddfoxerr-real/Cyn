// Cyn YAMNet native asynchronous ONNX inference. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "DfxDsp.h"
#include "../../../third_party/onnxruntime/onnxruntime_c_api.h"
#include <array>
#include <atomic>
#include <algorithm>

// Worker runs at low priority, never on the DSP real-time audio callback.
class CynYamNetWorker final : private juce::Thread
{
public:
    explicit CynYamNetWorker(DfxDsp& source)
        : juce::Thread("Cyn YAMNet"), source_(source) {}
    ~CynYamNetWorker() override { signalThreadShouldExit(); stopThread(15000); }

    void startIfAvailable()
    {
        const auto dir=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
        model_=dir.getChildFile("yamnet-16k-0p975s.onnx");
        runtime_=dir.getChildFile("onnxruntime.dll");
        if(model_.existsAsFile() && model_.getSize()>1000000 &&
           runtime_.existsAsFile() && runtime_.getSize()>100000)
            startThread(2);
    }

    bool latest(float& speech,float& music,float& impacts) const noexcept
    {
        const auto stamp=lastResultMs_.load(std::memory_order_acquire);
        if(stamp==0 || (uint32_t)(juce::Time::getMillisecondCounter()-stamp)>2500) return false;
        speech=speech_.load(std::memory_order_relaxed);
        music=music_.load(std::memory_order_relaxed);
        impacts=impacts_.load(std::memory_order_relaxed);
        return true;
    }

private:
    void run() override
    {
        juce::DynamicLibrary library;
        if(!library.open(runtime_.getFullPathName())) return;
        using GetApiBaseFn=const OrtApiBase*(ORT_API_CALL*)();
        const auto address=library.getFunction("OrtGetApiBase");
        if(address==nullptr) return;
        const OrtApiBase* apiBase=reinterpret_cast<GetApiBaseFn>(address)();
        if(apiBase==nullptr) return;
        const OrtApi* api=apiBase->GetApi(ORT_API_VERSION);
        if(api==nullptr) return;
        const auto ok=[api](OrtStatus* error) {
            if(error==nullptr) return true;
            api->ReleaseStatus(error);
            return false;
        };
        OrtEnv* env=nullptr;
        OrtSessionOptions* options=nullptr;
        OrtSession* session=nullptr;
        OrtMemoryInfo* memory=nullptr;
        OrtAllocator* allocator=nullptr;
        char* inName=nullptr;
        char* outName0=nullptr;
        char* outName1=nullptr;
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

            while(!threadShouldExit())
            {
                if(!source_.copyYamNetWindow(waveform,lastWindow)) {
                    wait(220);
                    continue;
                }
                OrtValue* input=nullptr;
                OrtValue* outputs[2] {nullptr,nullptr};
                bool valid=ok(api->CreateTensorWithDataAsOrtValue(
                    memory,waveform.data(),waveform.size()*sizeof(float),
                    shape,1,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&input));
                if(valid) {
                    const OrtValue* const inputValues[]={input};
                    valid=ok(api->Run(session,nullptr,inputNames,inputValues,1,outputNames,2,outputs));
                }
                const float* scores=nullptr;
                if(valid) {
                    for(auto* output:outputs) {
                        if(output==nullptr) {valid=false;break;}
                        OrtTensorTypeAndShapeInfo* info=nullptr;
                        valid=ok(api->GetTensorTypeAndShape(output,&info));
                        if(valid) {
                            size_t dims=0;
                            valid=ok(api->GetDimensionsCount(info,&dims));
                            if(valid && dims==2) {
                                int64_t sizes[2] {};
                                valid=ok(api->GetDimensions(info,sizes,2));
                                if(valid && sizes[0]==1 && sizes[1]==521) {
                                    void* raw=nullptr;
                                    valid=ok(api->GetTensorMutableData(output,&raw));
                                    if(valid) scores=static_cast<const float*>(raw);
                                }
                            }
                            api->ReleaseTensorTypeAndShapeInfo(info);
                        }
                        if(!valid) break;
                    }
                }
                if(valid && scores!=nullptr) {
                    // Verified official AudioSet class positions: speech 0,2,3;
                    // music 132,133; explosion/gunshot 420,421.
                    const float speech=std::clamp(std::max({scores[0],scores[2],scores[3]}),0.0f,1.0f);
                    const float music=std::clamp(std::max(scores[132],scores[133]),0.0f,1.0f);
                    const float impact=std::clamp(std::max(scores[420],scores[421]),0.0f,1.0f);
                    const auto blend=[](std::atomic<float>& store,float value) {
                        const float old=store.load(std::memory_order_relaxed);
                        store.store(old*0.65f+value*0.35f,std::memory_order_relaxed);
                    };
                    blend(speech_,speech);
                    blend(music_,music);
                    blend(impacts_,impact);
                    lastResultMs_.store(juce::Time::getMillisecondCounter(),std::memory_order_release);
                }
                for(auto* value:outputs) if(value!=nullptr) api->ReleaseValue(value);
                if(input!=nullptr) api->ReleaseValue(input);
                if(!valid) break; // fail closed: existing DSP/personal AI unchanged
                wait(380);
            }
        } while(false);

        lastResultMs_.store(0,std::memory_order_release);
        if(allocator!=nullptr) {
            if(inName!=nullptr) api->AllocatorFree(allocator,inName);
            if(outName0!=nullptr) api->AllocatorFree(allocator,outName0);
            if(outName1!=nullptr) api->AllocatorFree(allocator,outName1);
        }
        if(memory) api->ReleaseMemoryInfo(memory);
        if(session) api->ReleaseSession(session);
        if(options) api->ReleaseSessionOptions(options);
        if(env) api->ReleaseEnv(env);
    }

    DfxDsp& source_;
    juce::File model_,runtime_;
    std::atomic<float> speech_{0.0f},music_{0.0f},impacts_{0.0f};
    std::atomic<uint32_t> lastResultMs_ {0};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CynYamNetWorker)
};
