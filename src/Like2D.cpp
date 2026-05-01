#define SDL_MAIN_HANDLED
#include "Like2D.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include "scripting/LuaBindings.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <Luau/Compiler.h>

namespace Like2D {

Application::Application() 
    : m_engine(nullptr), m_renderer(nullptr), m_luaState(nullptr), m_running(false) {
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
    
    if (!initializeLua()) {
        return false;
    }
    
    if (!loadScript()) {
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
    
    // Debug output
    std::cerr << "Main Lua Path: " << m_mainLuaPath << std::endl;
    std::cerr << "Project Folder: " << m_projectFolder << std::endl;
    
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

bool Application::initializeLua() {
    m_luaState = luaL_newstate();
    if (!m_luaState) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return false;
    }
    
    luaL_openlibs(m_luaState);
    register_lua_functions(m_luaState, m_engine, m_renderer);
    
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
