#include "native_scene.hpp"
#include <random>

using namespace rr;
static int assertions = 0;
static void expect(bool ok, const std::string &message) {
    assertions++;
    require(ok, "test failed: " + message);
}
static void rejects(const std::function<void()> &f, const std::string &label) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    expect(rejected, label);
}
static void put(Bytes &b, size_t p, uint32_t v, int size = 4) {
    require(p + size <= b.size(), "bad fixture write");
    for (int i = 0; i < size; i++)
        b[p + i] = uint8_t(v >> (i * 8));
}
static void putFloat(Bytes &b, size_t p, float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, 4);
    put(b, p, bits);
}
static void putText(Bytes &b, size_t p, const std::string &s) {
    require(p + s.size() < b.size(), "fixture name overflow");
    std::copy(s.begin(), s.end(), b.begin() + p);
}
static Bytes modelFixture() {
    Bytes b(878);
    putText(b, 0, "EADB");
    std::array<uint32_t, 6> fields{38, 1, 1, 0, 1, 1};
    for (int i = 0; i < 6; i++)
        put(b, 4 + i * 4, fields[i]);
    std::array<uint32_t, 9> offsets{3, 128, 592, 592, 748, 748, 796, 800, 872};
    for (int i = 0; i < 9; i++)
        put(b, 64 + i * 4, offsets[i]);
    putText(b, 128, "mat");
    putText(b, 592, "body");
    Matrix m = identity;
    setBasis(m, 3, {10, 20, 30});
    for (int i = 0; i < 12; i++)
        putFloat(b, 624 + i * 4, m[i]);
    std::array<uint32_t, 6> mesh{0x30003000, 24, 0, 3, 0, 3};
    for (int i = 0; i < 6; i++)
        put(b, 748 + i * 4, mesh[i]);
    put(b, 796, 0, 2);
    put(b, 798, 0xffff, 2);
    std::array<V, 3> positions{{{0, 0, 0}, {1, 0, 0}, {0, 0, 1}}};
    for (int i = 0; i < 3; i++) {
        size_t p = 800 + i * 24;
        for (int k = 0; k < 3; k++)
            putFloat(b, p + k * 4, positions[i][k]);
        put(b, p + 14, 0x3c00, 2);
        put(b, p + 18, 0x3c00, 2);
        put(b, p + 20, 0x3400, 2);
        put(b, p + 22, 0x3a00, 2);
        put(b, 872 + i * 2, i, 2);
    }
    return b;
}
static Bytes textureFixture() {
    Bytes dds(136);
    putText(dds, 0, "DDS ");
    put(dds, 4, 124);
    put(dds, 12, 4);
    put(dds, 16, 4);
    put(dds, 28, 1);
    putText(dds, 84, "DXT1");
    put(dds, 128, 0xf800, 2);
    Bytes b(54);
    putText(b, 0, "TABT");
    std::array<uint32_t, 6> header{0xffee, 4, 54, 1, 2, uint32_t(dds.size())};
    for (int i = 0; i < 6; i++)
        put(b, 4 + i * 4, header[i]);
    std::array<uint32_t, 6> record{0, 0, uint32_t(dds.size()), 4, 4, 0};
    for (int i = 0; i < 6; i++)
        put(b, 28 + i * 4, record[i]);
    b[52] = 'x';
    b.insert(b.end(), dds.begin(), dds.end());
    return b;
}
static Bytes packed(Json profile, Json analysis) {
    std::array<std::string, 2> strings{profile.dump(), analysis.dump()};
    std::array<Bytes, 2> compressed;
    for (int i = 0; i < 2; i++) {
        strings[i].push_back(0);
        uLongf size = compressBound(strings[i].size());
        compressed[i].resize(size);
        require(compress(compressed[i].data(), &size,
                         reinterpret_cast<const Bytef *>(strings[i].data()),
                         strings[i].size()) == Z_OK,
                "fixture compression failed");
        compressed[i].resize(size);
    }
    Bytes b;
    for (uint32_t n : {0x534e4730u, uint32_t(compressed[0].size()), uint32_t(compressed[1].size()),
                       uint32_t(strings[0].size()), uint32_t(strings[1].size())})
        appendU32(b, n);
    for (auto &p : compressed)
        b.insert(b.end(), p.begin(), p.end());
    return b;
}
static void synthetic(const fs::path &dir) {
    // Sibling config keys can be empty (arrays) or duplicated. Array order and
    // first-value-wins map behavior both affect original weighted selection.
    Bytes cfg(20 + 3 * 32 + 3);
    putText(cfg, 0, "GFCF");
    put(cfg, 4, 3);
    put(cfg, 8, 3);
    put(cfg, 12, 3);
    put(cfg, 16, 1);
    put(cfg, 20, 1);
    put(cfg, 28, 1);
    put(cfg, 40, 2);
    put(cfg, 44, 2);
    putFloat(cfg, 20 + 32 + 8, 160);
    put(cfg, 20 + 32 + 20, 1);
    putFloat(cfg, 20 + 64 + 8, 200);
    put(cfg, 20 + 64 + 20, 1);
    cfg[20 + 96 + 1] = 'x';
    auto tree = config(cfg);
    expect(tree.at("x").is_array() && tree.at("x").size() == 2 && tree.at("x")[1] == 200,
           "GFCF preserves empty-key siblings");
    auto cfgBad = cfg;
    put(cfgBad, 28, 0);
    rejects([&] { config(cfgBad); }, "GFCF rejects cycles");
    cfgBad = cfg;
    cfgBad.pop_back();
    rejects([&] { config(cfgBad); }, "GFCF rejects truncated string pool");
    // A stored ZIP fixture exercises central/local boundaries and CRC checks.
    Bytes zip(30);
    put(zip, 0, 0x04034b50);
    put(zip, 4, 20, 2);
    put(zip, 14, 0x352441c2);
    put(zip, 18, 3);
    put(zip, 22, 3);
    put(zip, 26, 1, 2);
    zip.insert(zip.end(), {'x', 'a', 'b', 'c'});
    size_t central = zip.size();
    zip.resize(central + 46);
    put(zip, central, 0x02014b50);
    put(zip, central + 4, 20, 2);
    put(zip, central + 6, 20, 2);
    put(zip, central + 16, 0x352441c2);
    put(zip, central + 20, 3);
    put(zip, central + 24, 3);
    put(zip, central + 28, 1, 2);
    zip.push_back('x');
    size_t end = zip.size();
    zip.resize(end + 22);
    put(zip, end, 0x06054b50);
    put(zip, end + 8, 1, 2);
    put(zip, end + 10, 1, 2);
    put(zip, end + 12, uint32_t(end - central));
    put(zip, end + 16, uint32_t(central));
    write(dir / "stored.zip", zip);
    Archive archived((dir / "stored.zip").string());
    expect(archived.entries.size() == 1 &&
               archived.load(archived.entries[0]) == Bytes({'a', 'b', 'c'}),
           "ZIP stored payload and CRC");
    auto zipBad = zip;
    zipBad[32] ^= 1;
    write(dir / "bad-crc.zip", zipBad);
    Archive badArchive((dir / "bad-crc.zip").string());
    rejects([&] { badArchive.load(badArchive.entries[0]); }, "ZIP CRC mismatch");
    zipBad = zip;
    put(zipBad, end + 4, 1, 2);
    write(dir / "split.zip", zipBad);
    rejects([&] { Archive split((dir / "split.zip").string()); }, "ZIP split archive");
    auto bytes = modelFixture();
    auto model = decodeModel(bytes);
    expect(model.meshes.size() == 1 && model.nodes.size() == 1, "mesh count");
    expect(model.meshes[0].positions[1].x == 1, "positions");
    expect(model.meshes[0].uv[0][0] == .25f && model.meshes[0].uv[0][1] == .75f, "half UV");
    expect(model.meshes[0].indices == std::vector<uint32_t>({0, 1, 2}), "mesh indices");
    expect(model.nodes[0].matrix[9] == 10, "node transform");
    for (auto change : std::array<std::pair<int, int>, 5>{
             {{4, 37}, {92, 99999}, {752, 20}, {768, 4}, {712, 99999}}}) {
        auto bad = bytes;
        put(bad, change.first, change.second);
        rejects([&] { decodeModel(bad); }, "invalid EADB layout");
    }
    auto bad = bytes;
    put(bad, 872, 3, 2);
    rejects([&] { decodeModel(bad); }, "invalid EADB index");
    auto textureBytes = textureFixture();
    auto textures = decodeTextures(textureBytes);
    expect(textures.size() == 1 && textures[0].name == "x" && textures[0].fourcc == "DXT1",
           "TABT metadata");
    auto image = decodeDDS(textures[0]);
    expect(image.pixels.size() == 64 && image.pixels[0] == 255 && image.pixels[1] == 0 &&
               image.pixels[3] == 255,
           "DXT1 known red block");
    for (auto change :
         std::array<std::pair<int, int>, 4>{{{8, 3}, {12, 99999}, {32, 99999}, {40, 8}}}) {
        auto data = textureBytes;
        put(data, change.first, change.second);
        rejects([&] { decodeTextures(data); }, "invalid TABT bounds");
    }
    for (int n : {0, 4, 27}) {
        rejects([&] { decodeModel(Bytes(bytes.begin(), bytes.begin() + n)); }, "truncated EADB");
        rejects([&] { decodeTextures(Bytes(textureBytes.begin(), textureBytes.begin() + n)); },
                "truncated TABT");
    }
    auto broken = textures[0];
    broken.dds.pop_back();
    rejects([&] { decodeDDS(broken); }, "truncated DDS");
    expect(half(0x3c00) == 1 && half(0xb800) == -.5f, "half signs/exponents");
    rejects([] { half(0x7c00); }, "half infinity");
    auto car = vehicleBuffer(model);
    writeMesh(dir / "vehicle.rmesh", car, false);
    auto mesh = read((dir / "vehicle.rmesh").string());
    expect(mesh.size() == 40 + 3 * 44 + 3 * 4 && f32(mesh, 16) == 10 && f32(mesh, 40) == 10,
           "runtime buffer bounds");
    auto effect = model.nodes[0];
    effect.name = "electric";
    model.nodes.push_back(effect);
    exportGLTF(model, {}, dir);
    auto full = loadJson(dir / "vehicle.gltf"), clean = loadJson(dir / "vehicle-inspection.gltf");
    expect(full["nodes"].size() == 4 && clean["nodes"].size() == 2, "clean glTF excludes effects");
    expect(full["nodes"][0]["matrix"][12] == 10 && clean["nodes"][0]["children"][0] == 1,
           "glTF transforms/children");
    expect(read((dir / "vehicle.bin").string()).size() ==
               full["buffers"][0]["byteLength"].get<size_t>(),
           "glTF buffer lengths");
    for (auto view : full["bufferViews"])
        expect(view["byteOffset"].get<int>() % 4 == 0, "aligned glTF view");
    Json profile = {{"song", {{"id", "Example"}, {"title", "Test"}}}},
         analysis = {
             {"track", {{"tempo", 120}}},
             {"beats", {0, .5, 1, 1.5}},
             {"beatVols", {0, .2, .5, 1}},
             {"sections",
              Json::array({Json{{"beat", 0}, {"start", 0}, {"duration", 2}, {"loudness", .5}}})}};
    auto song = packed(profile, analysis);
    auto decoded = decodeSong(song);
    expect(decoded.first == profile && decoded.second == analysis, "SNG0 round trip");
    for (auto change :
         std::array<std::pair<int, uint32_t>, 4>{{{0, 0}, {4, 0}, {12, 0}, {16, 0xffffffffu}}}) {
        auto b = song;
        put(b, change.first, change.second);
        rejects([&] { decodeSong(b); }, "invalid SNG0 header");
    }
    auto a = analysis;
    a["beats"][2] = .25;
    rejects([&] { decodeSong(packed(profile, a)); }, "unordered beats");
    a = analysis;
    a["beatVols"][0] = 1.1;
    rejects([&] { decodeSong(packed(profile, a)); }, "invalid loudness");
    a = analysis;
    a["sections"][0]["beat"] = 1;
    rejects([&] { decodeSong(packed(profile, a)); }, "missing first section");
    a = analysis;
    a["track"]["tempo"] = 1000;
    rejects([&] { decodeSong(packed(profile, a)); }, "invalid tempo");
    song.back() ^= 1;
    rejects([&] { decodeSong(song); }, "bad compressed stream");
    std::mt19937 mt(5489);
    Random random(5489);
    bool sequence = true;
    for (int i = 0; i < 3200; i++)
        sequence &= mt() == random.integer();
    expect(sequence, "MT19937 refresh parity");
    Random r(5489);
    expect(r.integer() == 3499211612u, "MT golden first value");
    expect(songSeed("17SJWbBsrv") == 305432532u, "original case-sensitive song seed");
    expect(tempoValue(Json::array({160, 200}), 90) == 160 &&
               tempoValue(Json::array({160, 200}), 190) == 200,
           "clamped tempo interpolation");
    expect(volume({0, 1}, 0, 2, true) == F(std::sqrt(.5)), "RMS loudness");
    expect(volume({.5f, .5f}, -1, 2, true) == .5f, "negative loudness range");
    expect(volume({}, 0, 5) == 0, "empty loudness");
    std::vector<float> drops(60, .1f);
    drops[19] = drops[20] = .95f;
    expect(detectDrops(drops) == std::vector<int>({20}), "strong separated drop");
    auto m = identity;
    setBasis(m, 3, {3, 4, 5});
    expect(compose(m, m)[9] == 6 && compose(m, m)[11] == 10, "matrix composition translation");
    std::vector<V> points{{0, 0, 0}, {1, 2, 3}, {4, 8, 12}, {5, 10, 15}, {6, 12, 18}, {7, 14, 21}};
    auto p = reriff::track::evaluate(points.data(), points.size(), -2);
    expect(p.x == -2 && p.y == -4 && p.z == -6, "linear spline extrapolation");
    expect(reriff::track::evaluate(points.data(), points.size(), 100).x == 5,
           "original endpoint clamp");
    rejects([&] { reriff::track::evaluate(points.data(), 3, 1); }, "invalid spline count");
    rejects([&] { reriff::track::evaluate(points.data(), points.size(), NAN); },
            "invalid spline parameter");
    auto f = reriff::track::orientation({1, 2, 3}, {0, 1, 0});
    expect(std::abs(length(f.right) - 1) < 1e-6 && std::abs(dot(f.right, f.up)) < 1e-6,
           "orientation orthonormality");
    expect(reriff::track::orientation({0, 0, 0}, {0, 1, 0}).forward.z == 1,
           "normalization fallback");
    expect(length(reriff::track::orientation({0, 0, 1}, {0, 0, 1}).up) == 0,
           "original parallel-guide degeneracy");
    auto left = wallProfile(1, 1), right = wallProfile(-1, 1);
    expect(left[0][0] == 12 && left[0][1] == -1 && left[6][0] == 0 && left[6][1] == -8,
           "wall end profile");
    expect(left[4][0] == 12.625 && left[4][1] == .5, "wall crease profile");
    for (int i = 0; i < 7; i++)
        expect(left[i][0] == -right[i][0] && left[i][1] == right[i][1], "mirrored walls");
    std::vector<Triangle> triangles{{V{-2, 0, -2}, V{-2, 0, 2}, V{2, 0, -2}}};
    expect(clearance({0, 1, 0}, {0, 1, 0}, triangles) == 1, "support above road");
    expect(clearance({0, -1, 0}, {0, 1, 0}, triangles) == -1, "support below road");
    expect(!clearance({10, 0, 0}, {0, 1, 0}, triangles), "support miss");
    for (const auto *kind : {"curved", "uphill", "downhill", "banked", "combined"}) {
        auto road = fixture(kind);
        expect(road.parameters().size() > 10, "fixture sample count");
        auto frame = road.frame(4).second;
        expect(std::abs(length(frame.up) - 1) < 1e-5, "fixture unit normal");
        if (kind == std::string("banked"))
            expect(std::abs(frame.right.y) > .4, "banked fixture");
        if (kind == std::string("uphill"))
            expect(frame.forward.y > 0, "uphill fixture");
        if (kind == std::string("downhill"))
            expect(frame.forward.y < 0, "downhill fixture");
    }
    Json n = {{"position", {0, 0, 0}}, {"up", {0, 1, 0}}, {"widths", {1, 1}},
              {"walls", {1, 1}},       {"flags", 0},      {"visible", 1}};
    Json actual = {{"song", {{"id", "Example"}}}, {"genre", "dance"}, {"nodes", Json::array({n})}},
         ref = {{"source", "original-game"},
                {"executable_sha256", gameHash},
                {"song_id", "Example"},
                {"genre", "dance"},
                {"nodes", Json::array({n})}};
    expect(compareNodes(actual, ref)["passed"].get<bool>(), "identical reference");
    ref["nodes"][0]["position"][0] = .25;
    expect(!compareNodes(actual, ref)["passed"].get<bool>(), "reference detects geometry");
    ref["analysis_sha256"] = "wrong";
    rejects([&] { compareNodes(actual, ref); }, "reference validates inputs");
}
static void installed(const fs::path &game, const fs::path &root, const fs::path &dir) {
    if (!fs::exists(game / "vfs.zip")) {
        std::cout << "Installed-game checks skipped (assets unavailable)\n";
        return;
    }
    Archive archive((game / "vfs.zip").string());
    int configs = 0, vehicles = 0;
    for (const auto &e : archive.entries) {
        if (e.name.size() >= 4 && e.name.substr(e.name.size() - 4) == ".cfg") {
            config(archive.load(e));
            configs++;
        }
        if (e.name.rfind("graphics/vehicles/", 0) == 0 &&
            e.name.find("/lod0_livery.bdae") != std::string::npos) {
            auto model = decodeModel(archive.load(e));
            expect(!model.meshes.empty(), "installed vehicle layout");
            auto prefix = e.name.substr(0, e.name.rfind('/') + 1);
            auto textures = decodeTextures(asset(archive, prefix + "textures/batchTextures.brres"));
            expect(!textures.empty(), "installed vehicle texture batch");
            for (const auto &texture : textures) {
                auto image = decodeDDS(texture);
                expect(image.pixels.size() == size_t(texture.width) * texture.height * 4,
                       "installed DDS pixels");
                if (texture.caps)
                    for (int face = 1; face < 6; ++face)
                        require(decodeDDS(texture, face).pixels.size() == image.pixels.size(),
                                "installed cubemap face");
            }
            vehicles++;
        }
    }
    expect(configs > 0 && vehicles > 0, "installed config/vehicle inventory");
    auto songs = catalog(game, root / "NativeLinux");
    expect(songs.size() == 25, "bundled inventory");
    Json report = Json::array();
    for (const auto &e : songs) {
        auto input = loadSong(game, root / "NativeLinux", e["id"]);
        Generator generator(game, archive, input, "dance");
        auto track = generator.generate();
        expect(track["node_count"].get<int>() == int(input.analysis["beats"].size()) + 34,
               "bundled node count");
        for (const auto &n : track["nodes"])
            for (auto v : n["position"])
                require(std::isfinite(v.get<double>()), "nonfinite generated song");
        report.push_back({{"id", e["id"]},
                          {"nodes", track["node_count"]},
                          {"segments", track["segments"].size()}});
    }
    save(root / "analysis/native-port/cpp-migration/all-songs.json", report);
    auto input = loadSong(game, root / "NativeLinux", "Rolling On");
    Generator generator(game, archive, input, "dance");
    auto track = generator.generate();
    expect(track["node_count"] == 635 && track["segments"].size() == 132,
           "Rolling On golden count");
    expect(track["start_node"] == 10 && track["finish_node"] == 610, "song boundary nodes");
    auto original = root / "analysis/native-port/original-rolling-on/original-nodes.json";
    if (fs::exists(original))
        expect(compareNodes(track, loadJson(original))["passed"].get<bool>(),
               "full original node comparison");
    auto previous = root / "analysis/native-port/song-17sjwbbsrv-dance-alf/song-nodes.json";
    if (fs::exists(previous)) {
        auto old = loadJson(previous);
        if (old.at("song").at("id") == input.profile.at("song").at("id") &&
            old.at("genre") == "dance") {
            expect(track["nodes"] == old["nodes"], "migration preserves all node values");
            for (size_t i = 0; i < track["segments"].size(); i++)
                for (const auto *key : {"start", "end", "type", "params", "section"})
                    expect(track["segments"][i][key] == old["segments"][i][key],
                           "migration preserves segment parameters");
        }
    }
    auto model = decodeModel(asset(archive, "graphics/vehicles/alf/lod0_livery.bdae"));
    for (const auto *kind : {"curved", "uphill", "downhill", "banked", "combined"}) {
        auto manifest = buildRoad(fixture(kind), model, Json::object(), dir / kind);
        expect(manifest["validation"]["degenerate_triangles"] == 0 &&
                   manifest["validation"]["inverted_face_count"] == 0,
               "installed fixture geometry");
    }
    auto straight = dir / "straight";
    auto textures = sceneTextures(archive, "dance", straight, true);
    auto rampScene = buildStraight(archive, "dance", model,
                                   textures["texture_layers"]["ramp_mapping"], straight);
    expect(rampScene["ramp"]["included"].get<bool>() && rampScene["road"]["triangles"] == 192,
           "straight road and original ramp pipeline");
    std::cout << "Validated " << songs.size() << " generated songs, " << vehicles << " vehicles, "
              << configs << " configs and five road fixtures\n";
}
int main() {
    auto root = fs::canonical("/proc/self/exe").parent_path().parent_path().parent_path();
    auto dir = root / "build/native-test-output";
    try {
        fs::create_directories(dir);
        synthetic(dir);
        auto game = fs::path(
            std::getenv("RERIFF_GAME_DIR")
                ? std::getenv("RERIFF_GAME_DIR")
                : (fs::path(std::getenv("HOME")) / ".local/share/Steam/steamapps/common/Riff Racer")
                      .c_str());
        installed(game, root, dir);
        std::cout << assertions << " C++ assertions passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
