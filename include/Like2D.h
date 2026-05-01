#ifndef LIKE2D_H
#define LIKE2D_H

#ifdef LIKE2D_EXPORTS
#define LIKE2D_API __declspec(dllexport)
#else
#define LIKE2D_API __declspec(dllimport)
#endif

#include <SDL3/SDL.h>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <filesystem>

// Forward declarations
class Engine;
class Renderer;

namespace Like2D {
    // Application class to manage the framework
    class LIKE2D_API Application {
    public:
        Application();
        ~Application();
        
        bool initialize(int argc, char* argv[]);
        void run();
        void shutdown();
        
    private:
        Engine* m_engine;
        Renderer* m_renderer;
        lua_State* m_luaState;
        std::string m_projectFolder;
        std::string m_mainLuaPath;
        bool m_running;
        
        bool parseArguments(int argc, char* argv[]);
        bool initializeSDL();
        bool initializeLua();
        bool loadScript();
        void handleEvents();
        void update();
        void render();
        
        std::string read_file(const std::string& path);
        bool execute_lua_code(lua_State* L, const std::string& code);
    };
    
    // Lua bindings
    namespace LuaBindings {
        int setWindowTitle(lua_State* L);
        void registerAll(lua_State* L, SDL_Window* window);
    }
}

#endif // LIKE2D_H
