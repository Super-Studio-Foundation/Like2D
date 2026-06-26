#ifndef LIKE2D_H
#define LIKE2D_H

#ifdef _WIN32
    #ifdef LIKE2D_EXPORTS
    #define LIKE2D_API __declspec(dllexport)
    #else
    #define LIKE2D_API __declspec(dllimport)
    #endif
#else
    #define LIKE2D_API
#endif

#include <SDL3/SDL.h>
#include <lua.h>
#include <lualib.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

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

        // Hot reload: re-reads main.luau and returns new lua_State (nullptr = no reload)
        lua_State* hotReload();

        struct DofileState {
            std::string projectFolder;
            std::string exeDir;
        };
        
    private:
        Engine* m_engine;
        Renderer* m_renderer;
        lua_State* m_luaState;
        std::string m_projectFolder;
        std::string m_mainLuaPath;
        bool m_running;
        std::vector<char> m_gameLikeZipData;  // cached decrypted zip bytes
        
        // Hot reload (dev mode only)
        bool m_hotReloadEnabled = false;
        std::filesystem::file_time_type m_lastModTime{};
        int m_hotReloadFrameCounter = 0;
        bool m_isProduction = false;
        std::vector<std::pair<std::string, std::filesystem::file_time_type>> m_watchedDofileFiles;
        
        // Member variables instead of statics for per-instance state
        std::vector<char> m_zipCache;          // per-instance game.like cache
        std::string m_zipCachePath;            // per-instance game.like path
        
        DofileState m_dofileState;
        void* m_dofileUpvalues;  // pointer to DofileUpvalues (for cleanup)
        
        bool parseArguments(int argc, char* argv[]);
        bool initializeSDL();
        bool initializeLua();
        bool loadScript();
        bool loadFromGameLike(const std::filesystem::path& gameLikePath);
        bool loadLuaFromGameLike(const std::filesystem::path& gameLikePath);
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
