#pragma once
#include "vehicle_behavior.hpp"

namespace reriff::drive {
// Suspension portion of phys_applySuspAndTyreForces_HI. Wheel queries and the
// body's local-to-world transform supply these inputs; neither is approximated here.
struct SuspensionInput {
    float dt, mass, stiffness, damping, smoothing, compression, wheelExtent;
    bool stick;
    Vec3 bodyUp, roadUp, contactNormal, worldPoint, velocity, angularVelocity;
};
struct SuspensionResult {
    float travel, positiveLoad, smoothedLoad;
    Vec3 normal, pointVelocity, force;
};
inline float suspensionGradientCosine() {
    return float(std::cos(.6981317400932312));
}
inline SuspensionResult suspension(const SuspensionInput &in, float previousLoad) {
    float travel = in.compression;
    if (!in.stick)
        travel = std::max(travel, 0.f);
    else if (double(travel) <= .0010000000474974513 - double(in.wheelExtent) * .5)
        travel = 0;
    Vec3 normal = in.contactNormal;
    if (originalDot(normal, in.roadUp) < suspensionGradientCosine())
        normal = in.roadUp;
    Vec3 velocity = track::add(in.velocity, originalCross(in.angularVelocity, in.worldPoint));
    float load = float(double(in.stiffness) * travel * originalDot(in.bodyUp, normal));
    float smooth =
        in.smoothing != 0
            ? float(double(float(double(in.smoothing) * in.dt)) * (double(load) - previousLoad) +
                    previousLoad)
            : load;
    SuspensionResult result{travel,   std::max(load, 0.f), std::max(smooth, 0.f), normal, velocity,
                            {0, 0, 0}};
    load = float(double(load) - double(in.damping) * originalDot(velocity, normal));
    if (!in.stick)
        load = std::max(load, 0.f);
    if (travel != 0)
        result.force = track::scale(normal, float(double(in.mass) * load));
    return result;
}
} // namespace reriff::drive
