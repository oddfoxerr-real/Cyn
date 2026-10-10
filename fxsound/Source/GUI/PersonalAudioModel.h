// Cyn local preference learning. SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "InputAudioFeatures.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <atomic>
#include <functional>
#include <mutex>

struct AudioExample { InputAudioFeatures audio; int effect=4; bool more=false; int64_t id=0; bool starter=false; };
struct AudioFrame { InputAudioFeatures audio; int64_t id=0; };

// Live inference telemetry, sampled on the JUCE message thread, never audio callback.
struct NeuralActivitySnapshot {
    uint32_t sequence = 0;
    bool applied = false, training = false;
    int epoch = 0;
    std::array<float,11> inputs {};
    std::array<float,12> hidden {};
    std::array<float,5> outputs {};
    std::array<float,132> inputConnections {};
    std::array<float,60> outputConnections {};
    std::array<bool,5> ready {};
    float rms = 0.0f;
};
// Only the background trainer takes this lock; no audio callback uses it.
struct NeuralTrainingTelemetry {
    mutable std::mutex mutex;
    NeuralActivitySnapshot snapshot {};
    bool available = false;
    void publish(NeuralActivitySnapshot value, int epoch) {
        std::lock_guard<std::mutex> guard(mutex);
        value.training=true; value.epoch=epoch; value.sequence=epoch;
        snapshot=value; available=true;
    }
    bool read(NeuralActivitySnapshot& value) const {
        std::lock_guard<std::mutex> guard(mutex);
        if(available) value=snapshot;
        return available;
    }
};
// 11 inputs -> 12 tanh neurons -> 5 sigmoid outputs (209 learned parameters).
// No hand-written audio classification rules. Bounds/smoothing live separately.
class PersonalAudioModel
{
public:
    static constexpr int inputs=11, hidden=12, outputs=5, weightCount=209;
    std::array<float,weightCount> weights {};
    std::array<bool,outputs> ready {};
    PersonalAudioModel() {
        uint32_t seed=717;
        for(auto& w:weights) { seed=1664525*seed+1013904223; w=((seed>>8)/16777216.0f-0.5f)*0.3f; }
    }
    static std::array<float,inputs> normalize(const InputAudioFeatures& a) {
        std::array<float,inputs> x {};
        for(int i=0;i<inputs;++i) {
            float v=std::isfinite(a.values[i])?a.values[i]:0;
            x[i]=i<8 ? (std::clamp(v,-12.0f,0.0f)+6)/6 : i==8 ? std::clamp(v,0.0f,4.0f)/2-1
                : i==9 ? std::clamp(v,0.0f,2.0f)-1 : (std::clamp(v,-8.0f,0.0f)+4)/4;
        }
        return x;
    }
    std::array<float,outputs> predict(const InputAudioFeatures& a) const {
        auto x=normalize(a); std::array<float,hidden> h {}; std::array<float,outputs> out {};
        for(int j=0;j<hidden;++j) { float z=weights[132+j]; for(int i=0;i<inputs;++i) z+=x[i]*weights[j*inputs+i]; h[j]=std::tanh(z); }
        for(int o=0;o<outputs;++o) { float z=weights[204+o]; for(int j=0;j<hidden;++j) z+=h[j]*weights[144+o*hidden+j]; out[o]=1/(1+std::exp(-std::clamp(z,-30.0f,30.0f))); }
        return out;
    }
    NeuralActivitySnapshot trace(const InputAudioFeatures& a) const {
        NeuralActivitySnapshot s;
        s.sequence=a.sequence;
        s.inputs=normalize(a); s.ready=ready; s.rms=std::isfinite(a.rms)?a.rms:0.0f;
        for(int j=0;j<hidden;++j) {
            float z=weights[132+j];
            for(int i=0;i<inputs;++i) {
                const float e=s.inputs[i]*weights[j*inputs+i];
                s.inputConnections[j*inputs+i]=e; z+=e;
            }
            s.hidden[j]=std::tanh(z);
        }
        for(int o=0;o<outputs;++o) {
            float z=weights[204+o];
            for(int j=0;j<hidden;++j) {
                const float e=s.hidden[j]*weights[144+o*hidden+j];
                s.outputConnections[o*hidden+j]=e; z+=e;
            }
            s.outputs[o]=1.0f/(1.0f+std::exp(-std::clamp(z,-30.0f,30.0f)));
        }
        return s;
    }
    bool fit(const std::vector<AudioExample>& examples, std::atomic<int>* epochsDone=nullptr,
             std::function<void(const NeuralActivitySnapshot&,int)> telemetry={}) {
        int counts[outputs][2] {};
        for(auto& e:examples) if(e.effect>=0 && e.effect<outputs) ++counts[e.effect][e.more?1:0];
        bool any=false;
        for(int o=0;o<outputs;++o) { ready[o]=counts[o][0]>=5 && counts[o][1]>=5; any=any||ready[o]; }
        if(!any) return false;
        std::array<float,weightCount> momentum {},variance {};
        float beta1=1,beta2=1;
        for(int epoch=1;epoch<=240;++epoch) {
            std::array<float,weightCount> grad {};
            for(const auto& e:examples) {
                int o=e.effect; if(o<0 || o>=outputs || !ready[o]) continue;
                auto x=normalize(e.audio); std::array<float,hidden> h {};
                for(int j=0;j<hidden;++j) { float z=weights[132+j]; for(int i=0;i<inputs;++i) z+=x[i]*weights[j*inputs+i]; h[j]=std::tanh(z); }
                float z=weights[204+o]; for(int j=0;j<hidden;++j) z+=h[j]*weights[144+o*hidden+j];
                const float sampleWeight=(e.starter?0.2f:1.0f)/counts[o][e.more?1:0];
                const float d=(1/(1+std::exp(-std::clamp(z,-30.0f,30.0f)))-(e.more?1.0f:0.0f))*sampleWeight;
                grad[204+o]+=d;
                for(int j=0;j<hidden;++j) {
                    grad[144+o*hidden+j]+=d*h[j];
                    float dh=d*weights[144+o*hidden+j]*(1-h[j]*h[j]);
                    grad[132+j]+=dh; for(int i=0;i<inputs;++i) grad[j*inputs+i]+=dh*x[i];
                }
            }
            beta1*=0.9f; beta2*=0.999f;
            for(int i=0;i<weightCount;++i) {
                float g=grad[i]+0.0001f*weights[i];
                momentum[i]=0.9f*momentum[i]+0.1f*g; variance[i]=0.999f*variance[i]+0.001f*g*g;
                weights[i]-=0.012f*(momentum[i]/(1-beta1))/(std::sqrt(variance[i]/(1-beta2))+1e-7f);
                if(!std::isfinite(weights[i])) return false;
            }
            if(epochsDone) epochsDone->store(epoch,std::memory_order_relaxed);
            if(telemetry && (epoch==1 || epoch%12==0)) {
                for(const auto& e:examples) if(e.effect>=0 && e.effect<outputs && ready[e.effect]) {
                    telemetry(trace(e.audio),epoch); break;
                }
            }
        }
        return true;
    }
};
