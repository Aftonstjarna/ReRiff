#include "build_evidence.hpp"
#include "native_blocks.hpp"
#include "matching.hpp"
#include "coverage.hpp"
int main(int argc, char **argv) {
  try {
    require(argc >= 2, "usage: reriff-match baseline ORIGINAL OUTPUT_DIR | "
                       "build-evidence ORIGINAL REPORT.json | "
                       "entry-check ORIGINAL BUILT_ENTRY.bin REPORT.json | "
                       "native-check ORIGINAL BLOCK_NAME BUILT.bin REPORT.json | "
                       "coverage ORIGINAL BUILD_DIR REPORT.json | "
                       "compare ORIGINAL CANDIDATE REPORT.json");
    std::string command = argv[1];
    if (command == "coverage") {
      require(argc == 5, "coverage requires ORIGINAL BUILD_DIR REPORT.json");
      auto output = rr::fs::weakly_canonical(argv[4]);
      require(output != rr::fs::canonical(argv[2]), "report must not overwrite original");
      require(output.extension() == ".json", "coverage report must use .json extension");
      auto report = rr::matching::recoveredCoverage(read(argv[2]), argv[3]);
      rr::save(argv[4], report);
      report.erase("blocks");
      std::cout << report.dump(2) << '\n';
      return report["all_registered_blocks_match"].get<bool>() ? 0 : 1;
    }
    if(command=="native-check") {
      require(argc==6,"native-check requires ORIGINAL BLOCK_NAME BUILT.bin REPORT.json");
      require(rr::fs::weakly_canonical(argv[5])!=rr::fs::canonical(argv[2]) &&
              rr::fs::weakly_canonical(argv[5])!=rr::fs::canonical(argv[4]),"report must not overwrite input");
      auto report=rr::matching::compareNativeBlock(read(argv[2]),read(argv[4]),argv[3]);
      rr::save(argv[5],report);std::cout<<report.dump(2)<<'\n';
      return report["byte_equal"].get<bool>()?0:1;
    }
    if (command == "build-evidence") {
      require(argc == 4, "build-evidence requires ORIGINAL REPORT.json");
      auto original = read(argv[2]);
      require(rr::sha(original) == rr::gameHash,
              "unsupported original Game.exe hash");
      require(rr::fs::weakly_canonical(argv[3]) != rr::fs::canonical(argv[2]),
              "report must not overwrite original");
      auto report = rr::matching::buildEvidence(original);
      rr::save(argv[3], report);
      std::cout << report.dump(2) << '\n';
      return 0;
    }
    if (command == "baseline") {
      require(argc == 4, "baseline requires ORIGINAL OUTPUT_DIR");
      auto report = rr::matching::baseline(read(argv[2]), argv[3]);
      rr::save(rr::fs::path(argv[3]) / "baseline.json", report);
      report.erase("methods");
      report.erase("sections");
      std::cout << report.dump(2) << '\n';
      return 0;
    }
    require((command == "compare" || command == "entry-check") && argc == 5,
            "compare requires ORIGINAL CANDIDATE REPORT.json");
    auto original = read(argv[2]);
    require(rr::sha(original) == rr::gameHash,
            "unsupported original Game.exe hash");
    require(rr::fs::weakly_canonical(argv[4]) != rr::fs::canonical(argv[2]) &&
                rr::fs::weakly_canonical(argv[4]) != rr::fs::canonical(argv[3]),
            "report must not overwrite an input");
    auto candidate = read(argv[3]);
    auto report = rr::Json::object();
    if (command == "entry-check") {
      require(candidate.size() == 6,
              "recovered entry instruction must be exactly six bytes");
      rr::matching::Image image(original);
      require(u32(original, image.optional + 16) == 0x4fd6d0,
              "unexpected EXE entrypoint");
      report = rr::matching::compare(
          rr::matching::slice(original, image.offset(0x4fd6d0, 6), 6),
          candidate);
      report["reference_fragment_sha256"] = report["original_sha256"];
      report.erase("original_sha256");
      report["original_exe_sha256"] = rr::sha(original);
      report["scope"] = "six-byte PE entry instruction only";
      report["whole_executable_compared"] = false;
    } else
      report = rr::matching::compare(original, candidate);
    rr::save(argv[4], report);
    bool equal = report["byte_equal"].get<bool>();
    report.erase("difference_ranges");
    std::cout << report.dump(2) << '\n';
    return equal ? 0 : 1;
  } catch (const std::exception &e) {
    std::cerr << "matching baseline: " << e.what() << '\n';
    return 2;
  }
}
