#pragma once
#include "native_assets.hpp"
#include "native_song.hpp"
#include "playback.hpp"

namespace rr {
struct Road {
    std::vector<V> points, ups;
    std::array<std::vector<V>, 2> widths, walls;
    std::vector<int> visible;
    bool song = false;
    std::string kind;
    V evaluate(const std::vector<V> &p, float t, bool tangent = false) const {
        return reriff::track::evaluate(p.data(), p.size(), t, tangent);
    }
    std::pair<V, Frame> frame(float t) const {
        return {evaluate(points, t),
                reriff::track::orientation(evaluate(points, t, true), norm(evaluate(ups, t)))};
    }
    std::array<float, 2> width(float t) const {
        if (!song)
            return {1, 1};
        return {evaluate(widths[0], t).x, evaluate(widths[1], t).x};
    }
    std::array<float, 2> wall(float t) const {
        if (!song)
            return {1, 1};
        return {evaluate(walls[0], t).x, evaluate(walls[1], t).x};
    }
    bool rowVisible(float t) const {
        int i = int(t);
        return !song || visible.at(i) || (t > 1 && t == std::floor(t) && visible.at(i - 1));
    }
    std::vector<float> parameters() const {
        std::vector<float> out;
        if (song) {
            int count = int(std::ceil((points.size() - 3) / .15));
            for (int i = 0; i < count; i++)
                out.push_back(F(i * 3. / 20.));
        } else {
            double end = points.size() - 3;
            int count = int((end - 1) / .15) + 1;
            for (int i = 0; i < count; i++)
                out.push_back(F(1 + i * .15));
            if (std::abs(out.back() - end) > 1e-6)
                out.push_back(F(end));
        }
        return out;
    }
};
inline Road songRoad(const Json &track) {
    Road road;
    road.song = true;
    road.kind = "song";
    for (const auto &n : track.at("nodes")) {
        road.points.push_back(vec(n.at("position")));
        road.ups.push_back(vec(n.at("up")));
        road.visible.push_back(n.at("visible"));
        for (int k = 0; k < 2; k++) {
            road.widths[k].push_back({n.at("widths")[k].get<float>(), 0, 0});
            road.walls[k].push_back({n.at("walls")[k].get<float>(), 0, 0});
        }
    }
    return road;
}
inline Road fixture(const std::string &kind) {
    Road road;
    road.kind = kind;
    std::vector<float> rolls;
    if (kind == "combined") {
        road.points = {{0, 0, -20},   {0, 0, 0},     {0, 0, 20},    {2, 3, 40},    {10, 10, 60},
                       {24, 17, 80},  {42, 20, 100}, {62, 17, 120}, {82, 10, 140}, {98, 3, 160},
                       {108, 0, 180}, {112, 0, 200}, {116, 0, 220}};
        rolls = {0, 0, 0, 5, 15, 25, 30, 20, 10, 5, 0, 0, 0};
    } else {
        for (int i = 0; i < 9; i++)
            road.points.push_back({0, 0, float(i * 20 - 20)});
        rolls.resize(9, 0);
        if (kind == "curved")
            road.points = {{0, 0, -20}, {0, 0, 0},   {0, 0, 20},   {3, 0, 40},  {12, 0, 60},
                           {28, 0, 78}, {48, 0, 92}, {70, 0, 100}, {94, 0, 100}};
        else if (kind == "uphill" || kind == "downhill") {
            float sign = kind == "uphill" ? 1 : -1;
            std::array<float, 9> y{0, 0, 0, 4, 12, 20, 28, 36, 44};
            for (int i = 0; i < 9; i++)
                road.points[i].y = sign * y[i];
        } else if (kind == "banked")
            rolls = {0, 0, 0, 15, 30, 35, 35, 35, 35};
        else
            require(kind == "straight", "unknown fixture");
    }
    for (size_t i = 0; i < rolls.size(); i++) {
        V tangent = rr::sub(road.points[std::min(i + 1, road.points.size() - 1)],
                            road.points[i ? i - 1 : 0]);
        auto f = reriff::track::orientation(tangent, {0, 1, 0});
        double angle = double(rolls[i]) * 3.14159265358979323846 / 180;
        V up{};
        for (int k = 0; k < 3; k++)
            up[k] = F(f.up[k] * std::cos(angle) - f.right[k] * std::sin(angle));
        road.ups.push_back(up);
    }
    return road;
}
inline std::array<std::array<double, 2>, 7> wallProfile(int side, double height) {
    std::array<std::array<double, 2>, 7> out{};
    for (int i = 0; i < 7; i++) {
        double x = side * 12., y = -1;
        if (i >= 1)
            y += height * .75 + 1;
        if (i >= 2) {
            y += height * .25;
            x += side * .125;
        }
        if (i >= 3)
            x += side * .45;
        if (i >= 4) {
            x += side * .05;
            y -= height * .5;
        }
        if (i >= 5)
            y -= 4.5;
        if (i >= 6) {
            x = 0;
            y = -8;
        }
        out[i] = {x, y};
    }
    return out;
}
inline V faceNormal(V a, V b, V c) {
    V u = rr::sub(b, a), v = rr::sub(c, a), n = rr::cross(u, v);
    double len = length(n);
    return len > 1e-12 ? V{F(n.x / len), F(n.y / len), F(n.z / len)} : V{0, 1, 0};
}
inline void appendVertex(Buffer &out, V p, V n, float u, float v, float layer, float metal = 0,
                         float rough = .75f) {
    out.vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, 1, 1, 1, metal, rough, u, v, layer});
}
inline void quad(Buffer &out, const std::array<V, 4> &p,
                 const std::array<std::array<float, 2>, 4> &uv, float layer = 2) {
    uint32_t first = out.vertices.size();
    V n = faceNormal(p[0], p[1], p[2]);
    for (int k = 0; k < 4; k++)
        appendVertex(out, p[k], n, uv[k][0], uv[k][1], layer, .2f, .5f);
    for (int i : {0, 1, 2, 1, 3, 2})
        out.indices.push_back(first + i);
}
inline std::vector<std::vector<V>> wheelSupports(const Model &model) {
    std::vector<std::vector<V>> out;
    for (const auto &n : model.nodes)
        if (n.name.rfind("wheel_", 0) == 0) {
            std::set<std::array<float, 3>> unique;
            for (int index : n.meshes)
                for (V v : model.meshes[index].positions) {
                    V p = transform(n.matrix, v, true);
                    unique.insert({p.x, p.y, p.z});
                }
            std::vector<V> points;
            for (auto p : unique)
                points.push_back({p[0], p[1], p[2]});
            std::stable_sort(points.begin(), points.end(), [](V a, V b) { return a.y < b.y; });
            require(points.size() >= 32, "too few wheel support points");
            points.resize(32);
            out.push_back(points);
        }
    require(out.size() == 4, "expected four wheel nodes");
    return out;
}
using Triangle = std::array<V, 3>;
inline std::optional<double> clearance(V point, V direction,
                                       const std::vector<Triangle> &triangles) {
    std::optional<double> best;
    for (const auto &tri : triangles) {
        V a = tri[0], u = rr::sub(tri[1], a), v = rr::sub(tri[2], a), n = rr::cross(u, v);
        double denom = dot(n, direction);
        if (std::abs(denom) < 1e-10)
            continue;
        double along = dot(n, rr::sub(a, point)) / denom;
        V w{};
        for (int k = 0; k < 3; k++)
            w[k] = F(double(point[k]) + direction[k] * along - a[k]);
        double uu = dot(u, u), uv = dot(u, v), vv = dot(v, v);
        denom = uu * vv - uv * uv;
        if (std::abs(denom) < 1e-12)
            continue;
        double wu = dot(w, u), wv = dot(w, v), s = (vv * wu - uv * wv) / denom,
               t = (uu * wv - uv * wu) / denom;
        if (s >= -1e-6 && t >= -1e-6 && s + t <= 1.000001 &&
            (!best || std::abs(along) < std::abs(*best)))
            best = -along;
    }
    return best;
}
inline std::optional<Json> placeCar(const Road &road, float t, const Buffer &car,
                                    const std::vector<std::vector<V>> &supports,
                                    const std::vector<Triangle> &triangles, Buffer &out) {
    auto [position, frame] = road.frame(t);
    auto world = [&](V p, double lift = 0) {
        V delta = transform(frame, p), result{};
        for (int k = 0; k < 3; k++)
            result[k] = F(double(position[k]) + delta[k] + frame.up[k] * lift);
        return result;
    };
    auto separations = [&](double lift) -> std::optional<std::vector<double>> {
        std::vector<double> result;
        for (const auto &patch : supports) {
            double separation = INFINITY;
            for (V p : patch) {
                auto hit = clearance(world(p, lift), frame.up, triangles);
                if (!hit)
                    return std::nullopt;
                separation = std::min(separation, *hit);
            }
            result.push_back(separation);
        }
        return result;
    };
    auto initial = separations(0);
    if (!initial)
        return std::nullopt;
    double lift = -*std::min_element(initial->begin(), initial->end()) + .002;
    auto final = separations(lift);
    if (!final || *std::min_element(final->begin(), final->end()) < -1e-5 ||
        *std::max_element(final->begin(), final->end()) >= .2)
        return std::nullopt;
    uint32_t first = out.vertices.size();
    for (auto vertex : car.vertices) {
        V p = world({vertex[0], vertex[1], vertex[2]}, lift),
          n = transform(frame, {vertex[3], vertex[4], vertex[5]});
        for (int k = 0; k < 3; k++) {
            vertex[k] = p[k];
            vertex[k + 3] = n[k];
        }
        out.vertices.push_back(vertex);
    }
    for (auto index : car.indices)
        out.indices.push_back(first + index);
    return Json{
        {"parameter", t},
        {"position", json(position)},
        {"local_up_lift", lift},
        {"frame",
         {{"right", json(frame.right)}, {"up", json(frame.up)}, {"forward", json(frame.forward)}}},
        {"wheel_support_clearances", *final},
        {"note", "Rigid inspection pose; suspension and driving physics are not implemented."}};
}
inline Image bake(const Image &image, const Json &palette) {
    Image out{512, 512, Bytes(512 * 512 * 4)};
    auto colors =
        std::array<std::vector<double>, 3>{palette.at("base").get<std::vector<double>>(),
                                           palette.at("accent").get<std::vector<double>>(),
                                           palette.at("pulse").get<std::vector<double>>()};
    // Bilinear resampling replaces Pillow's Lanczos. This affects inspection
    // textures only, never the recovered geometry or track-generation state.
    for (int y = 0; y < 512; y++)
        for (int x = 0; x < 512; x++) {
            double sx = (x + .5) * image.width / 512 - .5, sy = (y + .5) * image.height / 512 - .5;
            int ax = int(std::floor(sx)), ay = int(std::floor(sy));
            double tx = sx - ax, ty = sy - ay;
            std::array<double, 3> pixel{};
            for (int dy = 0; dy < 2; dy++)
                for (int dx = 0; dx < 2; dx++) {
                    int px = std::clamp(ax + dx, 0, int(image.width) - 1),
                        py = std::clamp(ay + dy, 0, int(image.height) - 1);
                    double weight = (dx ? tx : 1 - tx) * (dy ? ty : 1 - ty);
                    for (int c = 0; c < 3; c++)
                        pixel[c] +=
                            weight * image.pixels[(size_t(py) * image.width + px) * 4 + c] / 255.;
                }
            for (int c = 0; c < 3; c++) {
                double linear = 0;
                for (int i = 0; i < 3; i++)
                    linear += pixel[i] * colors[i][c];
                out.pixels[(y * 512 + x) * 4 + c] =
                    uint8_t(std::lround(std::pow(std::clamp(linear, 0., 1.), 1 / 2.2) * 255));
            }
            out.pixels[(y * 512 + x) * 4 + 3] = 255;
        }
    return out;
}
inline Json sceneTextures(const Archive &archive, const std::string &genre, const fs::path &out,
                          bool ramp) {
    auto visuals = config(asset(archive, "scripts/config/visualSections.cfg"));
    require(visuals.contains(genre), "unknown visual genre");
    auto styles = visuals.at(genre);
    require(!styles.empty(), "empty visual palette");
    auto style = styles.begin();
    auto road = style.value().at("road");
    Json palette = Json::object();
    for (const char *k : {"base", "accent", "pulse"}) {
        palette[k] = Json::array();
        for (auto v : road.at(k))
            palette[k].push_back(v.get<double>() / 255);
    }
    std::vector<Image> layers{{512, 512, Bytes(512 * 512 * 4, 255)}};
    Json rampLayers = Json::object();
    std::map<std::string, Image> images;
    for (const auto *category : {"road", "sidewall", "ramps"}) {
        if (category == std::string("ramps") && !ramp)
            continue;
        std::string path = "graphics/genre/" + genre + "/" + category +
                           (category == std::string("ramps") ? "/textures" : "") +
                           "/batchTextures.brres";
        auto textures = decodeTextures(asset(archive, path));
        for (const auto &t : textures) {
            auto image = decodeDDS(t);
            std::string group = category == std::string("sidewall") ? "wall"
                                : category == std::string("ramps")  ? "ramp"
                                                                    : "road";
            images[group + "/" + lower(t.name)] = image;
            write(out / "textures" / (group + "-" + safe(t.name) + ".dds"), t.dds);
            savePNG(out / "textures" / (group + "-" + safe(t.name) + ".png"), image);
        }
    }
    layers.push_back(bake(images.at("road/road"), palette));
    layers.push_back(bake(images.at("wall/sidewall"), palette));
    if (ramp) {
        auto model = decodeModel(asset(archive, "graphics/genre/" + genre + "/ramps/ramp_8m.bdae"));
        for (const auto &m : model.materials) {
            std::string texture = lower(fs::path(m["textures"][0].get<std::string>()).stem());
            if (rampLayers.contains(texture))
                continue;
            rampLayers[texture] = layers.size();
            Json base = texture.find("glow") != std::string::npos ? Json::array({.2, .3, .4})
                        : (texture.find("anim") != std::string::npos ||
                           texture.find("metal") != std::string::npos)
                            ? Json::array({.3, .35, .4})
                            : Json::array({.15, .18, .22});
            layers.push_back(
                bake(images.at("ramp/" + texture),
                     {{"base", base}, {"accent", {.08, .5, .65}}, {"pulse", {.18, .2, .24}}}));
        }
    }
    require(layers.size() <= 16, "too many texture layers");
    Bytes bytes{'R', 'R', 'T', 'E', 'X', '1', 0, 0};
    appendU32(bytes, 512);
    appendU32(bytes, 512);
    appendU32(bytes, layers.size());
    for (size_t i = 0; i < layers.size(); i++) {
        savePNG(out / "textures" / ("inspection-layer-" + std::to_string(i) + ".png"), layers[i]);
        bytes.insert(bytes.end(), layers[i].pixels.begin(), layers[i].pixels.end());
    }
    write(out / "scene.rtex", bytes);
    return {{"palette_style", style.key()},
            {"palette", palette},
            {"texture_layers", {{"count", layers.size()}, {"ramp_mapping", rampLayers}}},
            {"texture_resampling",
             "Native bilinear inspection masks; original animated shaders remain pending."}};
}
inline Json buildRoad(const Road &road, const Model &model, const Json &track,
                      const fs::path &out) {
    auto parameters = road.parameters();
    size_t count = parameters.size();
    std::vector<std::array<V, 9>> rows;
    std::vector<Frame> frames;
    std::vector<std::array<float, 2>> widths, heights;
    std::vector<V> centers;
    std::vector<bool> visible;
    Buffer buffer;
    for (float t : parameters) {
        auto [p, f] = road.frame(t);
        centers.push_back(p);
        frames.push_back(f);
        auto w = road.width(t);
        for (float &v : w)
            v = F(double(v) * .6666666865348816 + .3333333432674408);
        widths.push_back(w);
        heights.push_back(road.wall(t));
        visible.push_back(road.rowVisible(t));
        std::array<V, 9> row{};
        for (int j = 0; j < 9; j++) {
            float x = F((j * 3 - 12) * double(w[j < 4 ? 1 : 0]));
            for (int k = 0; k < 3; k++)
                row[j][k] = F(double(p[k]) + F(double(f.right[k]) * x));
            appendVertex(buffer, row[j], f.up, F(double(x) / 24) + .5f,
                         F(double(t) * 1.600000023841858), 1);
        }
        rows.push_back(row);
    }
    for (size_t i = 0; i + 1 < count; i++)
        if (visible[i] && visible[i + 1])
            for (int j = 0; j < 8; j++) {
                uint32_t a = i * 9 + j, b = a + 9;
                for (uint32_t index : {a, b, a + 1, b, b + 1, a + 1})
                    buffer.indices.push_back(index);
            }
    auto roadIndices = buffer.indices;
    double widthError = 0, normalError = 0, ortho = 0, minArea = INFINITY, alignment = 1,
           maxCoordinate = 0;
    int degenerate = 0, inverted = 0;
    for (size_t i = 0; i < count; i++) {
        widthError = std::max(widthError, std::abs(length(rr::sub(rows[i][8], rows[i][0])) -
                                                   12 * (double(widths[i][0]) + widths[i][1])));
        normalError = std::max(normalError, std::abs(length(frames[i].up) - 1));
        ortho = std::max({ortho, std::abs(dot(frames[i].right, frames[i].up)),
                          std::abs(dot(frames[i].right, frames[i].forward)),
                          std::abs(dot(frames[i].up, frames[i].forward))});
        for (const auto &p : rows[i])
            for (int k = 0; k < 3; k++)
                maxCoordinate = std::max(maxCoordinate, std::abs(double(p[k])));
    }
    for (size_t j = 0; j < roadIndices.size(); j += 3) {
        auto get = [&](uint32_t i) { return rows[i / 9][i % 9]; };
        V a = get(roadIndices[j]), b = get(roadIndices[j + 1]), c = get(roadIndices[j + 2]);
        double area = length(rr::cross(rr::sub(b, a), rr::sub(c, a))) * .5;
        minArea = std::min(minArea, area);
        if (area < 1e-8) {
            degenerate++;
            continue;
        }
        double match = dot(faceNormal(a, b, c), frames[roadIndices[j] / 9].up);
        alignment = std::min(alignment, match);
        if (match <= 0)
            inverted++;
    }
    double tolerance = std::max(1e-4, maxCoordinate * 2e-6), maxJoin = 0;
    for (size_t i = 1; i < road.points.size() - 3; i++) {
        float l = F(double(i) - 1e-4), r = F(double(i) + 1e-4);
        V p = road.evaluate(road.points, l), q = road.evaluate(road.points, r),
          a = road.evaluate(road.points, l, true), b = road.evaluate(road.points, r, true);
        double sum = 0;
        for (int k = 0; k < 3; k++)
            sum +=
                sq(double(p[k]) + a[k] * (double(i) - l) - (double(q[k]) - b[k] * (r - double(i))));
        maxJoin = std::max(maxJoin, std::sqrt(sum));
    }
    require(widthError < tolerance && normalError < 1e-5 && ortho < 1e-5 && degenerate == 0 &&
                inverted == 0 && maxJoin < tolerance * 2,
            "road geometry validation failed");
    for (int side : {-1, 1}) {
        int slot = side == 1 ? 0 : 1;
        auto wallRow = [&](size_t i) {
            auto profile = wallProfile(side, heights[i][slot]);
            std::array<V, 7> result{};
            for (int j = 0; j < 7; j++) {
                V delta =
                    transform(frames[i], {F(profile[j][0] * widths[i][slot]), F(profile[j][1]), 0});
                for (int k = 0; k < 3; k++)
                    result[j][k] = F(double(centers[i][k]) + delta[k]);
            }
            return result;
        };
        std::array<float, 7> wallU{-.4f, .4f, .55f, .65f, .75f, .85f, 1};
        auto previous = wallRow(0);
        for (size_t i = 0; i + 1 < count; i++) {
            auto following = wallRow(i + 1);
            if (visible[i] && visible[i + 1])
                for (int j = 0; j < 6; j++)
                    quad(buffer, {previous[j], following[j], previous[j + 1], following[j + 1]},
                         {{{wallU[j], parameters[i]},
                           {wallU[j], parameters[i + 1]},
                           {wallU[j + 1], parameters[i]},
                           {wallU[j + 1], parameters[i + 1]}}});
            previous = following;
        }
    }
    Buffer car = vehicleBuffer(model);
    auto supports = wheelSupports(model);
    Json cars = Json::array();
    std::vector<float> candidates;
    if (road.song) {
        candidates.push_back(12);
        for (const auto &s : track.at("segments"))
            if (s.at("end").get<int>() - s.at("start").get<int>() >= 6 &&
                (s["type"] == 0 || s["type"] == 4))
                candidates.push_back(float(s["start"].get<int>() + 2));
    } else if (road.kind == "combined")
        candidates = {2.6f, 4.7f, 6.7f, 8.8f};
    else
        candidates = {3.5f};
    for (float t : candidates) {
        if (cars.size() >= 4)
            break;
        bool tooClose = false;
        if (road.song)
            for (const auto &c : cars)
                if (std::abs(t - c.at("parameter").get<float>()) < 25)
                    tooClose = true;
        if (tooClose)
            continue;
        size_t row = 0;
        for (size_t i = 1; i < count; i++)
            if (std::abs(parameters[i] - t) < std::abs(parameters[row] - t))
                row = i;
        if (road.song) {
            if (std::min(widths[row][0], widths[row][1]) < .9)
                continue;
            bool intact = true;
            for (int i = std::max(0, int(row) - 4); i < std::min(int(count), int(row) + 5); i++)
                if (!visible[i])
                    intact = false;
            if (!intact)
                continue;
        }
        std::vector<Triangle> triangles;
        for (size_t j = 0; j < roadIndices.size(); j += 3)
            if (!road.song || std::abs(int(roadIndices[j] / 9) - int(row)) <= 5) {
                Triangle tri;
                for (int k = 0; k < 3; k++) {
                    auto i = roadIndices[j + k];
                    tri[k] = rows[i / 9][i % 9];
                }
                triangles.push_back(tri);
            }
        auto placed = placeCar(road, t, car, supports, triangles, buffer);
        if (placed)
            cars.push_back(*placed);
    }
    require(!cars.empty(), "no supported vehicle pose");
    double outputNormalError = 0;
    for (const auto &v : buffer.vertices)
        outputNormalError = std::max(outputNormalError, std::abs(length({v[3], v[4], v[5]}) - 1));
    require(outputNormalError < 1e-4, "invalid output normals");
    writeMesh(out / "scene.rmesh", buffer);
    Json validation = {{"max_width_error", widthError},
                       {"world_space_tolerance", tolerance},
                       {"max_normal_length_error", normalError},
                       {"max_frame_axis_dot", ortho},
                       {"minimum_triangle_area", minArea},
                       {"degenerate_triangles", degenerate},
                       {"inverted_face_count", inverted},
                       {"minimum_face_to_row_normal_dot", alignment},
                       {"max_knot_position_jump_estimate", maxJoin},
                       {"visible_rows", std::count(visible.begin(), visible.end(), true)},
                       {"hidden_rows", std::count(visible.begin(), visible.end(), false)},
                       {"vehicle_poses", cars.size()},
                       {"max_output_vertex_normal_length_error", outputNormalError}};
    if (track.contains("parity") && track["parity"].contains("comparison"))
        validation["original_comparison"] = track["parity"]["comparison"];
    else
        validation["original_comparison"] = {{"full_track_layout", "Not compared in this run"}};
    save(out / (road.song ? "song-validation.json" : "geometry-validation.json"), validation);
    auto f = cars[0]["frame"]["up"].get<std::array<float, 3>>(),
         p = cars[0]["position"].get<std::array<float, 3>>();
    for (int k = 0; k < 3; k++)
        p[k] += f[k];
    return {{"implementation", "C++"},
            {"section", road.kind},
            {"node_count", road.points.size()},
            {"road",
             {{"rows", count},
              {"triangles", roadIndices.size() / 3},
              {"hidden_rows", std::count(visible.begin(), visible.end(), false)}}},
            {"validation", validation},
            {"vehicles", cars},
            {"viewer_focus", p},
            {"viewer_distance", road.song ? 55 : 42},
            {"viewer_elevation", road.song ? 45 : 55},
            {"total_vertices", buffer.vertices.size()},
            {"total_triangles", buffer.indices.size() / 3},
            {"limitations",
             {"Static inspection materials; props, animated shaders, suspension and driving "
              "physics remain pending."}}};
}
inline Json buildStraight(const Archive &archive, const std::string &genre, const Model &model,
                          const Json &rampLayers, const fs::path &out) {
    Buffer buffer = vehicleBuffer(model);
    float bottom = INFINITY;
    for (const auto &node : model.nodes)
        if (node.name.rfind("wheel_", 0) == 0)
            for (int index : node.meshes)
                for (V p : model.meshes[index].positions)
                    bottom = std::min(bottom, transform(node.matrix, p, true).y);
    require(std::isfinite(bottom), "wheel geometry missing");
    for (auto &v : buffer.vertices) {
        v[1] = F(double(v[1]) - bottom);
        v[2] = F(double(v[2]) - 8);
    }
    uint32_t first = buffer.vertices.size();
    for (int row = 0; row < 13; row++)
        for (int j = 0; j < 9; j++) {
            float x = float(j * 3 - 12), t = F(row * 3. / 20);
            appendVertex(buffer, {x, 0, float(-18 + row * 3)}, {0, 1, 0}, x / 24 + .5f,
                         F(double(t) * 1.600000023841858), 1);
        }
    for (int row = 0; row < 12; row++)
        for (int j = 0; j < 8; j++) {
            uint32_t a = first + row * 9 + j, b = a + 9;
            for (auto i : {a, b, a + 1, b, b + 1, a + 1})
                buffer.indices.push_back(i);
        }
    std::array<float, 7> u{-.4f, .4f, .55f, .65f, .75f, .85f, 1};
    for (int side : {-1, 1}) {
        auto profile = wallProfile(side, 1);
        for (int row = 0; row < 12; row++)
            for (int j = 0; j < 6; j++) {
                float x = F(profile[j][0]), y = F(profile[j][1]), nx = F(profile[j + 1][0]),
                      ny = F(profile[j + 1][1]), z = float(-18 + row * 3);
                quad(buffer, {V{x, y, z}, V{x, y, z + 3}, V{nx, ny, z}, V{nx, ny, z + 3}},
                     {{{u[j], F(row * .15)},
                       {u[j], F((row + 1) * .15)},
                       {u[j + 1], F(row * .15)},
                       {u[j + 1], F((row + 1) * .15)}}});
            }
    }
    auto ramp = decodeModel(asset(archive, "graphics/genre/" + genre + "/ramps/ramp_8m.bdae"));
    size_t rampStart = buffer.vertices.size();
    for (const auto &node : ramp.nodes)
        for (int index : node.meshes) {
            const auto &mesh = ramp.meshes[index];
            auto texture = lower(
                fs::path(ramp.materials[mesh.material]["textures"][0].get<std::string>()).stem());
            float layer = rampLayers.at(texture).get<float>();
            uint32_t begin = buffer.vertices.size();
            for (size_t i = 0; i < mesh.positions.size(); i++) {
                V p = transform(node.matrix, mesh.positions[i], true),
                  n = transform(node.matrix, mesh.normals[i]);
                p.y = F(double(p.y) + .03);
                p.z = F(double(p.z) + 16);
                appendVertex(buffer, p, n, mesh.uv[i][0], mesh.uv[i][1], layer, .2f, .5f);
            }
            for (auto i : mesh.indices)
                buffer.indices.push_back(begin + i);
        }
    writeMesh(out / "scene.rmesh", buffer);
    return {{"implementation", "C++"},
            {"section", "straight"},
            {"road",
             {{"width", 24},
              {"length", 36},
              {"rows", 13},
              {"cross_section_vertices", 9},
              {"triangles", 192}}},
            {"car", {{"translation", {0, -bottom, -8}}, {"wheel_lowest_after_placement", 0}}},
            {"ramp",
             {{"included", true},
              {"vertices", buffer.vertices.size() - rampStart},
              {"translation", {0, .03, 16}}}},
            {"viewer_focus", {0, 0, 1}},
            {"viewer_distance", 42},
            {"viewer_elevation", 30},
            {"vehicles", Json::array()},
            {"total_vertices", buffer.vertices.size()},
            {"total_triangles", buffer.indices.size() / 3},
            {"limitations",
             {"Straight road fixture; the ramp is placed flat without collision or animation."}}};
}
inline Json preparePlayback(const Archive &, const fs::path &game, const fs::path &output,
                            const Json &track, const Json &analysis, const Buffer &sceneCar,
                            bool mute) {
    auto road = songRoad(track);
    std::vector<float> d{0};
    for (size_t i = 1; i < road.points.size(); i++) {
        V delta = rr::sub(road.points[i], road.points[i - 1]);
        float span = F(std::sqrt(double(F(dot(delta, delta)))));
        require(span > 0, "zero playback span");
        d.push_back(F(double(d.back()) + span));
    }
    int start = track.at("start_node"), finish = track.at("finish_node");
    float span = analysis.at("track").at("beat_span").get<float>();
    require(span > 0, "invalid beat span");
    auto entry = track.at("bundled_reference");
    float first = entry.at("first_beat"), mid = entry.at("mid_beat");
    if (!mid)
        mid = F((double(analysis.at("beats")[0].get<float>()) +
                 analysis.at("beats").back().get<float>()) *
                .5);
    float speed = track.at("speed_metres_per_second"), spacing = track.at("default_units_per_beat"),
          midDistance = F((double(d[finish]) + F(start * double(spacing))) * .5),
          offset = F(double(F((double(midDistance) - d[start]) / speed)) - F(double(first) + mid));
    auto raw = read((output / "scene.rmesh").string());
    uint32_t vc = u32(raw, 8), ic = u32(raw, 12);
    require(raw.size() == 40 + uint64_t(vc) * 56 + uint64_t(ic) * 4,
            "invalid scene mesh for playback");
    Buffer moving;
    for (uint32_t i = 0; i < vc; i++) {
        Vertex v{};
        for (int k = 0; k < 14; k++)
            v[k] = f32(raw, 40 + uint64_t(i) * 56 + k * 4);
        moving.vertices.push_back(v);
    }
    for (uint32_t i = 0; i < ic; ++i) {
        auto index = u32(raw, 40 + uint64_t(vc) * 56 + i * 4);
        require(index < vc, "invalid playback scene index");
        moving.indices.push_back(index);
    }
    ic = uint32_t(moving.indices.size());
    moving.vertices.insert(moving.vertices.end(), sceneCar.vertices.begin(),
                           sceneCar.vertices.end());
    for (auto i : sceneCar.indices)
        moving.indices.push_back(vc + i);
    writeMesh(output / "playback-scene.rmesh", moving);
    float bottom = INFINITY;
    for (const auto &v : sceneCar.vertices)
        bottom = std::min(bottom, v[1]);
    float lift = F(-double(bottom) + .02);
    Bytes data{'R', 'R', 'P', 'L', 'A', 'Y', '1', 0};
    for (auto n : {uint32_t(d.size()), uint32_t(start), uint32_t(finish), ic,
                   uint32_t(sceneCar.indices.size())})
        appendU32(data, n);
    for (auto f : {span, offset, lift})
        appendFloat(data, f);
    for (size_t i = 0; i < d.size(); i++) {
        for (V v : {road.points[i], road.ups[i]})
            for (int k = 0; k < 3; k++)
                appendFloat(data, v[k]);
        appendFloat(data, d[i]);
        appendU32(data, road.visible[i]);
    }
    write(output / "song.rplay", data);
    if (!mute) {
        auto source = game / "audio/music/bundled" /
                      (lower(entry.value("audio_id", entry.at("id").get<std::string>())) + ".mp3");
        std::string hash = sha(read(source.string()));
        fs::path cache = output / "song-audio.json", audio = output / "song.wav";
        Json previous = fs::exists(cache) ? loadJson(cache) : Json::object();
        if (!fs::exists(audio) || previous.value("source_sha256", std::string()) != hash) {
            fs::path temporary = output / "song.tmp.wav";
            require(run({"ffmpeg", "-v", "error", "-y", "-i", source.string(), "-vn", "-ar",
                         "48000", "-ac", "2", "-c:a", "pcm_s16le", temporary.string()}) == 0,
                    "ffmpeg audio decoding failed");
            fs::rename(temporary, audio);
            save(cache, {{"source_sha256", hash}, {"rate", 48000}, {"channels", 2}});
        }
    }
    Json result = {{"implementation", "C++"},
                   {"beat_span", span},
                   {"pace_offset_seconds", offset},
                   {"node_count", d.size()},
                   {"track_length", F(double(d[finish]) - d[start])},
                   {"first_beat", first},
                   {"mid_beat", mid},
                   {"audio_clock", "Consumed SDL input frames; output latency is uncalibrated."}};
    save(output / "playback-manifest.json", result);
    return result;
}
} // namespace rr
