#include "native_scene.hpp"

int main(int argc, char **argv) {
    try {
        using namespace rr;
        fs::path exe = fs::canonical("/proc/self/exe"),
                 root = exe.parent_path().parent_path().parent_path(),
                 source = root / "NativeLinux";
        fs::path game = fs::path(std::getenv("HOME") ? std::getenv("HOME") : "") /
                        ".local/share/Steam/steamapps/common/Riff Racer",
                 output, reference, screenshot;
        std::string song, genre = "dance", vehicle = "alf", section = "straight";
        bool list = false, trackMode = false, play = false, mute = false, overview = false,
             prepareOnly = false, nodesOnly = false;
        double time = 0;
        int frames = 0;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--help") {
                std::cout << "Usage: reriff [--game-dir DIR] [--vehicle alf] [--song TITLE|ID] "
                             "[--play] [--mute] [--time SECONDS]\n  [--track] [--section "
                             "straight|curved|uphill|downhill|banked|combined] [--genre "
                             "dance|rock|hiphop]\n  [--overview] [--reference-nodes JSON] "
                             "[--screenshot IMAGE.ppm] [--frames N]\n  [--prepare-only] "
                             "[--nodes-only] [--list-songs] [--output DIR]\nAll asset loading, "
                             "generation, scene building and playback preparation run in C++.\n";
                return 0;
            }
            if (arg == "--list-songs") {
                list = true;
                continue;
            }
            if (arg == "--track") {
                trackMode = true;
                continue;
            }
            if (arg == "--play") {
                play = true;
                continue;
            }
            if (arg == "--drive" || arg == "--drive-demo")
                throw std::runtime_error(
                    "Prototype physics removed. Original vehicle physics integration is not ported "
                    "yet. Use --song 'Rolling On' --play to inspect recovered track and assets.");
            if (arg == "--mute") {
                mute = true;
                continue;
            }
            if (arg == "--overview") {
                overview = true;
                continue;
            }
            if (arg == "--prepare-only") {
                prepareOnly = true;
                continue;
            }
            if (arg == "--nodes-only") {
                nodesOnly = true;
                prepareOnly = true;
                continue;
            }
            require(i + 1 < argc, "missing option value for " + arg);
            std::string value = argv[++i];
            if (arg == "--reference-nodes")
                reference = value;
            else if (arg == "--game-dir")
                game = value;
            else if (arg == "--output")
                output = value;
            else if (arg == "--song")
                song = value;
            else if (arg == "--genre")
                genre = value;
            else if (arg == "--vehicle")
                vehicle = value;
            else if (arg == "--section") {
                section = value;
                trackMode = true;
            } else if (arg == "--screenshot")
                screenshot = value;
            else if (arg == "--time")
                time = std::stod(value);
            else if (arg == "--frames") {
                frames = std::stoi(value);
                require(frames > 0, "frames must be positive");
            } else
                throw std::runtime_error("unknown option: " + arg);
        }
        require(genre == "dance" || genre == "rock" || genre == "hiphop", "invalid genre");
        require(std::isfinite(time) && time >= 0 && time <= 86400,
                "time must be between 0 and 86400");
        // A selected real song starts its music and preview without requiring
        // an extra --play flag. Explicit fixtures use Rolling On as reference audio.
        if (!song.empty())
            play = true;
        require((!play && !mute && !time && reference.empty()) || !song.empty(),
                "playback/reference options require --song");
        require(reference.empty() || !song.empty(), "reference-nodes requires a bundled song");
        require((!mute && !time) || play, "mute/time require --play");
        if (list) {
            for (const auto &e : catalog(game, source))
                std::cout << e.at("id").get<std::string>() << "\t"
                          << e.at("title").get<std::string>() << " — "
                          << e.at("artist").get<std::string>() << '\n';
            return 0;
        }
        Archive archive((game / "vfs.zip").string());
        Json generated, report;
        std::optional<SongInput> input;
        if (!song.empty()) {
            trackMode = true;
            input = loadSong(game, source, song);
            Generator generator(game, archive, *input, genre);
            generated = generator.generate();
            std::cout << "Generated " << generator.count << " nodes, " << generator.segments.size()
                      << " segments in C++\n";
            if (!reference.empty()) {
                auto comparison = compareNodes(generated, loadJson(reference));
                comparison["reference_sha256"] = sha(read(reference.string()));
                require(comparison["passed"].get<bool>(),
                        "original node comparison failed: " + comparison.dump());
                generated["parity"] = {{"full_original_node_dump_compared", true},
                                       {"comparison", comparison}};
                std::cout << comparison.dump(2) << '\n';
            }
        }
        if (output.empty())
            output = root / "analysis/native-port" /
                     (!song.empty() ? "song-" + lower(input->entry["id"].get<std::string>()) + "-" +
                                          genre + "-" + vehicle
                      : trackMode ? "track-" + genre + "-" + vehicle +
                                        (section == "straight" ? "" : "-" + section)
                                  : "vehicle-" + vehicle);
        output = fs::absolute(output);
        fs::create_directories(output);
        if (input) {
            save(output / "song-nodes.json", generated);
            save(output / "song-analysis.json", input->analysis);
            save(output / "song-profile.json", input->profile);
            if (!reference.empty())
                save(output / "original-comparison.json", generated["parity"]["comparison"]);
        }
        if (nodesOnly)
            return 0;
        auto vehicleOut = trackMode ? output / "vehicle" : output;
        auto model = exportVehicle(archive, vehicle, vehicleOut);
        auto car = vehicleBuffer(model);
        if (trackMode) {
            report = sceneTextures(archive, genre, output, section == "straight" && song.empty());
            Road road = song.empty() ? fixture(section) : songRoad(generated);
            auto geometry = song.empty() && section == "straight"
                                ? buildStraight(archive, genre, model,
                                                report["texture_layers"]["ramp_mapping"], output)
                                : buildRoad(road, model, generated, output);
            report.update(geometry);
            report["genre"] = genre;
            if (input) {
                report["song"] = input->profile["song"];
                report["segments"] = generated["segments"].size();
            }
            save(output / "scene-manifest.json", report);
            std::cout << "Prepared " << report["total_triangles"] << " scene triangles in C++\n";
            if (play) {
                preparePlayback(archive, game, output, generated, input->analysis, car,
                                mute || !screenshot.empty());
            }
        }
        if (prepareOnly) {
            std::cout << "Output: " << output << '\n';
            return 0;
        }
        std::vector<std::string> command{(exe.parent_path() / "reriff-viewer").string(),
                                         (output / (play        ? "playback-scene.rmesh"
                                                    : trackMode ? "scene.rmesh"
                                                                : "vehicle.rmesh"))
                                             .string()};
        if (trackMode && !overview) {
            auto focus = report.at("viewer_focus");
            command.insert(command.end(),
                           {"--focus",
                            std::to_string(focus[0].get<float>()) + "," +
                                std::to_string(focus[1].get<float>()) + "," +
                                std::to_string(focus[2].get<float>()),
                            "--distance", std::to_string(report.at("viewer_distance").get<float>()),
                            "--elevation",
                            std::to_string(report.at("viewer_elevation").get<float>())});
            for (size_t i = 1; i < report["vehicles"].size(); i++) {
                auto p = report["vehicles"][i]["position"];
                command.insert(command.end(),
                               {"--focus", std::to_string(p[0].get<float>()) + "," +
                                               std::to_string(p[1].get<float>() + 1) + "," +
                                               std::to_string(p[2].get<float>())});
            }
        }
        if (play) {
            command.insert(command.end(), {"--playback", (output / "song.rplay").string(),
                                           "--playback-time", std::to_string(time)});
            if (!mute && screenshot.empty())
                command.insert(command.end(), {"--audio", (output / "song.wav").string()});
            if (overview)
                command.push_back("--no-follow");
        }

        if (!screenshot.empty())
            command.insert(command.end(), {"--screenshot", fs::absolute(screenshot).string()});
        if (frames)
            command.insert(command.end(), {"--frames", std::to_string(frames)});
        std::cout.flush();
        return run(command);
    } catch (const std::exception &e) {
        std::cerr << "reriff: " << e.what() << '\n';
        return 1;
    }
}
