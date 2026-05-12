/**
 * @file AudioManager.hpp
 * @brief 音频管理器 - 背景音乐(BGM)和音效(SFX)统一管理
 *
 * 单例模式，管理背景音乐和音效的加载、播放、音量控制。
 * BGM使用sf::Music流式播放，SFX使用sf::SoundBuffer+sf::Sound，
 * 支持最多32个SFX通道轮换播放，避免音效被截断。
 * 支持BGM淡入淡出效果。
 */

#pragma once
#include <SFML/Audio.hpp>
#include <map>
#include <string>
#include <memory>

/**
 * @class AudioManager
 * @brief 音频管理器单例
 *
 * 统一管理BGM和SFX，支持音量控制、淡入淡出、多通道SFX。
 * 使用方式：AudioManager::instance().playBGM("main_menu");
 */
class AudioManager {
public:
    /// 获取单例引用
    static AudioManager& instance();

    /// 加载所有音频资源（BGM和SFX）
    void loadAll();

    /**
     * @brief 播放背景音乐
     * @param name    BGM名称（如"main_menu"、"game"）
     * @param volume  音量 (0-100)，默认50
     * @param loop    是否循环播放，默认true
     *
     * 若同名BGM已在播放则跳过，否则先停止当前BGM再播放新的。
     */
    void playBGM(const std::string& name, float volume = 50.f, bool loop = true);

    /// 停止当前背景音乐
    void stopBGM();

    /**
     * @brief 设置BGM音量
     * @param volume 音量 (0-100)
     *
     * 实际音量 = volume * masterVolume / 100
     */
    void setBGMVolume(float volume);

    /**
     * @brief BGM淡入淡出
     * @param targetVolume 目标音量
     * @param duration     淡化持续时间（秒），默认1秒
     */
    void fadeBGM(float targetVolume, float duration = 1.f);

    /**
     * @brief 播放音效
     * @param name    SFX名称（如"player_attack"、"pickup"）
     * @param volume  音量 (0-100)，默认80
     *
     * 轮换使用32个SFX通道，避免音效被截断。
     */
    void playSFX(const std::string& name, float volume = 80.f);

    /**
     * @brief 设置SFX音量
     * @param volume 音量 (0-100)
     */
    void setSFXVolume(float volume);

    /**
     * @brief 设置主音量（影响BGM和SFX）
     * @param volume 音量 (0-100)
     */
    void setMasterVolume(float volume);

    /// BGM是否正在加载中（当前始终返回false）
    bool isBGMLoading() const;

private:
    AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    std::map<std::string, std::unique_ptr<sf::Music>> bgm_;          ///< BGM资源表（名称→Music）
    std::map<std::string, std::unique_ptr<sf::SoundBuffer>> sfxBuffers_; ///< SFX音效缓冲表（名称→Buffer）

    std::string currentBGM_;          ///< 当前播放的BGM名称
    float bgmVolume_ = 50.f;          ///< BGM音量 (0-100)
    float sfxVolume_ = 80.f;          ///< SFX音量 (0-100)
    float masterVolume_ = 100.f;      ///< 主音量 (0-100)

    bool bgmFading_ = false;          ///< BGM是否正在淡化
    float fadeTarget_ = 0.f;          ///< 淡化目标音量
    float fadeSpeed_ = 0.f;           ///< 淡化速度（每帧音量变化量）

    static constexpr int MAX_SFX_CHANNELS = 32; ///< SFX最大通道数
    int sfxChannelIndex_ = 0;                     ///< 当前SFX通道索引（轮换）
    std::unique_ptr<sf::Sound> sfxChannels_[MAX_SFX_CHANNELS]; ///< SFX通道数组
    std::unique_ptr<sf::SoundBuffer> dummyBuffer_; ///< 占位缓冲（初始化SFX通道用）

    /**
     * @brief 解析音频文件路径
     * @param filename 文件名
     * @return 完整路径（依次尝试assets/sounds/、../assets/sounds/等目录）
     */
    std::string resolvePath(const std::string& filename);
};