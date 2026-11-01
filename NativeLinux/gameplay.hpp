#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace reriff {
// Recovered from Game.exe SHA-256 2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2.
// gaTrackGen.calcVol, MethodDef 0x060008d6, RVA 0x24e5fc.
// Keep the original intermediate float conversions: they affect track geometry.
inline float trackVolume(const std::vector<float>& volumes, int32_t start,
                         int32_t length, bool raw) {
    int64_t available=length;
    if (start < 0) {
        available += start;
        start = 0;
    }
    float sum = 0, count = 0;
    // Widen range arithmetic to avoid C++ signed overflow on malformed inputs.
    const int64_t end = std::min<int64_t>(int64_t(start)+available, volumes.size());
    for (int64_t i=start; i<end; ++i) {
        const double value=volumes[static_cast<size_t>(i)];
        sum=static_cast<float>(double(static_cast<float>(value*value))+sum);
        count += 1.0f;
    }
    if (count <= 0) return 0;
    float result=static_cast<float>(std::sqrt(double(static_cast<float>(double(sum)/count))));
    if (!raw) result=static_cast<float>(std::pow(double(result), double(0.8f)));
    if (result < 0) result=0;
    else if (result >= 1) result=1;
    return result;
}

// gaVehicleGearbox.torqueFunction, MethodDef 0x06000b54, RVA 0x27e5c4.
// The caller selects the reverse/forward gear and passes its torque and ramp end.
// Threshold units remain those of the original caller; no RPM assumption.
inline float gearTorque(bool neutral, float input, float torque, float rampEnd,
                        float plateauEnd, float cutoff, bool limit) {
    if (neutral) return 0;
    if (input < rampEnd) {
        const float fraction=static_cast<float>(double(input)/rampEnd);
        const double remaining=1.0-double(fraction);
        return static_cast<float>(double(static_cast<float>(1.0-remaining*remaining*remaining))*torque);
    }
    if (input < plateauEnd || !limit) return torque;
    float fraction=static_cast<float>((double(input)-plateauEnd)/(double(cutoff)-plateauEnd));
    if (fraction > 1) fraction=1;
    return static_cast<float>((0.0-double(torque))*fraction+torque);
}
}
