#include "../fxsound/Source/GUI/CynEqUiCallbacks.h"
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
int main() {
    for (int n : std::array<int,3>{{31,35,39}}) {
        std::vector<double> bandGain(static_cast<size_t>(n), 0.0);
        int writes = 0;
        auto read = [&](int band) { assert(band>=0 && band<n); return bandGain[static_cast<size_t>(band)]; };
        auto write = [&](int band, double value) { assert(band>=0 && band<n); bandGain[static_cast<size_t>(band)]=value; ++writes; };
        for (int band=0;band<n;++band) {
            const double value=band%2 ? -7.5 : 6.25;
            assert(CynEqUiCallbacks::dispatchGain(band,value,read,write));
            assert(!CynEqUiCallbacks::dispatchGain(band,value,read,write));
            assert(read(band)==value);
        }
        assert(writes==n);
        assert(!CynEqUiCallbacks::dispatchGain(-1,0.0,read,write));
        assert(!CynEqUiCallbacks::dispatchGain(0,std::numeric_limits<double>::quiet_NaN(),read,write));
        assert(!CynEqUiCallbacks::dispatchGain(0,std::numeric_limits<double>::infinity(),read,write));
        assert(writes==n);
        for (int band=0;band<n;++band) assert(CynEqUiCallbacks::dispatchGain(band,0.0,read,write));
        assert(writes==2*n);
    }
    std::cout<<"PASS real EQ UI dispatch: 31/35/39 bands, no duplicate writes or invalid values\\n";
}
