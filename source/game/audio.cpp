// miniaudio implementation must be defined in exactly one TU. Keep all miniaudio
// usage inside this file.
#define MINIAUDIO_IMPLEMENTATION
#include "audio.h"

#include "miniaudio.h"

#include <iostream>

struct Audio::Impl {
    ma_engine engine;
    std::map<std::string, ma_sound*> sounds;
    bool engineOk = false;
};

Audio::Audio() : _impl(new Impl()) {}

Audio::~Audio() {
    if (_impl) {
        for (auto& kv : _impl->sounds) {
            if (kv.second) {
                ma_sound_uninit(kv.second);
                delete kv.second;
            }
        }
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
    if (!_ok) {
        return false;
    }
    ma_sound* s = new ma_sound();
    if (ma_sound_init_from_file(&_impl->engine, filepath.c_str(), 0, nullptr, nullptr, s) !=
        MA_SUCCESS) {
        delete s;
        return false;
    }
    _impl->sounds[name] = s;
    return true;
}

void Audio::play(const std::string& name) {
    if (!_ok) {
        return;
    }
    auto it = _impl->sounds.find(name);
    if (it == _impl->sounds.end() || !it->second) {
        return;
    }
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
}
