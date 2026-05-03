#include "AudioManager.hpp"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

AudioManager::AudioManager() {
    dummyBuffer_ = std::make_unique<sf::SoundBuffer>();
    std::vector<int16_t> samples(1, 0);
    (void)dummyBuffer_->loadFromSamples(samples.data(), 1, 1, 44100, {sf::SoundChannel::Mono});
    for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
        sfxChannels_[i] = std::make_unique<sf::Sound>(*dummyBuffer_);
    }
}

AudioManager& AudioManager::instance() {
    static AudioManager inst;
    return inst;
}

std::string AudioManager::resolvePath(const std::string& filename) {
    std::vector<std::string> candidates = {
        "assets/sounds/" + filename,
        "../assets/sounds/" + filename,
        "../../assets/sounds/" + filename,
        "sounds/" + filename
    };
    for (const auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return candidates[0];
}

void AudioManager::loadAll() {
    std::cout << "[AudioManager] Loading audio assets..." << std::endl;

    auto loadBGM = [&](const std::string& name, const std::string& file) {
        auto music = std::make_unique<sf::Music>();
        std::string path = resolvePath(file);
        if (music->openFromFile(path)) {
            bgm_[name] = std::move(music);
            std::cout << "  BGM loaded: " << name << " (" << path << ")" << std::endl;
        } else {
            std::cerr << "  BGM FAILED: " << name << " (" << path << ")" << std::endl;
        }
    };

    auto loadSFX = [&](const std::string& name, const std::string& file) {
        auto buf = std::make_unique<sf::SoundBuffer>();
        std::string path = resolvePath(file);
        if (buf->loadFromFile(path)) {
            sfxBuffers_[name] = std::move(buf);
            std::cout << "  SFX loaded: " << name << " (" << path << ")" << std::endl;
        } else {
            std::cerr << "  SFX FAILED: " << name << " (" << path << ")" << std::endl;
        }
    };

    loadBGM("main_menu", "main_menu_bgm.wav");
    loadBGM("game",      "game_bgm.wav");

    loadSFX("player_attack",  "player_attack.wav");
    loadSFX("player_crit",    "player_crit.wav");
    loadSFX("player_hurt",    "player_hurt.wav");
    loadSFX("player_roll",    "player_roll.wav");
    loadSFX("player_death",   "player_death.wav");
    loadSFX("enemy_hurt",     "enemy_hurt.wav");
    loadSFX("enemy_death",    "enemy_death.wav");
    loadSFX("enemy_shoot",    "enemy_shoot.wav");
    loadSFX("explosion",      "explosion.wav");
    loadSFX("pickup",         "pickup.wav");
    loadSFX("level_up",       "level_up.wav");
    loadSFX("button_click",   "button_click.wav");
    loadSFX("connect",        "connect.wav");
    loadSFX("disconnect",     "disconnect.wav");
    loadSFX("victory",        "victory.wav");

    for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
        sfxChannels_[i]->setVolume(sfxVolume_ * masterVolume_ / 100.f);
    }

    std::cout << "[AudioManager] Loading complete." << std::endl;
}

void AudioManager::playBGM(const std::string& name, float volume, bool loop) {
    if (currentBGM_ == name && bgm_.count(name) && bgm_[name]->getStatus() == sf::SoundSource::Status::Playing) {
        return;
    }
    stopBGM();
    auto it = bgm_.find(name);
    if (it != bgm_.end()) {
        it->second->setLooping(loop);
        bgmVolume_ = volume;
        it->second->setVolume(volume * masterVolume_ / 100.f);
        it->second->play();
        currentBGM_ = name;
    }
}

void AudioManager::stopBGM() {
    if (!currentBGM_.empty()) {
        auto it = bgm_.find(currentBGM_);
        if (it != bgm_.end()) {
            it->second->stop();
        }
        currentBGM_.clear();
    }
}

void AudioManager::setBGMVolume(float volume) {
    bgmVolume_ = volume;
    if (!currentBGM_.empty()) {
        auto it = bgm_.find(currentBGM_);
        if (it != bgm_.end()) {
            it->second->setVolume(volume * masterVolume_ / 100.f);
        }
    }
}

void AudioManager::fadeBGM(float targetVolume, float duration) {
    fadeTarget_ = targetVolume;
    fadeSpeed_ = (targetVolume - bgmVolume_) / (duration * 60.f);
    bgmFading_ = true;
}

void AudioManager::playSFX(const std::string& name, float volume) {
    auto it = sfxBuffers_.find(name);
    if (it == sfxBuffers_.end()) return;

    sf::Sound& ch = *sfxChannels_[sfxChannelIndex_];
    ch.setBuffer(*it->second);
    ch.setVolume(volume * sfxVolume_ * masterVolume_ / (100.f * 100.f));
    ch.play();
    sfxChannelIndex_ = (sfxChannelIndex_ + 1) % MAX_SFX_CHANNELS;
}

void AudioManager::setSFXVolume(float volume) {
    sfxVolume_ = volume;
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume_ = volume;
    setBGMVolume(bgmVolume_);
    for (int i = 0; i < MAX_SFX_CHANNELS; ++i) {
        sfxChannels_[i]->setVolume(sfxVolume_ * masterVolume_ / 100.f);
    }
}

bool AudioManager::isBGMLoading() const {
    return false;
}
