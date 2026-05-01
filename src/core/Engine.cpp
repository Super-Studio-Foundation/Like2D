#include "Engine.h"
#include "renderer/Renderer.h"
#include "scripting/LuaBindings.h"
#include <iostream>
#include <lua.h>
#include <lualib.h>
#include <unordered_map>
#include <string>

Engine::Engine() : window(nullptr), renderer(nullptr), running(false), keyboardState(nullptr) {}

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
        SDL_PumpEvents();
        keyboardState = (const Uint8*)SDL_GetKeyboardState(NULL);
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
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

void Engine::setFullscreen(bool fullscreen) {
    if (window) {
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    }
}

void Engine::getWindowSize(int& width, int& height) const {
    if (window) {
        SDL_GetWindowSize(window, &width, &height);
    }
}

bool Engine::isKeyDown(const std::string& keyName) {
    if (!keyboardState) return false;
    
    static const std::unordered_map<std::string, SDL_Scancode> keyMap = {
        {"A", SDL_SCANCODE_A},
        {"B", SDL_SCANCODE_B},
        {"C", SDL_SCANCODE_C},
        {"D", SDL_SCANCODE_D},
        {"E", SDL_SCANCODE_E},
        {"F", SDL_SCANCODE_F},
        {"G", SDL_SCANCODE_G},
        {"H", SDL_SCANCODE_H},
        {"I", SDL_SCANCODE_I},
        {"J", SDL_SCANCODE_J},
        {"K", SDL_SCANCODE_K},
        {"L", SDL_SCANCODE_L},
        {"M", SDL_SCANCODE_M},
        {"N", SDL_SCANCODE_N},
        {"O", SDL_SCANCODE_O},
        {"P", SDL_SCANCODE_P},
        {"Q", SDL_SCANCODE_Q},
        {"R", SDL_SCANCODE_R},
        {"S", SDL_SCANCODE_S},
        {"T", SDL_SCANCODE_T},
        {"U", SDL_SCANCODE_U},
        {"V", SDL_SCANCODE_V},
        {"W", SDL_SCANCODE_W},
        {"X", SDL_SCANCODE_X},
        {"Y", SDL_SCANCODE_Y},
        {"Z", SDL_SCANCODE_Z},
        {"0", SDL_SCANCODE_0},
        {"1", SDL_SCANCODE_1},
        {"2", SDL_SCANCODE_2},
        {"3", SDL_SCANCODE_3},
        {"4", SDL_SCANCODE_4},
        {"5", SDL_SCANCODE_5},
        {"6", SDL_SCANCODE_6},
        {"7", SDL_SCANCODE_7},
        {"8", SDL_SCANCODE_8},
        {"9", SDL_SCANCODE_9},
        {"Space", SDL_SCANCODE_SPACE},
        {"Enter", SDL_SCANCODE_RETURN},
        {"Escape", SDL_SCANCODE_ESCAPE},
        {"Tab", SDL_SCANCODE_TAB},
        {"Backspace", SDL_SCANCODE_BACKSPACE},
        {"Delete", SDL_SCANCODE_DELETE},
        {"Up", SDL_SCANCODE_UP},
        {"Down", SDL_SCANCODE_DOWN},
        {"Left", SDL_SCANCODE_LEFT},
        {"Right", SDL_SCANCODE_RIGHT},
        {"Shift", SDL_SCANCODE_LSHIFT},
        {"Ctrl", SDL_SCANCODE_LCTRL},
        {"Alt", SDL_SCANCODE_LALT},
        {"F1", SDL_SCANCODE_F1},
        {"F2", SDL_SCANCODE_F2},
        {"F3", SDL_SCANCODE_F3},
        {"F4", SDL_SCANCODE_F4},
        {"F5", SDL_SCANCODE_F5},
        {"F6", SDL_SCANCODE_F6},
        {"F7", SDL_SCANCODE_F7},
        {"F8", SDL_SCANCODE_F8},
        {"F9", SDL_SCANCODE_F9},
        {"F10", SDL_SCANCODE_F10},
        {"F11", SDL_SCANCODE_F11},
        {"F12", SDL_SCANCODE_F12}
    };
    
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        return keyboardState[it->second] != 0;
    }
    
    return false;
}
