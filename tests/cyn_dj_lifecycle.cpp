// Uses the production worker with an isolated storage folder and mock capture.
// Exercises real HTTPS, SHA-256, native ONNX loading/inference, unload and remove.
#include "CynDjWorker.h"
#include <cassert>
#include <iostream>
static std::atomic<int> results{0};
static std::atomic<int> permitted{0};
DfxDsp::DfxDsp():data_(nullptr) {}
DfxDsp::~DfxDsp() {}
void DfxDsp::enableDj(bool) {}
void DfxDsp::configureDj(float,float,bool) {}
void DfxDsp::setDjScene(float music,float speech) {assert(std::isfinite(music)&&std::isfinite(speech));++results;if(music>=0.35f&&speech<0.25f)++permitted;}
void DfxDsp::invalidateDjScene() {}
void DfxDsp::setDjVoiceGate(bool) {}
uint32_t DfxDsp::djEffectsCount() const {return 0;}
int DfxDsp::djLastEffect() const {return 0;}
bool DfxDsp::copyDjWindow(std::array<float,128000>& audio,uint64_t& number) const {
    for(size_t i=0;i<audio.size();++i)audio[i]=0.1f*float(std::sin(i*6.283185307179586*440/16000));
    number+=128000;return true;
}
int main(int argc,char** argv) {
    assert(argc==3);
    juce::File storage(juce::String::fromUTF8(argv[1]));
    juce::File cached(juce::String::fromUTF8(argv[2]));
    assert(storage.getFileName()=="lifecycle-storage");storage.createDirectory();
    const auto model=storage.getChildFile("ast-audioset-int8.onnx");
    assert(cached.copyFileTo(model));
    DfxDsp source;
    {
        CynDjWorker dj(source,storage);
        assert(dj.installed());assert(!dj.enabled());dj.setEnabled(true);
        for(int i=0;i<180&&dj.analyses()==0;++i){dj.liveScene(true,0.9f,0.01f,0);juce::Thread::sleep(500);}
        std::cout<<dj.status()<<std::endl;
        assert(dj.analyses()>0&&permitted>0);dj.setEnabled(false);
        dj.remove();for(int i=0;i<180&&dj.busy();++i)juce::Thread::sleep(500);
        assert(!dj.busy()&&!dj.installed()&&!model.existsAsFile());
        dj.download();juce::Thread::sleep(200);dj.cancelDownload();
        for(int i=0;i<90&&dj.busy();++i)juce::Thread::sleep(500);
        std::cout<<dj.status()<<std::endl;
        assert(!dj.busy()&&!dj.installed());
        assert(!storage.getChildFile("ast-audioset-int8.onnx.part").existsAsFile());
        dj.download();
        for(int i=0;i<600&&dj.busy();++i)juce::Thread::sleep(500);
        std::cout<<dj.status()<<std::endl;
        assert(!dj.busy()&&dj.installed());
        assert(juce::SHA256(model).toHexString()==CynDjWorker::modelHash);
        dj.remove();for(int i=0;i<90&&dj.busy();++i)juce::Thread::sleep(500);
        assert(!dj.busy()&&!model.existsAsFile());
    }
    // Corrupt a full-size file: size-only installation discovery must never
    // authorise inference; production verification must reject it on enable.
    assert(cached.copyFileTo(model));
    {auto out=model.createOutputStream();assert(out);out->setPosition(128);out->writeInt(0x12345678);}
    {
        CynDjWorker dj(source,storage);dj.setEnabled(true);
        for(int i=0;i<90&&dj.enabled();++i)juce::Thread::sleep(500);
        std::cout<<dj.status()<<std::endl;
        assert(!dj.enabled()&&!dj.installed());
    }
    model.deleteFile();
    std::cout<<"PASS: native ONNX inference, disable/unload, remove, cancel, HTTPS download, retry, SHA verification, corrupt model rejection"<<std::endl;
}
