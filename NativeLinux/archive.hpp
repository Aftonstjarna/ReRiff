#pragma once
// Native Linux asset/configuration bootstrap. Does not execute Windows code.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <zlib.h>

using Bytes = std::vector<uint8_t>;
static void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
static uint32_t u32(const Bytes &b, size_t p) {
    require(p <= b.size() && b.size() - p >= 4, "truncated data");
    return uint32_t(b[p]) | uint32_t(b[p + 1]) << 8 | uint32_t(b[p + 2]) << 16 |
           uint32_t(b[p + 3]) << 24;
}
static uint16_t u16(const Bytes &b, size_t p) {
    require(p <= b.size() && b.size() - p >= 2, "truncated data");
    return uint16_t(b[p]) | uint16_t(b[p + 1]) << 8;
}
static Bytes read(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    require(bool(f), "cannot open " + path);
    return Bytes(std::istreambuf_iterator<char>(f), {});
}
static std::string quote(const std::string &s) {
    std::ostringstream o;
    o << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\')
            o << '\\' << c;
        else if (c < 32 || c >= 127)
            o << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c)
              << std::dec;
        else
            o << c;
    }
    o << '"';
    return o.str();
}

struct Entry {
    std::string name;
    uint32_t crc, compressed, size, offset;
    uint16_t method, flags;
};
class Archive {
    Bytes bytes;

  public:
    std::vector<Entry> entries;
    explicit Archive(const std::string &path) : bytes(read(path)) {
        require(bytes.size() >= 22, "truncated ZIP");
        size_t end = bytes.size() - 22;
        const size_t first = bytes.size() > 65557 ? bytes.size() - 65557 : 0;
        while (
            !(u32(bytes, end) == 0x06054b50 && end + 22 + u16(bytes, end + 20) == bytes.size())) {
            require(end > first, "ZIP end record missing");
            --end;
        }
        require(u16(bytes, end + 4) == 0 && u16(bytes, end + 6) == 0 &&
                    u16(bytes, end + 8) == u16(bytes, end + 10),
                "split ZIP unsupported");
        uint32_t count = u16(bytes, end + 10), start = u32(bytes, end + 16),
                 length = u32(bytes, end + 12);
        require(count != 65535 && start != 0xffffffff && length != 0xffffffff, "ZIP64 unsupported");
        require(uint64_t(start) + length == end, "invalid central directory bounds");
        size_t p = start;
        std::set<std::string> names;
        for (uint32_t i = 0; i < count; ++i) {
            require(p <= end && end - p >= 46 && u32(bytes, p) == 0x02014b50,
                    "invalid ZIP directory entry");
            auto n = u16(bytes, p + 28), extra = u16(bytes, p + 30), comment = u16(bytes, p + 32);
            require(uint64_t(p) + 46 + n + extra + comment <= end, "truncated ZIP filename");
            Entry e{std::string(bytes.begin() + p + 46, bytes.begin() + p + 46 + n),
                    u32(bytes, p + 16),
                    u32(bytes, p + 20),
                    u32(bytes, p + 24),
                    u32(bytes, p + 42),
                    u16(bytes, p + 10),
                    u16(bytes, p + 8)};
            require(names.insert(e.name).second, "duplicate ZIP filename");
            entries.push_back(e);
            p += 46 + n + extra + comment;
        }
        require(p == end, "invalid ZIP directory length");
    }
    Bytes load(const Entry &e) const {
        require(!(e.flags & 1), "encrypted ZIP unsupported");
        require(e.size <= 256 * 1024 * 1024, "asset exceeds 256 MiB limit");
        size_t p = e.offset;
        require(p <= bytes.size() && bytes.size() - p >= 30 && u32(bytes, p) == 0x04034b50,
                "invalid local ZIP header");
        require(u16(bytes, p + 8) == e.method && u16(bytes, p + 6) == e.flags,
                "ZIP header mismatch");
        p += 30 + u16(bytes, p + 26) + u16(bytes, p + 28);
        require(p <= bytes.size() && e.compressed <= bytes.size() - p, "truncated asset");
        Bytes output(e.size);
        if (e.method == 0) {
            require(e.compressed == e.size, "invalid stored asset length");
            std::copy_n(bytes.begin() + p, e.size, output.begin());
        } else if (e.method == 8) {
            z_stream stream{};
            stream.next_in = const_cast<Bytef *>(bytes.data() + p);
            stream.avail_in = e.compressed;
            uint8_t empty = 0;
            stream.next_out = output.empty() ? &empty : output.data();
            stream.avail_out = output.empty() ? 1 : e.size;
            require(inflateInit2(&stream, -MAX_WBITS) == Z_OK, "inflate initialization failed");
            int status = inflate(&stream, Z_FINISH);
            bool valid = status == Z_STREAM_END && stream.total_out == e.size &&
                         stream.total_in == e.compressed;
            inflateEnd(&stream);
            require(valid, "invalid deflated asset");
        } else
            throw std::runtime_error("unsupported ZIP method");
        require(crc32(0, output.data(), output.size()) == e.crc, "asset CRC mismatch");
        return output;
    }
};

// GFCF v3: 20-byte header, N 32-byte records, then a shared string pool.
// A table's value is its first child index; its count is at record +24.
class Config {
    const Bytes &bytes;
    uint32_t count, roots;
    size_t strings;
    std::vector<unsigned char> visited;
    std::string stringAt(uint32_t offset) const {
        require(offset < bytes.size() - strings, "invalid config string offset");
        auto begin = bytes.begin() + strings + offset;
        auto end = std::find(begin, bytes.end(), 0);
        require(end != bytes.end(), "unterminated config string");
        return std::string(begin, end);
    }
    void nodes(std::ostream &out, uint32_t start, uint32_t size, unsigned depth) {
        require(depth < 128 && start <= count && size <= count - start,
                "invalid config child range/depth");
        out << '[';
        for (uint32_t i = start; i < start + size; ++i) {
            require(!visited[i], "overlapping/cyclic config nodes");
            visited[i] = 1;
            if (i != start)
                out << ',';
            size_t p = 20 + size_t(i) * 32;
            auto name = u32(bytes, p), value = u32(bytes, p + 8), type = u32(bytes, p + 20),
                 children = u32(bytes, p + 24);
            require(u32(bytes, p + 4) == 0 && u32(bytes, p + 12) == 0 && u32(bytes, p + 28) == 0,
                    "nonzero config reserved field");
            out << "{\"name\":" << quote(stringAt(name)) << ",\"hash\":" << u32(bytes, p + 16)
                << ",\"type\":" << type << ",\"value\":";
            if (type == 0)
                out << quote(stringAt(value));
            else if (type == 1) {
                float f;
                std::memcpy(&f, &value, sizeof(f));
                require((value & 0x7f800000) != 0x7f800000, "non-finite config number");
                out << std::setprecision(9) << f;
            } else if (type == 2)
                nodes(out, value, children, depth + 1);
            else
                throw std::runtime_error("unknown config type " + std::to_string(type));
            require(type == 2 || children == 0, "scalar config has children");
            out << '}';
        }
        out << ']';
    }

  public:
    explicit Config(const Bytes &b) : bytes(b) {
        require(bytes.size() >= 20 && u32(bytes, 0) == 0x46434647 && u32(bytes, 4) == 3,
                "expected GFCF version 3");
        count = u32(bytes, 12);
        roots = u32(bytes, 16);
        uint64_t boundary = 20 + uint64_t(count) * 32;
        require(boundary + u32(bytes, 8) == bytes.size(), "invalid config layout");
        strings = boundary;
        visited.resize(count);
    }
    std::string json() {
        std::ostringstream out;
        nodes(out, 0, roots, 0);
        require(std::all_of(visited.begin(), visited.end(), [](auto x) { return x == 1; }),
                "unreachable config nodes");
        return out.str();
    }
};
