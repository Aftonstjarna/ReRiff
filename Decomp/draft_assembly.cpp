// Draft symbolic assembly from GNU objdump --insn-width=16 -d -Mintel output.
// Never emits original byte arrays. Output is unreviewed and earns no coverage.
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

struct Instruction { unsigned long address; std::vector<unsigned> bytes; std::string text; };
static void need(bool ok, const std::string &message) {
  if (!ok) throw std::runtime_error(message);
}
static std::string hex(unsigned long value) {
  std::ostringstream out; out << std::hex << value; return out.str();
}
int main(int argc, char **argv) {
  try {
    need(argc == 6, "usage: draft-assembly LISTING START_VA END_VA SYMBOL OUTPUT.S");
    const auto start = std::stoul(argv[2], nullptr, 16);
    const auto end = std::stoul(argv[3], nullptr, 16);
    const std::string symbol = argv[4];
    need(start < end, "empty or reversed range");
    need(std::regex_match(symbol, std::regex("[A-Za-z_][A-Za-z_0-9]*")), "invalid symbol");
    need(!std::filesystem::exists(argv[5]), "refusing to overwrite existing source");
    std::ifstream input(argv[1]); need(bool(input), "cannot open listing");
    std::regex linePattern(R"(^\s*([0-9a-fA-F]+):\s*\t([0-9a-fA-F ]+)\t\s*(.*?)\s*$)");
    std::vector<Instruction> instructions;
    std::map<unsigned long, std::string> labels;
    std::string line;
    unsigned long next = start;
    while (std::getline(input, line)) {
      std::smatch match;
      if (!std::regex_match(line, match, linePattern)) continue;
      auto address = std::stoul(match[1].str(), nullptr, 16);
      if (address < start || address >= end) continue;
      need(address == next, "gap/overlap in listing at " + hex(address));
      Instruction ins{address, {}, match[3]};
      std::istringstream bytes(match[2]); std::string byte;
      while (bytes >> byte) {
        need(byte.size() == 2, "invalid instruction bytes");
        ins.bytes.push_back(unsigned(std::stoul(byte, nullptr, 16)));
      }
      need(!ins.bytes.empty() && ins.bytes.size() <= 15, "invalid instruction length");
      need(ins.text.find(".byte") == std::string::npos &&
           ins.text.find("(bad)") == std::string::npos, "undecoded bytes require manual recovery");
      next += ins.bytes.size();
      need(next <= end, "range ends inside instruction");
      labels[address] = ".L" + hex(address);
      instructions.push_back(std::move(ins));
    }
    need(next == end, "incomplete listing; use --insn-width=16 and exact boundaries");
    std::map<unsigned long, std::string> externals;
    std::ostringstream source;
    source << "# UNREVIEWED generated draft: inspect control flow and verify rebuilt bytes.\n"
           << "# Original VA range 0x" << hex(start) << "..0x" << hex(end) << "\n"
           << ".intel_syntax noprefix\n.code32\n.text\n.global " << symbol << '\n'
           << symbol << ":\n";
    const std::regex branch(R"(^((?:j[a-z]+)|call|loop[a-z]*)\s+(?:0x)?([0-9a-f]+)(?:\s+<[^>]*>)?$)");
    const std::regex registers(R"(^[a-z]+\s+(?:e?[abcd]x|e?[sd]i|e?[bs]p|[abcd][hl]),(?:e?[abcd]x|e?[sd]i|e?[bs]p|[abcd][hl])$)");
    for (const auto &ins : instructions) {
      std::string text = ins.text;
      std::smatch match;
      if (std::regex_match(text, match, branch)) {
        auto target = std::stoul(match[2].str(), nullptr, 16);
        std::string destination;
        if (target >= start && target < end) {
          need(labels.count(target), "branch enters middle of instruction");
          destination = labels.at(target);
        } else {
          destination = "target_" + hex(target);
          externals[target] = destination;
        }
        text = match[1].str() + " " + destination;
      }
      // Preserve memory addressing: removing ds: alone would turn moffs loads
      // into immediates. Explicit DS-prefixed instructions remain review cases.
      text = std::regex_replace(text, std::regex(R"(ds:(0x[0-9a-f]+))"), "[$1]");
      size_t op = 0;
      while (op < ins.bytes.size() && ins.bytes[op] == 0x66) ++op;
      need(op < ins.bytes.size(), "prefix-only instruction");
      const unsigned opcode = ins.bytes[op];
      if (std::regex_match(text, registers) &&
          (opcode == 0x8b || opcode == 0x8a || opcode == 0x03 || opcode == 0x0b ||
           opcode == 0x13 || opcode == 0x1b || opcode == 0x23 || opcode == 0x2b ||
           opcode == 0x33 || opcode == 0x32 || opcode == 0x3b || opcode == 0x3a))
        text = "{load} " + text;
      if (opcode == 0x8d && op + 1 < ins.bytes.size() && text.find("+0x0]") != std::string::npos) {
        unsigned mode = ins.bytes[op+1] >> 6;
        if (mode == 1) text = "{disp8} " + text;
        if (mode == 2) text = "{disp32} " + text;
      }
      source << labels.at(ins.address) << ":\n " << text << '\n';
    }
    for (const auto &[target, name] : externals)
      source << ".equ " << name << ",0x" << hex(target) << '\n';
    std::ofstream output(argv[5]); need(bool(output), "cannot create draft");
    output << source.str(); need(bool(output), "cannot write draft");
    std::cout << "Drafted " << instructions.size() << " instructions spanning "
              << end - start << " bytes; no coverage credit assigned.\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
