#pragma once
#include "driving.hpp"
#include <array>

// Direct ports from this installation's Game.exe. See PHYSICS-RECOVERY.md for
// MethodDef/RVA evidence and the unchanged-executable comparison harness.
// These routines consume real caller state; they do not integrate a substitute body.
namespace reriff::drive {
using track::Vec3;
inline float originalDot(Vec3 a, Vec3 b) {
    return float(double(a.y) * b.y + double(a.x) * b.x + double(a.z) * b.z);
}
inline float originalLength(Vec3 v) {
    return float(
        std::sqrt(double(float(double(v.x) * v.x + double(v.y) * v.y + double(v.z) * v.z))));
}
inline float deadZone(float value, float low, float high) {
    bool negative = value < 0;
    if (negative)
        value = -value;
    float result = value <= low    ? 0
                   : value >= high ? 1
                                   : float((double(value) - low) / (double(high) - low));
    return negative ? -result : result;
}
inline void moveTowards(float &value, float target, float step) {
    if (value > target) {
        value = float(double(value) - step);
        if (value <= target)
            value = target;
    } else {
        value = float(double(value) + step);
        if (value >= target)
            value = target;
    }
}
struct Spring {
    float stiffness = 0, friction = 0, velocity = 0, position = 0;
    float tick(float target, float dt) {
        float acceleration =
            float((double(target) - position) * stiffness - double(friction) * velocity);
        velocity = float(double(acceleration) * dt + velocity);
        position = float(double(velocity) * dt + position);
        return position;
    }
};
// Original gaSpring table, including its linear extrapolation at both ends.
inline float matchingFriction(float k) {
    constexpr std::array<float, 30> friction{6,     8,     10,    11.5f, 12.5f, 14,    15,    15.5f,
                                             16.5f, 17.5f, 18.5f, 19,    19.5f, 20.5f, 21,    21.5f,
                                             22.5f, 23,    23.5f, 23.5f, 24.5f, 25,    25.5f, 26,
                                             26.5f, 27,    27.5f, 28,    28,    28.5f};
    if (k < 10)
        return float(double(float((double(k) - 10) / 10)) * (friction[1] - double(friction[0])) +
                     friction[0]);
    for (size_t i = 0; i + 1 < friction.size(); ++i) {
        float low = float((i + 1) * 10), high = low + 10;
        if (k < high)
            return float(double(float((double(k) - low) / (double(high) - low))) *
                             (double(friction[i + 1]) - friction[i]) +
                         friction[i]);
    }
    float extrapolated = float(double(friction.back()) * 2 - friction[15]);
    return float(double(float((double(k) - 300) / (300. - 160))) *
                     (double(extrapolated) - friction.back()) +
                 friction.back());
}
struct Pedals {
    float accelerator, brake;
};
inline Pedals autoAccelBrake(float target, float speed, bool raceActive, bool onRamp) {
    float delta = float(double(target) - speed);
    float range = delta > 0 ? 1.34112f : 4.4704f;
    float feed = (speed / 44.704f) * .01f;
    float pedal = float(double(float(double(delta) / range)) + feed);
    if (raceActive)
        pedal = std::clamp(pedal, -1.f, 1.f);
    return {std::max(0.f, pedal), onRamp ? 0.f : std::max(0.f, -pedal)};
}
// tickInput changes the player's drive impulse offset in manual speed mode.
// Its accelerator/brake outputs still pass through calcAutoAccelBrake.
inline float manualSpeedOffset(bool automatic, float accelerator, float brake, float paceSpeed) {
    if (automatic)
        return 0;
    accelerator = std::clamp(accelerator, 0.f, 1.f);
    brake = std::clamp(brake, 0.f, 1.f);
    float offset = float(double(accelerator) * 24 - double(brake) * 8 -
                         (1. - accelerator) * .1875 * paceSpeed);
    return double(offset) + paceSpeed < 0 ? -paceSpeed : offset;
}
struct PaceState {
    Spring spring;
    float targetSpeed = 0;
};
struct PaceInput {
    float dt, paceSpeed, speed, distanceToMarker, desiredOffset, vehicleOffset;
    float topSpeed;
    Vec3 velocity, gravityDirection;
    bool markerActive, speedOverride, onRamp;
};
struct PaceResult {
    Vec3 velocity;
    bool applyVelocity;
};
inline PaceResult targetSpeed(PaceState &state, const PaceInput &in) {
    if (!in.markerActive)
        return {in.velocity, false};
    float delta = float((double(in.paceSpeed) - in.speed) * 0.10000000149011612);
    float offset = in.vehicleOffset * .5f;
    float error = float(double(delta) + in.distanceToMarker - in.desiredOffset - offset - 1.);
    if (state.spring.position == 0)
        state.spring.position = error;
    float filtered = state.spring.tick(error, in.dt);
    float fraction = float(double(filtered) / (filtered > 0 ? 100 : 10));
    fraction = deadZone(std::clamp(fraction, -1.f, 1.f), .001f, 1);
    float range = fraction > 0 ? in.topSpeed : 8.9408f;
    state.targetSpeed =
        std::clamp(float(double(range) * fraction + in.paceSpeed), 0.f, in.topSpeed);
    Vec3 horizontal =
        track::sub(in.velocity, track::scale(in.gravityDirection,
                                             originalDot(in.gravityDirection, in.velocity)));
    float horizontalSpeed = originalLength(horizontal);
    if (in.speedOverride)
        state.targetSpeed = horizontalSpeed;
    if (error != 0 && !in.onRamp && horizontalSpeed > state.targetSpeed && horizontalSpeed > .001f)
        return {track::scale(in.velocity, float(double(state.targetSpeed) / horizontalSpeed)),
                true};
    return {in.velocity, false};
}
inline bool stickToTrack(uint32_t nodeFlags, bool onRamp, bool releaseForTrackAngle) {
    bool forceStick = (nodeFlags & 0x8000) != 0;
    if (!forceStick && onRamp)
        return false;
    if (nodeFlags & 0x20000)
        return false;
    return forceStick || !releaseForTrackAngle;
}
inline float grip(float angleDegrees, float slipCone) {
    float angle = angleDegrees > 90 ? float(180. - angleDegrees) : angleDegrees;
    if (slipCone == 0)
        return 1;
    float ratio = std::min(1.f, float(double(angle) / slipCone));
    double remaining = 1. - ratio;
    return float(1. - remaining * remaining);
}
inline float laneCount(bool hasBothNodes, float left0, float right0, float left1, float right1,
                       float t) {
    if (!hasBothNodes)
        return 3;
    float left = float((double(left1) - left0) * t + left0),
          right = float((double(right1) - right0) * t + right0);
    return float(double(float(3. - (1. - left))) - (1. - right));
}
struct FrictionParams {
    float grip, slipCone, dirt, sand, grass, tarmac, ice, wet;
    float drift2, drift3, braking, liftOff, driftSlow, brakeSlow, sandSlow;
};
inline float lateralFriction(const FrictionParams &p, int surface, float throttle, float steering,
                             float drift, float brake, float lanes) {
    float multiplier = surface == 1    ? p.dirt
                       : surface == 9  ? p.sand
                       : surface == 7  ? p.wet
                       : surface == 6  ? p.ice
                       : surface == 13 ? p.grass
                                       : p.tarmac;
    float base = float(double(multiplier) * p.grip);
    float lift = std::min(1.f, float(1. - throttle));
    float factor = float(double(float(double(std::abs(steering)) * float((1. - drift) * lift))) *
                             (double(p.liftOff) - 1.) +
                         1.);
    float width = std::clamp(lanes - 2.f, 0.f, 1.f);
    float driftGrip = float((double(p.drift3) - p.drift2) * width + p.drift2);
    factor = float((double(driftGrip) - factor) * drift + factor);
    factor = float((double(p.braking) - factor) * std::max(0.f, brake) + factor);
    return float(double(factor) * base);
}
inline float forwardFriction(const FrictionParams &p, bool parked, float speed, float drift,
                             float brake, int sandContacts, Vec3 velocity) {
    if (parked) {
        float xy = float(std::sqrt(double(
                       float(double(velocity.x) * velocity.x + double(velocity.y) * velocity.y)))) /
                   10.f;
        return float((double(p.brakeSlow) - 100) * std::clamp(xy, 0.f, 1.f) + 100.);
    }
    float slow = brake > 0 ? float(double(p.brakeSlow) * brake) : 0;
    slow = float(double(p.driftSlow) * drift + slow);
    float ground = float(double(float(sandContacts)) * p.sandSlow + 0.);
    return float(double(std::abs(speed)) * ground * .25 + slow);
}
inline float steeringAngle(const Params &p, float exitAngleDegrees, float speed, float drift,
                           float bias, float input, float selfSteer, Vec3 velocity, Vec3 forward) {
    float rawInput = input;
    input = std::clamp(input, -1.f, 1.f);
    float lengthSquared = float(double(velocity.x) * velocity.x + double(velocity.y) * velocity.y +
                                double(velocity.z) * velocity.z);
    Vec3 heading = lengthSquared < 1 ? forward : track::normalize(velocity);
    heading = track::normalize(heading);
    float relative = float(double(float(std::atan2(double(heading.x), double(heading.z)))) -
                           float(std::atan2(double(forward.x), double(forward.z))));
    constexpr float tau = 6.2831854820251465f;
    relative = relative >= 0
                   ? float(std::fmod(double(relative), double(tau)))
                   : float(double(tau) - float(std::fmod(double(-relative), double(tau))));
    if (double(relative) > 3.1415927410125732)
        relative = float(double(relative) - 6.2831854820251465);
    float limit = steerLimit(p, std::abs(speed), drift, rawInput, bias);
    float wide = std::max(limit, float(double(exitAngleDegrees) * .017453293005625408));
    float left = std::clamp(relative, -wide, -limit), right = std::clamp(relative, limit, wide);
    selfSteer = std::clamp(selfSteer, left, right);
    return input < 0 ? float((double(left) - selfSteer) * (-input) + selfSteer)
                     : float((double(right) - selfSteer) * input + selfSteer);
}
inline Vec3 originalCross(Vec3 a, Vec3 b) {
    return {float(double(a.y) * b.z - double(a.z) * b.y),
            float(double(a.z) * b.x - double(a.x) * b.z),
            float(double(b.y) * a.x - double(b.x) * a.y)};
}
struct InputTuning {
    float steerOnset, driftDelay, driftOnset, driftRelease, driftHold;
};
struct VehicleInput {
    float accelerator = 0, brake = 0, steer = 0, auxiliary = 0;
    bool driftPressed = false, disabled = false;
};
struct InputState {
    float steering = 0, auxiliary = 0, pedal = 0, buttonBlend = 0, latch = 0, driftTimer = 0,
          heldBlend = 0, latchTimer = 0, drift = 0, peakDrift = 0;
    bool exited = false;
    float lateralAcceleration = 0, trackAngle = 0;
};
// gaVehiclePhys.phys_tickInputs: state includes the drift timer and lateral
// acceleration supplied by the rest of the original vehicle update.
inline void tickInputs(InputState &s, VehicleInput &in, const InputTuning &p, float dt,
                       Vec3 tangent0, Vec3 tangent1, Vec3 tangent2) {
    if (in.disabled)
        in.accelerator = in.brake = in.steer = 0;
    if ((double(in.steer) - s.steering) * s.steering < 0)
        s.steering = in.steer;
    float onset = std::max(p.steerOnset, .0001f);
    moveTowards(s.steering, in.steer, float(double(float(1. / onset)) * dt));
    moveTowards(s.auxiliary, in.auxiliary, dt * 10.f);
    float target = std::clamp(float(double(in.accelerator) - double(in.brake) * 2.), -2.f, 2.f);
    moveTowards(s.pedal, target, float(double(dt) * 6.666666507720947));
    s.trackAngle = float(std::asin(
        double(std::clamp(originalDot(originalCross(tangent1, tangent0), tangent2), -1.f, 1.f))));
    float button = in.driftPressed ? 1.f : 0.f;
    target = button;
    float rate = float(
        1. / std::max(s.buttonBlend < button ? p.driftOnset : std::abs(p.driftRelease), .001f));
    moveTowards(s.buttonBlend, button, float(double(dt) * rate));
    float timerBlend =
        std::clamp(float((double(std::abs(s.driftTimer)) - p.driftDelay) / p.driftOnset), 0.f, 1.f);
    s.buttonBlend = std::max(s.buttonBlend, timerBlend);
    if (p.driftRelease <= 0) {
        if (s.latch == 0 && (in.driftPressed || s.buttonBlend > p.driftHold)) {
            s.latch = in.steer > 0 ? -1.f : in.steer < 0 ? 1.f : 0.f;
            s.latchTimer = 0;
            s.exited = false;
        }
        if (double(s.latch) * in.steer > 0) {
            if (s.latch != 0)
                s.exited = true;
            s.latch = 0;
        }
        if (double(s.latch) * in.steer < 0)
            s.latchTimer = 0;
        if (s.latch != 0 && double(s.latch) * s.lateralAcceleration > -2.) {
            s.latchTimer = float(double(s.latchTimer) + dt);
            if (s.latchTimer > 0)
                s.latch = 0;
        } else
            s.latchTimer = 0;
        if (s.latch != 0)
            target = 1;
        rate = float(
            1. / std::max(s.heldBlend < target ? p.driftOnset : std::abs(p.driftRelease), .001f));
        moveTowards(s.heldBlend, target, float(double(dt) * rate));
    }
    s.drift = std::max(s.buttonBlend, s.heldBlend);
    moveTowards(s.peakDrift, 0, dt);
    s.peakDrift = std::max(s.peakDrift, s.drift);
}
struct DriveImpulse {
    float driveForce, cachedSpeed;
    Vec3 direction, impulse;
    bool applied;
};
// gaVehiclePhys.phys_tickDrive. The returned impulse is applied by the original
// rigid-body boundary; this function does not substitute a velocity integrator.
inline DriveImpulse tickDrive(float mass, float gearTorque, float speed, float playerOffset,
                              float playerBoost, int contactCount, bool fullForward, Vec3 forward,
                              Vec3 normal, float dt) {
    Vec3 direction = fullForward
                         ? forward
                         : track::sub(forward, track::scale(normal, originalDot(normal, forward)));
    DriveImpulse result{float(double(mass) * gearTorque), speed, direction, {0, 0, 0}, false};
    float extra = float(double(playerOffset) + playerBoost);
    if (extra == 0 || contactCount <= 0)
        return result;
    float along = originalDot(forward, direction), amount = float(double(dt) * extra);
    if (extra < 0 && float(double(amount) * along + speed) < 0 && std::abs(along) > .001f)
        amount = std::min(0.f, float(-double(speed) / along));
    result.impulse = track::scale(track::scale(direction, amount), mass);
    result.applied = true;
    return result;
}
struct SkidState {
    float smoke = 0, burnout = 0, braking = 0;
};
// gaVehiclePhys.phys_calcSkid. The gear value is the original gearbox field
// at +28; zero suppresses burnout even when the pedal would otherwise produce it.
inline SkidState skidState(float lateralTotal, float speed, float pedal, float drift, int contacts,
                           int gear) {
    SkidState out;
    out.smoke = std::clamp(float((double(lateralTotal) - 1.) / (12.5 - 1.)), 0.f, 1.f);
    if (contacts > 0) {
        bool forward = speed >= -.25f;
        if (forward ? pedal > .9f : pedal < -.9f) {
            float ratio = std::clamp(speed / (forward ? 10.f : -10.f), 0.f, 1.f);
            out.burnout = float(1. - float(double(ratio) * ratio));
        } else if (forward ? pedal < -.9f : pedal > .9f) {
            out.braking = 1;
        }
        if (drift > .5f && std::abs(speed) > 13.411199569702148)
            out.braking = 1;
    }
    if (gear == 0)
        out.burnout = 0;
    return out;
}
struct SkidForce {
    Vec3 force{0, 0, 0};
    bool applied = false;
};
// gaVehiclePhys.phys_applySkidSpeedup. Return the original center force,
// including a zero-force call when at least two wheels contact the track.
inline SkidForce skidSpeedup(float mass, float coefficient, float lateralSpeed, int contacts,
                             Vec3 right, Vec3 response, Vec3 velocity, Vec3 forward,
                             Vec3 boostDirection) {
    if (contacts < 2)
        return {};
    float opposing = originalDot(response, right);
    opposing = double(lateralSpeed) * opposing > 0 ? 0.f : std::abs(opposing);
    float acceleration = float(double(coefficient) * opposing);
    float squared = float(double(velocity.x) * velocity.x + double(velocity.y) * velocity.y +
                          double(velocity.z) * velocity.z);
    Vec3 heading = squared < 1 ? forward : track::normalize(velocity);
    float sideways = std::abs(originalDot(right, heading));
    float threshold = float(std::sin(.1745329350233078));
    float blend = std::clamp(float((double(sideways) - threshold) / threshold), 0.f, 1.f);
    acceleration = std::min(float(double(blend) * acceleration), 60.f);
    return {track::scale(boostDirection, float(double(mass) * acceleration)), true};
}
struct YawState {
    float responseLatch = 0, steeringBias = 0;
};
struct YawInput {
    float dt, travel, damping0, damping1, lateralAcceleration, lateralSpeed, drift;
    int contacts;
    float angleLimit2, angleLimit3, lanes;
    Vec3 up, right, trackRight, angularVelocity, tireTorque;
};
struct YawResult {
    Vec3 angularVelocity, torque{0, 0, 0};
    bool applyAngularVelocity = false, applyTorque = false;
};
// gaVehiclePhys.phys_applyYawDamping. Preserve the body setter boundary: even
// an unchanged angular velocity must wake the body when applyAngularVelocity is true.
inline YawResult yawDamping(YawState &state, const YawInput &in) {
    YawResult out{in.angularVelocity};
    float blend = std::min(std::max(in.damping0, in.damping1), 1.f);
    if (blend != 0) {
        float yawTorque = originalDot(in.tireTorque, in.up);
        out.torque = track::scale(in.up, float(double(0.f - yawTorque) * blend));
        out.applyTorque = true;
    }
    state.steeringBias = 0;
    if (double(in.lateralAcceleration) * state.responseLatch < 0 && in.drift < .5f)
        state.responseLatch = 0;
    moveTowards(state.responseLatch, 0, in.dt);
    if (in.contacts > 0 && double(in.lateralSpeed) * in.lateralAcceleration < -1. &&
        std::abs(in.lateralSpeed) > 4)
        state.responseLatch = (in.lateralAcceleration < 0 ? -1.f : 1.f) * .3f;
    if (in.travel < .001f)
        return out;
    float damping = float(double(in.damping0) * 7. + 1.);
    float yaw = originalDot(in.angularVelocity, in.up);
    if (std::abs(yaw) > 6)
        damping = 2;
    if (in.angleLimit2 != 0 && in.angleLimit3 != 0) {
        float laneBlend = std::clamp(in.lanes - 2.f, 0.f, 1.f);
        float limit = float(
            double(float((double(in.angleLimit3) - in.angleLimit2) * laneBlend + in.angleLimit2)) *
            .017453293005625408);
        float side = originalDot(in.right, in.trackRight);
        float angle = 0.f - float(std::asin(double(std::clamp(side, -1.f, 1.f))));
        angle = std::abs(float(double(yaw) * .10000000149011612 + angle));
        float excess = std::clamp(float((double(angle) - limit) / .2617993950843811), 0.f, 1.f);
        float sign = side < 0 ? -1.f : side > 0 ? 1.f : 0.f;
        state.steeringBias = float(double(sign) * excess);
        damping = float(double(std::abs(state.steeringBias)) * 2. + damping);
    }
    Vec3 perpendicular =
        track::sub(in.angularVelocity, track::scale(in.up, originalDot(in.up, in.angularVelocity)));
    out.angularVelocity =
        track::add(in.angularVelocity, track::scale(track::sub(perpendicular, in.angularVelocity),
                                                    float(double(in.dt) * damping)));
    out.applyAngularVelocity = true;
    return out;
}
} // namespace reriff::drive
