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
    std::cout << "[init] parseArguments..." << std::endl;
    if (!parseArguments(argc, argv)) {
        std::cerr << "[init] parseArguments FAILED" << std::endl;
        return false;
    }
    std::cout << "[init] projectFolder = " << m_projectFolder << std::endl;
    std::cout << "[init] mainLuaPath = " << m_mainLuaPath << std::endl;
    
    std::cout << "[init] initializeSDL..." << std::endl;
    if (!initializeSDL()) {
        std::cerr << "[init] initializeSDL FAILED" << std::endl;
        return false;
    }
    
    if (!m_engine || !m_renderer) {
        std::cerr << "[init] Engine or Renderer is null" << std::endl;
        return false;
    }
    
    std::cout << "[init] initializeLua..." << std::endl;
    if (!initializeLua()) {
        std::cerr << "[init] initializeLua FAILED" << std::endl;
        return false;
    }
    
    std::cout << "[init] loadScript..." << std::endl;
    if (!loadScript()) {
#ifdef _WIN32
        MessageBoxA(NULL, "Failed to load main.lua. Please check if the game files are valid.", "Error...", MB_OK | MB_ICONERROR);
#endif
        return false;
    }
    
    std::cout << "[init] All OK!" << std::endl;
    m_running = true;

    // Enable hot reload in dev mode only
    if (!m_isProduction && std::filesystem::exists(m_mainLuaPath)) {
        m_lastModTime = std::filesystem::last_write_time(m_mainLuaPath);
        m_hotReloadEnabled = true;
        std::cout << "[init] Hot reload enabled — watching " << m_mainLuaPath << std::endl;
    }

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
    std::vector<std::pair<std::string, std::filesystem::file_time_type>>* watchedFiles;
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
                // Track this file for hot reload
                if (uv->watchedFiles) {
                    std::string resolved = std::filesystem::absolute(p).string();
                    auto& wf = *uv->watchedFiles;
                    bool found = false;
                    for (auto& entry : wf) {
                        if (entry.first == resolved) { found = true; break; }
                    }
                    if (!found) {
                        std::error_code rec;
                        auto ft = std::filesystem::last_write_time(p, rec);
                        wf.push_back({resolved, rec ? std::filesystem::file_time_type{} : ft});
                    }
                }
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

    // Inject built-in Animation library (pure Lua, uses loadImage + renderImageRegion)
    {
        const char* animLib = R"LUA(
local Animation = {}
Animation.__index = Animation

function loadAnimation(filename, frameWidth, frameHeight, frameCount, frameDuration)
    assert(type(filename) == "string", "loadAnimation: filename must be a string")
    assert(type(frameWidth) == "number" and frameWidth > 0, "loadAnimation: frameWidth must be > 0")
    assert(type(frameHeight) == "number" and frameHeight > 0, "loadAnimation: frameHeight must be > 0")
    assert(type(frameCount) == "number" and frameCount > 0, "loadAnimation: frameCount must be > 0")
    frameDuration = frameDuration or 0.1

    local ok = loadImage(filename)
    if not ok then
        print("loadAnimation: failed to load image '" .. filename .. "'")
        return nil
    end

    local anim = setmetatable({}, Animation)
    anim._filename = filename
    anim._frameW = frameWidth
    anim._frameH = frameHeight
    anim._frameCount = frameCount
    anim._frameDuration = frameDuration
    anim._timer = 0
    anim._currentFrame = 0
    anim._playing = false
    anim._looping = true
    anim._pingpong = false
    anim._reverse = false
    anim._direction = 1
    anim._speed = 1
    anim._finished = false
    anim._alpha = 1
    return anim
end

function Animation:update(dt)
    if not self._playing then return end
    self._timer = self._timer + dt * self._speed
    local elapsed = self._frameDuration
    while self._timer >= elapsed do
        self._timer = self._timer - elapsed
        local next = self._currentFrame + self._direction
        if next >= self._frameCount then
            if self._pingpong then
                self._direction = -1
                self._currentFrame = self._frameCount - 2
                if self._currentFrame < 0 then self._currentFrame = 0 end
            elseif self._looping then
                self._currentFrame = 0
            else
                self._currentFrame = self._frameCount - 1
                self._playing = false
                self._finished = true
                return
            end
        elseif next < 0 then
            if self._pingpong then
                self._direction = 1
                self._currentFrame = 1
                if self._currentFrame >= self._frameCount then self._currentFrame = 0 end
            elseif self._looping then
                self._currentFrame = self._frameCount - 1
            else
                self._currentFrame = 0
                self._playing = false
                self._finished = true
                return
            end
        else
            self._currentFrame = next
        end
    end
end

function Animation:draw(x, y, w, h)
    local sx = self._currentFrame * self._frameW
    local dw = w or self._frameW
    local dh = h or self._frameH
    return renderImageRegion(
        self._filename,
        x, y, dw, dh,
        sx, 0, self._frameW, self._frameH,
        0, false, false, self._alpha, 0, 0
    )
end

function Animation:drawEx(x, y, w, h, angle, flipH, flipV, alpha, ox, oy)
    local sx = self._currentFrame * self._frameW
    local dw = w or self._frameW
    local dh = h or self._frameH
    local a = alpha or self._alpha
    return renderImageRegion(
        self._filename,
        x, y, dw, dh,
        sx, 0, self._frameW, self._frameH,
        angle or 0, flipH or false, flipV or false,
        a, ox or 0, oy or 0
    )
end

function Animation:play()       self._playing = true; self._finished = false end
function Animation:pause()      self._playing = false end
function Animation:resume()     self._playing = true end
function Animation:stop()       self._playing = false; self._currentFrame = 0; self._timer = 0; self._direction = 1; self._finished = false end
function Animation:reset()      self:stop() end
function Animation:setFrame(n)  self._currentFrame = math.max(0, math.min(n, self._frameCount - 1)); self._timer = 0 end
function Animation:isPlaying()  return self._playing end
function Animation:isFinished() return self._finished end
function Animation:getCurrentFrame() return self._currentFrame end
function Animation:getFrameCount()   return self._frameCount end
function Animation:setLooping(v)     self._looping = v end
function Animation:setSpeed(s)       self._speed = s end
function Animation:setPingPong(v)    self._pingpong = v end
function Animation:setReverse(v)     self._reverse = v; self._direction = v and -1 or 1 end
function Animation:setAlpha(a)       self._alpha = a end
function Animation:setDuration(d)    self._frameDuration = d end
)LUA";
        std::string code(animLib);
        execute_lua_code(m_luaState, code);
    }

    // Register dofile with project folder + exe dir as upvalue
    m_dofileState.projectFolder = m_projectFolder;
    m_dofileState.exeDir        = getExeDir().string();
    
    // Store upvalues (will be freed in shutdown())
    DofileUpvalues* uv = new DofileUpvalues{&m_dofileState, &m_zipCache, &m_zipCachePath, &m_watchedDofileFiles};
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
        m_isProduction = true;
        return this->loadFromGameLike(gameLikePath);
    }
    
    m_isProduction = false;
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
        m_engine->setHotReloadChecker([this]() -> lua_State* {
            return this->hotReload();
        });
        m_engine->run(m_luaState, m_renderer);
    }
}

lua_State* Application::hotReload() {
    if (!m_hotReloadEnabled) return nullptr;

    m_hotReloadFrameCounter++;
    // Check every 30 frames (~0.5s at 60fps) to avoid excessive I/O
    if (m_hotReloadFrameCounter < 30) return nullptr;
    m_hotReloadFrameCounter = 0;

    std::error_code ec;
    auto currentTime = std::filesystem::last_write_time(m_mainLuaPath, ec);
    bool needsReload = false;
    std::string changedFile;

    if (!ec && currentTime != m_lastModTime) {
        needsReload = true;
        changedFile = m_mainLuaPath;
    }

    // Also check all dofile'd files for changes
    if (!needsReload) {
        for (auto& entry : m_watchedDofileFiles) {
            auto ft = std::filesystem::last_write_time(entry.first, ec);
            if (!ec && ft != entry.second) {
                needsReload = true;
                changedFile = entry.first;
                break;
            }
        }
    }

    if (!needsReload) return nullptr;

    std::cout << "[hot-reload] " << changedFile << " changed — reloading..." << std::endl;
    m_lastModTime = currentTime;

    // Call onCleanup on old state before destroying
    if (m_luaState) {
        lua_getglobal(m_luaState, "onCleanup");
        if (lua_isfunction(m_luaState, -1)) {
            if (lua_pcall(m_luaState, 0, 0, 0) != 0) {
                std::cerr << "[hot-reload] onCleanup error: " << lua_tostring(m_luaState, -1) << std::endl;
                lua_pop(m_luaState, 1);
            }
        } else {
            lua_pop(m_luaState, 1);
        }
    }

    // Close old Lua state
    if (m_luaState) {
        lua_close(m_luaState);
        m_luaState = nullptr;
    }

    // Free old dofile upvalues
    if (m_dofileUpvalues) {
        delete (DofileUpvalues*)m_dofileUpvalues;
        m_dofileUpvalues = nullptr;
    }

    // Clear watched dofile list — will be re-populated during re-execution
    m_watchedDofileFiles.clear();

    // Create fresh Lua state
    if (!initializeLua()) {
        std::cerr << "[hot-reload] Failed to re-initialize Lua!" << std::endl;
        return nullptr;
    }

    // Re-read and execute main.luau
    std::string luaCode = read_file(m_mainLuaPath);
    if (luaCode.empty()) {
        std::cerr << "[hot-reload] Failed to read " << m_mainLuaPath << " — keeping old state" << std::endl;
        return nullptr;
    }

    if (!execute_lua_code(m_luaState, luaCode)) {
        std::cerr << "[hot-reload] Failed to execute main.luau — script error (keeping running)" << std::endl;
        // Don't return nullptr here — the new state exists, just has no user code
        // Engine will call onInit which won't exist, so it's harmless
        return m_luaState;
    }

    // Call onInit on the new state (Engine::run only calls it once at startup)
    lua_getglobal(m_luaState, "onInit");
    if (lua_isfunction(m_luaState, -1)) {
        if (lua_pcall(m_luaState, 0, 0, 0) != 0) {
            std::cerr << "[hot-reload] onInit error: " << lua_tostring(m_luaState, -1) << std::endl;
            lua_pop(m_luaState, 1);
        }
    } else {
        lua_pop(m_luaState, 1);
    }

    // Record fresh timestamps for all newly-watched dofile'd files
    {
        std::error_code rec;
        for (auto& entry : m_watchedDofileFiles) {
            auto ft = std::filesystem::last_write_time(entry.first, rec);
            if (!rec) entry.second = ft;
        }
    }

    std::cout << "[hot-reload] Reload complete!" << std::endl;
    return m_luaState;
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
