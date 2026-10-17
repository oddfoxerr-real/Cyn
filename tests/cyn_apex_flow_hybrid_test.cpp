// SPDX-License-Identifier: AGPL-3.0-or-later
#include "../fxsound/Source/GUI/CynApexFlowHybrid.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

static CynApexFlowHybrid::Observation sample(int tick,float rms) {
    CynApexFlowHybrid::Observation o;
    o.ms=1000+100*tick;o.rms=rms;o.peak=0.42f;o.music=0.92f;
    o.percussion=0.86f;o.bassDrum=0.88f;o.vocals=0.1f;
    o.pretrainedSceneValid=true;return o;
}
int main(){
    using Engine=CynApexFlowHybrid;
    Engine m;
    auto a=m.observe(sample(0,0.07f)); assert(!a.valid);
    for(int t=1;t<50;t++) a=m.observe(sample(t,0.07f));
    assert(a.valid && a.impact<0.01f && a.bassBias<0.02f);
    for(int t=50;t<56;t++) a=m.observe(sample(t,0.23f));
    assert(a.valid && a.impact>0.08f && a.bassBias>0.01f);
    assert(a.lowEqDb>0 && a.lowEqDb<=0.8f);
    const float kickBoost=a.bassBias;
    auto dupe=m.observe(sample(55,0.3f));assert(dupe.impact==a.impact);
    for(int t=56;t<68;t++) a=m.observe(sample(t,0.23f));
    assert(a.bassBias<kickBoost);
    m.reset();
    for(int t=0;t<45;t++){auto o=sample(t,0.06f);o.vocals=0.95f;a=m.observe(o);}
    assert(a.valid && a.clarityBias>0 && a.clarityBias<=0.035f);
    for(int t=45;t<52;t++){auto o=sample(t,0.23f);o.peak=0.99f;a=m.observe(o);}
    assert(a.impact>0 && a.lowEqDb==0 && a.clarityBias==0);
    auto o=sample(53,0.09f);o.speech=0.96f;o.music=0.60f;
    a=m.observe(o);assert(!a.valid && a.bassBias==0);
    o=sample(54,0.09f);o.pretrainedSceneValid=false;
    a=m.observe(o);assert(!a.valid);
    o=sample(55,0.09f);o.rms=std::numeric_limits<float>::quiet_NaN();
    a=m.observe(o);assert(!a.valid);
    for(int t=0;t<45;t++)a=m.observe(sample(100+t,0.08f));
    o=sample(200,0.23f);a=m.observe(o);assert(!a.valid);
    std::cout<<"PASS: APEX Flow Hybrid builds, impacts, releases, vocal guards, "
             <<"peak guards, invalid/stale data and fallback\n";
}
