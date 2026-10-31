#include "song_audio.hpp"
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    // Generated silence verifies timing/control without bundled assets or a
    // sound device. The dummy driver still consumes the real SDL stream.
    const auto path = std::filesystem::temp_directory_path() /
                      ("reriff-audio-test-" + std::to_string(SDL_GetPerformanceCounter()) + ".wav");
    try {
        const uint32_t size = 48000 * 4 * 2;
        std::ofstream wave(path, std::ios::binary);
        auto u32 = [&](uint32_t v) {
            char b[4] = {char(v), char(v >> 8), char(v >> 16), char(v >> 24)};
            wave.write(b, 4);
        };
        auto u16 = [&](uint16_t v) {
            char b[2] = {char(v), char(v >> 8)};
            wave.write(b, 2);
        };
        wave.write("RIFF", 4);
        u32(size + 36);
        wave.write("WAVEfmt ", 8);
        u32(16);
        u16(1);
        u16(2);
        u32(48000);
        u32(48000 * 4);
        u16(4);
        u16(16);
        wave.write("data", 4);
        u32(size);
        std::vector<char> silence(size, 0);
        wave.write(silence.data(), silence.size());
        wave.close();
        SongClock audio;
        audio.open(path);
        audio.seek(.25);
        SDL_Delay(140);
        double advanced = audio.time();
        assert(advanced > .30 && advanced < .8);
        audio.toggle();
        double paused = audio.time();
        SDL_Delay(100);
        assert(std::fabs(audio.time() - paused) < .025);
        audio.seek(1.25);
        assert(std::fabs(audio.time() - 1.25) < .001);
        SDL_Delay(80);
        assert(std::fabs(audio.time() - 1.25) < .025);
        audio.toggle();
        SDL_Delay(140);
        assert(audio.time() > 1.30);
        audio.seek(100);
        SDL_Delay(40);
        assert(std::fabs(audio.time() - 2) < .001);
        audio.seek(0);
        SDL_Delay(80);
        assert(audio.time() < .3);
        audio.close();
        SongClock silent;
        silent.seek(5);
        SDL_Delay(50);
        assert(silent.time() > 5.04);
        silent.toggle();
        paused = silent.time();
        SDL_Delay(50);
        assert(silent.time() == paused);
        silent.seek(10);
        assert(silent.time() == 10);
        silent.toggle();
        SDL_Delay(50);
        assert(silent.time() > 10.04);
        SDL_Quit();
        std::filesystem::remove(path);
        std::cout << "Audio and silent clocks: advance, pause, paused seek, resume, EOF and "
                     "restart passed\n";
        return 0;
    } catch (const std::exception &error) {
        SDL_Quit();
        std::filesystem::remove(path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
