#define SDL_MAIN_HANDLED
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

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool execute_lua_code(lua_State* L, const std::string& code) {
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

int main(int argc, char* argv[]) {
    std::string mainLuaPath = "scripts/main.luau";
    
    if (argc > 1) {
        std::string inputPath = argv[1];
        
        if (std::filesystem::is_directory(inputPath)) {
            mainLuaPath = inputPath;
            if (mainLuaPath.back() != '/' && mainLuaPath.back() != '\\') {
                mainLuaPath += '/';
            }
            mainLuaPath += "main.luau";
        } else if (std::filesystem::exists(inputPath)) {
            mainLuaPath = inputPath;
        } else {
            std::cerr << "Path does not exist: " << inputPath << std::endl;
            return 1;
        }
    }
    
    Engine engine;
    if (!engine.initialize("Like2D Framework", 800, 600)) {
        return 1;
    }
    
    std::filesystem::path luaPath(mainLuaPath);
    engine.setProjectFolder(luaPath.parent_path().string());
    
    Renderer renderer(engine.getRenderer(), engine.getProjectFolder());
    
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return 1;
    }
    
    luaL_openlibs(L);
    register_lua_functions(L, &engine, &renderer);
    
    std::string luaCode = read_file(mainLuaPath);
    if (luaCode.empty()) {
        std::cerr << "Failed to read main.luau from: " << mainLuaPath << std::endl;
        std::cerr << "Usage: " << argv[0] << " [path/to/main.luau]" << std::endl;
        std::cerr << "   Or: Drag a folder containing main.luau onto Like2D.exe" << std::endl;
        lua_close(L);
        return 1;
    }
    
    if (!execute_lua_code(L, luaCode)) {
        std::cerr << "Failed to execute main.luau" << std::endl;
        lua_close(L);
        return 1;
    }
    
    engine.run(L, &renderer);
    lua_close(L);
    return 0;
}
