#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <memory>

class AudioManager {
public:
    static AudioManager& instance();

    void loadAll();
    void playBGM(const std::string& name, float volume = 50.f, bool loop = true);
    void stopBGM();
    void setBGMVolume(float volume);
    void fadeBGM(float targetVolume, float duration = 1.f);

    void playSFX(const std::string& name, float volume = 80.f);
    void setSFXVolume(float volume);
    void setMasterVolume(float volume);

    bool isBGMLoading() const;

private:
    AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::map<std::string, std::unique_ptr<sf::Music>> bgm_;
    std::map<std::string, std::unique_ptr<sf::SoundBuffer>> sfxBuffers_;

    std::string currentBGM_;
    float bgmVolume_ = 50.f;
    float sfxVolume_ = 80.f;
    float masterVolume_ = 100.f;

    bool bgmFading_ = false;
    float fadeTarget_ = 0.f;
    float fadeSpeed_ = 0.f;

    static constexpr int MAX_SFX_CHANNELS = 32;
    int sfxChannelIndex_ = 0;
    std::unique_ptr<sf::Sound> sfxChannels_[MAX_SFX_CHANNELS];
    std::unique_ptr<sf::SoundBuffer> dummyBuffer_;

    std::string resolvePath(const std::string& filename);
};