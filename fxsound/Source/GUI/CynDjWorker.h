// Optional pretrained AST scene analysis and model lifecycle.
// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <JuceHeader.h>
#include "DfxDsp.h"
#include "CynAstFeatures.h"
#include "CynDjPolicy.h"
#include "../../../third_party/onnxruntime/onnxruntime_c_api.h"
#include <atomic>
#include <stdexcept>

class CynDjWorker final : private juce::Thread {
public:
    static constexpr int64_t modelBytes=90807386;
    static constexpr const char* modelHash="807d244b58a30eaa89f0a721fb841619c696857aabe4eac93769bd0b61497f61";
    static constexpr const char* modelUrl="https://huggingface.co/Xenova/ast-finetuned-audioset-10-10-0.4593/resolve/249a1fbf0286b40e7f1ed687a8ae396997bf7dc6/onnx/model_quantized.onnx";
    explicit CynDjWorker(DfxDsp& source,juce::File storage=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Cyn/AI-DJ"))
        :juce::Thread("Cyn AI DJ"),source_(source),folder_(storage),
        model_(folder_.getChildFile("ast-audioset-int8.onnx")) {
        installed_.store(model_.getSize()==modelBytes);
        auto settings=juce::JSON::parse(folder_.getChildFile("settings.json"));
        if(settings.isObject()) {
            amount_=juce::jlimit(0.0f,1.0f,float(settings["intensity"]));
            spacing_=juce::jlimit(4.0f,45.0f,float(settings["spacing"]));
            stereo_=bool(settings["stereo"]);
        }
        source_.configureDj(amount_,spacing_,stereo_);
        setStatus(installed_?"Installed - enable AI DJ to start":"Optional download: 91 MB. Runs locally after installation.");
        startThread(2);
    }
    ~CynDjWorker() override {
        source_.enableDj(false); source_.invalidateDjScene();
        signalThreadShouldExit(); cancel_.store(true); notify();
        stopThread(-1); // never terminate a thread inside ONNX / file writes
    }
    bool installed() const { return installed_.load(); }
    bool busy() const { return busy_.load(); }
    bool enabled() const { return enabled_.load(); }
    double progress() const { return progress_.load(); }
    float intensity() const { return amount_; }
    float spacing() const { return spacing_; }
    bool stereo() const { return stereo_; }
    juce::String status() const {
        const juce::ScopedLock lock(statusLock_);
        if(enabled_) {
            switch(fastState_.load()) {
                case 0:return "Waiting for fresh live audio analysis";
                case 1:return "Voice detected now - effects paused";
                case 2:return "Waiting for music in the current audio";
                default:break;
            }
            if(!contextReady_) return "Live DJ active - background context is warming up";
        }
        return status_;
    }
    void liveScene(bool fresh,float music,float speech,float silence) {
        const auto stamp=contextStamp_.load();
        const bool context=contextReady_ && stamp!=0 && uint32_t(juce::Time::getMillisecondCounter()-stamp)<40000;
        const bool safe=enabled_ && verified_ && CynDjPolicy::permitLive(fresh,music,speech,silence,context);
        source_.setDjVoiceGate(safe);
        source_.setDjScene(safe?1.0f:0.0f,speech);
        fastState_=!fresh?0:speech>=0.25f?1:!safe?2:3;
    }
    uint32_t effects() const { return source_.djEffectsCount(); }
    uint32_t analyses() const {return analyses_.load();}
    juce::String lastEffect() const {
        switch(source_.djLastEffect()) {case 1:return "Stereo sweep";case 2:return "Echo accent";case 3:return "Shimmer";default:return "No effects yet";}
    }
    void download() { if(busy()) return; setEnabled(false); cancel_=false; busy_=true; command_=1; notify(); }
    void cancelDownload() { cancel_=true; }
    void remove() { if(busy()) return; setEnabled(false); busy_=true; command_=2; notify(); }
    void setEnabled(bool value) {
        value=value&&installed()&&!busy(); if(value==enabled_)return;
        enabled_=value; contextReady_=false; verified_=false;generation_.fetch_add(1);
        source_.enableDj(value); if(!value) source_.invalidateDjScene(); notify();
    }
    void configure(float amount,float spacing,bool stereo) {
        amount_=juce::jlimit(0.0f,1.0f,amount); spacing_=juce::jlimit(4.0f,45.0f,spacing); stereo_=stereo;
        source_.configureDj(amount_,spacing_,stereo_);
        juce::DynamicObject::Ptr o=new juce::DynamicObject();
        o->setProperty("intensity",amount_);o->setProperty("spacing",spacing_);o->setProperty("stereo",stereo_);
        folder_.createDirectory();folder_.getChildFile("settings.json").replaceWithText(juce::JSON::toString(juce::var(o.get())));
    }
private:
    void setStatus(const juce::String& text) { const juce::ScopedLock lock(statusLock_);status_=text; }
    bool verify() const { return model_.getSize()==modelBytes&&juce::SHA256(model_).toHexString()==modelHash; }
    void install() {
        progress_=0;setStatus("Downloading AI DJ - you can keep listening");
        const auto partial=folder_.getChildFile("ast-audioset-int8.onnx.part");
        try {
            if(folder_.createDirectory().failed()) throw std::runtime_error("Cannot create AI DJ folder");
            if(partial.existsAsFile()&&!partial.deleteFile()) throw std::runtime_error("Cannot clear incomplete download");
            int code=0;
            auto stream=juce::URL(modelUrl).createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(10000).withNumRedirectsToFollow(5).withStatusCode(&code));
            if(!stream || code!=200) throw std::runtime_error("Download could not connect. Check your connection and retry.");
            auto out=partial.createOutputStream();
            if(!out || out->failedToOpen()) throw std::runtime_error("Cannot save model. Check available disk space.");
            std::array<char,65536> buffer{}; int64_t total=0;
            while(!cancel_&&!threadShouldExit()) {
                int read=stream->read(buffer.data(),int(buffer.size()));
                if(read<=0) break;
                total+=read;
                if(total>modelBytes || !out->write(buffer.data(),size_t(read))) throw std::runtime_error("Model download is incomplete or disk is full.");
                progress_=double(total)/modelBytes;
            }
            out->flush();if(out->getStatus().failed()) throw std::runtime_error("Model could not be written to disk.");out.reset();stream.reset();
            if(cancel_||threadShouldExit()) throw std::runtime_error("Download cancelled. You can retry anytime.");
            setStatus("Verifying AI DJ download...");
            if(total!=modelBytes || juce::SHA256(partial).toHexString()!=modelHash) throw std::runtime_error("Download verification failed. Please retry.");
            if(!partial.moveFileTo(model_)) throw std::runtime_error("Could not finish installation.");
            installed_=true;setStatus("Installed - enable AI DJ to start");
        } catch(const std::exception& e) { partial.deleteFile();setStatus(e.what()); }
        busy_=false;
    }
    void infer() {
        const auto generation=generation_.load();
        source_.invalidateDjScene();setStatus("Loading AI DJ model...");
        juce::DynamicLibrary library;
        const auto dll=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("onnxruntime.dll");
        if(!verify()) { installed_=false;enabled_=false;source_.enableDj(false);setStatus("Model missing or damaged. Download AI DJ again.");return; }
        verified_=true;
        using GetApi=const OrtApiBase*(ORT_API_CALL*)();
        if(!library.open(dll.getFullPathName())) { fail("Audio runtime is missing. Reinstall the latest Cyn version.");return; }
        auto get=reinterpret_cast<GetApi>(library.getFunction("OrtGetApiBase"));
        const OrtApi* api=get?get()->GetApi(ORT_API_VERSION):nullptr;
        if(!api) { fail("Audio runtime is incompatible. Update Cyn.");return; }
        OrtEnv* env=nullptr;OrtSessionOptions* options=nullptr;OrtSession* session=nullptr;OrtMemoryInfo* memory=nullptr;
        const auto check=[api](OrtStatus* s) { if(s) { std::string message=api->GetErrorMessage(s);api->ReleaseStatus(s);throw std::runtime_error(message); } };
        try {
            check(api->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"CynDj",&env));
            check(api->CreateSessionOptions(&options));check(api->SetIntraOpNumThreads(options,2));check(api->SetInterOpNumThreads(options,1));
            check(api->SetSessionGraphOptimizationLevel(options,ORT_ENABLE_ALL));
            check(api->CreateSession(env,model_.getFullPathName().toWideCharPointer(),options,&session));
            check(api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&memory));
            auto frontend=std::make_unique<CynAstFeatures>();
            auto waveform=std::make_unique<std::array<float,CynAstFeatures::samples>>();
            std::vector<float> features; uint64_t window=0;
            while(enabled_&&!threadShouldExit()&&command_==0&&generation_.load()==generation) {
                if(!source_.copyDjWindow(*waveform,window)) { setStatus("Listening - gathering 8 seconds of audio");wait(300);continue; }
                frontend->compute(waveform->data(),waveform->size(),features);
                OrtValue* input=nullptr;OrtValue* output=nullptr;OrtTensorTypeAndShapeInfo* info=nullptr;
                try {
                    const int64_t shape[]={1,1024,128};const char* inputs[]={"input_values"};const char* outputs[]={"logits"};
                    check(api->CreateTensorWithDataAsOrtValue(memory,features.data(),features.size()*sizeof(float),shape,3,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&input));
                    const OrtValue* values[]={input};
                    const auto start=juce::Time::getMillisecondCounterHiRes();
                    check(api->Run(session,nullptr,inputs,values,1,outputs,1,&output));
                    check(api->GetTensorTypeAndShape(output,&info));size_t count=0;check(api->GetTensorShapeElementCount(info,&count));
                    if(count!=527) throw std::runtime_error("Unexpected AST output shape");
                    void* raw=nullptr;check(api->GetTensorMutableData(output,&raw));auto* logits=static_cast<float*>(raw);
                    const auto score=[&](int i) { if(!std::isfinite(logits[i])) throw std::runtime_error("Invalid model output");return 1.0f/(1.0f+std::exp(-std::clamp(logits[i],-50.0f,50.0f))); };
                    const float music=score(137),speech=(std::max)(score(0),(std::max)(score(4),score(5)));
                    const double duration=juce::Time::getMillisecondCounterHiRes()-start;
                    const bool delayed=!CynDjPolicy::contextTimely(duration);
                    contextReady_=!delayed && music>=0.35f && speech<0.25f;
                    contextStamp_=juce::Time::getMillisecondCounter();
                    analyses_.fetch_add(1);
                    setStatus("Live DJ active / background analysis "+juce::String(duration/1000.0,1)+" s");
                } catch(...) { if(info) api->ReleaseTensorTypeAndShapeInfo(info);if(output) api->ReleaseValue(output);if(input) api->ReleaseValue(input);throw; }
                if(info) api->ReleaseTensorTypeAndShapeInfo(info);if(output) api->ReleaseValue(output);if(input) api->ReleaseValue(input);
                wait(2500);
            }
        } catch(const std::exception& e) { fail("AI DJ stopped: "+juce::String(e.what()).substring(0,180)); }
        source_.invalidateDjScene();
        if(memory) api->ReleaseMemoryInfo(memory);if(session) api->ReleaseSession(session);
        if(options) api->ReleaseSessionOptions(options);if(env) api->ReleaseEnv(env);
        if(!enabled_&&command_==0&&!failed_) setStatus("AI DJ off - model unloaded from memory");
    }
    void fail(const juce::String& reason) { failed_=true;enabled_=false;source_.enableDj(false);source_.invalidateDjScene();setStatus(reason); }
    void run() override {
        while(!threadShouldExit()) {
            const int task=command_.exchange(0);
            if(task==1) install();
            else if(task==2) {
                if(model_.deleteFile()&&folder_.getChildFile("ast-audioset-int8.onnx.part").deleteFile()) {
                    installed_=false;setStatus("AI DJ removed - 91 MB reclaimed");
                } else setStatus("Could not remove AI DJ. Close other Cyn windows and retry.");
                busy_=false;
            } else if(enabled_) { failed_=false;infer(); }
            else wait(300);
        }
    }
    DfxDsp& source_;
    juce::File folder_,model_;
    mutable juce::CriticalSection statusLock_;
    juce::String status_;
    std::atomic<bool> installed_{false},busy_{false},enabled_{false},cancel_{false};
    std::atomic<bool> contextReady_{false},verified_{false};std::atomic<uint32_t> contextStamp_{0};std::atomic<int> fastState_{0};
    std::atomic<uint32_t> analyses_{0},generation_{0};
    std::atomic<int> command_{0};std::atomic<double> progress_{0};
    float amount_=0.25f,spacing_=18.0f;bool stereo_=true,failed_=false;
};
