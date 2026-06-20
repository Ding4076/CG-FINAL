// miniaudio implementation must be defined in exactly one TU. Keep all miniaudio
// usage inside this file.
#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"

#include "miniaudio.h"

#include <iostream>

struct Audio::Impl {
    ma_engine engine;
    std::map<std::string, std::string> paths;  // name -> filepath
    bool engineOk = false;
};

Audio::Audio() : _impl(new Impl()) {}

Audio::~Audio() {
    if (_impl) {
        if (_impl->engineOk) {
            ma_engine_uninit(&_impl->engine);
        }
        delete _impl;
    }
}

bool Audio::init() {
    if (ma_engine_init(nullptr, &_impl->engine) != MA_SUCCESS) {
        std::cerr << "Audio: ma_engine_init failed; audio disabled." << std::endl;
        _impl->engineOk = false;
        _ok = false;
        return false;
    }
    _impl->engineOk = true;
    _ok = true;
    return true;
}

bool Audio::load(const std::string& name, const std::string& filepath) {
    if (!_ok) return false;
    _impl->paths[name] = filepath;
    return true;
}

void Audio::play(const std::string& name) {
    if (!_ok) return;
    auto it = _impl->paths.find(name);
    if (it == _impl->paths.end()) return;
    // ma_engine_play_sound spawns a fire-and-forget sound each call,
    // which works correctly for both WAV and MP3 without seek issues.
    ma_engine_play_sound(&_impl->engine, it->second.c_str(), nullptr);
}
