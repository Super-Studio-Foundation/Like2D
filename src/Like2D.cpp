#define SDL_MAIN_HANDLED
#include "Like2D.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include "scripting/LuaBindings.h"
#include "platform.h"
#include "../external/miniz/miniz.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <lua.h>
#include <lualib.h>
#include <Luau/Compiler.h>

namespace Like2D {

Application::Application() 
    : m_engine(nullptr), m_renderer(nullptr), m_luaState(nullptr), m_running(false),
      m_dofileUpvalues(nullptr) {
}

Application::~Application() {
    shutdown();
}

bool Application::initialize(int argc, char* argv[]) {
    if (!parseArguments(argc, argv)) {
        return false;
    }
    
    if (!initializeSDL()) {
        return false;
    }
    
    if (!m_engine || !m_renderer) {
        std::cerr << "Engine or Renderer initialization failed" << std::endl;
        return false;
    }
    
    if (!initializeLua()) {
        return false;
    }
    
    if (!loadScript()) {
#ifdef _WIN32
        MessageBoxA(NULL, "Failed to load main.luau/main.lua. Please check if the game files are valid.", "Like2D Engine Error", MB_OK | MB_ICONERROR);
#endif
        return false;
    }
    
    m_running = true;
    return true;
}

bool Application::parseArguments(int argc, char* argv[]) {
    m_mainLuaPath = "scripts/main.luau";
    
    if (argc > 1) {
        std::string inputPath = argv[1];
        
        if (std::filesystem::is_directory(inputPath)) {
            m_mainLuaPath = inputPath;
            if (m_mainLuaPath.back() != '/' && m_mainLuaPath.back() != '\\') {
                m_mainLuaPath += '/';
            }
            m_mainLuaPath += "main.luau";
        } else if (std::filesystem::exists(inputPath)) {
            m_mainLuaPath = inputPath;
        } else {
            std::cerr << "Path does not exist: " << inputPath << std::endl;
            return false;
        }
    }
    
    std::filesystem::path luaPath(m_mainLuaPath);
    m_projectFolder = luaPath.parent_path().string();
    return true;
}

bool Application::initializeSDL() {
    m_engine = new Engine();
    if (!m_engine->initialize("Like2D Framework", 800, 600)) {
        delete m_engine;
        m_engine = nullptr;
        return false;
    }
    
    m_engine->setProjectFolder(m_projectFolder);
    
    m_renderer = new Renderer(m_engine->getRenderer(), m_engine->getProjectFolder());
    
    return true;
}

// Structure for dofile upvalues
struct DofileUpvalues {
    Like2D::Application::DofileState* dofileState;
    std::vector<char>* zipCache;
    std::string* zipCachePath;
};

static int like2d_dofile(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);

    // Retrieve stored upvalues
    DofileUpvalues* uv = (DofileUpvalues*)lua_tolightuserdata(L, lua_upvalueindex(1));
    Like2D::Application::DofileState* ds = uv->dofileState;
    std::vector<char>* zipCache = uv->zipCache;
    std::string* zipCachePath = uv->zipCachePath;

    // Build search paths
    std::vector<std::filesystem::path> searchPaths;
    if (!ds->projectFolder.empty())
        searchPaths.push_back(std::filesystem::path(ds->projectFolder) / filename);
    searchPaths.push_back(std::filesystem::path(ds->exeDir) / filename);
    searchPaths.push_back(std::filesystem::path(filename));

    std::string code;

    // Production mode: check game.like (with decrypt support)
    std::filesystem::path gameLike = std::filesystem::path(ds->exeDir) / "game.like";
    if (std::filesystem::exists(gameLike)) {
        static const uint8_t ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
        static const uint8_t XOR_KEY[]   = {
            0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
            0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
        };
        static const size_t XOR_KEY_LEN = sizeof(XOR_KEY);

        // Cache the decrypted zip per Application instance
        std::string pathStr = gameLike.string();
        if (*zipCachePath != pathStr || zipCache->empty()) {
            std::ifstream rf(gameLike, std::ios::binary);
            if (rf.is_open()) {
                std::vector<char> raw((std::istreambuf_iterator<char>(rf)), {});
                rf.close();
                if (raw.size() >= 4 &&
                    (uint8_t)raw[0] == ENC_MAGIC[0] && (uint8_t)raw[1] == ENC_MAGIC[1] &&
                    (uint8_t)raw[2] == ENC_MAGIC[2] && (uint8_t)raw[3] == ENC_MAGIC[3])
                {
                    zipCache->assign(raw.begin() + 4, raw.end());
                    for (size_t i = 0; i < zipCache->size(); i++)
                        (*zipCache)[i] ^= (char)XOR_KEY[i % XOR_KEY_LEN];
                } else {
                    *zipCache = std::move(raw);
                }
                *zipCachePath = pathStr;
            }
        }

        if (!zipCache->empty()) {
            mz_zip_archive zip = {};
            if (mz_zip_reader_init_mem(&zip, zipCache->data(), zipCache->size(), 0)) {
                std::string bare = std::filesystem::path(filename).filename().string();
                std::vector<std::string> tries = { filename, bare };
                for (const auto& t : tries) {
                    int idx = mz_zip_reader_locate_file(&zip, t.c_str(), nullptr, 0);
                    if (idx >= 0) {
                        mz_zip_archive_file_stat stat;
                        mz_zip_reader_file_stat(&zip, idx, &stat);
                        std::vector<char> buf(stat.m_uncomp_size);
                        mz_zip_reader_extract_to_mem(&zip, idx, buf.data(), stat.m_uncomp_size, 0);
                        code = std::string(buf.data(), stat.m_uncomp_size);
                        break;
                    }
                }
                mz_zip_reader_end(&zip);
            }
        }
    }

    // Dev mode: read from disk
    if (code.empty()) {
        for (const auto& p : searchPaths) {
            std::ifstream f(p);
            if (f.is_open()) {
                std::ostringstream ss; ss << f.rdbuf();
                code = ss.str();
                break;
            }
        }
    }

    if (code.empty()) {
        luaL_error(L, "dofile: cannot open '%s'", filename);
        return 0;
    }

    // Compile with Luau
    std::string bytecode = Luau::compile(code, Luau::CompileOptions());
    if (luau_load(L, filename, bytecode.data(), bytecode.size(), 0) != 0) {
        luaL_error(L, "dofile compile error in '%s': %s", filename, lua_tostring(L, -1));
        return 0;
    }

    int top = lua_gettop(L) - 1;
    if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
        luaL_error(L, "dofile runtime error in '%s': %s", filename, lua_tostring(L, -1));
        return 0;
    }
    return lua_gettop(L) - top;
}

bool Application::initializeLua() {
    m_luaState = luaL_newstate();
    if (!m_luaState) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }

    luaL_openlibs(m_luaState);
    register_lua_functions(m_luaState, m_engine, m_renderer);

    // Register dofile with project folder + exe dir as upvalue
    m_dofileState.projectFolder = m_projectFolder;
    m_dofileState.exeDir        = getExeDir().string();
    
    // Store upvalues (will be freed in shutdown())
    DofileUpvalues* uv = new DofileUpvalues{&m_dofileState, &m_zipCache, &m_zipCachePath};
    m_dofileUpvalues = uv;

    lua_pushlightuserdata(m_luaState, uv);
    lua_pushcclosure(m_luaState, like2d_dofile, "dofile", 1);
    lua_setglobal(m_luaState, "dofile");

    return true;
}

std::string Application::read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool Application::execute_lua_code(lua_State* L, const std::string& code) {
    std::string bytecode = Luau::compile(code, Luau::CompileOptions());
    if (luau_load(L, "code", bytecode.data(), bytecode.size(), 0) != 0) {
        std::cerr << "Lua compilation error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }
    
    if (lua_pcall(L, 0, 0, 0) != 0) {
        std::cerr << "Lua runtime error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return false;
    }
    
    return true;
}

bool Application::loadScript() {
    std::filesystem::path gameLikePath = getExeDir() / "game.like";
    
    if (std::filesystem::exists(gameLikePath)) {
        std::cout << "Production mode: Loading from game.like" << std::endl;
        return this->loadFromGameLike(gameLikePath);
    }
    
    std::cout << "Development mode: Loading from " << m_mainLuaPath << std::endl;
    std::string luaCode = read_file(m_mainLuaPath);
    if (luaCode.empty()) {
        std::cerr << "Failed to read main.luau from: " << m_mainLuaPath << std::endl;
        std::cerr << "Usage: Like2D.exe [path/to/main.luau]" << std::endl;
        std::cerr << "   Or: Drag a folder containing main.luau onto Like2D.exe" << std::endl;
        return false;
    }
    
    if (!execute_lua_code(m_luaState, luaCode)) {
        std::cerr << "Failed to execute main.luau" << std::endl;
        return false;
    }
    
    return true;
}

bool Application::loadFromGameLike(const std::filesystem::path& gameLikePath) {
    m_mainLuaPath = "main.luau";

    // Read + decrypt shared helper (inline here)
    static const uint8_t ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
    static const uint8_t XOR_KEY[]   = {
        0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
        0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
    };
    static const size_t XOR_KEY_LEN = sizeof(XOR_KEY);

    std::ifstream rawFile(gameLikePath, std::ios::binary);
    if (!rawFile.is_open()) {
        std::cerr << "ERROR: Failed to open game.like archive" << std::endl;
        return false;
    }
    std::vector<char> rawData((std::istreambuf_iterator<char>(rawFile)), {});
    rawFile.close();

    std::vector<char> zipData;
    if (rawData.size() >= 4 &&
        (uint8_t)rawData[0] == ENC_MAGIC[0] && (uint8_t)rawData[1] == ENC_MAGIC[1] &&
        (uint8_t)rawData[2] == ENC_MAGIC[2] && (uint8_t)rawData[3] == ENC_MAGIC[3])
    {
        zipData.assign(rawData.begin() + 4, rawData.end());
        for (size_t i = 0; i < zipData.size(); i++)
            zipData[i] ^= (char)XOR_KEY[i % XOR_KEY_LEN];
    } else {
        zipData = std::move(rawData);
    }

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, zipData.data(), zipData.size(), 0)) {
        std::cerr << "ERROR: Failed to open game.like archive" << std::endl;
        return false;
    }

    bool foundMain = false;
    int fileCount = mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < fileCount; i++) {
        char filename[256];
        mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
        std::string fn(filename);
        if (fn == "main.luau" || fn == "main.lua") { foundMain = true; break; }
    }
    mz_zip_reader_end(&zip);

    if (!foundMain) {
        std::cerr << "ERROR: main.luau not found in game.like" << std::endl;
        return false;
    }

    // Pass the already-decrypted bytes to loadLuaFromGameLike via a temp member
    m_gameLikeZipData = std::move(zipData);
    return loadLuaFromGameLike(gameLikePath);
}

bool Application::loadLuaFromGameLike(const std::filesystem::path& gameLikePath) {
    // Use pre-decrypted zip data cached by loadFromGameLike
    // (avoids reading + decrypting the file twice)
    std::vector<char>& zipData = m_gameLikeZipData;

    // If called directly without loadFromGameLike, read+decrypt now
    if (zipData.empty()) {
        static const uint8_t ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
        static const uint8_t XOR_KEY[]   = {
            0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
            0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
        };
        static const size_t XOR_KEY_LEN = sizeof(XOR_KEY);
        std::ifstream rawFile(gameLikePath, std::ios::binary);
        if (!rawFile.is_open()) { std::cerr << "ERROR: Failed to open game.like" << std::endl; return false; }
        std::vector<char> rawData((std::istreambuf_iterator<char>(rawFile)), {});
        rawFile.close();
        if (rawData.size() >= 4 &&
            (uint8_t)rawData[0] == ENC_MAGIC[0] && (uint8_t)rawData[1] == ENC_MAGIC[1] &&
            (uint8_t)rawData[2] == ENC_MAGIC[2] && (uint8_t)rawData[3] == ENC_MAGIC[3])
        {
            zipData.assign(rawData.begin() + 4, rawData.end());
            for (size_t i = 0; i < zipData.size(); i++)
                zipData[i] ^= (char)XOR_KEY[i % XOR_KEY_LEN];
        } else {
            zipData = std::move(rawData);
        }
    }

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, zipData.data(), zipData.size(), 0)) {
        std::cerr << "ERROR: Failed to open game.like archive for Lua loading" << std::endl;
        return false;
    }

    int fileIndex = mz_zip_reader_locate_file(&zip, m_mainLuaPath.c_str(), nullptr, 0);
    if (fileIndex < 0) {
        std::cerr << "ERROR: Could not find " << m_mainLuaPath << " in game.like" << std::endl;
        mz_zip_reader_end(&zip);
        return false;
    }

    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(&zip, fileIndex, &stat)) {
        std::cerr << "ERROR: Failed to get file stats for " << m_mainLuaPath << std::endl;
        mz_zip_reader_end(&zip);
        return false;
    }

    std::vector<char> fileData(stat.m_uncomp_size);
    if (!mz_zip_reader_extract_to_mem(&zip, fileIndex, fileData.data(), stat.m_uncomp_size, 0)) {
        std::cerr << "ERROR: Failed to extract " << m_mainLuaPath << " from game.like" << std::endl;
        mz_zip_reader_end(&zip);
        return false;
    }

    std::string luaCode(fileData.data(), stat.m_uncomp_size);
    mz_zip_reader_end(&zip);

    if (luaCode.empty()) {
        std::cerr << "ERROR: Empty Lua code from " << m_mainLuaPath << std::endl;
        return false;
    }

    if (!execute_lua_code(m_luaState, luaCode)) {
        std::cerr << "ERROR: Failed to execute " << m_mainLuaPath << std::endl;
        return false;
    }

    return true;
}

void Application::run() {
    if (m_engine && m_luaState && m_renderer) {
        m_engine->run(m_luaState, m_renderer);
    }
}

void Application::shutdown() {
    if (m_luaState) {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }
    
    // Free dofile upvalues
    if (m_dofileUpvalues) {
        delete (DofileUpvalues*)m_dofileUpvalues;
        m_dofileUpvalues = nullptr;
    }
    
    // Clear cached data so next run is fresh
    m_zipCache.clear();
    m_zipCachePath.clear();
    
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }
    
    if (m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }
    
    m_running = false;
}

} // namespace Like2D
