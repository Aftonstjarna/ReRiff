#pragma once
#include "../NativeLinux/native_common.hpp"

namespace rr::matching {
inline size_t align4(size_t n) { return (n + 3) & ~size_t(3); }
inline Bytes slice(const Bytes &b, size_t p, size_t n) {
  require(p <= b.size() && n <= b.size() - p, "range outside executable");
  return Bytes(b.begin() + p, b.begin() + p + n);
}
struct Section {
  std::string name;
  uint32_t rva, offset, size;
};
struct Image {
  const Bytes &b;
  size_t optional;
  std::vector<Section> sections;
  explicit Image(const Bytes &bytes) : b(bytes) {
    require(u16(b, 0) == 0x5a4d, "not an MZ executable");
    size_t pe = u32(b, 0x3c);
    require(u32(b, pe) == 0x4550, "not a PE executable");
    optional = pe + 24;
    require(u16(b, optional) == 0x10b && u16(b, pe + 4) == 0x14c,
            "expected original x86 PE32 image");
    size_t table = optional + u16(b, pe + 20);
    for (size_t i = 0; i < u16(b, pe + 6); ++i) {
      size_t p = table + i * 40;
      auto name = slice(b, p, 8);
      auto end = std::find(name.begin(), name.end(), 0);
      Section s{std::string(name.begin(), end), u32(b, p + 12), u32(b, p + 20),
                u32(b, p + 16)};
      slice(b, s.offset, s.size);
      sections.push_back(s);
    }
  }
  size_t offset(uint32_t rva, size_t size = 1) const {
    for (const auto &s : sections)
      if (rva >= s.rva && uint64_t(rva) - s.rva < s.size &&
          size <= s.size - (rva - s.rva))
        return s.offset + (rva - s.rva);
    throw std::runtime_error("RVA outside file-backed section");
  }
  Json methods() const {
    size_t cli = offset(u32(b, optional + 96 + 14 * 8), 72);
    size_t root = offset(u32(b, cli + 8), u32(b, cli + 12));
    require(u32(b, root) == 0x424a5342, "invalid CLR metadata root");
    size_t cursor = align4(root + 16 + u32(b, root + 12));
    unsigned streams = u16(b, cursor + 2);
    cursor += 4;
    size_t tables = 0, strings = 0, stringSize = 0;
    for (unsigned i = 0; i < streams; ++i) {
      size_t start = root + u32(b, cursor), size = u32(b, cursor + 4);
      slice(b, start, size);
      std::string name =
          text(b, cursor + 8, std::min(size_t(32), b.size() - cursor - 8));
      if (name == "#~" || name == "#-")
        tables = start;
      if (name == "#Strings") {
        strings = start;
        stringSize = size;
      }
      cursor = align4(cursor + 8 + name.size() + 1);
    }
    require(tables && strings, "missing CLR metadata streams");
    unsigned heaps = b.at(tables + 6);
    uint64_t valid =
        uint64_t(u32(b, tables + 8)) | uint64_t(u32(b, tables + 12)) << 32;
    std::array<uint32_t, 64> counts{};
    cursor = tables + 24;
    for (unsigned i = 0; i < 64; ++i)
      if (valid & (uint64_t(1) << i)) {
        counts[i] = u32(b, cursor);
        cursor += 4;
      }
    auto index = [&](unsigned table) -> size_t {
      return counts[table] < 65536 ? 2 : 4;
    };
    auto coded = [&](unsigned bits,
                     std::initializer_list<unsigned> ts) -> size_t {
      for (auto t : ts)
        if (counts[t] >= (1u << (16 - bits)))
          return 4;
      return 2;
    };
    size_t str = heaps & 1 ? 4 : 2, guid = heaps & 2 ? 4 : 2,
           blob = heaps & 4 ? 4 : 2;
    std::array<size_t, 6> sizes{2 + str + 3 * guid,
                                coded(2, {0, 26, 35, 1}) + 2 * str,
                                4 + 2 * str + coded(2, {2, 1, 27}) + index(4) +
                                    index(6),
                                index(4),
                                2 + str + blob,
                                index(6)};
    for (size_t i = 0; i < sizes.size(); ++i)
      cursor += sizes[i] * counts[i];
    size_t rowSize = 8 + str + blob + index(8);
    Json result = Json::array();
    for (uint32_t i = 0; i < counts[6]; ++i) {
      size_t p = cursor + i * rowSize;
      slice(b, p, rowSize);
      size_t name = str == 2 ? u16(b, p + 8) : u32(b, p + 8);
      require(name < stringSize, "method name outside string heap");
      unsigned impl = u16(b, p + 4);
      result.push_back({{"token", 0x06000001u + i},
                        {"name", text(b, strings + name, stringSize - name)},
                        {"rva", u32(b, p)},
                        {"implementation_flags", impl},
                        {"flags", u16(b, p + 6)}});
    }
    return result;
  }
  // Exact IL bytes: method header, instructions, alignment before sections,
  // and all method data sections (including exception tables).
  size_t ilSize(uint32_t rva) const {
    size_t p = offset(rva), codeEnd;
    bool more;
    uint8_t first = b.at(p);
    if ((first & 3) == 2) {
      codeEnd = p + 1 + (first >> 2);
      more = false;
    } else {
      require((first & 3) == 3, "invalid IL method header");
      unsigned flags = u16(b, p), header = (flags >> 12) * 4;
      require(header >= 12, "invalid fat IL header");
      codeEnd = p + header + u32(b, p + 4);
      more = flags & 8;
    }
    size_t end = codeEnd;
    while (more) {
      size_t s = align4(end);
      uint8_t kind = b.at(s);
      size_t size = kind & 0x40 ? (u32(b, s) >> 8) : b.at(s + 1);
      require(size >= 4, "invalid IL data section");
      end = s + size;
      more = kind & 0x80;
    }
    offset(rva, end - p);
    return end - p;
  }
};
inline Json baseline(const Bytes &b, const fs::path &output) {
  require(sha(b) == gameHash, "unsupported original Game.exe hash");
  Image image(b);
  Json methods = image.methods(), sections = Json::array();
  size_t il = 0, native = 0, noBody = 0;
  for (auto &m : methods) {
    uint32_t rva = m["rva"],
             kind = m["implementation_flags"].get<unsigned>() & 3;
    if (rva == 0) {
      ++noBody;
      m["body_status"] = "no RVA";
      continue;
    }
    m["file_offset"] = image.offset(rva);
    if (kind != 0) {
      ++native;
      m["body_status"] = "native boundary unresolved";
      continue;
    }
    size_t size = image.ilSize(rva);
    auto bytes = slice(b, image.offset(rva), size);
    std::ostringstream name;
    name << "0x" << std::hex << m["token"].get<uint32_t>() << ".cil";
    write(output / "original-il" / name.str(), bytes);
    m["body_status"] = "exact original IL bytes; not reconstructed source";
    m["body_size"] = size;
    m["body_sha256"] = sha(bytes);
    ++il;
  }
  for (auto &s : image.sections)
    sections.push_back({{"name", s.name},
                        {"rva", s.rva},
                        {"file_offset", s.offset},
                        {"size", s.size},
                        {"sha256", sha(slice(b, s.offset, s.size))}});
  return {{"schema", 1},
          {"original_sha256", sha(b)},
          {"original_size", b.size()},
          {"source_rebuild_verified", false},
          {"matching_decompilation_established", false},
          {"il_bodies_preserved", il},
          {"native_entries_without_proven_extent", native},
          {"methods_without_rva", noBody},
          {"sections", sections},
          {"methods", methods}};
}
inline Json compare(const Bytes &original, const Bytes &candidate) {
  size_t extent = std::max(original.size(), candidate.size()), different = 0;
  Json ranges = Json::array();
  size_t p = 0;
  auto differs = [&](size_t i) {
    return i >= original.size() || i >= candidate.size() ||
           original[i] != candidate[i];
  };
  while (p < extent) {
    if (!differs(p)) {
      ++p;
      continue;
    }
    size_t start = p;
    while (p < extent && differs(p)) {
      ++different;
      ++p;
    }
    ranges.push_back({{"offset", start}, {"length", p - start}});
  }
  return {{"schema", 1},
          {"byte_equal", different == 0},
          {"different_bytes", different},
          {"original_size", original.size()},
          {"candidate_size", candidate.size()},
          {"original_sha256", sha(original)},
          {"candidate_sha256", sha(candidate)},
          {"masked_bytes", 0},
          {"source_rebuild_verified", false},
          {"note",
           "Binary equality alone does not prove reconstruction from source."},
          {"difference_ranges", ranges}};
}
} // namespace rr::matching
