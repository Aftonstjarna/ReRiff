#include "native_reference.hpp"
int main(int argc, char **argv) {
    try {
        using namespace rr;
        require(argc >= 2,
                "usage: reriff-reference capture|prepare-pe|compare|timing|math [options]");
        fs::path root = fs::canonical("/proc/self/exe").parent_path().parent_path().parent_path(),
                 home = std::getenv("HOME");
        fs::path game = home / ".local/share/Steam/steamapps/common/Riff Racer",
                 prefix = root / "build/song-reference/dotnet452-proton1102-win64",
                 wine = home / ".local/share/Steam/steamapps/common/Proton 11.0/files/bin/wine",
                 dotnet = "/tmp/reriff-dotnet/dotnet", output,
                 reference = root / "analysis/native-port/original-rolling-on", nodes,
                 playback = root / "analysis/native-port/song-17sjwbbsrv-dance-alf/song.rplay";
        std::string mode = argv[1], song = "Rolling On", genre = "dance";
        int cases = 400;
        double tolerance = .01;
        for (int i = 2; i < argc; i++) {
            std::string key = argv[i];
            require(i + 1 < argc, "missing argument");
            std::string v = argv[++i];
            if (key == "--game-dir")
                game = v;
            else if (key == "--prefix")
                prefix = v;
            else if (key == "--wine")
                wine = v;
            else if (key == "--dotnet")
                dotnet = v;
            else if (key == "--output")
                output = v;
            else if (key == "--reference")
                reference = v;
            else if (key == "--nodes")
                nodes = v;
            else if (key == "--playback")
                playback = v;
            else if (key == "--song")
                song = v;
            else if (key == "--genre")
                genre = v;
            else if (key == "--cases")
                cases = std::stoi(v);
            else if (key == "--tolerance")
                tolerance = std::stod(v);
            else
                throw std::runtime_error("unknown option " + key);
        }
        Json report;
        if (mode == "capture")
            report = captureSong(game, root, song, genre, output, prefix, wine, dotnet);
        else if (mode == "prepare-pe") {
            require(output.empty(),
                    "prepare-pe uses the fixed ignored build/song-reference location");
            report = preparePE(game, root);
        } else if (mode == "compare") {
            require(!nodes.empty(), "--nodes JSON required");
            require(std::isfinite(tolerance) && tolerance > 0 && tolerance <= 1,
                    "tolerance must be in (0,1]");
            report = compareNodes(loadJson(nodes), loadJson(reference), tolerance);
            report["reference_sha256"] = sha(read(reference.string()));
        } else if (mode == "timing") {
            report = compareTiming(playback, reference);
            if (output.empty())
                output = playback.parent_path() / "timing-comparison.json";
        } else if (mode == "math") {
            require(cases > 0 && cases <= 10000, "cases must be 1..10000");
            report = verifyMath(game, root, nodes, cases);
            if (output.empty())
                output = root / "analysis/native-port/cpp-primitive-parity.json";
        } else
            throw std::runtime_error("unknown reference mode");
        if (!output.empty() && mode != "capture") {
            save(output, report);
        }
        std::cout << report.dump(2) << '\n';
        return report.value("passed", true) ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << "reriff-reference: " << e.what() << '\n';
        return 1;
    }
}
