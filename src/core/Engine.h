#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <functional>
#include <SDL3/SDL.h>
#include <lua.h>
#include <box2d/box2d.h>

class Renderer;
class NetworkManager;

class Engine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    std::string projectFolder;
    const bool* keyboardState;
    bool prevKeyboardState[SDL_SCANCODE_COUNT];
    Uint64 lastTime;
    double deltaTime;
    b2WorldId m_worldId;
    float m_timeStep;
    float m_accumulator;

    // Mouse scroll accumulator
    float m_scrollX;
    float m_scrollY;

    // Camera
    float m_cameraX;
    float m_cameraY;
    float m_cameraZoom;

    // Escape override
    bool m_escapeQuits;
    
    // Letterbox/aspect ratio support
    int m_logicalWidth;
    int m_logicalHeight;
    bool m_useLetterbox;
    float m_letterboxR, m_letterboxG, m_letterboxB;

    // Physics optimization: skip step if no dynamic bodies
    bool m_hasDynamicBodies;

    // Letterbox cache (avoid recalculating every frame)
    float m_cachedLetterboxScale, m_cachedLetterboxOX, m_cachedLetterboxOY;
    int m_cachedWindowW, m_cachedWindowH;
    bool m_letterboxDirty;

    // Networking
    NetworkManager* m_network;

    // Hot reload callback: returns new lua_State* if reload happened, nullptr otherwise
    std::function<lua_State*()> m_hotReloadChecker;

public:
    Engine();
    ~Engine();
    
    bool initialize(const std::string& title, int width, int height);
    void run(lua_State* L, Renderer* renderer);
    void shutdown();

    // Hot reload: set callback that checks for script changes each frame
    void setHotReloadChecker(std::function<lua_State*()> checker) { m_hotReloadChecker = checker; }
    
    SDL_Window* getWindow() const;
    SDL_Renderer* getRenderer() const;
    const std::string& getProjectFolder() const;
    void setProjectFolder(const std::string& folder);
    
    void setWindowTitle(const std::string& title);
    void setWindowSize(int width, int height);
    void setFullscreen(bool fullscreen);
    void getWindowSize(int& width, int& height) const;
    
    // Letterbox functions
    void setLogicalSize(int width, int height);
    void setUseLetterbox(bool use);
    void setLetterboxColor(int r, int g, int b);
    void getLogicalSize(int& width, int& height) const;
    void calculateLetterbox(float& scale, float& offsetX, float& offsetY, int& windowW, int& windowH) const;

    // Input
    bool isKeyDown(const char* keyName);
    bool isKeyJustPressed(const char* keyName);
    bool isKeyJustReleased(const char* keyName);
    void getMouseScroll(float& x, float& y) const;
    void setEscapeQuits(bool value);

    // Time
    float getDeltaTime();

    // Physics
    b2WorldId getWorldId() const;
    void setGravity(float x, float y);
    void setHasDynamicBodies(bool value) { m_hasDynamicBodies = value; }

    // Camera
    void setCameraPosition(float x, float y);
    void setCameraZoom(float zoom);
    void getCameraPosition(float& x, float& y) const;
    float getCameraZoom() const;
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;

    // Networking
    NetworkManager* getNetworkManager() const { return m_network; }
};
