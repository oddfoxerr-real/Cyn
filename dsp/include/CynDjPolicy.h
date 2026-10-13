// Slow musical context is usable only alongside fresh current music detection.
#pragma once
#include <cmath>
namespace CynDjPolicy {
constexpr int contextLeaseSeconds=40;
inline bool contextTimely(double milliseconds) { return std::isfinite(milliseconds) && milliseconds>=0 && milliseconds<=30000; }
inline bool currentMusic(bool fresh,float music,float speech,float silence) {
    return fresh && std::isfinite(music) && std::isfinite(speech) && std::isfinite(silence)
        && music>=0.35f && speech<0.25f && silence<0.6f;
}
// The fast classifier can run the DJ while expensive context is loading or late.
inline bool permitLive(bool fresh,float music,float speech,float silence,bool context) {
    return currentMusic(fresh,music,speech,silence) && (context || music>=0.65f);
}
}
