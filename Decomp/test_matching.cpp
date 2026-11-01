#include "build_evidence.hpp"
#include "matching.hpp"
#include "coverage.hpp"
int main() {
  try {
    using namespace rr::matching;
    auto check = [](bool value) {
      require(value, "matching regression failed");
    };
    Bytes a{1, 2, 3, 4}, b = a;
    auto coverage = coverageForSpans(a, {{"tail", 2, 2, {3,4}}, {"head", 0, 2, {1,2}}});
    check(coverage["verified_bytes"] == 4 && coverage["verified_percent"] == 100.0);
    check(coverage["whole_executable_compared"] == false &&
          coverage["source_rebuild_verified"] == false);
    coverage = coverageForSpans(a, {{"head", 0, 2, {1,9}}, {"tail", 2, 2, {}, false}});
    check(coverage["verified_bytes"] == 0 && coverage["remaining_bytes"] == 4);
    coverage = coverageForSpans(a, {{"head", 0, 2, {1,2}}, {"tail", 2, 2, {3}}});
    check(coverage["verified_bytes"] == 2 && coverage["verified_percent"] == 50.0);
    for (const auto &spans : std::vector<std::vector<CoverageSpan>>{
             {{"one",0,3,{1,2,3}}, {"two",2,2,{3,4}}},
             {{"same",0,2,{1,2}}, {"same",2,2,{3,4}}},
             {{"outside",3,2,{4,5}}}}) {
      bool failed = false;
      try { coverageForSpans(a, spans); } catch (const std::exception &) { failed = true; }
      check(failed);
    }
    check(compare(a, b)["byte_equal"] == true);
    b[0] = 9;
    b[3] = 8;
    auto diff = compare(a, b);
    check(diff["different_bytes"] == 2 &&
          diff["difference_ranges"].size() == 2);
    check(diff["difference_ranges"][1]["offset"] == 3);
    b = a;
    b.push_back(0);
    check(compare(a, b)["different_bytes"] == 1);
    b = {1, 2};
    diff = compare(a, b);
    check(diff["different_bytes"] == 2 &&
          diff["difference_ranges"][0]["offset"] == 2);
    check(compare({}, Bytes{0})["byte_equal"] == false);
    check(compare(a, a)["source_rebuild_verified"] == false);
    Bytes pe(1024);
    auto put16 = [&](size_t p, uint16_t value) {
      pe[p] = uint8_t(value);
      pe[p + 1] = uint8_t(value >> 8);
    };
    auto put32 = [&](size_t p, uint32_t value) {
      for (size_t i = 0; i < 4; ++i)
        pe[p + i] = uint8_t(value >> (8 * i));
    };
    put16(0, 0x5a4d);
    put32(0x3c, 0x80);
    put32(0x80, 0x4550);
    put16(0x84, 0x14c);
    put16(0x86, 1);
    put16(0x94, 224);
    put16(0x98, 0x10b);
    put32(0x178 + 12, 0x1000);
    put32(0x178 + 16, 512);
    put32(0x178 + 20, 512);
    pe[512] = 14; // Tiny header, three instruction bytes.
    Image image(pe);
    check(image.ilSize(0x1000) == 4);
    put16(0x210, 0x300b);
    put32(0x214, 2); // Fat header + two bytes, then alignment.
    pe[0x220] = 1;
    pe[0x221] = 16;
    check(image.ilSize(0x1010) == 32);
    pe[0x220] = 0x81;
    put32(0x230, (28 << 8) | 0x41); // Chained small/fat EH tables.
    check(image.ilSize(0x1010) == 60);
    put32(0x230, (900 << 8) | 0x41);
    bool rejected = false;
    try {
      image.ilSize(0x1010);
    } catch (const std::exception &) {
      rejected = true;
    }
    check(rejected);
    // Independent synthetic PE: Rich records, RSDS, normal/delay imports, CLR.
    pe.resize(2048);
    put32(0x178 + 16, 1536);
    uint32_t key = 0x12345678;
    put32(0x40, 0x536e6144 ^ key);
    put32(0x44, key);
    put32(0x48, key);
    put32(0x4c, key);
    put32(0x50, ((225u << 16) | 40629u) ^ key);
    put32(0x54, 3 ^ key);
    put32(0x58, 0x68636952);
    put32(0x5c, key);
    auto rich = richEvidence(pe);
    check(rich["entries"][0]["build"] == 40629 &&
          rich["entries"][0]["count"] == 3);
    pe[0x44] ^= 1;
    rejected = false;
    try {
      richEvidence(pe);
    } catch (const std::exception &) {
      rejected = true;
    }
    check(rejected);
    pe[0x44] ^= 1;
    auto directory = [&](unsigned i, uint32_t rva, uint32_t size) {
      put32(0x98 + 96 + i * 8, rva);
      put32(0x98 + 100 + i * 8, size);
    };
    auto stringAt = [&](size_t p, const std::string &s) {
      std::copy(s.begin(), s.end(), pe.begin() + p);
      pe[p + s.size()] = 0;
    };
    directory(6, 0x1050, 28);
    put32(0x250 + 12, 2);
    put32(0x250 + 16, 30);
    put32(0x250 + 24, 0x280);
    put32(0x280, 0x53445352);
    put32(0x294, 10);
    stringAt(0x298, "x.pdb");
    directory(14, 0x1100, 72);
    put32(0x300 + 8, 0x1150);
    put32(0x300 + 12, 64);
    put32(0x350, 0x424a5342);
    put32(0x350 + 12, 12);
    stringAt(0x360, "v4.0.30319");
    directory(1, 0x1200, 40);
    put32(0x400 + 12, 0x1230);
    stringAt(0x430, "test.dll");
    directory(13, 0x1250, 64);
    put32(0x450, 1);
    put32(0x454, 0x12a0);
    stringAt(0x4a0, "delay.dll");
    auto evidence = buildEvidence(pe);
    check(evidence["debug"][0]["pdb_path"] == "x.pdb" &&
          evidence["debug"][0]["age"] == 10);
    check(evidence["imports"][0] == "test.dll" &&
          evidence["delay_imports"][0] == "delay.dll");
    check(evidence["clr"]["metadata_version"] == "v4.0.30319");
    check(evidence["source_rebuild_verified"] == false);
    directory(6, 0x1050, 27);
    rejected = false;
    try {
      buildEvidence(pe);
    } catch (const std::exception &) {
      rejected = true;
    }
    check(rejected);
    std::cout << "Exact comparison and IL header/exception-section regressions "
                 "passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
