#define MINIAUDIO_IMPLEMENTATION
#include "AudioManager.h"
#include "../external/miniz/miniz.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

AudioManager::AudioManager() {}

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize() {
    ma_engine_config cfg = ma_engine_config_init();
    if (ma_engine_init(&cfg, &m_engine) != MA_SUCCESS) {
        std::cerr << "AudioManager: Failed to initialize miniaudio engine." << std::endl;
        return false;
    }
    m_initialized = true;
    std::cout << "AudioManager: Initialized." << std::endl;
    return true;
}

void AudioManager::shutdown() {
    if (!m_initialized) return;
    unloadAll();
    ma_engine_uninit(&m_engine);
    m_initialized = false;
    cleanupTempFiles();
}

// ---- Loading ----

bool AudioManager::loadSound(const std::string& key, const std::string& filename) {
    if (!m_initialized) return false;
    if (m_sounds.count(key)) return true; // already loaded

    // Production mode: check for game.like next to exe
#ifdef _WIN32
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
#else
    char exePathBuf[4096] = {};
    readlink("/proc/self/exe", exePathBuf, sizeof(exePathBuf) - 1);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
#endif
    std::filesystem::path gameLikePath = exePath / "game.like";
    if (std::filesystem::exists(gameLikePath)) {
        return loadSoundFromGameLike(key, filename);
    }

    SoundEntry* entry = new SoundEntry();
    if (ma_sound_init_from_file(&m_engine, filename.c_str(), 0, NULL, NULL, &entry->sound) != MA_SUCCESS) {
        std::cerr << "AudioManager: Failed to load sound: " << filename << std::endl;
        delete entry;
        return false;
    }
    entry->loaded = true;
    m_sounds[key] = entry;
    return true;
}

bool AudioManager::loadSoundFromGameLike(const std::string& key, const std::string& filename) {
    if (!m_initialized) return false;

#ifdef _WIN32
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
#else
    char exePathBuf[4096] = {};
    readlink("/proc/self/exe", exePathBuf, sizeof(exePathBuf) - 1);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
#endif
    std::filesystem::path gameLikePath = exePath / "game.like";

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_file(&zip, gameLikePath.string().c_str(), 0)) {
        std::cerr << "AudioManager: Failed to open game.like" << std::endl;
        return false;
    }

    // Normalise and build search paths (exact first, then common subdirs)
    std::string norm = filename;
    for (char& c : norm) if (c == '\\') c = '/';

    std::vector<std::string> searchPaths = { norm };
    if (norm.find('/') == std::string::npos) {
        searchPaths.push_back("audio/"  + norm);
        searchPaths.push_back("sounds/" + norm);
        searchPaths.push_back("music/"  + norm);
        searchPaths.push_back("assets/" + norm);
    } else {
        auto slash = norm.rfind('/');
        searchPaths.push_back(norm.substr(slash + 1));
    }

    for (const std::string& sp : searchPaths) {
        int idx = mz_zip_reader_locate_file(&zip, sp.c_str(), nullptr, 0);
        if (idx < 0) continue;

        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, idx, &stat)) continue;

        std::vector<unsigned char> data(stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip, idx, data.data(), data.size(), 0)) continue;

        mz_zip_reader_end(&zip);

        // Write to a temp file — miniaudio needs a file path for streaming
        std::filesystem::path tmpDir = std::filesystem::temp_directory_path();
        std::string ext = std::filesystem::path(norm).extension().string();
        std::string tmpPath = (tmpDir / ("like2d_audio_" + key + ext)).string();

        std::ofstream out(tmpPath, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "AudioManager: Failed to write temp audio file: " << tmpPath << std::endl;
            return false;
        }
        out.write(reinterpret_cast<char*>(data.data()), data.size());
        out.close();

        SoundEntry* entry = new SoundEntry();
        if (ma_sound_init_from_file(&m_engine, tmpPath.c_str(), 0, NULL, NULL, &entry->sound) != MA_SUCCESS) {
            std::cerr << "AudioManager: Failed to init sound from temp file: " << tmpPath << std::endl;
            delete entry;
            std::filesystem::remove(tmpPath);
            return false;
        }
        entry->loaded = true;
        m_sounds[key] = entry;
        m_tempFiles.push_back(tmpPath);
        return true;
    }

    mz_zip_reader_end(&zip);
    std::cerr << "AudioManager: Sound not found in game.like: " << filename << std::endl;
    return false;
}

// ---- Playback ----

bool AudioManager::playSound(const std::string& key, bool loop) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end() || !it->second->loaded) return false;
    ma_sound_set_looping(&it->second->sound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_seek_to_pcm_frame(&it->second->sound, 0);
    return ma_sound_start(&it->second->sound) == MA_SUCCESS;
}

bool AudioManager::stopSound(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end() || !it->second->loaded) return false;
    ma_sound_stop(&it->second->sound);
    ma_sound_seek_to_pcm_frame(&it->second->sound, 0);
    return true;
}

bool AudioManager::pauseSound(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end() || !it->second->loaded) return false;
    return ma_sound_stop(&it->second->sound) == MA_SUCCESS;
}

bool AudioManager::resumeSound(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end() || !it->second->loaded) return false;
    return ma_sound_start(&it->second->sound) == MA_SUCCESS;
}

bool AudioManager::isSoundPlaying(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end() || !it->second->loaded) return false;
    return ma_sound_is_playing(&it->second->sound) == MA_TRUE;
}

// ---- Volume / Pitch ----

void AudioManager::setSoundVolume(const std::string& key, float volume) {
    auto it = m_sounds.find(key);
    if (it != m_sounds.end() && it->second->loaded)
        ma_sound_set_volume(&it->second->sound, volume);
}

void AudioManager::setMasterVolume(float volume) {
    if (m_initialized) ma_engine_set_volume(&m_engine, volume);
}

void AudioManager::setSoundPitch(const std::string& key, float pitch) {
    auto it = m_sounds.find(key);
    if (it != m_sounds.end() && it->second->loaded)
        ma_sound_set_pitch(&it->second->sound, pitch);
}

// ---- Cleanup ----

void AudioManager::unloadSound(const std::string& key) {
    auto it = m_sounds.find(key);
    if (it == m_sounds.end()) return;
    if (it->second->loaded) ma_sound_uninit(&it->second->sound);
    delete it->second;
    m_sounds.erase(it);
}

void AudioManager::unloadAll() {
    for (auto& pair : m_sounds) {
        if (pair.second->loaded) ma_sound_uninit(&pair.second->sound);
        delete pair.second;
    }
    m_sounds.clear();
}

void AudioManager::cleanupTempFiles() {
    for (const auto& path : m_tempFiles) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    m_tempFiles.clear();
}
