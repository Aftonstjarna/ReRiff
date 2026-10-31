#pragma once
#include "native_common.hpp"
#include <png.h>

namespace rr {
using Vertex = std::array<float, 14>;
struct Buffer {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
struct Image {
    uint32_t width = 0, height = 0;
    Bytes pixels;
};
struct Texture {
    std::string name, fourcc;
    uint32_t width, height, flags, mips, caps;
    Bytes dds;
};
struct AssetMesh {
    uint32_t material, flags, stride;
    std::vector<V> positions, normals;
    std::vector<std::array<float, 2>> uv;
    std::vector<uint32_t> indices;
};
struct AssetNode {
    std::string name;
    Matrix matrix;
    std::vector<int> meshes;
};
struct Model {
    Json materials;
    std::vector<AssetMesh> meshes;
    std::vector<AssetNode> nodes;
};
inline float half(uint16_t h) {
    int sign = (h & 0x8000) ? -1 : 1;
    unsigned exp = (h >> 10) & 31, mant = h & 1023;
    require(exp != 31, "nonfinite half-float");
    return F(sign * (exp ? std::ldexp(1. + double(mant) / 1024, int(exp) - 15)
                         : std::ldexp(double(mant), -24)));
}
inline std::string safe(std::string name) {
    for (char &c : name)
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-')
            c = '_';
    require(!name.empty() && name != "." && name != "..", "invalid asset filename");
    return name;
}
inline Model decodeModel(const Bytes &b) {
    require(b.size() >= 128 && std::memcmp(b.data(), "EADB", 4) == 0 && u32(b, 4) == 38 &&
                u32(b, 8) == 1,
            "expected PC EADB v38");
    uint32_t mc = u32(b, 12), pc = u32(b, 16), nc = u32(b, 20), sc = u32(b, 24);
    uint32_t ma = u32(b, 68), pa = u32(b, 72), na = u32(b, 76), sa = u32(b, 84), la = u32(b, 88),
             va = u32(b, 92), ia = u32(b, 96);
    require(128 <= ma && ma <= pa && pa <= na && na <= sa && sa <= la && la <= va && va <= ia &&
                ia <= b.size(),
            "invalid EADB boundaries");
    require(uint64_t(ma) + uint64_t(mc) * 464 <= pa && uint64_t(pa) + uint64_t(pc) * 128 <= na &&
                uint64_t(na) + uint64_t(nc) * 156 <= sa && uint64_t(sa) + uint64_t(sc) * 48 <= la,
            "invalid EADB tables");
    Json params = Json::array();
    for (uint32_t i = 0; i < pc; i++) {
        size_t p = pa + i * 128;
        Json vals = Json::array();
        for (int j = 0; j < 4; j++)
            vals.push_back(f32(b, p + 112 + j * 4));
        params.push_back(
            {{"name", text(b, p, 64)}, {"runtime_word", u32(b, p + 100)}, {"value", vals}});
    }
    Model out;
    out.materials = Json::array();
    for (uint32_t i = 0; i < mc; i++) {
        size_t p = ma + i * 464;
        Json slots = Json::array();
        for (int j = 0; j < 8; j++)
            slots.push_back(text(b, p + 64 + j * 32, 32));
        uint32_t n = u32(b, p + 436), first = u32(b, p + 440);
        require(first <= pc && n <= pc - first, "invalid material parameters");
        Json selected = Json::array();
        for (uint32_t j = first; j < first + n; j++)
            selected.push_back(params[j]);
        out.materials.push_back({{"name", text(b, p, 64)},
                                 {"textures", slots},
                                 {"shader_id", u32(b, p + 444)},
                                 {"parameters", selected}});
    }
    for (uint32_t i = 0; i < sc; i++) {
        size_t p = sa + i * 48;
        uint32_t flags = u32(b, p), stride = u32(b, p + 4), fv = u32(b, p + 8), vc = u32(b, p + 12),
                 fi = u32(b, p + 16), ic = u32(b, p + 20);
        require(stride >= 24 && stride <= 48 && stride % 4 == 0 && (flags & 0xfff) < mc &&
                    ic % 3 == 0,
                "unsupported mesh format");
        require(uint64_t(va) + (uint64_t(fv) + vc) * stride <= ia &&
                    uint64_t(ia) + (uint64_t(fi) + ic) * 2 <= b.size(),
                "mesh buffer outside file");
        AssetMesh m;
        m.material = flags & 0xfff;
        m.flags = flags;
        m.stride = stride;
        for (uint32_t j = 0; j < vc; j++) {
            size_t v = va + (uint64_t(fv) + j) * stride;
            m.positions.push_back({f32(b, v), f32(b, v + 4), f32(b, v + 8)});
            m.normals.push_back({half(u16(b, v + 12)), half(u16(b, v + 14)), half(u16(b, v + 16))});
            m.uv.push_back({half(u16(b, v + 20)), half(u16(b, v + 22))});
        }
        for (uint32_t j = 0; j < ic; j++) {
            uint32_t index = u16(b, ia + (uint64_t(fi) + j) * 2);
            require(index >= fv && index - fv < vc, "mesh index outside vertices");
            m.indices.push_back(index - fv);
        }
        out.meshes.push_back(std::move(m));
    }
    std::set<int> referenced;
    for (uint32_t i = 0; i < nc; i++) {
        size_t p = na + i * 156;
        AssetNode n;
        n.name = text(b, p, 32);
        for (int j = 0; j < 12; j++)
            n.matrix[j] = f32(b, p + 32 + j * 4);
        int32_t first = int32_t(u32(b, p + 120));
        require(first >= 0, "negative mesh link");
        size_t cursor = la + uint64_t(first) * 2;
        while (true) {
            require(cursor + 2 <= va, "unterminated mesh links");
            int16_t link = int16_t(u16(b, cursor));
            cursor += 2;
            if (link == -1)
                break;
            require(link >= 0 && link < int(sc), "invalid mesh link");
            n.meshes.push_back(link);
            referenced.insert(link);
        }
        out.nodes.push_back(std::move(n));
    }
    require(referenced.size() == sc, "unreferenced mesh");
    return out;
}
inline std::vector<Texture> decodeTextures(const Bytes &b) {
    require(b.size() >= 28 && std::memcmp(b.data(), "TABT", 4) == 0 && u32(b, 4) == 0xffee &&
                u32(b, 8) == 4,
            "expected TABT v4");
    uint32_t count = u32(b, 16), poolSize = u32(b, 20), payloadSize = u32(b, 24);
    size_t strings = 28 + uint64_t(count) * 24, payload = strings + poolSize;
    require(payload == u32(b, 12) && payload <= b.size() && payload + payloadSize == b.size(),
            "invalid TABT lengths");
    std::set<std::string> names;
    std::vector<Texture> out;
    for (uint32_t i = 0; i < count; i++) {
        size_t p = 28 + i * 24;
        uint32_t name = u32(b, p), offset = u32(b, p + 4), size = u32(b, p + 8),
                 width = u32(b, p + 12), height = u32(b, p + 16), flags = u32(b, p + 20);
        require(name < poolSize && offset <= payloadSize && size <= payloadSize - offset,
                "invalid texture range");
        std::string label = text(b, strings + name, poolSize - name);
        require(!label.empty() && names.insert(label).second, "duplicate texture name");
        Bytes dds(b.begin() + payload + offset, b.begin() + payload + offset + size);
        require(dds.size() >= 128 && std::memcmp(dds.data(), "DDS ", 4) == 0 &&
                    u32(dds, 4) == 124 && u32(dds, 12) == height && u32(dds, 16) == width,
                "invalid DDS header");
        out.push_back({label, std::string(dds.begin() + 84, dds.begin() + 88), width, height, flags,
                       u32(dds, 28), u32(dds, 112), std::move(dds)});
    }
    return out;
}
inline Image decodeDDS(const Texture &t, int face = 0) {
    require(t.width && t.height && t.width <= 4096 && t.height <= 4096,
            "unsupported texture dimensions");
    Image out{t.width, t.height, Bytes(size_t(t.width) * t.height * 4)};
    bool dxt1 = t.fourcc == "DXT1", dxt3 = t.fourcc == "DXT3", dxt5 = t.fourcc == "DXT5";
    require(dxt1 || dxt3 || dxt5 || u32(t.dds, 88) == 32, "unsupported DDS pixel format");
    size_t faceSize = 0;
    for (uint32_t level = 0; level < std::max(1u, t.mips); level++) {
        require(level < 32, "invalid mip count");
        uint32_t w = std::max(1u, t.width >> level), h = std::max(1u, t.height >> level);
        faceSize += (dxt1 || dxt3 || dxt5) ? size_t((w + 3) / 4) * ((h + 3) / 4) * (dxt1 ? 8 : 16)
                                           : size_t(w) * h * 4;
    }
    require((t.caps == 0 && face == 0) || (t.caps == 0xfe00 && face >= 0 && face < 6),
            "unsupported DDS cube/volume layout");
    require(128 + faceSize * (t.caps ? 6 : 1) <= t.dds.size(), "truncated DDS image");
    size_t start = 128 + face * faceSize;
    if (!(dxt1 || dxt3 || dxt5)) {
        uint32_t masks[4] = {u32(t.dds, 92), u32(t.dds, 96), u32(t.dds, 100), u32(t.dds, 104)};
        for (size_t i = 0; i < size_t(t.width) * t.height; i++) {
            uint32_t pixel = u32(t.dds, start + i * 4);
            for (int c = 0; c < 4; c++) {
                uint32_t mask = masks[c];
                if (!mask) {
                    out.pixels[i * 4 + c] = c == 3 ? 255 : 0;
                    continue;
                }
                int shift = 0;
                while ((mask & 1) == 0) {
                    mask >>= 1;
                    shift++;
                }
                out.pixels[i * 4 + c] = uint8_t(((pixel >> shift) & mask) * 255u / mask);
            }
        }
        return out;
    }
    size_t p = start;
    for (uint32_t by = 0; by < t.height; by += 4)
        for (uint32_t bx = 0; bx < t.width; bx += 4) {
            size_t color = p + (dxt1 ? 0 : 8);
            uint16_t a = u16(t.dds, color), b = u16(t.dds, color + 2);
            std::array<std::array<uint8_t, 4>, 4> colors{};
            auto rgb = [](uint16_t x) {
                return std::array<uint8_t, 4>{uint8_t(((x >> 11) & 31) * 255 / 31),
                                              uint8_t(((x >> 5) & 63) * 255 / 63),
                                              uint8_t((x & 31) * 255 / 31), 255};
            };
            colors[0] = rgb(a);
            colors[1] = rgb(b);
            if (a > b || !dxt1) {
                for (int c = 0; c < 3; c++) {
                    colors[2][c] = uint8_t((2 * int(colors[0][c]) + colors[1][c]) / 3);
                    colors[3][c] = uint8_t((int(colors[0][c]) + 2 * colors[1][c]) / 3);
                }
                colors[2][3] = colors[3][3] = 255;
            } else {
                for (int c = 0; c < 3; c++)
                    colors[2][c] = uint8_t((int(colors[0][c]) + colors[1][c]) / 2);
                colors[2][3] = 255;
            }
            uint32_t indices = u32(t.dds, color + 4);
            std::array<uint8_t, 8> alpha{};
            uint64_t alphaBits = 0;
            if (dxt5) {
                alpha[0] = t.dds[p];
                alpha[1] = t.dds[p + 1];
                if (alpha[0] > alpha[1])
                    for (int i = 2; i < 8; i++)
                        alpha[i] = uint8_t(((8 - i) * int(alpha[0]) + (i - 1) * int(alpha[1])) / 7);
                else {
                    for (int i = 2; i < 6; i++)
                        alpha[i] = uint8_t(((6 - i) * int(alpha[0]) + (i - 1) * int(alpha[1])) / 5);
                    alpha[6] = 0;
                    alpha[7] = 255;
                }
                for (int i = 0; i < 6; i++)
                    alphaBits |= uint64_t(t.dds[p + 2 + i]) << (i * 8);
            }
            for (int i = 0; i < 16; i++) {
                uint32_t x = bx + i % 4, y = by + i / 4;
                if (x >= t.width || y >= t.height)
                    continue;
                auto rgba = colors[(indices >> (2 * i)) & 3];
                if (dxt5)
                    rgba[3] = alpha[(alphaBits >> (3 * i)) & 7];
                if (dxt3)
                    rgba[3] = uint8_t(((t.dds[p + i / 2] >> ((i % 2) * 4)) & 15) * 17);
                std::copy(rgba.begin(), rgba.end(),
                          out.pixels.begin() + (size_t(y) * t.width + x) * 4);
            }
            p += dxt1 ? 8 : 16;
        }
    return out;
}
inline void savePNG(const fs::path &p, const Image &image) {
    fs::create_directories(p.parent_path());
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = image.width;
    png.height = image.height;
    png.format = PNG_FORMAT_RGBA;
    require(png_image_write_to_file(&png, p.c_str(), 0, image.pixels.data(), 0, nullptr) != 0,
            "PNG write failed: " + p.string());
}
inline Json material(const Json &m) {
    std::string name = lower(m.at("name"));
    std::array<float, 4> color{1, 1, 1, 1};
    float metallic = 0, rough = .6f;
    auto contains = [&](const char *part) { return name.find(part) != std::string::npos; };
    if (contains("carpaint")) {
        color = {.18f, .3f, .55f, 1};
        for (const auto &p : m["parameters"])
            if (p["name"] == "g_color0")
                for (int i = 0; i < 3; i++)
                    color[i] = std::clamp(p["value"][i].get<float>(), 0.f, 1.f);
        metallic = .7f;
        rough = .25f;
    } else if (contains("glass")) {
        color = {.025f, .045f, .065f, 1};
        rough = .15f;
    } else if (contains("wheel") || (contains("metal") && !contains("black"))) {
        color = {.38f, .4f, .43f, 1};
        metallic = 1;
        rough = .25f;
    } else if (contains("tyre") || contains("blackplastic") || contains("undercarriage") ||
               contains("grille")) {
        color = {.018f, .022f, .028f, 1};
        rough = .8f;
    } else if (contains("blackmetal")) {
        color = {.045f, .05f, .06f, 1};
        metallic = .9f;
        rough = .3f;
    } else if (contains("bumper")) {
        color = {.12f, .13f, .15f, 1};
        rough = .5f;
    } else if (contains("lights")) {
        color = {.8f, .82f, .85f, 1};
        rough = .2f;
    }
    return {
        {"name", m["name"]},
        {"pbrMetallicRoughness",
         {{"baseColorFactor", color}, {"metallicFactor", metallic}, {"roughnessFactor", rough}}},
        {"doubleSided", true},
        {"extras",
         {{"originalTextureSlots", m["textures"]},
          {"originalShaderId", m["shader_id"]},
          {"originalParameters", m["parameters"]},
          {"inspectionApproximation", true}}}};
}
inline bool bodyNode(const AssetNode &n) {
    return n.name == "body" || n.name.rfind("wheel_", 0) == 0;
}
inline Buffer vehicleBuffer(const Model &model) {
    Buffer out;
    for (const auto &node : model.nodes)
        if (bodyNode(node))
            for (int index : node.meshes) {
                const auto &mesh = model.meshes[index];
                auto mat = material(model.materials[mesh.material])["pbrMetallicRoughness"];
                auto color = mat["baseColorFactor"].get<std::array<float, 4>>();
                uint32_t first = out.vertices.size();
                for (size_t i = 0; i < mesh.positions.size(); i++) {
                    V p = transform(node.matrix, mesh.positions[i], true),
                      n = transform(node.matrix, mesh.normals[i]);
                    double len = length(n);
                    require(len > 0, "zero runtime normal");
                    out.vertices.push_back({p.x, p.y, p.z, F(n.x / len), F(n.y / len), F(n.z / len),
                                            color[0], color[1], color[2],
                                            mat["metallicFactor"].get<float>(),
                                            mat["roughnessFactor"].get<float>(), 0, 0, 0});
                }
                for (auto i : mesh.indices)
                    out.indices.push_back(first + i);
            }
    require(!out.vertices.empty() && !out.indices.empty(), "no runtime body/wheels");
    return out;
}
inline void writeMesh(const fs::path &p, const Buffer &mesh, bool textured = true) {
    require(!mesh.vertices.empty() && !mesh.indices.empty() && mesh.indices.size() % 3 == 0,
            "invalid empty mesh");
    Bytes b;
    const std::string magic = textured ? std::string("RRMESH2\0", 8) : std::string("RRMESH1\0", 8);
    b.insert(b.end(), magic.begin(), magic.end());
    appendU32(b, mesh.vertices.size());
    appendU32(b, mesh.indices.size());
    for (int side = 0; side < 2; side++)
        for (int k = 0; k < 3; k++) {
            float bound = mesh.vertices[0][k];
            for (const auto &v : mesh.vertices)
                bound = side ? std::max(bound, v[k]) : std::min(bound, v[k]);
            appendFloat(b, bound);
        }
    for (const auto &v : mesh.vertices) {
        for (int k = 0; k < (textured ? 14 : 11); k++) {
            require(std::isfinite(v[k]), "nonfinite output vertex");
            appendFloat(b, v[k]);
        }
    }
    for (auto index : mesh.indices) {
        require(index < mesh.vertices.size(), "invalid output index");
        appendU32(b, index);
    }
    write(p, b);
}
inline void exportGLTF(const Model &model, const std::vector<Texture> &textures,
                       const fs::path &out) {
    Json gltf = {{"asset", {{"version", "2.0"}, {"generator", "ReRiff C++ asset decoder"}}},
                 {"scene", 0},
                 {"scenes", Json::array({Json{{"nodes", Json::array()}}})},
                 {"nodes", Json::array()},
                 {"meshes", Json::array()},
                 {"materials", Json::array()},
                 {"buffers", Json::array()},
                 {"bufferViews", Json::array()},
                 {"accessors", Json::array()},
                 {"images", Json::array()},
                 {"textures", Json::array()},
                 {"samplers", Json::array({Json{{"wrapS", 10497}, {"wrapT", 10497}}})}};
    Bytes binary;
    auto accessor = [&](const Bytes &data, int count, const char *kind, int component, int target,
                        Json bounds = Json::object()) {
        while (binary.size() % 4)
            binary.push_back(0);
        size_t start = binary.size();
        binary.insert(binary.end(), data.begin(), data.end());
        int view = gltf["bufferViews"].size();
        gltf["bufferViews"].push_back({{"buffer", 0},
                                       {"byteOffset", start},
                                       {"byteLength", data.size()},
                                       {"target", target}});
        int index = gltf["accessors"].size();
        Json value = {
            {"bufferView", view}, {"componentType", component}, {"count", count}, {"type", kind}};
        value.update(bounds);
        gltf["accessors"].push_back(value);
        return index;
    };
    for (size_t i = 0; i < textures.size(); i++) {
        gltf["images"].push_back({{"uri", "textures/" + safe(textures[i].name) + ".png"}});
        gltf["textures"].push_back({{"sampler", 0}, {"source", i}});
    }
    for (const auto &m : model.materials) {
        auto mat = material(m);
        mat["pbrMetallicRoughness"] = {
            {"baseColorFactor", {1, 1, 1, 1}}, {"metallicFactor", 0}, {"roughnessFactor", .6}};
        std::string diffuse = lower(fs::path(m["textures"][0].get<std::string>()).stem());
        for (size_t i = 0; i < textures.size(); i++)
            if (lower(textures[i].name) == diffuse)
                mat["pbrMetallicRoughness"]["baseColorTexture"] = {{"index", i}};
        gltf["materials"].push_back(mat);
    }
    for (const auto &m : model.meshes) {
        Bytes positions, normals, uvs, indices;
        std::array<float, 3> lo{INFINITY, INFINITY, INFINITY}, hi{-INFINITY, -INFINITY, -INFINITY};
        for (size_t i = 0; i < m.positions.size(); i++) {
            double len = length(m.normals[i]);
            require(len > 0, "zero glTF normal");
            for (int k = 0; k < 3; k++) {
                appendFloat(positions, m.positions[i][k]);
                appendFloat(normals, F(m.normals[i][k] / len));
                lo[k] = std::min(lo[k], m.positions[i][k]);
                hi[k] = std::max(hi[k], m.positions[i][k]);
            }
            for (float u : m.uv[i])
                appendFloat(uvs, u);
        }
        for (auto i : m.indices)
            appendU32(indices, i);
        int p = accessor(positions, m.positions.size(), "VEC3", 5126, 34962,
                         {{"min", lo}, {"max", hi}}),
            n = accessor(normals, m.normals.size(), "VEC3", 5126, 34962),
            uv = accessor(uvs, m.uv.size(), "VEC2", 5126, 34962),
            ix = accessor(indices, m.indices.size(), "SCALAR", 5125, 34963);
        gltf["meshes"].push_back(
            {{"primitives",
              Json::array(
                  {Json{{"attributes", {{"POSITION", p}, {"NORMAL", n}, {"TEXCOORD_0", uv}}},
                        {"indices", ix},
                        {"material", m.material},
                        {"mode", 4}}})}});
    }
    Json inspectionRoots = Json::array();
    for (const auto &node : model.nodes) {
        Json matrix = Json::array();
        for (int i = 0; i < 4; i++) {
            for (int k = 0; k < 3; k++)
                matrix.push_back(node.matrix[i * 3 + k]);
            matrix.push_back(i == 3 ? 1 : 0);
        }
        int parent = gltf["nodes"].size();
        gltf["scenes"][0]["nodes"].push_back(parent);
        if (bodyNode(node))
            inspectionRoots.push_back(parent);
        gltf["nodes"].push_back(
            {{"name", node.name}, {"matrix", matrix}, {"children", Json::array()}});
        for (int index : node.meshes) {
            int child = gltf["nodes"].size();
            gltf["nodes"][parent]["children"].push_back(child);
            gltf["nodes"].push_back(
                {{"name", node.name + "_" + std::to_string(index)}, {"mesh", index}});
        }
    }
    gltf["buffers"].push_back({{"uri", "vehicle.bin"}, {"byteLength", binary.size()}});
    write(out / "vehicle.bin", binary);
    save(out / "vehicle.gltf", gltf);
    std::set<int> retained;
    for (auto root : inspectionRoots) {
        retained.insert(root.get<int>());
        for (auto child : gltf["nodes"][root.get<int>()]["children"])
            retained.insert(child.get<int>());
    }
    std::map<int, int> mapping;
    for (int index : retained)
        mapping[index] = int(mapping.size());
    Json clean = Json::array();
    for (int index : retained) {
        auto n = gltf["nodes"][index];
        if (n.contains("children"))
            for (auto &c : n["children"])
                c = mapping.at(c.get<int>());
        clean.push_back(n);
    }
    gltf["nodes"] = clean;
    for (auto &r : inspectionRoots)
        r = mapping.at(r.get<int>());
    gltf["scenes"][0]["nodes"] = inspectionRoots;
    gltf["materials"] = Json::array();
    for (const auto &m : model.materials)
        gltf["materials"].push_back(material(m));
    save(out / "vehicle-inspection.gltf", gltf);
}
inline Model exportVehicle(const Archive &archive, const std::string &vehicle,
                           const fs::path &out) {
    require(!vehicle.empty() && std::all_of(vehicle.begin(), vehicle.end(),
                                            [](unsigned char c) {
                                                return (c >= 'a' && c <= 'z') ||
                                                       (c >= '0' && c <= '9');
                                            }),
            "invalid vehicle ID");
    std::string prefix = "graphics/vehicles/" + vehicle + "/";
    auto mesh = asset(archive, prefix + "lod0_livery.bdae"),
         textureBytes = asset(archive, prefix + "textures/batchTextures.brres");
    Model model = decodeModel(mesh);
    auto textures = decodeTextures(textureBytes);
    std::set<std::string> filenames;
    for (const auto &t : textures) {
        auto name = safe(t.name);
        require(filenames.insert(name).second, "texture filename collision");
        write(out / "textures" / (name + ".dds"), t.dds);
        savePNG(out / "textures" / (name + ".png"), decodeDDS(t));
        if (t.caps) {
            std::array<const char *, 6> suffix{"px", "nx", "py", "ny", "pz", "nz"};
            for (int face = 0; face < 6; face++)
                savePNG(out / "textures" / (name + "_" + suffix[face] + ".png"),
                        decodeDDS(t, face));
        }
    }
    auto buffer = vehicleBuffer(model);
    writeMesh(out / "vehicle.rmesh", buffer, false);
    exportGLTF(model, textures, out);
    Json nodes = Json::array();
    for (const auto &n : model.nodes)
        nodes.push_back({{"name", n.name}, {"transform", n.matrix}, {"meshes", n.meshes}});
    save(out / "manifest.json",
         {{"implementation", "C++"},
          {"vehicle", vehicle},
          {"source_mesh", prefix + "lod0_livery.bdae"},
          {"source_mesh_sha256", sha(mesh)},
          {"source_texture_sha256", sha(textureBytes)},
          {"mesh_count", model.meshes.size()},
          {"node_count", model.nodes.size()},
          {"vertices", buffer.vertices.size()},
          {"triangles", buffer.indices.size() / 3},
          {"materials", model.materials},
          {"nodes", nodes},
          {"limitations",
           {"Approximate static materials; original animated shaders are not implemented."}}});
    return model;
}
} // namespace rr
