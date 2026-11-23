// Copyright (c) 2026 ReRiff contributors
// SPDX-License-Identifier: MIT; see LICENSE.ReRiff
// Song identity registry. Artist/title metadata is the identity; content
// hashes are retained as evidence so differently encoded copies of the same
// song share one leaderboard identity. No gameplay or cache mutation.
#include <nlohmann/json.hpp>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <cctype>
#include <cstring>
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

static std::string lower_hash(std::string hash) {
    if (hash.size() != 64 || hash.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        throw std::runtime_error("Invalid SHA256");
    for (auto& c : hash) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return hash;
}

// Trim, collapse ASCII whitespace and fold ASCII case. UTF-8 bytes pass through.
static std::string normalized(const std::string& value) {
    const char* space = " \t\r\n";
    const auto first = value.find_first_not_of(space);
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(space);
    std::string out;
    out.reserve(last - first + 1);
    bool separated = false;
    for (auto i = first; i <= last; ++i) {
        const auto c = static_cast<unsigned char>(value[i]);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (!separated) out.push_back(' ');
            separated = true;
        } else {
            out.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : static_cast<char>(c));
            separated = false;
        }
    }
    return out;
}

// Different encodings of one song share artist/title. Unlabelled audio, or an
// empty artist, falls back to exact-source matching so unrelated tracks with a
// generic or missing title cannot merge.
static std::string group_key(const std::string& artist, const std::string& title, const std::string& hash) {
    const auto a = normalized(artist), t = normalized(title);
    if (a.empty() || t.empty()) return "audio:" + hash;
    return "meta:" + a + '\x1f' + t;
}

static std::set<std::string> record_hashes(const json& record, const std::string& id) {
    std::set<std::string> hashes;
    if (record.contains("hashes")) {
        if (!record.at("hashes").is_array()) throw std::runtime_error("Invalid song hash list: " + id);
        for (const auto& entry : record.at("hashes"))
            hashes.insert(lower_hash(entry.get<std::string>()));
    } else if (record.contains("sha256")) {
        hashes.insert(lower_hash(record.at("sha256").get<std::string>()));
    }
    return hashes;
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
            if (!identifier(id)) throw std::runtime_error("Invalid song ID");
            const auto hash = lower_hash(song.at("sha256").get<std::string>());
            for (const auto* key : {"title", "artist"}) {
                if (!song.contains(key) || !song[key].is_string() || song[key].get<std::string>().size() > 4096)
                    throw std::runtime_error("Invalid song metadata");
            }
            const auto title = song.at("title").get<std::string>();
            const auto artist = song.at("artist").get<std::string>();
            auto hashes = std::set<std::string>{hash};
            if (records.contains(id)) {
                const auto& existing = records.at(id);
                auto record_artist = existing.value("artist", std::string{});
                auto record_title = existing.value("title", std::string{});
                // A cache ID may gain a new encoding, but not silently move to
                // another song's leaderboard.
                if (normalized(record_artist) != normalized(artist) ||
                    normalized(record_title) != normalized(title))
                    throw std::runtime_error("Song ID already registered with different metadata: " + id);
                const auto previous = record_hashes(existing, id);
                hashes.insert(previous.begin(), previous.end());
            }
            // Store identity, display metadata and every observed encoding,
            // never client paths.
            records[id] = {
                {"title", title}, {"artist", artist},
                {"hashes", json::array()}
            };
            for (const auto& value : hashes) records[id]["hashes"].push_back(value);
        }
        std::map<std::string, std::string> canonical;
        json aliases = json::object();
        // Object keys are sorted: canonical choice is deterministic.
        for (auto it = records.begin(); it != records.end(); ++it) {
            const auto artist = it.value().value("artist", std::string{});
            const auto title = it.value().value("title", std::string{});
            const auto hashes = record_hashes(it.value(), it.key());
            if (hashes.empty()) throw std::runtime_error("Song has no registered audio: " + it.key());
            const auto key = group_key(artist, title, *hashes.begin());
            canonical.emplace(key, it.key());
            aliases[it.key()] = canonical.at(key);
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
