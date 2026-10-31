#pragma once
#include "native_common.hpp"
#include <optional>

namespace rr {
struct Random {
    std::array<uint32_t, 624> state{};
    int index = 624;
    explicit Random(uint32_t seed) {
        state[0] = seed;
        for (uint32_t i = 1; i < 624; i++)
            state[i] = 1812433253u * (state[i - 1] ^ (state[i - 1] >> 30)) + i;
    }
    uint32_t integer() {
        if (index == 624) {
            for (int i = 0; i < 624; i++) {
                uint32_t y = (state[i] & 0x80000000u) | (state[(i + 1) % 624] & 0x7fffffffu);
                state[i] = state[(i + 397) % 624] ^ (y >> 1) ^ ((y & 1) ? 0x9908b0dfu : 0);
            }
            index = 0;
        }
        uint32_t y = state[index++];
        y ^= y >> 11;
        y ^= (y << 7) & 0x9d2c5680u;
        y ^= (y << 15) & 0xefc60000u;
        y ^= y >> 18;
        return y;
    }
    float uniform(float a = 0, float b = 1) {
        float u = F(double(F(integer())) * F(1. / 4294967296.));
        return F(double(F(double(u) * F(double(b) - a))) + a);
    }
    int between(int a, int b) {
        require(a <= b, "invalid random range");
        return a + integer() % uint32_t(b - a + 1);
    }
    float sign() {
        return integer() & 0x8000u ? -1.f : 1.f;
    }
    int choice(const std::vector<std::pair<int, float>> &entries) {
        require(!entries.empty(), "empty weighted choice");
        float total = 0;
        for (auto e : entries)
            total = F(double(total) + e.second);
        float v = uniform(0, total);
        for (size_t i = 0; i + 1 < entries.size(); i++) {
            v = F(double(v) - entries[i].second);
            if (v < 0)
                return entries[i].first;
        }
        return entries.back().first;
    }
};
inline uint32_t songSeed(const std::string &id) {
    uint32_t seed = 0x12345678;
    for (size_t i = 0; i < id.size(); i++)
        seed += uint32_t(int32_t(int8_t(id[i]))) << ((i / 2 + 1) & 31);
    return seed;
}
inline int rounded(double v) {
    return int(std::floor(F(v + .5)));
}
inline float tempoValue(const Json &value, float tempo) {
    if (value.is_number()) {
        return value.get<float>();
    }
    auto list = values(value);
    require(list.size() == 2, "expected tempo range");
    double t = F(std::clamp(double(F((double(tempo) - 100) / 80)), 0., 1.));
    return F((list[1].get<double>() - list[0].get<double>()) * t + list[0].get<double>());
}
inline float volume(const std::vector<float> &values, int start, int count, bool raw = false) {
    if (start < 0) {
        count += start;
        start = 0;
    }
    float squared = 0;
    int n = 0;
    for (int i = start; i < std::min(int(values.size()), start + std::max(0, count)); i++) {
        squared = F(double(squared) + F(double(values[i]) * values[i]));
        n++;
    }
    if (!n) {
        return 0;
    }
    float result = F(std::sqrt(double(F(double(squared) / n))));
    if (!raw)
        result = F(std::pow(double(result), double(.8f)));
    return std::clamp(result, 0.f, 1.f);
}
inline std::vector<int> detectDrops(const std::vector<float> &v) {
    std::vector<std::pair<int, float>> candidates;
    int last = -1;
    float strength = 0;
    auto rms = [](float a, float b) {
        return F(std::sqrt(double(F(double(F(double(a) * a)) + F(double(b) * b)))));
    };
    for (int i = 10; i < int(v.size()) - 13; i++) {
        float change = F(double(rms(v[i - 1], v[i])) - rms(v[i + 1], v[i + 2]));
        if (change > .5f) {
            if (last == i - 1) {
                if (change > strength)
                    candidates.back() = {i, change};
            } else
                candidates.emplace_back(i, change);
            last = i;
            strength = change;
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](auto a, auto b) { return a.second < b.second; });
    std::vector<int> result;
    while (!candidates.empty() && result.size() < 6) {
        int i = candidates.back().first;
        candidates.pop_back();
        result.push_back(i);
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                        [i](auto c) { return std::abs(c.first - i) <= 8; }),
                         candidates.end());
    }
    return result;
}
inline Bytes expand(const Bytes &data, size_t offset, uint32_t compressed, uint32_t size) {
    require(size >= 1 && size <= 16 * 1024 * 1024 && offset <= data.size() &&
                compressed <= data.size() - offset,
            "invalid SNG0 stream length");
    Bytes out(size + 1);
    z_stream z{};
    z.next_in = const_cast<Bytef *>(data.data() + offset);
    z.avail_in = compressed;
    z.next_out = out.data();
    z.avail_out = size + 1;
    require(inflateInit(&z) == Z_OK, "SNG0 inflate init failed");
    int status = inflate(&z, Z_FINISH);
    bool valid = status == Z_STREAM_END && z.total_in == compressed && z.total_out == size;
    inflateEnd(&z);
    require(valid, "invalid SNG0 compressed stream");
    out.resize(size);
    require(out.back() == 0 && std::find(out.begin(), out.end() - 1, 0) == out.end() - 1,
            "SNG0 JSON must end in one NUL");
    out.pop_back();
    return out;
}
struct SongInput {
    Json entry, profile, analysis;
    std::string hash;
};
inline void validateSong(const Json &p, const Json &a) {
    require(p.is_object() && p.contains("song") && p["song"].contains("id") &&
                p["song"]["id"].is_string(),
            "invalid song ID");
    auto id = p["song"]["id"].get<std::string>();
    require(!id.empty() && id.size() <= 64, "invalid song ID");
    require(a.contains("track") && a["track"].is_object(), "missing track");
    double tempo = number(a["track"], "tempo", 120);
    if (tempo == 0)
        tempo = 120;
    require(std::isfinite(tempo) && tempo >= 20 && tempo <= 400, "unsupported tempo");
    const auto &beats = a.at("beats");
    const auto &vols = a.at("beatVols");
    require(beats.is_array() && beats.size() >= 4 && beats.size() <= 50000 && vols.is_array() &&
                vols.size() == beats.size(),
            "invalid beat/loudness counts");
    double previous = -1;
    for (const auto &b : beats) {
        double v = b.get<double>();
        require(std::isfinite(v) && v >= 0 && v > previous, "beat times must increase");
        previous = v;
    }
    for (const auto &b : vols) {
        double v = b.get<double>();
        require(std::isfinite(v) && v >= 0 && v <= 1, "invalid normalized loudness");
    }
    const auto &sections = a.at("sections");
    require(sections.is_array() && !sections.empty() && sections.size() <= beats.size(),
            "invalid sections");
    int prev = -1;
    for (const auto &sec : sections) {
        for (const auto *k : {"beat", "start", "duration", "loudness"}) {
            double v = sec.at(k).get<double>();
            require(std::isfinite(v) && v >= 0, "invalid section field");
        }
        int b = rounded(sec.at("beat").get<double>());
        require(b > prev && b < int(beats.size()), "section beats out of range/order");
        prev = b;
    }
    require(sections[0]["beat"] == 0, "first section must start at zero");
}
inline std::pair<Json, Json> decodeSong(const Bytes &b) {
    require(b.size() >= 20 && b.size() <= 32 * 1024 * 1024 && u32(b, 0) == 0x534e4730,
            "expected bounded SNG0 song");
    uint32_t c1 = u32(b, 4), c2 = u32(b, 8);
    require(c1 && c2 && uint64_t(c1) + c2 + 20 == b.size(), "invalid SNG0 lengths");
    auto p = Json::parse(expand(b, 20, c1, u32(b, 12))),
         a = Json::parse(expand(b, 20 + c1, c2, u32(b, 16)));
    validateSong(p, a);
    return {p, a};
}
inline Json catalog(const fs::path &game, const fs::path &source) {
    Json out = Json::array();
    for (auto e : loadJson(source / "bundled-songs.json"))
        if (fs::is_regular_file(game / "audio/music/bundled" /
                                (lower(e["id"].get<std::string>()) + ".json")))
            out.push_back(e);
    return out;
}
inline SongInput loadSong(const fs::path &game, const fs::path &source, const std::string &name) {
    std::vector<Json> found;
    for (auto e : catalog(game, source))
        if (lower(e["id"].get<std::string>()) == lower(name) ||
            lower(e["title"].get<std::string>()) == lower(name))
            found.push_back(e);
    require(found.size() == 1, "unknown bundled song; use --list-songs");
    auto e = found[0];
    auto b = read(
        (game / "audio/music/bundled" / (lower(e["id"].get<std::string>()) + ".json")).string());
    auto decoded = decodeSong(b);
    require(lower(decoded.first["song"]["id"].get<std::string>()) ==
                lower(e["id"].get<std::string>()),
            "bundled song ID mismatch");
    return {e, decoded.first, decoded.second, sha(b)};
}
struct Node {
    Matrix matrix = identity, local = identity;
    uint32_t flags = 0;
    int visible = 1, segment = -1, section = 0;
    std::array<float, 2> widths{1, 1}, walls{1, 1};
};
struct Segment {
    int start, end, type, section;
    std::string name;
    std::array<float, 4> params;
    float vol = 0;
};
struct Span {
    int start, end, section;
    bool drop = false;
};
struct EntryChoice {
    std::string name;
    Json cfg;
    int minimum, maximum;
    float weight, lo, hi, vol = 0;
};
struct Generator {
    SongInput input;
    std::string genre, exeHash;
    Json configs, sectionConfigs, configHashes, sections;
    float tempo, speed, spacing, rollSign = 0, clearance = 80;
    uint32_t seed;
    Random random, segmentRandom;
    std::vector<float> beats, volumes;
    int finish, count, attempts = 0, rejections = 0;
    std::vector<Node> nodes;
    std::vector<Segment> segments;
    Generator(const fs::path &game, const Archive &archive, SongInput in, std::string g)
        : input(std::move(in)), genre(std::move(g)), seed(songSeed(input.profile["song"]["id"])),
          random(seed), segmentRandom(seed) {
        exeHash = sha(read((game / "Game.exe").string()));
        require(exeHash == gameHash, "unsupported Game.exe SHA256");
        tempo = F(number(input.analysis["track"], "tempo", 120));
        if (!tempo)
            tempo = 120;
        for (const auto *name : {"segments_PC.cfg", "sections_PC.cfg"}) {
            std::string path = std::string("scripts/config/") + name;
            auto b = asset(archive, path);
            configHashes[path] = sha(b);
            if (name == std::string("segments_PC.cfg"))
                configs = config(b);
            else
                sectionConfigs = config(b).at("sections");
        }
        for (auto v : input.analysis["beats"]) {
            beats.push_back(v.get<float>());
        }
        for (auto v : input.analysis["beatVols"])
            volumes.push_back(v.get<float>());
        speed = F(double(tempoValue(Json::array({160, 200}), tempo)) * 0.44703999161720276);
        spacing = F(double(speed) / F(double(tempo) / 60));
        finish = int(beats.size()) + 9;
        count = int(beats.size()) + 34;
        nodes.resize(count);
        for (int i = 0; i < count; i++)
            nodes[i].matrix[11] = float(i);
        sections = input.analysis["sections"];
        assignSections();
    }
    float value(const Json &cfg, const std::string &key, float fallback = 0) const {
        return cfg.contains(key) ? tempoValue(cfg.at(key), tempo) : fallback;
    }
    void assignSections() {
        int selected = 0;
        auto cfgs = values(sectionConfigs);
        for (size_t i = 0; i < sections.size(); i++) {
            sections[i]["config_index"] = selected;
            sections[i]["node"] = 10 + rounded(sections[i]["beat"].get<double>());
            if (i + 1 < sections.size()) {
                float vol = sections[i + 1]["loudness"].get<float>();
                std::vector<std::pair<int, float>> choices;
                for (size_t j = 1; j < cfgs.size(); j++) {
                    auto range = values(cfgs[j].value("volRange", Json::array({0, 1})));
                    if (cfgs[j]["type"] != cfgs[selected]["type"] &&
                        range[0].get<double>() <= vol && vol <= range[1].get<double>())
                        choices.emplace_back(
                            j, value(cfgs[j], "weight_" + genre, value(cfgs[j], "weight")));
                }
                selected = random.choice(choices);
            }
        }
    }
    float nodeSpacing(int start, int end) const {
        int beat = start - 10, n = end - start;
        if (beat >= 0 && beat < int(beats.size()) - 1) {
            int avail = std::min(n, int(beats.size()) - 1 - beat);
            if (avail > 0)
                return F(double(F(double(F(double(beats[beat + avail]) - beats[beat])) / avail)) *
                         speed);
        }
        return spacing;
    }
    std::array<float, 4> parameters(const Json &cfg, float vol) {
        int type = int(number(cfg, "type"));
        float severity = F((double(vol) - 1) * value(cfg, "loudnessInfluence", 1) + 1);
        severity = F(double(severity) * value(cfg, "sev_" + genre, 1));
        auto rand = [&](const std::string &a, const std::string &b) {
            return random.uniform(value(cfg, a), value(cfg, b));
        };
        if (type == 0)
            return {0, 0, 0, 1};
        if (type == 3)
            return {F(double(value(cfg, "pitch")) * severity),
                    F(double(value(cfg, "yaw")) * severity), value(cfg, "roll"), 1};
        if (type == 4) {
            std::array<float, 3> signs{}, xyz{};
            std::string axes = "XYZ";
            for (int i = 0; i < 3; i++)
                signs[i] = value(cfg, "allowRandSign" + axes.substr(i, 1)) ? random.sign() : 1;
            for (int i = 0; i < 3; i++)
                xyz[i] = F(double(rand("min" + axes.substr(i, 1), "max" + axes.substr(i, 1))) *
                           signs[i]);
            return {F(double(xyz[0]) * severity), F(double(xyz[1]) * severity), xyz[2], 1};
        }
        if (type == 5) {
            float bump = rand("bumpMin", "bumpMax"), steer = rand("steerMin", "steerMax"),
                  tilt = rand("tiltMin", "tiltMax");
            steer = F(double(steer) * random.sign());
            tilt = F(double(tilt) * random.sign());
            return {F(double(bump) * severity), F(double(steer) * severity), tilt, 1};
        }
        if (type == 6 || type == 9)
            return {type == 6 ? value(cfg, "roll") : 0, rand("rampUpMin", "rampUpMax"),
                    rand("rampDownMin", "rampDownMax"), value(cfg, "gap")};
        if (type == 7) {
            float a = F(double(value(cfg, "inAngle")) * severity),
                  b = F(double(value(cfg, "outAngle")) * severity);
            if (random.uniform() < .5f) {
                a = -a;
                b = -b;
            }
            return {a, b, value(cfg, "mid"), 1};
        }
        if (type == 8) {
            return {value(cfg, "gapY"), value(cfg, "gapZ"), 0, 1};
        }
        throw std::runtime_error("unsupported segment type");
    }
    bool apply(int start, int end, int type, std::array<float, 4> params, const Json &cfg,
               int segment, int section) {
        auto [a, b, c, d] = params;
        float step = nodeSpacing(start, end);
        int first = start + 1;
        for (int i = first; i <= end; i++) {
            Node &node = nodes[i];
            std::array<float, 3> angles{};
            float extra = 0;
            int visible = 1;
            if (type == 3) {
                angles = {segmentRandom.uniform(-a, a), segmentRandom.uniform(-b, b),
                          segmentRandom.uniform(-c, c)};
            } else if (type == 4) {
                angles = {a, b, c};
                rollSign = c < 0 ? -1 : c > 0 ? 1 : 0;
            } else if (type == 5) {
                float u = F(double(i - first) / (end - first)),
                      sign = u <= .25f || u > .75f ? 1 : -1;
                angles = {F(double(sign) * a), F(double(sign) * b), F(double(sign) * c)};
            } else if (type == 7) {
                float u = F(double(i - first) / (end - first));
                angles[1] = u < c ? F(double(F(double(u) / c)) * (-double(a)) + a)
                                  : F(double(F((double(u) - c) / (1 - double(c)))) * b);
            } else if (type == 8) {
                float gap = F((double(a) + b) / 3), maxgap = F(double(step) - 1);
                if (gap > maxgap) {
                    float ratio = F(double(maxgap) / gap);
                    a = F(double(a) * ratio);
                    b = F(double(b) * ratio);
                    gap = maxgap;
                }
                if (i == first || i == first + 1 || i == end)
                    extra = -gap;
                if (i > first)
                    node.flags |= 8192;
                if (i == first + 2) {
                    extra = F(double(b) * .5);
                    angles[0] = 90;
                } else if (i == end - 2) {
                    extra = a;
                    angles[0] = -90;
                } else if (i == end - 1)
                    extra = F(double(b) * .5);
                visible = i == first || i == end - 1 || i == end;
            } else if (type == 6) {
                float limit = std::max(0.f, F(double(step) - 12.5));
                d = std::clamp(d, -limit, limit);
                float turn = F(double(rollSign) * a);
                angles[1] = F(double(turn) * 0.05999999865889549);
                if (i == first) {
                    angles[0] = -b;
                    angles[2] = turn;
                }
                if (i == first + 1) {
                    angles[2] = turn;
                    angles[0] = a ? F(double(b) - c) : b;
                    visible = 0;
                    if (a)
                        node.flags |= 4096;
                }
                if (i == first + 2) {
                    if (!a)
                        angles[0] = -c;
                    extra = d;
                }
                if (i == first + 3)
                    extra = -d;
            } else if (type == 9) {
                float offset =
                    F((1 - std::clamp(double(F((double(tempo) - 100) / 40)), 0., 1.)) * 25);
                if (i < first + 2)
                    angles[0] = F(-double(b) * .5);
                else if (i == first + 2) {
                    angles[0] = b;
                    visible = 0;
                    extra = F(-double(offset) * .5);
                } else if (i == first + 3) {
                    angles[0] = -c;
                    visible = 0;
                    extra = F(-double(offset) * .5);
                } else if (i == end - 1) {
                    angles[0] = c;
                    extra = F(double(offset) * .5);
                } else if (i == end)
                    extra = F(double(offset) * .5);
            }
            Matrix matrix = identity;
            for (int axis = 0; axis < 3; axis++)
                matrix = compose(matrix, rotation(axis, angles[axis]), true);
            setBasis(matrix, 3, {0, 0, F(double(extra) + step)});
            node.local = matrix;
            node.visible = visible;
            for (auto flag :
                 std::array<std::pair<const char *, uint32_t>, 4>{{{"stunt", 32768},
                                                                   {"selfIntersectAllow", 524288},
                                                                   {"noSuck", 131072},
                                                                   {"noBarricades", 262144}}})
                if (number(cfg, flag.first))
                    node.flags |= flag.second;
        }
        if (!cfg.empty() && !number(cfg, "disableRelax"))
            for (int i = start; i < end - 1; i++)
                nodes[i].flags |= 2048;
        for (int i = first; i <= end; i++) {
            auto &node = nodes[i];
            Matrix matrix = compose(node.local, nodes[i - 1].matrix);
            if (node.flags & 2048) {
                V up = basis(matrix, 1);
                up.y = F(double(up.y) + .15f);
                up = norm(up);
                V right = norm(rr::cross(basis(matrix, 2), up)), forward = rr::cross(up, right);
                setBasis(matrix, 0, right);
                setBasis(matrix, 1, up);
                setBasis(matrix, 2, forward);
            }
            node.matrix = matrix;
            for (int j = start - 4; j > std::max(-1, i - 39); j--) {
                auto &other = nodes[j];
                if ((other.segment < segment - 1 || i >= start + 5) &&
                    (!(other.flags & 524288) || other.segment < 0 ||
                     i >= segments.at(other.segment).end + 6)) {
                    V delta = rr::sub(basis(matrix, 3), basis(other.matrix, 3));
                    if (F(dot(delta, delta)) < F(double(clearance) * clearance)) {
                        rejections++;
                        return false;
                    }
                }
            }
        }
        if (!cfg.empty())
            for (int i = start; i < end; i++) {
                nodes[i].segment = segment;
                nodes[i].section = section;
            }
        if (type == 6 || type == 9) {
            for (int i = start; i < end - 1; i++)
                nodes[i].flags |= 16384;
        }
        return true;
    }
    std::vector<EntryChoice> entries(int section, const std::string &group) {
        auto cfgs = values(sectionConfigs);
        const auto &sec = cfgs.at(sections[section]["config_index"].get<int>());
        std::vector<std::string> names{"Drop"};
        for (auto n : sec.at("includeList"))
            names.push_back(n.get<std::string>());
        std::vector<EntryChoice> result;
        for (const auto &name : names) {
            if (!configs.at(group).contains(name))
                continue;
            auto cfg = configs[group][name];
            auto range = values(cfg.value("volRange", Json::array({0, 1})));
            result.push_back(
                {name, cfg, rounded(value(cfg, "minNodes")), rounded(value(cfg, "maxNodes")),
                 name == "Drop" && result.empty()
                     ? 0
                     : value(cfg, "weight_" + genre, float(rounded(value(cfg, "weight")))),
                 range[0].get<float>(), range[1].get<float>(), 0});
        }
        return result;
    }
    using Follow = std::pair<std::vector<std::string>, std::string>;
    std::optional<Follow> generateSpan(Span span, Follow follow, int startStraight,
                                       bool ignoreVol) {
        int start = span.start, end = span.end, section = span.section;
        bool drop = span.drop;
        if (startStraight) {
            int stop = std::min(end, start + startStraight);
            apply(start, stop, 0, {0, 0, 0, 1}, Json::object(), int(segments.size()), section);
            segments.push_back({start, stop, 0, section, "StartStraight", {0, 0, 0, 1}});
            start = stop;
        }
        std::array<std::vector<EntryChoice>, 2> groups{entries(section, "segments"),
                                                       entries(section, "checkpoints")};
        bool checkpoint = section != 0 && !groups[1].empty() && !drop;
        while (start < end) {
            int remaining = end - start;
            EntryChoice *selected = nullptr;
            bool forced = false;
            if (drop) {
                require(!groups[0].empty(), "missing drop segment");
                selected = &groups[0][0];
                checkpoint = false;
            } else if (!follow.second.empty()) {
                for (int g = 0; g < 2 && !forced; g++)
                    for (auto &e : groups[g])
                        if (e.name == follow.second) {
                            forced = true;
                            if (e.minimum <= remaining) {
                                selected = &e;
                                checkpoint = g == 1;
                            }
                            break;
                        }
            }
            if (!selected && !forced)
                for (int pass = 0; pass < 2; pass++) {
                    auto &group = groups[checkpoint ? 1 : 0];
                    std::vector<std::pair<int, float>> choices;
                    double total = 0;
                    for (size_t j = 0; j < group.size(); j++) {
                        auto &e = group[j];
                        if (e.minimum > remaining)
                            continue;
                        float v = volume(volumes, start - 10,
                                         pass == 0 ? (e.maximum + e.minimum) / 2 : 6);
                        if (!(checkpoint && ignoreVol) && !(e.lo <= v && v <= e.hi))
                            continue;
                        e.vol = v;
                        bool excluded = false;
                        for (const auto &name : follow.first)
                            if (lower(name) == lower(e.name))
                                excluded = true;
                        if (!excluded) {
                            choices.emplace_back(j, e.weight);
                            total += e.weight;
                        }
                    }
                    if (pass == 0 && (choices.empty() || total == 0)) {
                        continue;
                    }
                    if (!choices.empty()) {
                        selected = &group[random.choice(choices)];
                    }
                    break;
                }
            if (!selected) {
                require(groups[0].size() > 1, "missing safety segment");
                selected = &groups[0][1];
                checkpoint = false;
            }
            auto cfg = selected->cfg;
            int n = std::min(remaining, random.between(selected->minimum, selected->maximum));
            require(n > 0, "zero-length segment");
            bool success = false;
            std::array<float, 4> params{};
            for (int attempt = 0; attempt < 3; attempt++) {
                params = parameters(cfg, selected->vol);
                if (apply(start, start + n, int(number(cfg, "type")), params, cfg,
                          int(segments.size()), section)) {
                    success = true;
                    break;
                }
            }
            if (!success) {
                return std::nullopt;
            }
            segments.push_back({start, start + n, int(number(cfg, "type")), section, selected->name,
                                params, selected->vol});
            follow.first.clear();
            if (cfg.contains("excludeFollowList"))
                for (auto v : cfg["excludeFollowList"])
                    follow.first.push_back(v.get<std::string>());
            follow.second = cfg.value("forceNext", std::string());
            start += n;
            checkpoint = false;
            drop = false;
        }
        return follow;
    }
    void modulate() {
        std::array<float, 4> val{1, 1, 1, 1}, decrease{}, increase{};
        std::array<int, 4> remaining{};
        float chance = 0;
        bool allowWall = true, allowLane = true;
        int first = int(double(F(count)) * .20000000298023224),
            last = int(double(F(finish)) * .949999988079071);
        auto cfgs = values(sectionConfigs);
        auto trigger = [&](int k) {
            int duration = random.between(2, 6);
            decrease[k] = F(1.100000023841858 / duration);
            increase[k] = F(2. / duration);
            remaining[k] = duration + random.between(4, 8);
        };
        for (int i = 0; i < count; i++) {
            auto &n = nodes[i];
            if (n.flags & 1024) {
                const auto &cfg = cfgs.at(sections.at(n.section).at("config_index").get<int>());
                allowWall = number(cfg, "allowSidewallDropout", 1) != 0;
                allowLane = number(cfg, "allowLaneChanges", 1) != 0;
            }
            if (i + 1 > first) {
                chance = std::min(.1f, F(double(chance) + .005f));
                if (i + 1 >= last)
                    chance = 0;
                for (int k = 0; k < 4; k++) {
                    if (remaining[k] && (k < 2 ? allowLane : allowWall)) {
                        remaining[k]--;
                        val[k] = F(double(val[k]) - decrease[k]);
                    } else
                        val[k] = F(double(val[k]) + increase[k]);
                }
                float vol =
                    volume(volumes, std::min(int(volumes.size()) - 1, std::max(0, i - 10)), 3);
                vol = F(std::clamp((double(vol) - .5) / .25, 0., 1.));
                float laneChance = F(double(vol) * chance);
                bool gotoRight = false;
                if (remaining[0] == 0 && remaining[2] == 0 && val[0] >= 1) {
                    if (allowLane && random.uniform() < laneChance && remaining[1] == 0) {
                        trigger(0);
                        gotoRight = true;
                    } else if (allowWall && random.uniform() < chance)
                        trigger(2);
                }
                if (gotoRight || remaining[1] == 0)
                    if (remaining[3] == 0 && val[1] >= 1) {
                        if (allowLane && random.uniform() < laneChance && remaining[0] == 0)
                            trigger(1);
                        else if (allowWall && random.uniform() < chance)
                            trigger(3);
                    }
            }
            for (float &v : val) {
                v = std::clamp(v, 0.f, 1.f);
            }
            n.widths = {val[0], val[1]};
            n.walls = {val[2], val[3]};
        }
    }
    Json generate() {
        nodes[10].flags |= 16;
        nodes[14].flags |= 65536;
        nodes[finish].flags |= 512;
        std::vector<Span> spans;
        for (size_t i = 0; i < sections.size(); i++) {
            int start = sections[i]["node"],
                end = i + 1 < sections.size() ? sections[i + 1]["node"].get<int>() : finish;
            spans.push_back({start, end, int(i)});
            nodes[start].flags |= 1024;
        }
        auto drops = detectDrops(volumes);
        bool skip = false;
        for (int beat : drops) {
            if (skip) {
                skip = false;
                continue;
            }
            int node = beat + 8;
            for (size_t j = 0; j < spans.size(); j++) {
                auto span = spans[j];
                if (span.start <= node && node + 6 <= span.end) {
                    if (span.drop && node < span.start + 6)
                        skip = true;
                    else if (node == span.start)
                        spans[j].drop = true;
                    else {
                        spans[j].end = node;
                        span.start = node;
                        span.drop = true;
                        spans.insert(spans.begin() + j + 1, span);
                    }
                    break;
                }
            }
        }
        spans.push_back({finish, count - 1, 0});
        apply(0, 10, 0, {0, 0, 0, 1}, Json::object(), 0, 0);
        segments = {{0, 10, 0, -1, "LeadIn", {0, 0, 0, 1}}};
        for (auto &n : nodes) {
            n.flags &= uint32_t(-980993);
            n.visible = 1;
        }
        std::vector<int> retry(spans.size(), 3), budget(spans.size(), 4);
        std::vector<Follow> saved(spans.size());
        int i = 0;
        Follow follow;
        int straight = rounded(tempoValue(Json::array({2, 8}), tempo));
        while (i < int(spans.size())) {
            attempts++;
            require(attempts <= 10000, "song generation retry budget exhausted");
            auto result = generateSpan(spans[i], follow, straight, budget[i] == 0);
            if (!result) {
                while (retry[i] <= 0) {
                    retry[i]--;
                    budget[i] = std::max(0, budget[i] - 1);
                    retry[i] = 3;
                    if (i == 0)
                        clearance = std::max(0.f, clearance - 10);
                    else
                        i--;
                }
                retry[i]--;
                size_t start = segments.size();
                for (size_t j = 0; j < segments.size(); j++)
                    if (segments[j].start == spans[i].start) {
                        start = j;
                        break;
                    }
                follow = saved[i];
                segments.resize(start);
                for (int k = spans[i].start; k < count; k++) {
                    nodes[k].flags &= uint32_t(-980993);
                    nodes[k].visible = 1;
                }
                continue;
            }
            saved[i] = *result;
            follow = *result;
            i++;
            straight = 0;
        }
        auto &last = segments.back();
        apply(last.start, last.end, 0, {0, 0, 0, 1}, Json::object(), int(segments.size()) - 1,
              last.section);
        last.type = 0;
        last.params = {0, 0, 0, 1};
        modulate();
        Json out = {{"schema", "reriff-song-nodes-v1"},
                    {"implementation", "C++"},
                    {"song", input.profile["song"]},
                    {"tempo", tempo},
                    {"genre", genre},
                    {"seed", seed},
                    {"executable_sha256", exeHash},
                    {"config_sha256", configHashes},
                    {"analysis_sha256", input.hash},
                    {"bundled_reference", input.entry},
                    {"speed_metres_per_second", speed},
                    {"default_units_per_beat", spacing},
                    {"start_node", 10},
                    {"finish_node", finish},
                    {"node_count", count},
                    {"drops", drops},
                    {"sections", sections},
                    {"generation",
                     {{"span_attempts", attempts},
                      {"collision_rejections", rejections},
                      {"final_clearance", clearance}}},
                    {"parity", {{"full_original_node_dump_compared", false}}}};
        out["segments"] = Json::array();
        for (const auto &s : segments)
            out["segments"].push_back({{"start", s.start},
                                       {"end", s.end},
                                       {"type", s.type},
                                       {"section", s.section},
                                       {"name", s.name},
                                       {"params", s.params},
                                       {"volume", s.vol}});
        out["nodes"] = Json::array();
        for (int k = 0; k < count; k++) {
            const auto &n = nodes[k];
            out["nodes"].push_back({{"index", k},
                                    {"position", json(basis(n.matrix, 3))},
                                    {"up", json(basis(n.matrix, 1))},
                                    {"forward", json(basis(n.matrix, 2))},
                                    {"flags", n.flags},
                                    {"visible", n.visible},
                                    {"section", n.section},
                                    {"segment", n.segment},
                                    {"widths", n.widths},
                                    {"walls", n.walls}});
        }
        return out;
    }
};
inline Json compareNodes(const Json &actual, const Json &reference, double tolerance = .01) {
    require(reference.at("source") == "original-game" &&
                reference.at("executable_sha256") == gameHash,
            "invalid original reference identity");
    require(actual.at("song").at("id") == reference.at("song_id") &&
                actual.at("genre") == reference.at("genre"),
            "reference song/genre mismatch");
    for (const auto *key : {"analysis_sha256", "config_sha256"})
        if (reference.contains(key))
            require(actual.at(key) == reference.at(key), "reference input hash mismatch");
    const auto &a = actual.at("nodes");
    const auto &b = reference.at("nodes");
    require(a.is_array() && b.is_array() && !a.empty() && !b.empty(), "empty node comparison");
    Json r = {{"passed", a.size() == b.size()},
              {"node_counts", {a.size(), b.size()}},
              {"tolerance", tolerance},
              {"max_position_error", 0.},
              {"max_up_error", 0.},
              {"max_width_error", 0.},
              {"max_wall_error", 0.},
              {"flag_mismatches", Json::array()},
              {"visibility_mismatches", Json::array()}};
    for (size_t i = 0; i < std::min(a.size(), b.size()); i++) {
        for (auto pair : std::array<std::pair<const char *, const char *>, 4>{
                 {{"position", "max_position_error"},
                  {"up", "max_up_error"},
                  {"widths", "max_width_error"},
                  {"walls", "max_wall_error"}}}) {
            size_t size =
                std::string(pair.first) == "position" || std::string(pair.first) == "up" ? 3 : 2;
            require(a[i].at(pair.first).size() == size && b[i].at(pair.first).size() == size,
                    "invalid reference vector length");
            double squared = 0;
            for (size_t j = 0; j < size; j++) {
                double x = a[i][pair.first][j].get<double>(), y = b[i][pair.first][j].get<double>();
                require(std::isfinite(x) && std::isfinite(y), "nonfinite comparison");
                squared += sq(x - y);
            }
            double error = std::sqrt(squared);
            r[pair.second] = std::max(r[pair.second].get<double>(), error);
            if (error > tolerance)
                r["passed"] = false;
        }
        for (auto pair : std::array<std::pair<const char *, const char *>, 2>{
                 {{"flags", "flag_mismatches"}, {"visible", "visibility_mismatches"}}})
            if (a[i].at(pair.first) != b[i].at(pair.first)) {
                r[pair.second].push_back(i);
                r["passed"] = false;
            }
    }
    return r;
}
} // namespace rr
