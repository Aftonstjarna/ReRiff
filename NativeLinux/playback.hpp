#pragma once
#include "track_math.hpp"
#include <cstdint>
#include <vector>

namespace reriff {
struct SongPath {
    std::vector<track::Vec3> positions, ups;
    std::vector<float> distances;
    std::vector<uint32_t> visible;
    uint32_t start = 0, finish = 0, staticIndices = 0, carIndices = 0;
    float beatSpan = 0, paceOffset = 0, lift = 0;
    // gaTrackEntityManager.getSplineTForSongTime and
    // gaNodeManager.getNodeIndexAndT. Distances are node-link chord lengths,
    // not a dense spline arc-length table. Bounds avoid original linked-list
    // wraparound and let the inspection vehicle stop at the finish.
    float parameter(float fileSeconds) const {
        float length = distances[finish] - distances[start];
        float elapsed = fileSeconds + paceOffset;
        float distance = std::clamp(distances[start] + (elapsed / beatSpan) * length,
                                    distances[start], distances[finish]);
        auto hi = std::lower_bound(distances.begin() + start + 1, distances.begin() + finish + 1,
                                   distance);
        size_t i = static_cast<size_t>(hi - distances.begin()) - 1;
        float fraction = static_cast<float>(double(float(distance - distances[i])) /
                                            (double(distances[i + 1]) - distances[i]));
        return float(i) + std::clamp(fraction, 0.f, 1.f);
    }
    track::Vec3 position(float t) const {
        return track::evaluate(positions.data(), positions.size(), t);
    }
    track::Frame frame(float t) const {
        return track::orientation(track::evaluate(positions.data(), positions.size(), t, true),
                                  track::normalize(track::evaluate(ups.data(), ups.size(), t)));
    }
};
} // namespace reriff
