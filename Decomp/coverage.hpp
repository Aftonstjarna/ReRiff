#pragma once
#include "native_blocks.hpp"
#include <set>
namespace rr::matching {
struct CoverageSpan {
  std::string name;
  size_t offset, size;
  Bytes candidate;
  bool available = true;
};
// Credit whole matching spans only; reject overlap to prevent double counting.
inline Json coverageForSpans(const Bytes &original, std::vector<CoverageSpan> spans) {
  require(!original.empty(), "coverage requires a nonempty original");
  std::sort(spans.begin(), spans.end(), [](const auto &a, const auto &b) {
    return a.offset < b.offset;
  });
  size_t end = 0, verified = 0, exactCount = 0;
  std::set<std::string> names;
  Json rows = Json::array();
  for (const auto &span : spans) {
    require(names.insert(span.name).second, "duplicate coverage block");
    require(span.size && span.offset <= original.size() &&
                span.size <= original.size() - span.offset,
            "coverage block outside original");
    require(span.offset >= end, "overlapping coverage blocks");
    end = span.offset + span.size;
    auto reference = slice(original, span.offset, span.size);
    bool equal = span.available && reference == span.candidate;
    if (equal) { verified += span.size; ++exactCount; }
    Json row = {{"name", span.name}, {"file_offset", span.offset},
                {"size", span.size}, {"available", span.available},
                {"byte_equal", equal}, {"reference_sha256", sha(reference)}};
    if (span.available) row["candidate_sha256"] = sha(span.candidate);
    rows.push_back(row);
  }
  return {{"schema", 1}, {"original_sha256", sha(original)},
          {"original_size", original.size()}, {"verified_bytes", verified},
          {"remaining_bytes", original.size() - verified},
          {"verified_percent", 100.0 * double(verified) / double(original.size())},
          {"registered_blocks", spans.size()}, {"exact_blocks", exactCount},
          {"all_registered_blocks_match", exactCount == spans.size()},
          {"whole_executable_compared", false}, {"source_rebuild_verified", false},
          {"masked_bytes", 0}, {"blocks", rows},
          {"note", "Registered source-built fragment coverage only; not a complete EXE rebuild."}};
}
inline Json recoveredCoverage(const Bytes &original, const fs::path &directory) {
  require(sha(original) == gameHash, "unsupported original Game.exe hash");
  Image image(original);
  auto blocks = nativeBlocks();
  blocks.push_back({"exe-entry", 0x4fd6d0, 6});
  std::vector<CoverageSpan> spans;
  for (auto block : blocks) {
    std::string filename = block.name;
    std::replace(filename.begin(), filename.end(), '-', '_');
    auto path = directory / (filename + ".bin");
    bool available = fs::is_regular_file(path);
    spans.push_back({block.name, image.offset(block.rva, block.size), block.size,
                     available ? read(path.string()) : Bytes{}, available});
  }
  return coverageForSpans(original, std::move(spans));
}
} // namespace rr::matching
