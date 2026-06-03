#pragma once
#include <string>
#include <unordered_set>
#include <SDL3/SDL.h>
#include <lua.h>
#include <box2d/box2d.h>

class Renderer;

class Engine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    std::string projectFolder;
    const bool* keyboardState;
    bool prevKeyboardState[SDL_SCANCODE_COUNT];
    Uint32 lastTime;
    float deltaTime;
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

public:
    Engine();
    ~Engine();
    
    bool initialize(const std::string& title, int width, int height);
    void run(lua_State* L, Renderer* renderer);
    void shutdown();
    
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
    bool isKeyDown(const std::string& keyName);
    bool isKeyJustPressed(const std::string& keyName);
    bool isKeyJustReleased(const std::string& keyName);
    void getMouseScroll(float& x, float& y) const;
    void setEscapeQuits(bool value);

    // Time
    float getDeltaTime();

    // Physics
    b2WorldId getWorldId() const;
    void setGravity(float x, float y);

    // Camera
    void setCameraPosition(float x, float y);
    void setCameraZoom(float zoom);
    void getCameraPosition(float& x, float& y) const;
    float getCameraZoom() const;
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
};
