#pragma once
#include "matching.hpp"
namespace rr::matching {
struct NativeBlock { const char *name; uint32_t rva; size_t size; };
inline std::vector<NativeBlock> nativeBlocks() {
  return {NativeBlock{"security-cookie",0x4f9b78,156},
                     NativeBlock{"crt-entry",0x4f8a84,10},
                     NativeBlock{"crt-startup",0x4f8a8e,436},
                     NativeBlock{"crt-seh",0x4f9c90,89},
                     NativeBlock{"crt-scope",0x79e5c0,28},
                     NativeBlock{"application-entry",0x1cfc90,922},
                     NativeBlock{"object-5cfb10",0x1cfb10,72},
                     NativeBlock{"object-5cfb60",0x1cfb60,121},
                     NativeBlock{"object-5cfbe0",0x1cfbe0,121},
                     NativeBlock{"object-5cfc60",0x1cfc60,45},
                     NativeBlock{"message-pump",0x8ce10,85},
                     NativeBlock{"worker-delete",0x8db80,44},
                     NativeBlock{"worker-destroy",0x8dbb0,16},
                     NativeBlock{"worker-start",0x8dbc0,130},
                     NativeBlock{"worker-dispatch",0x8dc50,15},
                     NativeBlock{"worker-request",0xa1870,135},
                     NativeBlock{"request-dispatch",0xa1900,696},
                     NativeBlock{"request-cleanup",0xa1bc0,111},
                     NativeBlock{"request-case3",0xa2130,319},
                     NativeBlock{"request-case2",0xa1cd0,1112},
                     NativeBlock{"window-mode",0x8cb30,315},
                     NativeBlock{"cookie-check",0x4f8958,15},
                     NativeBlock{"secondary-window",0x8cc70,411},
                     NativeBlock{"window-frame",0x8cab0,123},
                     NativeBlock{"viewport-size",0xa22e0,212},
                     NativeBlock{"fit-dimensions",0xa1c30,159},
                     NativeBlock{"floor-float",0x26d20,53},
                     NativeBlock{"remove-registered",0xa23c0,148},
                     NativeBlock{"load-graphics",0xa2460,474},
                     NativeBlock{"resize-resources",0x128f80,299},
                     NativeBlock{"save-graphics",0xa2640,394},
                     NativeBlock{"save-device-ctor",0x135710,81},
                     NativeBlock{"save-device-close",0x135c20,38},
                     NativeBlock{"save-device-delete",0x135cc0,189},
                     NativeBlock{"save-device-read",0x135d80,92},
                     NativeBlock{"save-device-write",0x135de0,70},
                     NativeBlock{"save-device-open",0x135b10,267},
                     NativeBlock{"save-device-exists",0x135810,212},
                     NativeBlock{"save-device-create",0x1358f0,533},
                     NativeBlock{"save-device-size",0x135c50,99},
                     NativeBlock{"application-worker",0x1282e0,1611},
                     NativeBlock{"worker-queue",0x128930,417},
                     NativeBlock{"application-frame",0xa0af0,3441},
                     NativeBlock{"device-release",0xa6330,545},
                     NativeBlock{"device-restore",0xa6560,319},
                     NativeBlock{"query-create",0xa4c50,201},
                     NativeBlock{"query-release",0xa4d20,83},
                     NativeBlock{"colour-targets",0xa5240,752},
                     NativeBlock{"effect-target",0xa5530,397},
                     NativeBlock{"aux-target-a",0xa56c0,393},
                     NativeBlock{"aux-target-b",0xa5850,393},
                     NativeBlock{"device-defaults",0xa5e80,1194},
                     NativeBlock{"buffer-allocate",0x1429a0,155},
                     NativeBlock{"device-create",0xa3630,1027},
                     NativeBlock{"device-destroy",0xa5c40,566},
                     NativeBlock{"presentation-parameters",0xa33f0,575},
                     NativeBlock{"vertex-declarations",0xa3a40,4611},
                     NativeBlock{"graphics-startup",0xa0480,1110},
                     NativeBlock{"graphics-shutdown",0xa08e0,513},
                     NativeBlock{"graphics-capabilities",0xa2be0,2063},
                     NativeBlock{"graphics-options",0xa27d0,502},
                     NativeBlock{"minimum-caps",0xa59e0,281},
                     NativeBlock{"display-modes",0xa29d0,520}};
}
inline NativeBlock nativeBlock(const std::string &name) {
  for (auto block : nativeBlocks())
    if (name==block.name) return block;
  throw std::runtime_error("unknown recovered native block");
}
inline Json compareNativeBlock(const Bytes &original,const Bytes &candidate,
                               const std::string &name) {
  require(sha(original)==gameHash,"unsupported original Game.exe hash");
  auto block=nativeBlock(name);
  require(candidate.size()==block.size,"recovered native block has wrong size");
  Image image(original);
  auto report=compare(slice(original,image.offset(block.rva,block.size),block.size),candidate);
  report["reference_fragment_sha256"]=report["original_sha256"];
  report.erase("original_sha256");
  report["original_exe_sha256"]=sha(original);
  report["scope"]=block.name;report["rva"]=block.rva;
  report["whole_executable_compared"]=false;
  return report;
}
} // namespace rr::matching
