#include "Engine.h"
#include "renderer/Renderer.h"
#include "scripting/LuaBindings.h"
#include <iostream>
#include <lua.h>
#include <lualib.h>

Engine::Engine() : window(nullptr), renderer(nullptr), running(false) {}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const std::string& title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    window = SDL_CreateWindow(title.c_str(), width, height, 0);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    running = true;
    return true;
}

void Engine::run(lua_State* L, Renderer* luaRenderer) {
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                else if (event.key.key == SDLK_SPACE) {
                    static bool large = false;
                    if (large) {
                        SDL_SetWindowSize(window, 800, 600);
                        SDL_RenderPresent(this->renderer);
                    } else {
                        SDL_SetWindowSize(window, 1024, 768);
                        SDL_RenderPresent(this->renderer);
                    }
                    large = !large;
                }
            }
        }
        
        SDL_RenderClear(this->renderer);
        
        lua_getglobal(L, "onDraw");
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                std::cerr << "Error calling onDraw: " << lua_tostring(L, -1) << std::endl;
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }
        
        SDL_RenderPresent(this->renderer);
        SDL_Delay(16);
    }
}

void Engine::shutdown() {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

SDL_Window* Engine::getWindow() const {
    return window;
}

SDL_Renderer* Engine::getRenderer() const {
    return renderer;
}

const std::string& Engine::getProjectFolder() const {
    return projectFolder;
}

void Engine::setProjectFolder(const std::string& folder) {
    projectFolder = folder;
}

void Engine::setWindowTitle(const std::string& title) {
    if (window) {
        SDL_SetWindowTitle(window, title.c_str());
    }
}

void Engine::setWindowSize(int width, int height) {
    if (window) {
        SDL_SetWindowSize(window, width, height);
        SDL_RenderPresent(renderer);
    }
}

void Engine::getWindowSize(int& width, int& height) const {
    if (window) {
        SDL_GetWindowSize(window, &width, &height);
    }
}
