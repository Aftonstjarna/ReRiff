// Small build tool using shaderc's stable C ABI; no scripting runtime required.
#include "archive.hpp"
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
int main(int argc, char **argv) {
    void *lib = nullptr;
    try {
        require(argc == 3, "usage: compile-shaders SOURCE_DIR OUTPUT_DIR");
        for (const char *name : {"libshaderc_shared.so.1", "libshaderc_shared.so"}) {
            lib = dlopen(name, RTLD_NOW | RTLD_LOCAL);
            if (lib)
                break;
        }
        require(lib != nullptr, "Install libshaderc_shared or glslc to compile shaders");
        auto symbol = [&](const char *name) {
            auto p = dlsym(lib, name);
            require(p != nullptr, std::string("missing shaderc symbol: ") + name);
            return p;
        };
        auto init = reinterpret_cast<void *(*)()>(symbol("shaderc_compiler_initialize"));
        auto compile = reinterpret_cast<void *(*)(void *, const char *, size_t, int, const char *,
                                                  const char *, const void *)>(
            symbol("shaderc_compile_into_spv"));
        auto status = reinterpret_cast<int (*)(const void *)>(
            symbol("shaderc_result_get_compilation_status"));
        auto message = reinterpret_cast<const char *(*)(const void *)>(
            symbol("shaderc_result_get_error_message"));
        auto length =
            reinterpret_cast<size_t (*)(const void *)>(symbol("shaderc_result_get_length"));
        auto data =
            reinterpret_cast<const char *(*)(const void *)>(symbol("shaderc_result_get_bytes"));
        auto release = reinterpret_cast<void (*)(void *)>(symbol("shaderc_result_release"));
        auto cleanup = reinterpret_cast<void (*)(void *)>(symbol("shaderc_compiler_release"));
        std::filesystem::create_directories(argv[2]);
        void *compiler = init();
        require(compiler != nullptr, "shaderc initialization failed");
        for (const char *base : {"vehicle"})
            for (const char *suffix : {"vert", "frag"}) {
                int kind = std::string(suffix) == "vert" ? 0 : 1;
                auto source = std::filesystem::path(argv[1]) / (std::string(base) + "." + suffix);
                auto input = read(source.string());
                auto result = compile(compiler, reinterpret_cast<const char *>(input.data()),
                                      input.size(), kind, source.c_str(), "main", nullptr);
                require(result != nullptr, "shader compilation returned null");
                if (status(result)) {
                    std::string error = message(result);
                    release(result);
                    cleanup(compiler);
                    throw std::runtime_error(error);
                }
                auto output =
                    std::filesystem::path(argv[2]) / (std::string(base) + "." + suffix + ".spv");
                std::ofstream f(output, std::ios::binary);
                f.write(data(result), length(result));
                bool ok = bool(f);
                release(result);
                require(ok, "cannot write shader");
            }
        cleanup(compiler);
        dlclose(lib);
        return 0;
    } catch (const std::exception &e) {
        if (lib)
            dlclose(lib);
        std::cerr << e.what() << '\n';
        return 1;
    }
}
