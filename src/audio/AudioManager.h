#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

// miniaudio single-header — define implementation in AudioManager.cpp
#define MA_NO_ENCODING
#include "../../external/miniaudio/miniaudio.h"

struct SoundEntry {
    ma_sound sound;
    bool     loaded = false;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool initialize();
    void shutdown();

    // Load a sound from disk or game.like archive
    bool loadSound(const std::string& key, const std::string& filename);
    bool loadSoundFromGameLike(const std::string& key, const std::string& filename);

    // Playback
    bool playSound(const std::string& key, bool loop = false);
    bool stopSound(const std::string& key);
    bool pauseSound(const std::string& key);
    bool resumeSound(const std::string& key);
    bool isSoundPlaying(const std::string& key);

    // Volume  0.0 – 1.0
    void setSoundVolume(const std::string& key, float volume);
    void setMasterVolume(float volume);

    // Pitch multiplier (1.0 = normal)
    void setSoundPitch(const std::string& key, float pitch);

    void unloadSound(const std::string& key);
    void unloadAll();

    // Temp-file cleanup (used for game.like extraction)
    void cleanupTempFiles();

private:
    ma_engine m_engine;
    bool      m_initialized = false;

    std::unordered_map<std::string, SoundEntry*> m_sounds;
    std::vector<std::string>                      m_tempFiles; // paths to delete on shutdown
};
