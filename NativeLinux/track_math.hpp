#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace reriff::track {
struct Vec3 {
    float x,y,z;
    float operator[](size_t i) const { return i==0?x:i==1?y:z; }
    float& operator[](size_t i) { return i==0?x:i==1?y:z; }
};
inline Vec3 add(Vec3 a,Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 sub(Vec3 a,Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3 scale(Vec3 a,float b) { return {a.x*b,a.y*b,a.z*b}; }
inline Vec3 cross(Vec3 a,Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
// elVec3.normalise, RVA 0x26d90. The unusual fallback is part of the original.
inline Vec3 normalize(Vec3 v) {
    if(std::fabs(v.x)<=1e-6f && std::fabs(v.y)<=1e-6f && std::fabs(v.z)<=1e-6f) return {0,0,1};
    float squared=(v.y*v.y+v.x*v.x)+v.z*v.z;
    float length=static_cast<float>(std::sqrt(static_cast<double>(squared)));
    return scale(v,1.0f/length);
}
struct Frame { Vec3 right,up,forward; };
// elMatrix.setOrientation, RVA 0x582a0. Translation is set by its caller.
inline Frame orientation(Vec3 forward,Vec3 upHint) {
    forward=normalize(forward);
    Vec3 right=normalize(cross(upHint,forward));
    return {right,cross(forward,right),forward};
}

// elSpline.evaluate/evaluateTangent, RVAs 0x1d0030/0x1d0360.
// The expanded accumulation order matches the original scalar SSE operations.
// Compile without floating-point contraction/reassociation for reference parity.
inline Vec3 evaluate(const Vec3* points,size_t count,float t,bool tangent=false) {
    if(count<4 || !std::isfinite(t)) throw std::invalid_argument("Spline needs four points and a finite parameter");
    if(t<=1) {
        Vec3 delta=sub(points[1],points[0]);
        return tangent?delta:add(scale(delta,t),points[0]);
    }
    t=std::min(t,static_cast<float>(count-3));
    size_t i=static_cast<size_t>(t);
    t-=static_cast<float>(i);
    Vec3 p0=points[i-1],p1=points[i],p2=points[i+1],p3=points[i+2],result{};
    float cubic=t*t*(tangent?1.5f:0.5f);
    if(!tangent) cubic*=t;
    float quadratic=tangent?t:t*t*0.5f;
    float linear=tangent?0.5f:t*0.5f;
    for(size_t axis=0;axis<3;++axis) {
        float value=(-cubic)*p0[axis];
        value=(cubic*3.0f)*p1[axis]+value;
        value=(cubic*-3.0f)*p2[axis]+value;
        value=cubic*p3[axis]+value;
        value=(quadratic*2.0f)*p0[axis]+value;
        value=(quadratic*-5.0f)*p1[axis]+value;
        value=(quadratic*4.0f)*p2[axis]+value;
        value=(-quadratic)*p3[axis]+value;
        value=(-linear)*p0[axis]+value;
        value=linear*p2[axis]+value;
        if(!tangent) value=p1[axis]+value;
        result[axis]=value;
    }
    return result;
}
}
