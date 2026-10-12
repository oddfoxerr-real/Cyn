// Cyn AI DJ isolated regression tests. Build: g++ -std=c++17 -O2 tests/cyn_ai_dj_test.cpp -o cyn_ai_dj_test
#include "../dsp/include/CynAiDjEngine.h"
#include <cassert>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>

static bool changed(const std::vector<float>& a,const std::vector<float>& b) {
    for (std::size_t i=0;i<a.size();++i)
        if (std::abs(a[i]-b[i])>1e-7f) return true;
    return false;
}
static void feed(CynAiDjEngine& engine, float value, bool& everChanged) {
    std::vector<float> source(960,value), processed=source;
    engine.process(source.data(),processed.data(),480);
    everChanged |= changed(processed,source);
    for (float sample:processed) assert(std::isfinite(sample) && std::abs(sample)<1.0f);
}
int main() {
    CynAiDjEngine engine;
    engine.setFormat(32,2,48000);
    engine.setScene(.95f,0.0f,.9f,0.0f);
    bool emitted=false;
    for(int i=0;i<150;++i) feed(engine, i==120?.65f:.01f,emitted);
    assert(!emitted); // default off: bit-perfect output
    engine.configure(true,.45f);
    engine.setScene(.1f,.95f,.7f,0);
    for(int i=0;i<140;++i) feed(engine,i==120?.65f:.01f,emitted);
    assert(!emitted); // speech/unknown scenes remain untouched
    engine.setScene(.95f,.0f,.9f,0);
    for(int i=0;i<140;++i) feed(engine,i==120?.65f:.01f,emitted);
    assert(emitted); // confident musical onset yields a quiet stereo accent
    engine.configure(false,1.0f);
    std::vector<float> input(960,.5f), output=input;
    engine.process(input.data(),output.data(),480);
    assert(!changed(output,input)); // off must always leave audio alone
    engine.configure(true,1.0f);
    engine.setFormat(32,1,48000);
    engine.process(input.data(),output.data(),480);
    assert(!changed(output,input)); // mono stream: never touch
    std::puts("Cyn AI DJ regression tests passed");
}
