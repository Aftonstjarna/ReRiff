#pragma once
#include "vehicle_suspension.hpp"

namespace reriff::drive {
// Original drive allocation uses the uncorrected wheel normal and compression.
inline float wheelDriveWeight(int index, float rearBias, float compression, float extent,
                              bool stick, Vec3 up, Vec3 normal) {
    bool eligible =
        stick ? double(compression) > .0010000000474974513 - double(extent) * .5 : compression > 0;
    if (!eligible || originalDot(up, normal) < 0)
        return 0;
    return (index >= 2 ? rearBias : float(1. - double(rearBias))) * .5f;
}
struct TireInput {
    float baseLateralFriction, slipCone, throttle, equalisation, gripReduction;
    int contacts;
    float overrideLoad, tractionTimer, tractionBlend, driveForce, driveWeight, driveDenominator;
    float slopeAid;
    Vec3 right, forward, gravity;
};
struct TireResult {
    Vec3 force, forward;
    // Caller accumulates double(absoluteLateralSpeed) * travel + total, then
    // stores to float, as the original does; do not round that product first.
    float absoluteLateralSpeed;
};
inline float tireAcos(float value) {
    return float(std::acos(double(std::clamp(value, -1.f, 1.f))));
}
inline Vec3 tirePerpendicular(Vec3 value, Vec3 normal) {
    return track::sub(value, track::scale(normal, originalDot(normal, value)));
}
// Force assembly within phys_applySuspAndTyreForces_HI. The caller supplies the
// original steered front/unsteered rear wheel frame and precomputed surface friction.
inline TireResult tireForces(const SuspensionInput &in, const SuspensionResult &s,
                             const TireInput &t) {
    Vec3 forward = track::normalize(tirePerpendicular(t.forward, s.normal));
    Vec3 heading = track::normalize(tirePerpendicular(s.pointVelocity, s.normal));
    float slopeCos = 0.f - originalDot(s.normal, t.gravity), slopeBlend = 0;
    if (slopeCos < suspensionGradientCosine())
        slopeBlend = std::clamp(
            float((double(tireAcos(slopeCos)) - .6981317400932312) / .3490658700466156), 0.f, 1.f);
    float slipGrip = 1;
    if (originalLength(s.pointVelocity) > .5f) {
        float angle = float(double(tireAcos(originalDot(forward, heading))) * 57.29577791868205);
        slipGrip = grip(angle, t.slipCone);
        if (t.gripReduction != 0) {
            float remaining = float(1. - double(t.gripReduction));
            slipGrip = float(double(float(double(remaining) * remaining)) * slipGrip);
        }
    }
    float alignment = originalDot(in.bodyUp, s.normal);
    float tilt = std::clamp(float(1. - double(tireAcos(alignment)) / .7853981852531433), .2f, 1.f);
    float loadBlend =
        float((1. - double(t.equalisation)) * std::min(float(1. - double(t.throttle)), 1.f) +
              t.equalisation);
    if (t.contacts <= 0)
        loadBlend = 0;
    if (t.overrideLoad != 0)
        loadBlend = 1;
    float load =
        float(double(float((9.375 - double(s.smoothedLoad)) * loadBlend + s.smoothedLoad)) / 250.);
    float lateral =
        float(double(load) * slipGrip * tilt * (1. - double(slopeBlend)) * t.baseLateralFriction);
    lateral = std::min(lateral, 50.f);
    if (t.tractionTimer <= 0 && t.tractionBlend < 1)
        lateral =
            float(double(float(double(std::clamp(t.tractionBlend, 0.f, 1.f)) * .9975000000558794 +
                               .0024999999441206455)) *
                  lateral);
    Vec3 side = tirePerpendicular(t.right, s.normal); // Deliberately not normalized.
    float sideSpeed = originalDot(s.pointVelocity, side), absolute = std::abs(sideSpeed);
    float speedStep = float(double(lateral) * in.dt * float(t.contacts));
    if (speedStep > absolute)
        lateral = float(double(absolute) / speedStep * lateral);
    if (sideSpeed > 0)
        lateral = 0.f - lateral;
    Vec3 force = track::add(s.force, track::scale(side, float(double(in.mass) * lateral)));
    if (t.overrideLoad == 0 && slopeBlend > 0) {
        Vec3 down = track::normalize(t.gravity);
        Vec3 slope = track::normalize(tirePerpendicular(down, in.contactNormal));
        float along = originalDot(forward, slope);
        if (along > 0)
            forward = track::sub(forward, track::scale(track::scale(slope, along), slopeBlend));
    }
    float drive = float(double(t.driveForce) * t.driveWeight / t.driveDenominator);
    if (slopeCos > suspensionGradientCosine()) {
        float along = originalDot(forward, t.gravity);
        drive = float(double(float(double(0.f - along) * 37.5 * in.mass)) * t.slopeAid *
                          t.driveWeight / t.driveDenominator +
                      drive);
    }
    force = track::add(force, track::scale(forward, drive));
    return {force, forward, absolute};
}
} // namespace reriff::drive
