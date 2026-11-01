#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

inline void audioCheck(bool ok, const std::string &what) {
    if (!ok)
        throw std::runtime_error(what + ": " + SDL_GetError());
}

// Use consumed audio input frames as the presentation clock. This avoids
// accumulating render-frame deltas; actual hardware latency is not calibrated.
class SongClock {
    SDL_AudioStream *stream = nullptr;
    Uint8 *pcm = nullptr;
    Uint32 bytes = 0;
    int bytesPerFrame = 0, rate = 0;
    double base = 0, duration = 0;
    Uint32 submitted = 0;
    Uint64 started = 0;
    bool paused = false;

  public:
    ~SongClock() {
        close();
    }
    void close() {
        if (stream)
            SDL_DestroyAudioStream(stream);
        stream = nullptr;
        if (pcm)
            SDL_free(pcm);
        pcm = nullptr;
    }
    void open(const std::filesystem::path &path) {
        audioCheck(SDL_InitSubSystem(SDL_INIT_AUDIO), "Initialize song audio");
        SDL_AudioSpec spec{};
        audioCheck(SDL_LoadWAV(path.c_str(), &spec, &pcm, &bytes), "Load decoded song WAV");
        if (spec.freq != 48000 || spec.channels != 2 || spec.format != SDL_AUDIO_S16LE || !bytes ||
            bytes > 256 * 1024 * 1024)
            throw std::runtime_error("Playback audio must be bounded 48 kHz stereo PCM16");
        bytesPerFrame = 4;
        rate = spec.freq;
        duration = double(bytes) / bytesPerFrame / rate;
        stream =
            SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
        audioCheck(stream, "Open song audio device");
        std::cout << "Audio: " << SDL_GetCurrentAudioDriver() << " | 48000 Hz stereo | " << duration
                  << " seconds\n";
    }
    double time() const {
        if (stream) {
            int queued = SDL_GetAudioStreamQueued(stream);
            audioCheck(queued >= 0, "Read song audio clock");
            return base +
                   double(submitted - std::min(submitted, Uint32(queued))) / bytesPerFrame / rate;
        }
        return base + (paused ? 0 : double(SDL_GetTicksNS() - started) / 1e9);
    }
    void seek(double seconds) {
        if (stream) {
            audioCheck(SDL_PauseAudioStreamDevice(stream), "Pause song for seek");
            audioCheck(SDL_ClearAudioStream(stream), "Clear song audio queue");
            Uint32 offset =
                static_cast<Uint32>(std::clamp(seconds, 0., duration) * rate) * bytesPerFrame;
            offset = std::min(offset, bytes);
            base = double(offset) / bytesPerFrame / rate;
            submitted = bytes - offset;
            if (submitted)
                audioCheck(SDL_PutAudioStreamData(stream, pcm + offset, int(submitted)),
                           "Queue song audio");
            audioCheck(SDL_FlushAudioStream(stream), "Flush song audio");
            if (!paused)
                audioCheck(SDL_ResumeAudioStreamDevice(stream), "Resume song after seek");
        } else
            base = std::max(0., seconds);
        started = SDL_GetTicksNS();
    }
    void setPaused(bool value) {
        if (paused == value)
            return;
        if (stream) {
            if (value)
                audioCheck(SDL_PauseAudioStreamDevice(stream), "Pause song");
            else
                audioCheck(SDL_ResumeAudioStreamDevice(stream), "Resume song");
        } else {
            double current = time();
            base = current;
            started = SDL_GetTicksNS();
        }
        paused = value;
    }
    void toggle() {
        setPaused(!paused);
    }
    bool isPaused() const {
        return paused;
    }
};
