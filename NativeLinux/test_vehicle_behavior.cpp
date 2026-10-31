#include "native_common.hpp"
#include "vehicle_behavior.hpp"
#include "vehicle_body.hpp"
#include "vehicle_suspension.hpp"
#include "vehicle_tires.hpp"
#include <sstream>

using namespace reriff::drive;
int main(int argc, char **argv) {
    try {
        require(argc == 2 || argc == 3,
                "usage: test-vehicle-behavior ORIGINAL_CAPTURE_DIR [REPORT.json]");
        rr::fs::path directory = argv[1];
        auto provenance = rr::loadJson(directory / "behavior-provenance.json");
        auto captureHash = rr::sha(read((directory / "original-behavior.tsv").string()));
        require(provenance.at("schema") == 1 && provenance.at("method_bodies_changed") == false &&
                    provenance.at("game_sha256") ==
                        "2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2" &&
                    provenance.at("capture_sha256") == captureHash,
                "Original capture provenance/hash mismatch");
        std::ifstream input(directory / "original-behavior.tsv");
        require(bool(input), "Original behavior capture is required; run reriff-reference capture");
        rr::Json groups = rr::Json::object();
        size_t lines = 0, comparisons = 0;
        std::string line;
        while (std::getline(input, line)) {
            ++lines;
            if (line.empty() || line[0] == '#')
                continue;
            std::istringstream stream(line);
            std::string tag;
            stream >> tag;
            std::vector<float> v;
            float value;
            while (stream >> value)
                v.push_back(value);
            require(stream.eof(), "Malformed capture at line " + std::to_string(lines));
            for (float f : v)
                require(std::isfinite(f), "Nonfinite capture input");
            auto size = [&](size_t expected) {
                require(v.size() == expected, "Wrong " + tag + " field count");
            };
            auto compare = [&](float actual, float expected) {
                ++comparisons;
                double error = std::abs(double(actual) - expected);
                double tolerance = tag == "steer-angle" ? 1e-6 : 0;
                if (!std::isfinite(actual) || error > tolerance) {
                    std::ostringstream msg;
                    msg << std::setprecision(10) << tag << " line " << lines << ": native "
                        << actual << ", original " << expected << ", error " << error;
                    throw std::runtime_error(msg.str());
                }
                auto &group = groups[tag];
                if (group.is_null())
                    group = {{"cases", 0}, {"values", 0}, {"max_absolute_error", 0.}};
                group["values"] = group["values"].get<int>() + 1;
                group["max_absolute_error"] =
                    std::max(group["max_absolute_error"].get<double>(), error);
            };
            auto params = [&]() {
                return FrictionParams{v[0], v[1], v[2],  v[3],  v[4],  v[5],  v[6], v[7],
                                      v[8], v[9], v[10], v[11], v[12], v[13], v[14]};
            };
            if (tag == "friction-table") {
                size(2);
                compare(matchingFriction(v[0]), v[1]);
            } else if (tag == "spring") {
                size(8);
                Spring s{v[0], v[1], v[4], v[5]};
                s.tick(v[3], v[2]);
                compare(s.velocity, v[6]);
                compare(s.position, v[7]);
            } else if (tag == "auto") {
                size(6);
                auto p = autoAccelBrake(v[0], v[1], v[2] != 0, v[3] != 0);
                compare(p.accelerator, v[4]);
                compare(p.brake, v[5]);
            } else if (tag == "manual") {
                size(9);
                compare(manualSpeedOffset(v[0] != 0, v[1], v[2], v[3]), v[6]);
                auto p = autoAccelBrake(v[4], v[5], true, false);
                compare(p.accelerator, v[7]);
                compare(p.brake, v[8]);
            } else if (tag == "pace") {
                size(27);
                PaceState state{{200, matchingFriction(200), v[16], v[17]}, v[18]};
                PaceInput in{v[1],
                             v[2],
                             v[3],
                             v[4],
                             v[5],
                             v[6],
                             v[7],
                             {v[10], v[11], v[12]},
                             {v[13], v[14], v[15]},
                             v[0] != 0,
                             v[8] != 0,
                             v[9] != 0};
                auto result = targetSpeed(state, in);
                auto velocity = result.velocity;
                compare(state.spring.velocity, v[19]);
                compare(state.spring.position, v[20]);
                compare(state.targetSpeed, v[21]);
                compare(velocity.x, v[22]);
                compare(velocity.y, v[23]);
                compare(velocity.z, v[24]);
                compare(result.applyVelocity ? 1.f : 2.f, v[25]);
                compare(result.applyVelocity ? 0.f : 3.f, v[26]);
            } else if (tag == "adhesion") {
                size(4);
                compare(stickToTrack(uint32_t(v[0]), v[1] != 0, v[2] != 0) ? 1.f : 0.f, v[3]);
            } else if (tag == "grip") {
                size(3);
                compare(grip(v[1], v[0]), v[2]);
            } else if (tag == "lanes") {
                size(7);
                compare(laneCount(v[0] != 0, v[1], v[2], v[3], v[4], v[5]), v[6]);
            } else if (tag == "lateral") {
                size(22);
                compare(lateralFriction(params(), int(v[15]), v[16], v[17], v[18], v[19], v[20]),
                        v[21]);
            } else if (tag == "forward") {
                size(24);
                compare(forwardFriction(params(), v[15] != 0, v[16], v[17], v[18], int(v[19]),
                                        {v[20], v[21], v[22]}),
                        v[23]);
            } else if (tag == "steer-angle") {
                size(16);
                compare(steeringAngle({v[0], v[1], v[2]}, v[3], v[4], v[5], v[6], v[7], v[8],
                                      {v[9], v[10], v[11]}, {v[12], v[13], v[14]}),
                        v[15]);
            } else if (tag == "inputs") {
                size(50);
                InputState state{v[21], v[22], v[23], v[24],      v[25], v[26], v[27],
                                 v[28], v[29], v[30], v[31] != 0, v[32], v[33]};
                VehicleInput in{v[6], v[7], v[8], v[9], v[10] != 0, v[11] != 0};
                tickInputs(state, in, {v[0], v[1], v[2], v[3], v[4]}, v[5], {v[12], v[13], v[14]},
                           {v[15], v[16], v[17]}, {v[18], v[19], v[20]});
                std::array<float, 13> result{state.steering,
                                             state.auxiliary,
                                             state.pedal,
                                             state.buttonBlend,
                                             state.latch,
                                             state.driftTimer,
                                             state.heldBlend,
                                             state.latchTimer,
                                             state.drift,
                                             state.peakDrift,
                                             state.exited ? 1.f : 0.f,
                                             state.lateralAcceleration,
                                             state.trackAngle};
                for (size_t i = 0; i < result.size(); ++i)
                    compare(result[i], v[34 + i]);
                compare(in.accelerator, v[47]);
                compare(in.brake, v[48]);
                compare(in.steer, v[49]);
            } else if (tag == "tires") {
                size(96);
                Vec3 right{v[18], v[19], v[20]}, up{v[21], v[22], v[23]},
                    forward{v[24], v[25], v[26]};
                std::array<float, 4> weights{};
                float denominator = 0;
                for (size_t i = 0; i < 4; ++i) {
                    size_t p = 36 + i * 9;
                    weights[i] = wheelDriveWeight(int(i), v[6], v[p], v[p + 1], v[5] != 0, up,
                                                  {v[p + 3], v[p + 4], v[p + 5]});
                    denominator = float(double(weights[i]) + denominator);
                }
                denominator = std::max(denominator, .5f);
                BodyForces body;
                float rawTotal = 0, smoothTotal = 0;
                for (size_t i = 0; i < 4; ++i) {
                    size_t p = 36 + i * 9;
                    SuspensionInput in{v[0],
                                       v[1],
                                       v[2],
                                       v[3],
                                       v[4],
                                       v[p],
                                       v[p + 1],
                                       v[5] != 0,
                                       up,
                                       up,
                                       {v[p + 3], v[p + 4], v[p + 5]},
                                       {v[p + 6], v[p + 7], v[p + 8]},
                                       {v[30], v[31], v[32]},
                                       {v[33], v[34], v[35]}};
                    auto s = suspension(in, v[p + 2]);
                    TireInput t{v[10], v[9],       v[11],      v[12],
                                v[13], int(v[17]), v[14],      v[15],
                                v[16], v[7],       weights[i], denominator,
                                v[8],  right,      forward,    {v[27], v[28], v[29]}};
                    auto result = tireForces(in, s, t);
                    body.applyForce(result.force, in.worldPoint);
                    compare(s.smoothedLoad, v[72 + i * 4]);
                    for (size_t j = 0; j < 3; ++j)
                        compare(result.force[j], v[73 + i * 4 + j]);
                    rawTotal = float(double(rawTotal) + s.positiveLoad);
                    smoothTotal = float(double(s.smoothedLoad) + smoothTotal);
                }
                for (size_t i = 0; i < 3; ++i) {
                    compare(body.force[i], v[88 + i]);
                    compare(body.torque[i], v[91 + i]);
                }
                compare(rawTotal, v[94]);
                compare(smoothTotal, v[95]);
            } else if (tag == "suspension") {
                size(39);
                SuspensionInput in{v[0],
                                   v[1],
                                   v[2],
                                   v[3],
                                   v[4],
                                   v[5],
                                   v[6],
                                   v[7] != 0,
                                   {v[10], v[11], v[12]},
                                   {v[13], v[14], v[15]},
                                   {v[16], v[17], v[18]},
                                   {v[19], v[20], v[21]},
                                   {v[22], v[23], v[24]},
                                   {v[25], v[26], v[27]}};
                auto out = suspension(in, v[8]);
                compare(suspensionGradientCosine(), v[9]);
                compare(out.positiveLoad, v[28]);
                compare(out.smoothedLoad, v[29]);
                BodyForces body;
                body.applyForce(out.force, in.worldPoint);
                for (size_t i = 0; i < 3; ++i) {
                    compare(out.force[i], v[30 + i]);
                    compare(body.force[i], v[33 + i]);
                    compare(body.torque[i], v[36 + i]);
                }
            } else if (tag == "body-force") {
                size(45);
                BodyForces body;
                body.flags = int(v[1]);
                body.activation = int(v[2]);
                body.deactivationTime = 3;
                body.inverseMass = v[3];
                body.linearFactor = {v[4], v[5], v[6]};
                body.angularFactor = {v[7], v[8], v[9]};
                for (size_t i = 0; i < 3; ++i)
                    body.inverseInertia[i] = {v[10 + i * 3], v[11 + i * 3], v[12 + i * 3]};
                body.force = {v[19], v[20], v[21]};
                body.torque = {v[22], v[23], v[24]};
                Vec3 impulse{v[25], v[26], v[27]}, point{v[28], v[29], v[30]};
                if (v[0] == 0)
                    body.applyForce(impulse, point);
                else if (v[0] == 1)
                    body.applyTorque(impulse);
                auto linear = body.linearAcceleration(impulse),
                     angular = body.angularAcceleration(impulse);
                for (size_t i = 0; i < 3; ++i) {
                    compare(body.force[i], v[31 + i]);
                    compare(body.torque[i], v[34 + i]);
                    compare(linear[i], v[37 + i]);
                    compare(angular[i], v[40 + i]);
                }
                compare(float(body.activation), v[43]);
                compare(body.deactivationTime, v[44]);
            } else if (tag == "yaw") {
                size(38);
                YawState state{v[4], 9};
                YawInput in{v[0],
                            v[1],
                            v[2],
                            v[3],
                            v[5],
                            v[6],
                            v[7],
                            int(v[8]),
                            v[9],
                            v[10],
                            v[11],
                            {v[12], v[13], v[14]},
                            {v[15], v[16], v[17]},
                            {v[18], v[19], v[20]},
                            {v[21], v[22], v[23]},
                            {v[24], v[25], v[26]}};
                auto out = yawDamping(state, in);
                compare(state.responseLatch, v[27]);
                compare(state.steeringBias, v[28]);
                for (size_t i = 0; i < 3; ++i) {
                    compare(out.angularVelocity[i], v[29 + i]);
                    compare(out.torque[i], v[32 + i]);
                }
                compare(out.applyTorque ? 1.f : 0.f, v[35]);
                compare(out.applyAngularVelocity ? 1.f : 2.f, v[36]);
                compare(out.applyAngularVelocity ? 0.f : 3.f, v[37]);
            } else if (tag == "skid") {
                size(9);
                auto out = skidState(v[0], v[1], v[2], v[3], int(v[4]), int(v[5]));
                compare(out.smoke, v[6]);
                compare(out.burnout, v[7]);
                compare(out.braking, v[8]);
            } else if (tag == "skid-force") {
                size(23);
                auto out = skidSpeedup(v[0], v[1], v[2], int(v[3]), {v[4], v[5], v[6]},
                                       {v[7], v[8], v[9]}, {v[10], v[11], v[12]},
                                       {v[13], v[14], v[15]}, {v[16], v[17], v[18]});
                for (size_t i = 0; i < 3; ++i)
                    compare(out.force[i], v[19 + i]);
                compare(out.applied ? 1.f : 0.f, v[22]);
            } else if (tag == "drive-impulse") {
                size(23);
                auto out = tickDrive(v[0], v[1], v[2], v[3], v[4], int(v[5]), v[6] != 0,
                                     {v[8], v[9], v[10]}, {v[11], v[12], v[13]}, v[7]);
                compare(out.driveForce, v[14]);
                compare(out.cachedSpeed, v[22]);
                for (size_t i = 0; i < 3; i++) {
                    compare(out.direction[i], v[15 + i]);
                    compare(out.impulse[i], v[18 + i]);
                }
                compare(out.applied ? 1.f : 0.f, v[21]);
            } else
                throw std::runtime_error("Unknown capture group " + tag);
            groups[tag]["cases"] = groups[tag]["cases"].get<int>() + 1;
        }
        const std::map<std::string, int> expected{
            {"friction-table", 13}, {"spring", 540},     {"auto", 112},        {"manual", 120},
            {"pace", 576},          {"adhesion", 16},    {"grip", 60},         {"lanes", 72},
            {"lateral", 3024},      {"forward", 324},    {"steer-angle", 432}, {"inputs", 480},
            {"drive-impulse", 144}, {"skid", 8190},      {"skid-force", 1296}, {"yaw", 2880},
            {"body-force", 2304},   {"suspension", 576}, {"tires", 288}};
        for (const auto &[name, count] : expected)
            require(groups.contains(name) && groups[name]["cases"].get<int>() == count,
                    "Incomplete capture group " + name);
        rr::Json report = {{"passed", true},
                           {"comparisons", comparisons},
                           {"groups", groups},
                           {"capture_sha256", captureHash}};
        if (argc == 3)
            rr::save(argv[2], report);
        std::cout << report.dump(2) << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "vehicle behavior parity: " << e.what() << '\n';
        return 1;
    }
}
