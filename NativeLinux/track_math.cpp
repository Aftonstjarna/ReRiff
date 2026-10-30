#include "track_math.hpp"
#include <cstdint>
#include <vector>

// Small checked C ABI for the asset preparation tools and future native runtime.
extern "C" int reriff_spline(const float* xyz,uint32_t count,float t,int tangent,float* out) {
    if(!xyz || !out || count<4 || count>100000 || !std::isfinite(t)) return 1;
    std::vector<reriff::track::Vec3> points;
    points.reserve(count);
    for(uint32_t i=0;i<count;++i) {
        for(unsigned k=0;k<3;++k) if(!std::isfinite(xyz[i*3+k])) return 1;
        points.push_back({xyz[i*3],xyz[i*3+1],xyz[i*3+2]});
    }
    auto result=reriff::track::evaluate(points.data(),points.size(),t,tangent!=0);
    for(unsigned i=0;i<3;++i) { out[i]=result[i];if(!std::isfinite(out[i])) return 2; }
    return 0;
}
extern "C" int reriff_orientation(const float* forward,const float* up,float* out) {
    if(!forward || !up || !out) return 1;
    for(unsigned i=0;i<3;++i) if(!std::isfinite(forward[i]) || !std::isfinite(up[i])) return 1;
    auto frame=reriff::track::orientation({forward[0],forward[1],forward[2]},{up[0],up[1],up[2]});
    for(unsigned i=0;i<3;++i) { out[i]=frame.right[i];out[i+3]=frame.up[i];out[i+6]=frame.forward[i]; }
    for(unsigned i=0;i<9;++i) if(!std::isfinite(out[i])) return 2;
    return 0;
}
