// Exact-source song identity registry. No gameplay or cache mutation.
#include "third_party/json.hpp"
#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <cctype>
using nlohmann::json;

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

static bool identifier(const std::string& id) {
    return !id.empty() && id.size() <= 128 &&
        id.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos;
}

// Caller owns the output buffer. Return required bytes including the NUL.
// Size probing and actual evaluation have no side effects.
extern "C" EXPORT size_t reriff_catalog(const char* input, char* output, size_t capacity) noexcept {
    std::string result;
    try {
        const auto request = json::parse(input);
        auto records = request.at("records");
        const auto songs = request.at("songs");
        if (!records.is_object() || !songs.is_array() || songs.size() > 10000)
            throw std::runtime_error("Invalid song manifest");
        for (const auto& song : songs) {
            const auto id = song.at("song_id").get<std::string>();
            auto hash = song.at("sha256").get<std::string>();
            if (!identifier(id) || hash.size() != 64 ||
                hash.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
                throw std::runtime_error("Invalid song ID or SHA256");
            for (auto& c : hash) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            // A cache ID that changes source must not silently move leaderboards.
            if (records.contains(id) && records.at(id).at("sha256") != hash)
                throw std::runtime_error("Song ID already registered to different audio: " + id);
            auto record = song;
            record["sha256"] = hash;
            for (const auto* key : {"title", "artist"}) {
                if (!record.contains(key) || !record[key].is_string() || record[key].get<std::string>().size() > 4096)
                    throw std::runtime_error("Invalid song metadata");
            }
            // Store only identity and display metadata, never client paths.
            records[id] = {{"sha256", hash}, {"title", record["title"]}, {"artist", record["artist"]}};
        }
        std::map<std::string, std::string> roots;
        json aliases = json::object();
        // Object keys are sorted: canonical choice is deterministic.
        for (auto it = records.begin(); it != records.end(); ++it) {
            const auto hash = it.value().at("sha256").get<std::string>();
            auto inserted = roots.emplace(hash, it.key());
            aliases[it.key()] = inserted.first->second;
        }
        result = json({{"records", records}, {"aliases", aliases}}).dump();
    } catch (const std::exception& e) {
        result = json({{"error", e.what()}}).dump();
    } catch (...) {
        result = "{\"error\":\"Song registry failure\"}";
    }
    const size_t required = result.size() + 1;
    if (output && capacity >= required) std::memcpy(output, result.c_str(), required);
    return required;
}
