#pragma once
#include "track_math.hpp"
#include <array>

// Original elPhysBody native force boundary. Arithmetic follows the x86 SSE
// instructions, with float stores and no fused multiply-add. This is not a solver.
namespace reriff::drive {
struct BodyForces {
    using Vec3 = track::Vec3;
    int flags = 0, activation = 1;
    float deactivationTime = 0, inverseMass = 0;
    Vec3 linearFactor{1, 1, 1}, angularFactor{1, 1, 1};
    std::array<Vec3, 3> inverseInertia{}; // World-space rows, original body +0x100.
    Vec3 force{}, torque{};

    static Vec3 multiply(Vec3 a, Vec3 b) {
        return {a.x * b.x, a.y * b.y, a.z * b.z};
    }
    void wake() {
        if ((flags & 3) == 0) {
            if (activation != 4 && activation != 5)
                activation = 1;
            deactivationTime = 0;
        }
    }
    // Original vtable slot +32, RVA 0x3B660. Position is relative to the body.
    void applyForce(Vec3 value, Vec3 position) {
        wake();
        Vec3 scaled = multiply(linearFactor, value);
        force = track::add(scaled, force);
        torque = track::add(torque, multiply(track::cross(position, scaled), angularFactor));
    }
    // Original vtable slot +36, RVA 0x3BB50. This call does not wake the body.
    void applyTorque(Vec3 value) {
        torque = track::add(torque, multiply(angularFactor, value));
    }
    Vec3 linearAcceleration(Vec3 impulse) const {
        return track::scale(multiply(linearFactor, impulse), inverseMass);
    }
    Vec3 angularAcceleration(Vec3 impulse) const {
        Vec3 result{};
        for (size_t i = 0; i < 3; ++i) {
            auto row = inverseInertia[i];
            result[i] =
                ((row.y * impulse.y + row.x * impulse.x) + row.z * impulse.z) * angularFactor[i];
        }
        return result;
    }
};
} // namespace reriff::drive
