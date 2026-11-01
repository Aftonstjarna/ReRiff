#pragma once
#include "native_song.hpp"
#include "playback.hpp"
#include <chrono>
#include <fcntl.h>
#include <random>

namespace rr {
inline std::string windows(const fs::path &path) {
    auto s = fs::absolute(path).string();
    std::replace(s.begin(), s.end(), '/', '\\');
    return "Z:" + s;
}
inline bool within(const fs::path &path, const fs::path &parent) {
    auto p = fs::weakly_canonical(path), root = fs::weakly_canonical(parent);
    auto a = p.begin(), b = root.begin();
    for (; b != root.end(); a++, b++)
        if (a == p.end() || *a != *b)
            return false;
    return true;
}
inline Json preparePE(const fs::path &game, const fs::path &root) {
    auto original = read((game / "Game.exe").string());
    require(sha(original) == gameHash, "unsupported Game.exe hash");
    auto data = original;
    Json patches = Json::array();
    for (auto patch :
         std::array<std::array<uint32_t, 3>, 3>{{{358, 2, 8482}, {376, 4, 0}, {5866632, 4, 0}}}) {
        auto [offset, size, value] = patch;
        uint32_t before = size == 2 ? u16(data, offset) : u32(data, offset);
        patches.push_back({{"offset", offset}, {"before", before}, {"after", value}});
        for (uint32_t i = 0; i < size; i++)
            data[offset + i] = uint8_t(value >> (8 * i));
    }
    auto output = root / "build/song-reference";
    write(output / "Game.reference.dll", data);
    Json report = {{"original_sha256", gameHash},
                   {"reference_sha256", sha(data)},
                   {"patches", patches},
                   {"method_bodies_changed", false}};
    save(output / "reference-header-patches.json", report);
    return report;
}
inline Json referenceNodes(const Bytes &raw, const Json &native) {
    require(!raw.empty() && raw.size() % 244 == 0, "invalid original node vector");
    Json nodes = Json::array();
    for (size_t i = 0; i < raw.size() / 244; i++) {
        size_t p = i * 244;
        auto v = [&](size_t offset, int size) {
            Json out = Json::array();
            for (int k = 0; k < size; k++)
                out.push_back(f32(raw, p + offset + k * 4));
            return out;
        };
        nodes.push_back({{"index", i},
                         {"position", v(172, 3)},
                         {"up", v(148, 3)},
                         {"widths", v(208, 2)},
                         {"walls", v(216, 2)},
                         {"visible", raw[p + 228]},
                         {"flags", u32(raw, p + 240)},
                         {"segment", int32_t(u32(raw, p + 224))},
                         {"section", int32_t(u32(raw, p + 232))}});
    }
    return {{"source", "original-game"},
            {"capture_method", "Offline Microsoft CLR host; original generation methods unchanged; "
                               "read-only config I/O bridge"},
            {"executable_sha256", gameHash},
            {"song_id", native["song"]["id"]},
            {"genre", native["genre"]},
            {"analysis_sha256", native["analysis_sha256"]},
            {"config_sha256", native["config_sha256"]},
            {"nodes", nodes}};
}
inline Json compareSegments(const Json &native, const fs::path &file) {
    std::ifstream in(file);
    require(bool(in), "missing original segments");
    std::string line;
    int count = 0;
    Json mismatches = Json::array();
    const auto &segments = native.at("segments");
    while (std::getline(in, line)) {
        std::istringstream row(line);
        int id, start, end, type, unused;
        std::array<float, 4> params{};
        require(bool(row >> id >> start >> end >> type >> unused >> params[0] >> params[1] >>
                     params[2] >> params[3]),
                "invalid original segment row");
        if (size_t(count) < segments.size()) {
            auto &s = segments[count];
            if (start != s.at("start").get<int>() || end != s.at("end").get<int>() ||
                type != s.at("type").get<int>() ||
                params != s.at("params").get<std::array<float, 4>>())
                mismatches.push_back(count);
        }
        count++;
    }
    return {{"passed", size_t(count) == segments.size() && mismatches.empty()},
            {"segment_counts", {segments.size(), size_t(count)}},
            {"segment_mismatches", mismatches}};
}
inline Json compareTiming(const fs::path &playback, const fs::path &reference) {
    auto b = read(playback.string()), original = read((reference / "original-nodes.bin").string());
    require(b.size() >= 40 && std::memcmp(b.data(), "RRPLAY1\0", 8) == 0,
            "invalid playback buffer");
    uint32_t count = u32(b, 8);
    require(count >= 4 && count < 100000 && b.size() == 40 + uint64_t(count) * 32 &&
                original.size() == uint64_t(count) * 244,
            "invalid timing counts");
    reriff::SongPath native, exact;
    native.start = exact.start = u32(b, 12);
    native.finish = exact.finish = u32(b, 16);
    native.beatSpan = exact.beatSpan = f32(b, 28);
    require(native.start < native.finish && native.finish < count && native.beatSpan > 0,
            "invalid timing header");
    for (uint32_t i = 0; i < count; i++) {
        native.distances.push_back(f32(b, 40 + i * 32 + 24));
        exact.distances.push_back(f32(original, i * 244 + 24));
    }
    std::ifstream samples(reference / "original-timing.tsv");
    require(bool(samples), "original timing capture missing");
    float time, expected, distance;
    double error = 0, exactError = 0;
    int n = 0;
    while (samples >> time >> expected >> distance) {
        error = std::max(error, double(std::abs(native.parameter(time) - expected)));
        exactError = std::max(exactError, double(std::abs(exact.parameter(time) - expected)));
        n++;
    }
    require(n >= 100, "incomplete original timing samples");
    return {{"passed", error < .001 && exactError == 0},
            {"linux_nodes",
             {{"samples", n},
              {"max_parameter_error", error},
              {"tolerance", .001},
              {"passed", error < .001}}},
            {"original_distances",
             {{"samples", n}, {"max_parameter_error", exactError}, {"passed", exactError == 0}}},
            {"reference_samples_sha256", sha(read((reference / "original-timing.tsv").string()))},
            {"playback_sha256", sha(b)}};
}
inline Json captureSong(const fs::path &game, const fs::path &root, const std::string &song,
                        const std::string &genre, fs::path output, const fs::path &prefix,
                        const fs::path &wine, const fs::path &dotnet) {
    auto clr = prefix / "drive_c/windows/Microsoft.NET/Framework/v4.0.30319";
    require(fs::exists(clr / "clr.dll") && fs::exists(clr / "mscorlib.dll"),
            "existing Microsoft .NET prefix required");
    require(fs::canonical(prefix / "dosdevices/z:") == fs::path("/"),
            "standard Z: mapping required");
    Archive archive((game / "vfs.zip").string());
    auto input = loadSong(game, root / "NativeLinux", song);
    Generator generator(game, archive, input, genre);
    auto native = generator.generate();
    std::string id = lower(input.entry["id"].get<std::string>());
    if (output.empty()) {
        output = root / "analysis/native-port" / ("original-" + id + "-" + genre);
    }
    output = fs::absolute(output);
    require(within(output, root / "analysis"), "reference output must be inside project analysis/");
    fs::create_directories(output);
    auto build = root / "build/song-reference", inputs = build / ("input-" + id + "-" + genre);
    save(inputs / "profile.json", input.profile);
    save(inputs / "analysis.json", input.analysis);
    std::string g = genre + '\n';
    write(inputs / "genre.txt", Bytes(g.begin(), g.end()));
    std::ostringstream timing;
    timing << std::setprecision(17);
    for (const char *key : {"audible_start", "first_beat", "mid_beat"})
        timing << input.entry.at(key).get<double>() << '\n';
    std::string times = timing.str();
    write(inputs / "timing.txt", Bytes(times.begin(), times.end()));
    for (const char *name : {"segments_PC.cfg", "sections_PC.cfg", "visualSections.cfg"}) {
        write(inputs / name, asset(archive, std::string("scripts/config/") + name));
    }
    auto vehicleConfigBytes = asset(archive, "scripts/config/vehicles_PC.cfg");
    auto vehicleConfig = config(vehicleConfigBytes).at("carParams").at("base");
    std::ostringstream friction;
    friction << std::setprecision(9);
    for (const char *key : {"Grip", "GripSlipCone", "GripDirt", "GripSand", "GripGrass",
                            "GripTarmac", "GripIce", "GripWet", "GripDrift2", "GripDrift3",
                            "GripBrake", "GripLiftOff", "DriftSlow", "BrakeSlow", "SandSlow"})
        friction << float(number(vehicleConfig, key)) << '\n';
    auto frictionText = friction.str();
    write(inputs / "vehicle-friction.txt", Bytes(frictionText.begin(), frictionText.end()));
    std::ostringstream inputTuning;
    inputTuning << std::setprecision(9);
    for (const char *key :
         {"SteerOnset", "DriftDelay", "DriftOnset", "DriftRelease", "DriftHoldTrig", "Mass"})
        inputTuning << float(number(vehicleConfig, key)) << '\n';
    auto inputText = inputTuning.str();
    write(inputs / "vehicle-input.txt", Bytes(inputText.begin(), inputText.end()));
    std::ostringstream suspensionTuning;
    suspensionTuning << std::setprecision(9) << float(number(vehicleConfig, "SuspStiffness"))
                     << '\n'
                     << float(number(vehicleConfig, "SuspDamping")) << '\n';
    auto suspensionText = suspensionTuning.str();
    write(inputs / "vehicle-suspension.txt", Bytes(suspensionText.begin(), suspensionText.end()));
    save(output / "vehicle-config.json", {{"source", "scripts/config/vehicles_PC.cfg"},
                                          {"sha256", sha(vehicleConfigBytes)},
                                          {"preset", "base"},
                                          {"parameters", vehicleConfig}});
    auto headers = preparePE(game, root);
    fs::path sdkRoot = dotnet.parent_path() / "sdk", compiler;
    for (const auto &d : fs::directory_iterator(sdkRoot)) {
        auto candidate = d.path() / "Roslyn/bincore/csc.dll";
        if (fs::exists(candidate) && candidate > compiler)
            compiler = candidate;
    }
    require(!compiler.empty(), ".NET SDK compiler missing");
    auto host = build / "OriginalSongCapture.exe",
         source = root / "NativeLinux/reference/OriginalSongCapture.cs";
    setenv("DOTNET_ROOT", dotnet.parent_path().c_str(), 1);
    require(run({dotnet.string(), compiler.string(), "-nologo", "-unsafe", "-target:exe",
                 "-platform:x86", "-nostdlib", "-r:" + (clr / "mscorlib.dll").string(),
                 "-out:" + host.string(), source.string(),
                 (root / "NativeLinux/reference/OriginalVehicleCapture.cs").string()}) == 0,
            "capture host compilation failed");
    setenv("WINEPREFIX", fs::absolute(prefix).c_str(), 1);
    setenv("WINEDEBUG", "-all", 1);
    setenv("WINEDLLOVERRIDES", "winemenubuilder.exe=d;mscoree=n", 1);
    unsetenv("WINEARCH");
    auto log = output / "capture.log";
    pid_t pid = fork();
    require(pid >= 0, "capture fork failed");
    if (pid == 0) {
        int fd = open(log.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0600);
        if (fd < 0)
            _exit(126);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
        _exit(run({wine.string(), windows(host), windows(build / "Game.reference.dll"),
                   windows(game), windows(inputs), windows(output)}));
    }
    int status = 0;
    require(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "original capture failed; inspect " + log.string());
    auto logs = read(log.string());
    require(std::string(logs.begin(), logs.end()).find("RESULT=SONG_CAPTURE_OK") !=
                std::string::npos,
            "incomplete capture");
    auto ref = referenceNodes(read((output / "original-nodes.bin").string()), native);
    save(output / "original-nodes.json", ref);
    save(output / "linux-nodes.json", native);
    auto report = compareNodes(native, ref),
         segments = compareSegments(native, output / "original-segments.tsv");
    bool passed = report["passed"].get<bool>() && segments["passed"].get<bool>();
    report.update(segments);
    report["passed"] = passed;
    report["header_changes"] = headers;
    report["native_host_source_sha256"] = sha(read(source.string()));
    report["vehicle_host_source_sha256"] =
        sha(read((root / "NativeLinux/reference/OriginalVehicleCapture.cs").string()));
    report["reference_sha256"] = sha(read((output / "original-nodes.json").string()));
    report["original_installation_hash_unchanged"] =
        sha(read((game / "Game.exe").string())) == gameHash;
    std::string logText(logs.begin(), logs.end());
    auto runtimeAt = logText.find("MONO_RUNTIME=");
    require(runtimeAt != std::string::npos, "Reference runtime evidence missing");
    auto runtimeEnd = logText.find_first_of("\r\n", runtimeAt);
    save(output / "behavior-provenance.json",
         {{"schema", 1},
          {"game_sha256", gameHash},
          {"method_bodies_changed", headers.at("method_bodies_changed")},
          {"capture_sha256", sha(read((output / "original-behavior.tsv").string()))},
          {"vehicle_config_sha256", sha(vehicleConfigBytes)},
          {"host_source_sha256", report.at("native_host_source_sha256")},
          {"vehicle_host_source_sha256", report.at("vehicle_host_source_sha256")},
          {"runtime", logText.substr(runtimeAt, runtimeEnd - runtimeAt)},
          {"scope", "Isolated unchanged original routines with supplied object state; not a "
                    "rigid-body simulation"}});
    save(output / "comparison.json", report);
    return report;
}
// Direct original x86 primitives. PE mapping is local and hash-gated; imported
// sqrt alone is supplied by the existing standalone harness, not Windows startup.
inline fs::path buildMathReference(const fs::path &game, const fs::path &root) {
    auto b = read((game / "Game.exe").string());
    require(sha(b) == gameHash, "unsupported Game.exe hash");
    size_t pe = u32(b, 0x3c);
    require(u32(b, pe) == 0x4550, "invalid PE signature");
    uint16_t count = u16(b, pe + 6), optional = u16(b, pe + 20);
    size_t opt = pe + 24;
    uint32_t size = u32(b, opt + 56), headers = u32(b, opt + 60);
    require(size <= 64 * 1024 * 1024 && headers <= b.size() && headers <= size,
            "invalid PE bounds");
    Bytes image(size);
    std::copy_n(b.begin(), headers, image.begin());
    size_t sections = opt + optional;
    for (uint16_t i = 0; i < count; i++) {
        size_t p = sections + i * 40;
        uint32_t va = u32(b, p + 12), length = u32(b, p + 16), offset = u32(b, p + 20);
        require(va <= size && length <= size - va && offset <= b.size() &&
                    length <= b.size() - offset,
                "invalid PE section");
        std::copy_n(b.begin() + offset, length, image.begin() + va);
    }
    fs::path dir = root / "build/native-math-reference";
    write(dir / "image.bin", image);
    require(run({"objcopy", "-I", "binary", "-O", "elf32-i386", "-B", "i386",
                 (dir / "image.bin").string(), (dir / "image.o").string()}) == 0,
            "objcopy failed");
    require(run({"as", "--32", (root / "NativeLinux/reference/original_math.s").string(), "-o",
                 (dir / "harness.o").string()}) == 0,
            "assembler failed");
    // The linker script uses image.o/harness.o relative to its working directory.
    auto old = fs::current_path();
    fs::current_path(dir);
    int result = run({"ld", "-m", "elf_i386", "-T",
                      (root / "NativeLinux/reference/original_math.ld").string(), "harness.o",
                      "image.o", "-o", "original-math"});
    fs::current_path(old);
    require(result == 0, "reference linker failed");
    return dir / "original-math";
}
inline Bytes originalCall(const fs::path &exe, const Bytes &input, size_t size) {
    int in[2], out[2];
    require(pipe(in) == 0 && pipe(out) == 0, "reference pipe failed");
    pid_t pid = fork();
    require(pid >= 0, "reference fork failed");
    if (pid == 0) {
        dup2(in[0], 0);
        dup2(out[1], 1);
        close(in[0]);
        close(in[1]);
        close(out[0]);
        close(out[1]);
        execl(exe.c_str(), exe.c_str(), nullptr);
        _exit(127);
    }
    close(in[0]);
    close(out[1]);
    // Packets fit in a Linux pipe; largest song packets need concurrent output
    // only after input closes. Write and read fully to handle short syscalls.
    size_t written = 0;
    while (written < input.size()) {
        ssize_t n = ::write(in[1], input.data() + written, input.size() - written);
        require(n > 0, "reference input failed");
        written += size_t(n);
    }
    close(in[1]);
    Bytes result(size);
    size_t used = 0;
    while (used < size) {
        ssize_t n = ::read(out[0], result.data() + used, size - used);
        if (n <= 0)
            break;
        used += size_t(n);
    }
    close(out[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    require(WIFEXITED(status) && WEXITSTATUS(status) == 0 && used == size,
            "original reference failed; 32-bit Linux syscalls may be restricted");
    return result;
}
inline Json verifyMath(const fs::path &game, const fs::path &root, const fs::path &nodes,
                       int cases) {
    auto exe = buildMathReference(game, root);
    Json report = {{"executable_sha256", gameHash},   {"rng_values_compared", 0},
                   {"matrix_products_compared", 0},   {"spline_evaluations_compared", 0},
                   {"orientation_cases_compared", 0}, {"bitwise_matches", 0}};
    int matches = 0;
    for (uint32_t seed : {0u, 1u, 5489u, 0xffffffffu, 0x12345678u})
        for (uint32_t mode = 0; mode < 4; mode++) {
            Random rng(seed);
            Bytes packet;
            for (uint32_t n : {4u, seed, 1600u, mode})
                appendU32(packet, n);
            if (mode == 1) {
                appendFloat(packet, -2.5f);
                appendFloat(packet, 7.25f);
            } else {
                appendU32(packet, mode == 2 ? uint32_t(-20) : 0);
                appendU32(packet, mode == 2 ? 35 : 0);
            }
            Bytes actual;
            for (int i = 0; i < 1600; i++) {
                if (mode == 0)
                    appendU32(actual, rng.integer());
                else if (mode == 1)
                    appendFloat(actual, rng.uniform(-2.5f, 7.25f));
                else if (mode == 2)
                    appendU32(actual, uint32_t(rng.between(-20, 35)));
                else
                    appendFloat(actual, rng.sign());
            }
            require(actual == originalCall(exe, packet, actual.size()), "original RNG mismatch");
            report["rng_values_compared"] = report["rng_values_compared"].get<int>() + 1600;
            matches += 1600;
        }
    std::mt19937 rng(91);
    auto uniform = [&](float a, float b) {
        return F(double(a) + double(rng()) / 4294967296. * (double(b) - a));
    };
    for (int i = 0; i < 100; i++) {
        Matrix a{}, b{};
        for (float &f : a)
            f = uniform(-20, 20);
        for (float &f : b)
            f = uniform(-20, 20);
        Bytes packet, actual;
        for (uint32_t v : {5u, 0u, 0u, 0u})
            appendU32(packet, v);
        for (auto m : {a, b})
            for (float f : m)
                appendFloat(packet, f);
        for (float f : compose(a, b))
            appendFloat(actual, f);
        require(actual == originalCall(exe, packet, 48), "original matrix mismatch");
        matches++;
    }
    report["matrix_products_compared"] = 100;
    std::vector<V> songPoints;
    if (!nodes.empty())
        for (const auto &n : loadJson(nodes).at("nodes"))
            songPoints.push_back(vec(n.at("position")));
    int splines = 0, orientations = 0;
    for (int i = 0; i < cases; i++) {
        std::vector<V> points;
        for (int j = 0; j < 9; j++)
            points.push_back({uniform(-200, 200), uniform(-200, 200), uniform(-200, 200)});
        if (!songPoints.empty() && i < 100)
            points = songPoints;
        float t = uniform(1, F(points.size() - 3.001));
        if (songPoints.empty()) {
            std::array<float, 8> special{-1, 0, 1, 1.0001f, 2, 5.999f, 6, 100};
            if (i < 8)
                t = special[i];
        }
        V forward{uniform(-2, 2), uniform(-2, 2), uniform(-2, 2)},
            up{uniform(-2, 2), uniform(-2, 2), uniform(-2, 2)};
        if (i == 0) {
            forward = {0, 0, 0};
            up = {0, 0, 0};
        }
        if (i == 1) {
            forward = {0, 0, 1};
            up = {0, 0, 1};
        }
        if (i == 2)
            forward = {1e-7f, 0, 0};
        for (int mode = 1; mode <= 3; mode++) {
            Bytes packet, actual;
            appendU32(packet, mode);
            appendU32(packet, points.size());
            appendFloat(packet, t);
            appendU32(packet, 0);
            for (V v : {forward, up})
                for (int k = 0; k < 3; k++)
                    appendFloat(packet, v[k]);
            for (V v : points)
                for (int k = 0; k < 3; k++)
                    appendFloat(packet, v[k]);
            if (mode < 3) {
                V result = reriff::track::evaluate(points.data(), points.size(), t, mode == 2);
                for (int k = 0; k < 3; k++)
                    appendFloat(actual, result[k]);
                splines++;
            } else {
                auto f = reriff::track::orientation(forward, up);
                for (V v : {f.right, f.up, f.forward})
                    for (int k = 0; k < 3; k++)
                        appendFloat(actual, v[k]);
                orientations++;
            }
            require(actual == originalCall(exe, packet, actual.size()),
                    "original spline/orientation mismatch at case " + std::to_string(i));
            matches++;
        }
    }
    report["spline_evaluations_compared"] = splines;
    report["orientation_cases_compared"] = orientations;
    report["bitwise_matches"] = matches;
    report["passed"] = true;
    return report;
}
} // namespace rr
