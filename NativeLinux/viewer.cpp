// Native SDL3 GPU / Vulkan viewer for decoded ReRiff vehicle buffers.
#include "playback.hpp"
#include "song_audio.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
static void check(bool ok, const std::string &what) {
    if (!ok)
        throw std::runtime_error(what + ": " + SDL_GetError());
}
static std::vector<uint8_t> readFile(const fs::path &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Cannot open " + path.string());
    auto size = file.tellg();
    if (size < 0 || size > 256 * 1024 * 1024)
        throw std::runtime_error("Invalid file size: " + path.string());
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(bytes.data()), size))
        throw std::runtime_error("Truncated file");
    return bytes;
}
struct Vec {
    float x, y, z;
    Vec operator+(Vec b) const {
        return {x + b.x, y + b.y, z + b.z};
    }
    Vec operator-(Vec b) const {
        return {x - b.x, y - b.y, z - b.z};
    }
    Vec operator*(float b) const {
        return {x * b, y * b, z * b};
    }
};
static float dot(Vec a, Vec b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
static Vec cross(Vec a, Vec b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static Vec normalized(Vec v) {
    return v * (1 / std::sqrt(dot(v, v)));
}
using Mat = std::array<float, 16>;
static Mat multiply(const Mat &a, const Mat &b) {
    Mat out{};
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            for (int k = 0; k < 4; ++k)
                out[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
    return out;
}
static Mat cameraMatrix(Vec eye, Vec target, float aspect, float far, Vec up = {0, 1, 0}) {
    Vec f = normalized(target - eye), s = normalized(cross(f, up)), u = cross(s, f);
    Mat view = {s.x, u.x, -f.x, 0, s.y,          u.y,          -f.y,        0,
                s.z, u.z, -f.z, 0, -dot(s, eye), -dot(u, eye), dot(f, eye), 1};
    float focal = 1 / std::tan(0.6f / 2), near = 0.03f;
    Mat projection{};
    projection[0] = focal / aspect;
    projection[5] = focal;
    projection[10] = far / (near - far);
    projection[11] = -1;
    projection[14] = near * far / (near - far);
    return multiply(projection, view);
}
struct Vertex {
    float position[3], normal[3], color[3], material[2], texture[3];
};
static_assert(sizeof(Vertex) == 56);

static Mat identity() {
    return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

class Viewer {
    SDL_GPUDevice *device = nullptr;
    SDL_Window *window = nullptr;
    SDL_GPUBuffer *vertexBuffer = nullptr;
    SDL_GPUBuffer *indexBuffer = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;
    SDL_GPUTexture *depth = nullptr;
    SDL_GPUTexture *offscreen = nullptr;
    SDL_GPUTexture *surfaceTexture = nullptr;
    SDL_GPUSampler *surfaceSampler = nullptr;
    uint32_t width = 1280, height = 800, indexCount = 0, vertexCount = 0;
    Vec center{}, initialCenter{};
    float radius = 3, distance = 9, yaw = 0.75f, pitch = 0.3f;
    float initialPitch = 0.3f;
    bool wireframe = false;
    reriff::SongPath song;
    SongClock clock;
    bool playing = false, follow = true;
    Mat carModel = identity();
    float songParameter = 0;
    void updateSong(double seconds) {
        songParameter = song.parameter(static_cast<float>(seconds));
        auto p = song.position(songParameter);
        auto frame = song.frame(songParameter);
        p = reriff::track::add(p, reriff::track::scale(frame.up, song.lift));
        auto r = frame.right, u = frame.up, f = frame.forward;
        carModel = {r.x, r.y, r.z, 0, u.x, u.y, u.z, 0, f.x, f.y, f.z, 0, p.x, p.y, p.z, 1};
        if (follow) {
            center = {p.x + u.x, p.y + u.y, p.z + u.z};
            distance = 22;
            yaw = std::atan2(-f.x, -f.z);
            pitch = .32f;
        }
        if (window) {
            std::string title = "ReRiff | " + std::to_string(int(seconds)) + "s | node " +
                                std::to_string(songParameter) +
                                (clock.isPaused() ? " | Paused" : " | Playing") +
                                " | Space: pause | J/L: seek | Backspace: restart | C: follow";
            SDL_SetWindowTitle(window, title.c_str());
        }
    }
    Vec focusCenter{};
    std::vector<Vec> focusPoints;
    float focusDistance = 9;
    bool hasFocus = false;
    uint32_t highestTextureLayer = 0;
    fs::path shaderDir;
    SDL_GPUTextureFormat colorFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;

    SDL_GPUShader *shader(const std::string &name, SDL_GPUShaderStage stage) {
        auto bytes = readFile(shaderDir / name);
        SDL_GPUShaderCreateInfo info{};
        info.code = bytes.data();
        info.code_size = bytes.size();
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.stage = stage;
        info.num_uniform_buffers = 1;
        if (stage == SDL_GPU_SHADERSTAGE_FRAGMENT)
            info.num_samplers = 1;
        auto result = SDL_CreateGPUShader(device, &info);
        check(result, "Create shader " + name);
        return result;
    }
    void createPipeline() {
        if (pipeline) {
            SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
            pipeline = nullptr;
        }
        auto vert = shader("vehicle.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
        SDL_GPUShader *frag = nullptr;
        try {
            frag = shader("vehicle.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);
        } catch (...) {
            SDL_ReleaseGPUShader(device, vert);
            throw;
        }
        SDL_GPUVertexBufferDescription buffer{};
        buffer.slot = 0;
        buffer.pitch = sizeof(Vertex);
        buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
        SDL_GPUVertexAttribute attrs[5]{};
        for (uint32_t i = 0; i < 5; ++i) {
            attrs[i].location = i;
            attrs[i].buffer_slot = 0;
            attrs[i].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
        }
        attrs[0].offset = offsetof(Vertex, position);
        attrs[1].offset = offsetof(Vertex, normal);
        attrs[2].offset = offsetof(Vertex, color);
        attrs[3].offset = offsetof(Vertex, material);
        attrs[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
        attrs[4].offset = offsetof(Vertex, texture);
        SDL_GPUColorTargetDescription target{};
        target.format = colorFormat;
        SDL_GPUGraphicsPipelineCreateInfo info{};
        info.vertex_shader = vert;
        info.fragment_shader = frag;
        info.vertex_input_state.vertex_buffer_descriptions = &buffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = attrs;
        info.vertex_input_state.num_vertex_attributes = 5;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = wireframe ? SDL_GPU_FILLMODE_LINE : SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
        info.rasterizer_state.enable_depth_clip = true;
        info.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
        info.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
        info.depth_stencil_state.enable_depth_test = true;
        info.depth_stencil_state.enable_depth_write = true;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        info.target_info.has_depth_stencil_target = true;
        info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        pipeline = SDL_CreateGPUGraphicsPipeline(device, &info);
        SDL_ReleaseGPUShader(device, vert);
        SDL_ReleaseGPUShader(device, frag);
        check(pipeline, "Create graphics pipeline");
    }
    SDL_GPUTexture *texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage) {
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = format;
        info.usage = usage;
        info.width = width;
        info.height = height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        auto result = SDL_CreateGPUTexture(device, &info);
        check(result, "Create render texture");
        return result;
    }
    void resize(uint32_t w, uint32_t h) {
        if (depth && width == w && height == h)
            return;
        width = w;
        height = h;
        if (depth)
            SDL_ReleaseGPUTexture(device, depth);
        depth = nullptr;
        depth = texture(SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
    }
    void upload(const std::vector<uint8_t> &bytes) {
        const uint32_t vsize = vertexCount * sizeof(Vertex), isize = indexCount * sizeof(uint32_t),
                       size = vsize + isize;
        SDL_GPUBufferCreateInfo info{};
        info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        info.size = vsize;
        vertexBuffer = SDL_CreateGPUBuffer(device, &info);
        check(vertexBuffer, "Create vertex buffer");
        info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        info.size = isize;
        indexBuffer = SDL_CreateGPUBuffer(device, &info);
        check(indexBuffer, "Create index buffer");
        SDL_GPUTransferBufferCreateInfo tinfo{};
        tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tinfo.size = size;
        auto transfer = SDL_CreateGPUTransferBuffer(device, &tinfo);
        check(transfer, "Create upload buffer");
        void *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
        if (!mapped) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Map upload buffer");
        }
        std::memcpy(mapped, bytes.data() + 40, size);
        SDL_UnmapGPUTransferBuffer(device, transfer);
        auto command = SDL_AcquireGPUCommandBuffer(device);
        if (!command) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Acquire upload command");
        }
        auto pass = SDL_BeginGPUCopyPass(command);
        SDL_GPUTransferBufferLocation source{transfer, 0};
        SDL_GPUBufferRegion destination{vertexBuffer, 0, vsize};
        SDL_UploadToGPUBuffer(pass, &source, &destination, false);
        source.offset = vsize;
        destination = {indexBuffer, 0, isize};
        SDL_UploadToGPUBuffer(pass, &source, &destination, false);
        SDL_EndGPUCopyPass(pass);
        bool ok = SDL_SubmitGPUCommandBuffer(command);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        check(ok, "Submit upload");
    }
    void uploadTextures(const fs::path &path, bool textured) {
        uint32_t tw = 1, th = 1, layers = 1;
        std::vector<uint8_t> bytes(4, 255);
        size_t offset = 0;
        if (textured) {
            bytes = readFile(path);
            offset = 20;
            if (bytes.size() < 20 || std::memcmp(bytes.data(), "RRTEX1\0\0", 8))
                throw std::runtime_error("Invalid RRTEX1 texture array");
            std::memcpy(&tw, bytes.data() + 8, 4);
            std::memcpy(&th, bytes.data() + 12, 4);
            std::memcpy(&layers, bytes.data() + 16, 4);
            if (!tw || !th || !layers || tw > 2048 || th > 2048 || layers > 16 ||
                20 + uint64_t(tw) * th * layers * 4 != bytes.size())
                throw std::runtime_error("Invalid texture array dimensions/length");
        }
        if (highestTextureLayer >= layers)
            throw std::runtime_error("Vertex texture layer outside texture array");
        SDL_GPUTextureCreateInfo info{};
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = tw;
        info.height = th;
        info.layer_count_or_depth = layers;
        info.num_levels = 1;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        surfaceTexture = SDL_CreateGPUTexture(device, &info);
        check(surfaceTexture, "Create scene texture array");
        SDL_GPUSamplerCreateInfo sampler{};
        sampler.min_filter = SDL_GPU_FILTER_LINEAR;
        sampler.mag_filter = SDL_GPU_FILTER_LINEAR;
        sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
        sampler.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        sampler.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
        sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        surfaceSampler = SDL_CreateGPUSampler(device, &sampler);
        check(surfaceSampler, "Create scene sampler");
        SDL_GPUTransferBufferCreateInfo tinfo{};
        tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tinfo.size = tw * th * layers * 4;
        auto transfer = SDL_CreateGPUTransferBuffer(device, &tinfo);
        check(transfer, "Create texture upload buffer");
        auto mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
        if (!mapped) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Map texture upload");
        }
        std::memcpy(mapped, bytes.data() + offset, tinfo.size);
        SDL_UnmapGPUTransferBuffer(device, transfer);
        auto command = SDL_AcquireGPUCommandBuffer(device);
        if (!command) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Acquire texture command");
        }
        auto copy = SDL_BeginGPUCopyPass(command);
        for (uint32_t layer = 0; layer < layers; ++layer) {
            SDL_GPUTextureTransferInfo source{};
            source.transfer_buffer = transfer;
            source.offset = layer * tw * th * 4;
            source.pixels_per_row = tw;
            source.rows_per_layer = th;
            SDL_GPUTextureRegion destination{};
            destination.texture = surfaceTexture;
            destination.layer = layer;
            destination.w = tw;
            destination.h = th;
            destination.d = 1;
            SDL_UploadToGPUTexture(copy, &source, &destination, false);
        }
        SDL_EndGPUCopyPass(copy);
        bool ok = SDL_SubmitGPUCommandBuffer(command);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        check(ok, "Submit texture upload");
    }
    void draw(SDL_GPUCommandBuffer *command, SDL_GPUTexture *target) {
        Vec eye = center + Vec{std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                               std::cos(yaw) * std::cos(pitch)} *
                               distance;
        Vec targetPoint = center, cameraUp{0, 1, 0};
        if (playing && follow) {
            Vec p{carModel[12], carModel[13], carModel[14]},
                f{carModel[8], carModel[9], carModel[10]};
            cameraUp = {carModel[4], carModel[5], carModel[6]};
            eye = p - f * 18 + cameraUp * 7;
            targetPoint = p + f * 8 + cameraUp;
        }
        Mat vp = cameraMatrix(eye, targetPoint, float(width) / height,
                              std::max(200.0f, distance + radius * 4), cameraUp);
        std::array<Mat, 2> uniforms{vp, identity()};
        SDL_PushGPUVertexUniformData(command, 0, uniforms.data(), sizeof(uniforms));
        float camera[4] = {eye.x, eye.y, eye.z, 1};
        SDL_PushGPUFragmentUniformData(command, 0, camera, sizeof(camera));
        SDL_GPUColorTargetInfo color{};
        color.texture = target;
        color.clear_color = {0.035f, 0.045f, 0.065f, 1};
        color.load_op = SDL_GPU_LOADOP_CLEAR;
        color.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPUDepthStencilTargetInfo z{};
        z.texture = depth;
        z.clear_depth = 1;
        z.load_op = SDL_GPU_LOADOP_CLEAR;
        z.store_op = SDL_GPU_STOREOP_DONT_CARE;
        z.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        z.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        z.cycle = true;
        auto pass = SDL_BeginGPURenderPass(command, &color, 1, &z);
        check(pass, "Begin render pass");
        SDL_BindGPUGraphicsPipeline(pass, pipeline);
        SDL_GPUBufferBinding vertex{vertexBuffer, 0}, index{indexBuffer, 0};
        SDL_BindGPUVertexBuffers(pass, 0, &vertex, 1);
        SDL_BindGPUIndexBuffer(pass, &index, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_GPUTextureSamplerBinding binding{surfaceTexture, surfaceSampler};
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
        SDL_DrawGPUIndexedPrimitives(pass, playing ? song.staticIndices : indexCount, 1, 0, 0, 0);
        if (playing) {
            uniforms[1] = carModel;
            SDL_PushGPUVertexUniformData(command, 0, uniforms.data(), sizeof(uniforms));
            SDL_DrawGPUIndexedPrimitives(pass, song.carIndices, 1, song.staticIndices, 0, 0);
        }
        SDL_EndGPURenderPass(pass);
    }

  public:
    ~Viewer() {
        clock.close();
        if (device) {
            SDL_WaitForGPUIdle(device);
            if (depth)
                SDL_ReleaseGPUTexture(device, depth);
            if (offscreen)
                SDL_ReleaseGPUTexture(device, offscreen);
            if (surfaceTexture)
                SDL_ReleaseGPUTexture(device, surfaceTexture);
            if (surfaceSampler)
                SDL_ReleaseGPUSampler(device, surfaceSampler);
            if (pipeline)
                SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
            if (vertexBuffer)
                SDL_ReleaseGPUBuffer(device, vertexBuffer);
            if (indexBuffer)
                SDL_ReleaseGPUBuffer(device, indexBuffer);
            if (window)
                SDL_ReleaseWindowFromGPUDevice(device, window);
            SDL_DestroyGPUDevice(device);
        }
        if (window)
            SDL_DestroyWindow(window);
        SDL_Quit();
    }
    void init(const fs::path &mesh, const fs::path &shaders, bool headless) {
        shaderDir = shaders;
        auto bytes = readFile(mesh);
        bool textured = bytes.size() >= 40 && !std::memcmp(bytes.data(), "RRMESH2\0", 8);
        if (bytes.size() < 40 || (!textured && std::memcmp(bytes.data(), "RRMESH1\0", 8)))
            throw std::runtime_error("Expected RRMESH1 or RRMESH2 mesh");
        uint32_t stride = textured ? 56 : 44;
        std::memcpy(&vertexCount, bytes.data() + 8, 4);
        std::memcpy(&indexCount, bytes.data() + 12, 4);
        uint64_t expected = 40 + uint64_t(vertexCount) * stride + uint64_t(indexCount) * 4;
        if (!vertexCount || !indexCount || indexCount % 3 || expected != bytes.size())
            throw std::runtime_error("Invalid runtime mesh length/counts");
        float bounds[6];
        std::memcpy(bounds, bytes.data() + 16, 24);
        for (float v : bounds)
            if (!std::isfinite(v))
                throw std::runtime_error("Invalid mesh bounds");
        Vec min{bounds[0], bounds[1], bounds[2]}, max{bounds[3], bounds[4], bounds[5]};
        if (min.x > max.x || min.y > max.y || min.z > max.z)
            throw std::runtime_error("Inverted mesh bounds");
        center = initialCenter = (min + max) * 0.5f;
        radius = std::max(0.1f, std::sqrt(dot(max - min, max - min)) * 0.5f);
        distance = radius * 3.0f;
        for (size_t p = 40; p < 40 + size_t(vertexCount) * stride; p += 4) {
            float f;
            std::memcpy(&f, bytes.data() + p, 4);
            if (!std::isfinite(f))
                throw std::runtime_error("Non-finite runtime vertex");
        }
        if (textured)
            for (size_t i = 0; i < vertexCount; ++i) {
                float layer;
                std::memcpy(&layer, bytes.data() + 40 + i * stride + 52, 4);
                if (layer < 0 || layer > 15 || layer != std::floor(layer))
                    throw std::runtime_error("Invalid vertex texture layer");
                highestTextureLayer = std::max(highestTextureLayer, static_cast<uint32_t>(layer));
            }
        for (size_t p = 40 + size_t(vertexCount) * stride; p < bytes.size(); p += 4) {
            uint32_t i;
            std::memcpy(&i, bytes.data() + p, 4);
            if (i >= vertexCount)
                throw std::runtime_error("Runtime index out of range");
        }
        if (!textured) {
            std::vector<uint8_t> converted(
                40 + size_t(vertexCount) * sizeof(Vertex) + size_t(indexCount) * 4, 0);
            std::memcpy(converted.data(), bytes.data(), 40);
            for (size_t i = 0; i < vertexCount; ++i)
                std::memcpy(converted.data() + 40 + i * sizeof(Vertex),
                            bytes.data() + 40 + i * stride, stride);
            std::memcpy(converted.data() + 40 + size_t(vertexCount) * sizeof(Vertex),
                        bytes.data() + 40 + size_t(vertexCount) * stride, size_t(indexCount) * 4);
            bytes = std::move(converted);
        }
        if (headless)
            SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "offscreen");
        check(SDL_Init(SDL_INIT_VIDEO), "Initialize SDL3");
        if (!headless) {
            window = SDL_CreateWindow("ReRiff | Drag: orbit | Scroll: zoom | Arrows/QE: camera | "
                                      "1-4: poses | F: focus | Home: overview | W: wireframe",
                                      width, height,
                                      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
            check(window, "Create window");
        }
        device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
        check(device, "Create Vulkan device");
        std::cout << "GPU backend: " << SDL_GetGPUDeviceDriver(device) << " | " << vertexCount
                  << " vertices | " << indexCount / 3 << " triangles\n";
        if (window) {
            check(SDL_ClaimWindowForGPUDevice(device, window), "Claim viewer window");
            check(SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                                SDL_GPU_PRESENTMODE_VSYNC),
                  "Set swapchain");
            colorFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
        }
        upload(bytes);
        uploadTextures(mesh.parent_path() / "scene.rtex", textured);
        resize(width, height);
        createPipeline();
        if (headless)
            offscreen = texture(colorFormat, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    }
    void playback(const fs::path &path, const fs::path &audio, double seconds, bool followCamera) {
        follow = followCamera;
        auto bytes = readFile(path);
        if (bytes.size() < 40 || std::memcmp(bytes.data(), "RRPLAY1\0", 8))
            throw std::runtime_error("Invalid playback file");
        uint32_t fields[5];
        float values[3];
        std::memcpy(fields, bytes.data() + 8, 20);
        std::memcpy(values, bytes.data() + 28, 12);
        uint32_t count = fields[0];
        song.start = fields[1];
        song.finish = fields[2];
        song.staticIndices = fields[3];
        song.carIndices = fields[4];
        song.beatSpan = values[0];
        song.paceOffset = values[1];
        song.lift = values[2];
        if (count < 4 || count > 100000 || bytes.size() != 40 + uint64_t(count) * 32 ||
            song.start >= song.finish || song.finish > count - 3 || !song.staticIndices ||
            !song.carIndices || song.staticIndices % 3 || song.carIndices % 3 ||
            uint64_t(song.staticIndices) + song.carIndices != indexCount ||
            !std::isfinite(song.beatSpan) || song.beatSpan <= 0 ||
            !std::isfinite(song.paceOffset) || !std::isfinite(song.lift))
            throw std::runtime_error("Invalid playback header");
        for (uint32_t i = 0; i < count; ++i) {
            float row[7];
            uint32_t visible;
            std::memcpy(row, bytes.data() + 40 + i * 32, 28);
            std::memcpy(&visible, bytes.data() + 68 + i * 32, 4);
            for (float v : row)
                if (!std::isfinite(v))
                    throw std::runtime_error("Invalid playback node");
            if ((i && row[6] <= song.distances.back()) || row[6] < 0 || visible > 1)
                throw std::runtime_error("Invalid playback distance/visibility");
            song.positions.push_back({row[0], row[1], row[2]});
            song.ups.push_back({row[3], row[4], row[5]});
            song.distances.push_back(row[6]);
            song.visible.push_back(visible);
        }
        if (!audio.empty())
            clock.open(audio);
        playing = true;
        clock.setPaused(true);
        clock.seek(seconds);
        clock.setPaused(false);
        updateSong(seconds);
        std::cout << "Playback: " << count << " nodes | pace offset " << song.paceOffset << "s | "
                  << (audio.empty() ? "silent clock" : "SDL audio clock") << '\n';
    }
    void focus(Vec point, float range) {
        focusPoints.push_back(point);
        focusDistance = range;
        hasFocus = true;
        if (focusPoints.size() == 1) {
            center = focusCenter = point;
            distance = range;
        }
    }

    void elevation(float degrees) {
        pitch = initialPitch = degrees * 0.017453292519943295f;
    }
    void screenshot(const fs::path &path) {
        SDL_GPUTransferBufferCreateInfo info{};
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        info.size = width * height * 4;
        auto transfer = SDL_CreateGPUTransferBuffer(device, &info);
        check(transfer, "Create readback buffer");
        auto command = SDL_AcquireGPUCommandBuffer(device);
        if (!command) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Acquire capture command");
        }
        if (playing)
            std::cout << "Capture playback node: " << songParameter << '\n';
        draw(command, offscreen);
        auto copy = SDL_BeginGPUCopyPass(command);
        SDL_GPUTextureRegion source{};
        source.texture = offscreen;
        source.w = width;
        source.h = height;
        source.d = 1;
        SDL_GPUTextureTransferInfo destination{};
        destination.transfer_buffer = transfer;
        destination.pixels_per_row = width;
        destination.rows_per_layer = height;
        SDL_DownloadFromGPUTexture(copy, &source, &destination);
        SDL_EndGPUCopyPass(copy);
        auto fence = SDL_SubmitGPUCommandBufferAndAcquireFence(command);
        if (!fence) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Submit capture");
        }
        bool waited = SDL_WaitForGPUFences(device, true, &fence, 1);
        SDL_ReleaseGPUFence(device, fence);
        if (!waited) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Wait for capture");
        }
        auto pixels = static_cast<uint8_t *>(SDL_MapGPUTransferBuffer(device, transfer, false));
        if (!pixels) {
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            check(false, "Map capture");
        }
        std::ofstream output(path, std::ios::binary);
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (size_t i = 0; i < size_t(width) * height; ++i)
            output.write(reinterpret_cast<char *>(pixels + i * 4), 3);
        bool ok = bool(output);
        SDL_UnmapGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        if (!ok)
            throw std::runtime_error("Could not write screenshot " + path.string());
        std::cout << "Saved Vulkan render: " << path << '\n';
    }
    void run(int limit) {
        bool running = true;
        int frames = 0;
        uint64_t last = SDL_GetTicksNS();
        while (running && (!limit || frames < limit)) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT)
                    running = false;
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                    if (event.key.key == SDLK_ESCAPE)
                        running = false;
                    if (playing) {
                        if (event.key.key == SDLK_SPACE)
                            clock.toggle();
                        if (event.key.key == SDLK_J)
                            clock.seek(std::max(0., clock.time() - 5));
                        if (event.key.key == SDLK_L)
                            clock.seek(clock.time() + 5);
                        if (event.key.key == SDLK_BACKSPACE)
                            clock.seek(0);
                        if (event.key.key == SDLK_C)
                            follow = !follow;
                        if (event.key.key == SDLK_HOME)
                            follow = false;
                    }
                    if (event.key.key == SDLK_R || event.key.key == SDLK_F) {
                        center = hasFocus ? focusCenter : initialCenter;
                        distance = hasFocus ? focusDistance : radius * 3.0f;
                        yaw = 0.75f;
                        pitch = initialPitch;
                    }
                    if (event.key.key == SDLK_HOME) {
                        center = initialCenter;
                        distance = radius * 3.0f;
                    }
                    if ((event.key.key == SDLK_W || event.key.key == SDLK_TAB) ||
                        event.key.key == SDLK_F2) {
                        wireframe = !wireframe;
                        createPipeline();
                    }
                    if (event.key.key >= SDLK_1 && event.key.key <= SDLK_9) {
                        size_t index = static_cast<size_t>(event.key.key - SDLK_1);
                        if (index < focusPoints.size()) {
                            center = focusCenter = focusPoints[index];
                            distance = focusDistance;
                        }
                    }
                }
                if (event.type == SDL_EVENT_MOUSE_WHEEL)
                    distance =
                        std::clamp(distance * std::exp(-event.wheel.y * 0.12f), 0.5f, radius * 20);
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    if (event.motion.state & SDL_BUTTON_LMASK) {
                        yaw -= event.motion.xrel * 0.008f;
                        pitch = std::clamp(pitch + event.motion.yrel * 0.008f, -1.4f, 1.4f);
                    }
                    if (event.motion.state & (SDL_BUTTON_RMASK | SDL_BUTTON_MMASK)) {
                        Vec right{std::cos(yaw), 0, -std::sin(yaw)};
                        center = center + right * (-event.motion.xrel * distance / height) +
                                 Vec{0, 1, 0} * (event.motion.yrel * distance / height);
                    }
                }
            }
            if (!running)
                break;
            uint64_t now = SDL_GetTicksNS();
            float dt = std::min(0.05f, float(now - last) / 1e9f);
            last = now;
            const bool *keys = SDL_GetKeyboardState(nullptr);
            float speed = std::max(2.0f, distance * 0.6f) * dt;
            Vec right{std::cos(yaw), 0, -std::sin(yaw)}, forward{-std::sin(yaw), 0, -std::cos(yaw)};
            center = center +
                     right * (float(keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT]) * speed) +
                     forward * (float(keys[SDL_SCANCODE_UP] - keys[SDL_SCANCODE_DOWN]) * speed) +
                     Vec{0, 1, 0} * (float(keys[SDL_SCANCODE_E] - keys[SDL_SCANCODE_Q]) * speed);

            if (playing)
                updateSong(clock.time());
            auto command = SDL_AcquireGPUCommandBuffer(device);
            check(command, "Acquire frame command");
            SDL_GPUTexture *target = nullptr;
            uint32_t w = 0, h = 0;
            if (!SDL_WaitAndAcquireGPUSwapchainTexture(command, window, &target, &w, &h)) {
                SDL_CancelGPUCommandBuffer(command);
                check(false, "Acquire window image");
            }
            if (target) {
                resize(w, h);
                draw(command, target);
                ++frames;
            }
            check(SDL_SubmitGPUCommandBuffer(command), "Submit frame");
            if (!target)
                SDL_Delay(25);
        }
        if (playing)
            std::cout << "Playback ended at " << clock.time() << "s | node " << songParameter
                      << '\n';
    }
};

int main(int argc, char **argv) {
    try {
        fs::path mesh, shaders = fs::canonical("/proc/self/exe").parent_path() / "shaders", capture,
                       playback, audio;
        double playbackTime = 0;
        int frames = 0;
        std::vector<Vec> focuses;
        float focusDistance = 20;
        float elevation = 0;
        bool setElevation = false, followCamera = true;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--no-follow") {
                followCamera = false;
                continue;
            }
            if (arg == "--help") {
                std::cout << "Usage: reriff-viewer MESH.rmesh [--shaders DIR] [--screenshot "
                             "IMAGE.ppm] [--frames N] [--focus X,Y,Z (repeatable)] [--distance N] "
                             "[--elevation DEGREES] [--playback SONG.rplay] [--audio SONG.wav] "
                             "[--playback-time SECONDS] [--no-follow]\n";
                return 0;
            }
            if (arg == "--drive" || arg == "--drive-demo")
                throw std::runtime_error(
                    "Prototype physics removed; original rigid-body integration is not ported yet. "
                    "Use --playback for inspection.");
            if (arg == "--playback" || arg == "--audio" || arg == "--playback-time" ||
                arg == "--shaders" || arg == "--screenshot" || arg == "--frames" ||
                arg == "--focus" || arg == "--distance" || arg == "--elevation") {
                if (++i == argc)
                    throw std::runtime_error("Missing value for " + arg);
                if (arg == "--playback")
                    playback = argv[i];
                else if (arg == "--audio")
                    audio = argv[i];
                else if (arg == "--playback-time") {
                    playbackTime = std::stod(argv[i]);
                    if (!std::isfinite(playbackTime) || playbackTime < 0 || playbackTime > 86400)
                        throw std::runtime_error("Playback time must be between 0 and 86400");
                } else if (arg == "--shaders")
                    shaders = argv[i];
                else if (arg == "--screenshot")
                    capture = argv[i];
                else if (arg == "--focus") {
                    std::string point = argv[i];
                    size_t end = 0, start = 0;
                    float coordinates[3];
                    for (int k = 0; k < 3; ++k) {
                        size_t comma = point.find(',', start);
                        std::string part = point.substr(start, comma - start);
                        coordinates[k] = std::stof(part, &end);
                        if (end != part.size() || !std::isfinite(coordinates[k]) ||
                            (k < 2 && comma == std::string::npos) ||
                            (k == 2 && comma != std::string::npos))
                            throw std::runtime_error("Expected --focus X,Y,Z");
                        start = comma + 1;
                    }
                    focuses.push_back({coordinates[0], coordinates[1], coordinates[2]});
                } else if (arg == "--distance") {
                    focusDistance = std::stof(argv[i]);
                    if (!std::isfinite(focusDistance) || focusDistance < 0.5f ||
                        focusDistance > 150)
                        throw std::runtime_error("Distance must be between 0.5 and 150");
                } else if (arg == "--elevation") {
                    elevation = std::stof(argv[i]);
                    if (!std::isfinite(elevation) || std::fabs(elevation) > 80)
                        throw std::runtime_error("Elevation must be between -80 and 80 degrees");
                    setElevation = true;
                } else {
                    frames = std::stoi(argv[i]);
                    if (frames < 1)
                        throw std::runtime_error("Frame limit must be positive");
                }
            } else if (arg.size() && arg[0] == '-')
                throw std::runtime_error("Unknown option " + arg);
            else if (mesh.empty())
                mesh = arg;
            else
                throw std::runtime_error("Only one mesh can be opened");
        }
        if (mesh.empty())
            throw std::runtime_error("Supply a vehicle.rmesh file; use --help for usage");
        if (playback.empty() && (!audio.empty() || playbackTime))
            throw std::runtime_error("Audio/time requires --playback");
        Viewer viewer;
        viewer.init(mesh, shaders, !capture.empty());
        for (Vec point : focuses)
            viewer.focus(point, focusDistance);
        if (setElevation)
            viewer.elevation(elevation);
        if (!playback.empty())
            viewer.playback(playback, audio, playbackTime, followCamera);
        if (!capture.empty())
            viewer.screenshot(capture);
        else
            viewer.run(frames);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "ReRiff viewer: " << error.what() << '\n';
        return 1;
    }
}
