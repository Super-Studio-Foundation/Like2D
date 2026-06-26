#define MINIAUDIO_IMPLEMENTATION
#include "AudioManager.h"
#include "../platform.h"
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

static const uint8_t LIKE2D_ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
static const uint8_t LIKE2D_XOR_KEY[]   = {
    0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
    0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
};
static const size_t  LIKE2D_XOR_KEY_LEN = sizeof(LIKE2D_XOR_KEY);

// Helper to read and decrypt archive data (from game.like or fused exe)
static bool getArchiveData(std::vector<char>& outData, std::string& outPath) {
    static std::vector<char> s_cache;
    static std::string       s_cachePath;

    std::filesystem::path exeDir  = getExeDir();
    std::filesystem::path exePath = getExePath();
    std::filesystem::path gameLike = exeDir / "game.like";

    // Try game.like first, then fused EXE
    std::vector<std::filesystem::path> paths = { gameLike, exePath };
    
    for (const auto& p : paths) {
        if (!std::filesystem::exists(p)) continue;
        
        std::string pStr = p.string();
        if (s_cachePath == pStr && !s_cache.empty()) {
            outData = s_cache;
            outPath = s_cachePath;
            return true;
        }

        std::ifstream f(p, std::ios::binary);
        if (!f.is_open()) continue;
        std::vector<char> raw((std::istreambuf_iterator<char>(f)), {});
        f.close();

        // Check for encrypted L2DE format
        if (raw.size() >= 4 &&
            (uint8_t)raw[0] == LIKE2D_ENC_MAGIC[0] && (uint8_t)raw[1] == LIKE2D_ENC_MAGIC[1] &&
            (uint8_t)raw[2] == LIKE2D_ENC_MAGIC[2] && (uint8_t)raw[3] == LIKE2D_ENC_MAGIC[3])
        {
            s_cache.assign(raw.begin() + 4, raw.end());
            for (size_t i = 0; i < s_cache.size(); i++)
                s_cache[i] ^= (char)LIKE2D_XOR_KEY[i % LIKE2D_XOR_KEY_LEN];
            s_cachePath = pStr;
        } 
        // Or plain ZIP format
        else {
            mz_zip_archive zip = {};
            if (mz_zip_reader_init_mem(&zip, raw.data(), raw.size(), 0)) {
                mz_zip_reader_end(&zip);
                s_cache = std::move(raw);
                s_cachePath = pStr;
            } else {
                continue; // Not an archive
            }
        }

        outData = s_cache;
        outPath = s_cachePath;
        return true;
    }
    return false;
}

bool AudioManager::loadSound(const std::string& key, const std::string& filename) {
    if (!m_initialized) return false;
    if (m_sounds.count(key)) return true; // already loaded

    // 1. Production archive search (game.like or fused EXE)
    std::vector<char> zipData;
    std::string       zipPath;
    if (getArchiveData(zipData, zipPath)) {
        if (loadSoundFromArchive(key, filename, zipData)) {
            return true;
        }
    }

    // 2. Disk candidates search
    std::filesystem::path exeDir = getExeDir();
    std::vector<std::filesystem::path> basePaths;
    
    std::filesystem::path testPath(filename);
    if (testPath.is_absolute()) {
        if (std::filesystem::exists(testPath)) {
            return initSoundFromFile(key, testPath.string());
        }
        return false;
    }

    // Common root locations
    if (!m_projectFolder.empty()) basePaths.push_back(m_projectFolder);
    basePaths.push_back(exeDir / "userdata");
    basePaths.push_back(exeDir);
    basePaths.push_back(std::filesystem::current_path());

    // Search patterns (same as archive for consistency)
    std::string norm = filename;
    for (char& c : norm) if (c == '\\') c = '/';

    std::vector<std::string> subPaths = { norm };
    if (norm.find('/') == std::string::npos) {
        subPaths.push_back("audio/"  + norm);
        subPaths.push_back("sounds/" + norm);
        subPaths.push_back("music/"  + norm);
        subPaths.push_back("assets/" + norm);
    }

    for (const auto& base : basePaths) {
        for (const auto& sub : subPaths) {
            std::filesystem::path p = base / sub;
            if (std::filesystem::exists(p)) {
                return initSoundFromFile(key, p.string());
            }
        }
    }

    std::cerr << "AudioManager: Failed to locate '" << filename << "' (checked archive and disk)" << std::endl;
    return false;
}

// Helper to avoid code duplication
bool AudioManager::initSoundFromFile(const std::string& key, const std::string& path) {
    SoundEntry* entry = new SoundEntry();
    if (ma_sound_init_from_file(&m_engine, path.c_str(), 0, NULL, NULL, &entry->sound) != MA_SUCCESS) {
        std::cerr << "AudioManager: Failed to init sound: " << path << std::endl;
        delete entry;
        return false;
    }
    entry->loaded = true;
    m_sounds[key] = entry;
    return true;
}

bool AudioManager::loadSoundFromArchive(const std::string& key, const std::string& filename, const std::vector<char>& zipData) {
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, zipData.data(), zipData.size(), 0)) {
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
            return false;
        }
        out.write(reinterpret_cast<char*>(data.data()), data.size());
        out.close();

        SoundEntry* entry = new SoundEntry();
        if (ma_sound_init_from_file(&m_engine, tmpPath.c_str(), 0, NULL, NULL, &entry->sound) != MA_SUCCESS) {
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
