#pragma once
#include "archive.hpp"
#include "third_party/nlohmann/json.hpp"
#include "track_math.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <openssl/evp.h>
#include <sys/wait.h>
#include <unistd.h>

namespace rr {
namespace fs = std::filesystem;
using Json = nlohmann::ordered_json;
using V = reriff::track::Vec3;
using Frame = reriff::track::Frame;
using Matrix = std::array<float, 12>;
inline constexpr const char *gameHash =
    "2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2";
inline const Matrix identity{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
inline float F(double x) {
    return static_cast<float>(x);
}
inline double sq(double x) {
    return x * x;
}
inline std::string lower(std::string s) {
    for (char &c : s)
        c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
inline std::string sha(const Bytes &b) {
    std::array<unsigned char, 32> result{};
    unsigned size = 0;
    require(EVP_Digest(b.data(), b.size(), result.data(), &size, EVP_sha256(), nullptr) == 1 &&
                size == 32,
            "SHA256 failed");
    std::ostringstream out;
    for (auto c : result)
        out << std::hex << std::setw(2) << std::setfill('0') << unsigned(c);
    return out.str();
}
inline float f32(const Bytes &b, size_t p) {
    uint32_t bits = u32(b, p);
    float v;
    std::memcpy(&v, &bits, 4);
    require(std::isfinite(v), "nonfinite binary float");
    return v;
}
inline std::string text(const Bytes &b, size_t p, size_t n) {
    require(p <= b.size() && n <= b.size() - p, "string outside buffer");
    auto end = std::find(b.begin() + p, b.begin() + p + n, 0);
    require(end != b.begin() + p + n, "unterminated string");
    return std::string(b.begin() + p, end);
}
inline void write(const fs::path &path, const Bytes &bytes) {
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path());
    std::ofstream f(path, std::ios::binary);
    require(bool(f), "cannot write " + path.string());
    f.write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size()));
    require(bool(f), "write failed " + path.string());
}
inline void save(const fs::path &p, const Json &j) {
    std::string s = j.dump(2) + '\n';
    write(p, Bytes(s.begin(), s.end()));
}
inline Json loadJson(const fs::path &p) {
    return Json::parse(read(p.string()));
}
inline Bytes asset(const Archive &a, const std::string &name) {
    for (const auto &e : a.entries)
        if (e.name == name)
            return a.load(e);
    throw std::runtime_error("asset missing: " + name);
}
inline Json cfgValue(const Json &records) {
    bool array = !records.empty() && std::all_of(records.begin(), records.end(),
                                                 [](const auto &n) { return n.at("name") == ""; });
    Json result = array ? Json::array() : Json::object();
    for (const auto &n : records) {
        std::string key = n.at("name");
        if (!array && result.contains(key))
            continue;
        Json v = n.at("type") == 2 ? cfgValue(n.at("value")) : n.at("value");
        if (array)
            result.push_back(v);
        else
            result[key] = v;
    }
    return result;
}
inline Json config(const Bytes &data) {
    return cfgValue(Json::parse(Config(data).json()));
}
inline std::vector<Json> values(const Json &j) {
    std::vector<Json> out;
    for (const auto &v : j)
        out.push_back(v);
    return out;
}
inline double number(const Json &j, const std::string &key, double fallback = 0) {
    return j.contains(key) ? j.at(key).get<double>() : fallback;
}
inline V vec(const Json &j) {
    require(j.is_array() && j.size() == 3, "expected float3");
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}
inline Json json(V v) {
    return Json::array({v.x, v.y, v.z});
}
inline V add(V a, V b) {
    return reriff::track::add(a, b);
}
inline V sub(V a, V b) {
    return reriff::track::sub(a, b);
}
inline V scale(V a, float b) {
    return reriff::track::scale(a, b);
}
inline double dot(V a, V b) {
    return double(a.x) * b.x + double(a.y) * b.y + double(a.z) * b.z;
}
inline double length(V a) {
    return std::sqrt(dot(a, a));
}
inline V cross(V a, V b) {
    return {F(double(a.y) * b.z - double(a.z) * b.y), F(double(a.z) * b.x - double(a.x) * b.z),
            F(double(a.x) * b.y - double(a.y) * b.x)};
}
inline V norm(V a) {
    return reriff::track::normalize(a);
}
inline V basis(const Matrix &m, int i) {
    return {m[i * 3], m[i * 3 + 1], m[i * 3 + 2]};
}
inline void setBasis(Matrix &m, int i, V v) {
    m[i * 3] = v.x;
    m[i * 3 + 1] = v.y;
    m[i * 3 + 2] = v.z;
}
inline V transform(const Matrix &m, V p, bool translate = false) {
    V out{};
    for (int k = 0; k < 3; k++)
        out[k] = F(double(m[k]) * p.x + double(m[k + 3]) * p.y + double(m[k + 6]) * p.z +
                   (translate ? m[k + 9] : 0));
    return out;
}
inline V transform(const Frame &f, V p) {
    Matrix m{};
    setBasis(m, 0, f.right);
    setBasis(m, 1, f.up);
    setBasis(m, 2, f.forward);
    return transform(m, p);
}
inline Matrix compose(const Matrix &a, const Matrix &b, bool rotateOnly = false) {
    Matrix result{};
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 3; col++) {
            if (row == 3 && rotateOnly) {
                result[9 + col] = a[9 + col];
                continue;
            }
            float v =
                F(double(F(double(a[row * 3]) * b[col])) + F(double(a[row * 3 + 1]) * b[col + 3]));
            v = F(double(v) + F(double(a[row * 3 + 2]) * b[col + 6]));
            result[row * 3 + col] = row == 3 ? F(double(v) + b[9 + col]) : v;
        }
    return result;
}
inline Matrix rotation(int axis, float degrees) {
    float angle = F(double(degrees) * 0.017453293005625408), s = F(std::sin(double(angle))),
          c = F(std::cos(double(angle)));
    if (axis == 0)
        return {1, 0, 0, 0, c, s, 0, -s, c, 0, 0, 0};
    if (axis == 1)
        return {c, 0, -s, 0, 1, 0, s, 0, c, 0, 0, 0};
    return {c, s, 0, -s, c, 0, 0, 0, 1, 0, 0, 0};
}
inline void appendU32(Bytes &b, uint32_t value) {
    for (int k = 0; k < 4; k++)
        b.push_back(uint8_t(value >> (8 * k)));
}
inline void appendFloat(Bytes &b, float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, 4);
    appendU32(b, bits);
}
inline int run(const std::vector<std::string> &args) {
    require(!args.empty(), "empty process command");
    std::vector<char *> argv;
    for (const auto &s : args)
        argv.push_back(const_cast<char *>(s.c_str()));
    argv.push_back(nullptr);
    pid_t pid = fork();
    require(pid >= 0, "fork failed");
    if (pid == 0) {
        execvp(argv[0], argv.data());
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        require(errno == EINTR, "waitpid failed");
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
} // namespace rr
