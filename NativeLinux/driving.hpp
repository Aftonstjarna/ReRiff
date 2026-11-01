#pragma once
#include "playback.hpp"

namespace reriff::drive {
// Scalar ports only. No substitute vehicle integrator is provided.
// Source: analysis/native-port/decompiled/Module.cs, original Game.exe
// SHA-256 2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2.
struct Params {
    float steer20 = 30, steer100 = 6, driftExtra = 0;
};
// gaVehicleParams.updateInterpolatedValues + gaVehiclePhys.phys_calcSteer.
// Preserve original float stores between mph conversion and subtraction.
inline float steerLimit(const Params &p, float speed, float drift = 0, float input = 0,
                        float driftBias = 0) {
    float a = float(double(p.steer20) * 0.017453293005625408),
          b = float(double(p.steer100) * 0.017453293005625408);
    float decay = b == 0 ? 0 : float((double(a) / b - 1) / 80);
    float mph = std::fabs(speed) * 2.2369363f;
    mph -= 20.f;
    float result = mph > 0 ? float(double(a) / (double(decay) * mph + 1)) : a;
    float extra = float(double(p.driftExtra) * 0.017453293005625408);
    result = std::max(0.f, float(double(extra) * drift + double(result)));
    if (double(input) * driftBias < 0)
        result = float((1. - std::fabs(double(driftBias))) * result);
    return result;
}
// gaVehicleGearbox.determineBrakeInput, independent of the unported gear state machine.
inline float brakeInput(bool reverseToggle, int gear, float speed, float input) {
    if (reverseToggle) {
        if (gear > 0) {
            if (speed >= 0 && input < 0)
                return -input;
            if (double(speed) < -0.20000000298023224 && input > 0)
                return input;
        } else {
            if (speed <= 0 && input > 0)
                return input;
            if (double(speed) > 0.20000000298023224 && input < 0)
                return -input;
        }
        return 0;
    }
    return input > 0 ? 0 : -input;
}
} // namespace reriff::drive
