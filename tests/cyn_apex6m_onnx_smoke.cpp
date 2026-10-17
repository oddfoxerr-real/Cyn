// Headless Windows smoke test: load the exact live APEX ONNX with the same
// onnxruntime.dll C API that Cyn itself uses. No real-audio driver required.
#include "../third_party/onnxruntime/onnxruntime_c_api.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <vector>
#include <iostream>
#include <filesystem>

int wmain(int argc, wchar_t** argv) {
    if(argc!=2) return 2;
    const auto dir=std::filesystem::path(argv[1]);
    HMODULE dll=LoadLibraryW((dir/L"onnxruntime.dll").c_str());
    if(!dll) return 3;
    using Fn=const OrtApiBase*(ORT_API_CALL*)();
    auto entry=reinterpret_cast<Fn>(GetProcAddress(dll,"OrtGetApiBase"));
    if(!entry) return 4;
    const OrtApi* api=entry()->GetApi(ORT_API_VERSION);
    if(!api) return 5;
    const auto good=[api](OrtStatus* status) {
        if(!status)return true;
        std::cerr << api->GetErrorMessage(status) << "\n";
        api->ReleaseStatus(status);return false;
    };
    OrtEnv* env=nullptr;OrtSessionOptions* opts=nullptr;
    OrtSession* session=nullptr;OrtMemoryInfo* memory=nullptr;
    OrtAllocator* allocator=nullptr;char* inputName=nullptr,*outputName=nullptr,*auxName=nullptr;
    OrtValue* input=nullptr,*output[2]={nullptr,nullptr};
    int code=1;
    do {
        if(!good(api->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"APEXCI",&env)))break;
        if(!good(api->CreateSessionOptions(&opts)))break;
        if(!good(api->SetIntraOpNumThreads(opts,2)))break;
        if(!good(api->CreateSession(env,(dir/L"apex6m-225.onnx").c_str(),opts,&session)))break;
        if(!good(api->CreateCpuMemoryInfo(OrtArenaAllocator,OrtMemTypeDefault,&memory)))break;
        if(!good(api->GetAllocatorWithDefaultOptions(&allocator)))break;
        if(!good(api->SessionGetInputName(session,0,allocator,&inputName)))break;
        if(!good(api->SessionGetOutputName(session,0,allocator,&outputName)))break;
        if(!good(api->SessionGetOutputName(session,1,allocator,&auxName)))break;
        constexpr int bins=513, frames=189;
        const int64_t shape[]={1,bins,frames};
        std::vector<float> features((size_t)bins*frames,0.15f);
        if(!good(api->CreateTensorWithDataAsOrtValue(memory,features.data(),
            features.size()*sizeof(float),shape,3,
            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,&input)))break;
        const char* const inputNames[]={inputName};
        const char* const outputNames[]={outputName,auxName};
        const OrtValue* const params[]={input};
        if(!good(api->Run(session,nullptr,inputNames,params,1,outputNames,2,output)))break;
        void* ptr=nullptr;
        if(!output[0] || !good(api->GetTensorMutableData(output[0],&ptr)) || !ptr)break;
        const float* mask=(const float*)ptr;
        if(!(mask[0]>=0 && mask[0]<=2))break;
        std::cout<<"PASS: full 6.3M APEX ONNX loads/runs with Cyn's actual Windows DLL.\n";
        code=0;
    }while(false);
    if(input)api->ReleaseValue(input);
    for(auto* value:output)if(value)api->ReleaseValue(value);
    if(allocator) {
        if(inputName)api->AllocatorFree(allocator,inputName);
        if(outputName)api->AllocatorFree(allocator,outputName);
        if(auxName)api->AllocatorFree(allocator,auxName);
    }
    if(memory)api->ReleaseMemoryInfo(memory);
    if(session)api->ReleaseSession(session);
    if(opts)api->ReleaseSessionOptions(opts);
    if(env)api->ReleaseEnv(env);
    FreeLibrary(dll);
    return code;
}
