// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "InputAudioFeatures.h"
#include "CynBalanceWeights.h"
#include <array>
#include <algorithm>
#include <cmath>

struct CynSpectralRoles {
    std::array<float,5> share{}; // bass, kick, snare, high percussion, other
    bool valid=false;
};

// Spectral-role estimator used alongside YAMNet scene classification.
// Outputs mixture energy shares; it does not perform source separation.
// Called by the control thread; fixed weights, no heap, network or user labels.
class CynBalanceModel {
public:
    static CynSpectralRoles predict(const InputAudioFeatures& audio) noexcept {
        CynSpectralRoles result;
        if(!std::isfinite(audio.rms) || audio.rms<=.0001f) return result;
        std::array<float,11> x{}; std::array<float,24> h{};
        for(int i=0;i<11;++i) {
            if(!std::isfinite(audio.values[i])) return result;
            x[i]=std::clamp((audio.values[i]-CynBalanceWeights::mean[i])/CynBalanceWeights::scale[i],-6.0f,6.0f);
        }
        for(int j=0;j<24;++j) {
            float z=CynBalanceWeights::b1[j];
            for(int i=0;i<11;++i) z+=x[i]*CynBalanceWeights::w1[i*24+j];
            h[j]=std::tanh(z);
        }
        for(int j=0;j<5;++j) {
            float z=CynBalanceWeights::b2[j];
            for(int i=0;i<24;++i) z+=h[i]*CynBalanceWeights::w2[i*5+j];
            result.share[j]=z;
        }
        float maximum=*std::max_element(result.share.begin(),result.share.end()),sum=0;
        for(auto& p:result.share) { p=std::exp(p-maximum); sum+=p; }
        for(auto& p:result.share) p/=sum;
        result.valid=true;
        return result;
    }
};
