#pragma once

#include <map>
#include <string>

// Thin wrapper over miniaudio for one-shot sound effects. init() must be called
// once after GLFW (it needs a device); load() registers a wav; play() plays it.
class Audio {
public:
    Audio();
    ~Audio();

    // Initialize the engine. Returns false on failure (audio is then a no-op).
    bool init();

    // Load a .wav/.ogg/etc file under the given name (loaded lazily into memory).
    bool load(const std::string& name, const std::string& filepath);

    // Play a previously-loaded sound from the start. No-op if init failed.
    void play(const std::string& name);

private:
    struct Impl;
    Impl* _impl = nullptr;
    bool _ok = false;
};
