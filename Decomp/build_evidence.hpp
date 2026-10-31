#pragma once
#include "matching.hpp"

namespace rr::matching {
inline std::string hexBytes(const Bytes &b) {
  std::ostringstream out;
  for (auto value : b)
    out << std::hex << std::setw(2) << std::setfill('0') << unsigned(value);
  return out.str();
}
// Preserve raw Rich component IDs/build numbers without guessing product names
// or asserting that every object was compiled with the same toolchain.
inline Json richEvidence(const Bytes &b) {
  size_t pe = u32(b, 0x3c);
  require(pe <= b.size(), "PE header outside file");
  for (size_t end = 0x40; end + 8 <= pe; end += 4) {
    if (u32(b, end) != 0x68636952)
      continue;
    uint32_t key = u32(b, end + 4);
    for (size_t start = 0x40; start + 16 <= end; start += 4) {
      if ((u32(b, start) ^ key) != 0x536e6144)
        continue;
      require(((end - start - 16) % 8) == 0, "invalid Rich record alignment");
      require((u32(b, start + 4) ^ key) == 0 &&
                  (u32(b, start + 8) ^ key) == 0 &&
                  (u32(b, start + 12) ^ key) == 0,
              "invalid Rich header padding");
      Json entries = Json::array();
      for (size_t p = start + 16; p < end; p += 8) {
        uint32_t id = u32(b, p) ^ key, count = u32(b, p + 4) ^ key;
        entries.push_back({{"file_offset", p},
                           {"component_id", id >> 16},
                           {"build", id & 0xffff},
                           {"count", count}});
      }
      return {{"present", true},
              {"file_offset", start},
              {"end_offset", end + 8},
              {"xor_key", key},
              {"raw_sha256", sha(slice(b, start, end + 8 - start))},
              {"entries", entries}};
    }
    throw std::runtime_error("Rich marker without decoded DanS header");
  }
  return {{"present", false}};
}
inline Json buildEvidence(const Bytes &b) {
  Image image(b);
  auto directory = [&](unsigned i) {
    return std::pair<uint32_t, uint32_t>{u32(b, image.optional + 96 + i * 8),
                                         u32(b, image.optional + 100 + i * 8)};
  };
  auto stringRva = [&](uint32_t rva) {
    size_t p = image.offset(rva);
    return text(b, p, b.size() - p);
  };
  Json debug = Json::array(), imports = Json::array(), delay = Json::array();
  auto [debugRva, debugSize] = directory(6);
  if (debugSize) {
    require(debugSize % 28 == 0, "invalid debug directory size");
    size_t p = image.offset(debugRva, debugSize);
    for (size_t i = 0; i < debugSize; i += 28) {
      size_t row = p + i, size = u32(b, row + 16), raw = u32(b, row + 24);
      auto payload = slice(b, raw, size);
      Json entry = {{"type", u32(b, row + 12)},
                    {"timestamp", u32(b, row + 4)},
                    {"file_offset", raw},
                    {"size", size},
                    {"sha256", sha(payload)}};
      if (size >= 24 && u32(b, row + 12) == 2 &&
          u32(payload, 0) == 0x53445352) {
        entry["format"] = "RSDS";
        entry["guid_raw_hex"] = hexBytes(slice(payload, 4, 16));
        entry["age"] = u32(payload, 20);
        entry["pdb_path"] = text(payload, 24, size - 24);
      }
      // Unknown records remain unlabelled numeric evidence.
      if (size % 4 == 0 && size <= 128) {
        entry["payload_words_le32"] = Json::array();
        for (size_t j = 0; j < size; j += 4)
          entry["payload_words_le32"].push_back(u32(payload, j));
      }
      debug.push_back(entry);
    }
  }
  auto [importRva, importSize] = directory(1);
  if (importSize) {
    size_t p = image.offset(importRva, importSize);
    bool terminated = false;
    for (size_t i = 0; i + 20 <= importSize; i += 20) {
      uint32_t name = u32(b, p + i + 12);
      if (!name) {
        terminated = true;
        break;
      }
      imports.push_back(stringRva(name));
    }
    require(terminated, "unterminated import directory");
  }
  auto [delayRva, delaySize] = directory(13);
  if (delaySize) {
    size_t p = image.offset(delayRva, delaySize);
    bool terminated = false;
    for (size_t i = 0; i + 32 <= delaySize; i += 32) {
      uint32_t attributes = u32(b, p + i), name = u32(b, p + i + 4);
      if (!name) {
        terminated = true;
        break;
      }
      if (!(attributes & 1)) {
        require(name >= u32(b, image.optional + 28),
                "delay import VA below image base");
        name -= u32(b, image.optional + 28);
      }
      delay.push_back(stringRva(name));
    }
    require(terminated, "unterminated delay import directory");
  }
  auto [clrRva, clrSize] = directory(14);
  require(clrSize >= 72, "missing CLR header");
  size_t cli = image.offset(clrRva, 72),
         metadata = image.offset(u32(b, cli + 8), u32(b, cli + 12));
  require(u32(b, metadata) == 0x424a5342, "invalid CLR metadata root");
  uint32_t versionLength = u32(b, metadata + 12), flags = u32(b, cli + 16);
  Json markers = Json::array();
  for (const std::string &marker :
       {std::string(".NETFramework,Version=v4.0"),
        std::string("Microsoft (R) HLSL Shader Compiler ")}) {
    auto at = std::search(b.begin(), b.end(), marker.begin(), marker.end());
    if (at != b.end()) {
      size_t p = size_t(at - b.begin());
      markers.push_back(
          {{"file_offset", p}, {"text", text(b, p, b.size() - p)}});
    }
  }
  return {{"schema", 1},
          {"original_sha256", sha(b)},
          {"pe",
           {{"linker_major", b.at(image.optional + 2)},
            {"linker_minor", b.at(image.optional + 3)},
            {"timestamp", u32(b, image.optional - 16)},
            {"entrypoint_rva", u32(b, image.optional + 16)},
            {"image_base", u32(b, image.optional + 28)},
            {"section_alignment", u32(b, image.optional + 32)},
            {"file_alignment", u32(b, image.optional + 36)}}},
          {"clr",
           {{"runtime_major", u16(b, cli + 4)},
            {"runtime_minor", u16(b, cli + 6)},
            {"flags", flags},
            {"il_only", bool(flags & 1)},
            {"entrypoint_token_or_rva", u32(b, cli + 20)},
            {"metadata_version", text(b, metadata + 16, versionLength)}}},
          {"rich", richEvidence(b)},
          {"debug", debug},
          {"imports", imports},
          {"delay_imports", delay},
          {"embedded_string_evidence", markers},
          {"source_rebuild_verified", false},
          {"interpretation",
           "Recorded EXE evidence does not establish exact compiler flags, "
           "libraries or a matching source build."}};
}
} // namespace rr::matching
