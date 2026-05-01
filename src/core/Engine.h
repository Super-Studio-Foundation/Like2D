#pragma once
#include <string>
#include <SDL3/SDL.h>
#include <lua.h>

class Renderer;

class Engine {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    std::string projectFolder;
    const Uint8* keyboardState;
    Uint32 lastTime;
    float deltaTime;

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
    bool isKeyDown(const std::string& keyName);
    float getDeltaTime();
};
