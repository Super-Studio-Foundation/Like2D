#include "Engine.h"
#include "renderer/Renderer.h"
#include "scripting/LuaBindings.h"
#include "network/NetworkManager.h"
#include <iostream>
#include <cstring>
#include <string_view>
#include <lua.h>
#include <lualib.h>
#include <unordered_map>
#include <string>
#include <box2d/box2d.h>

// ── Error rate limiter — prevent std::cerr spam from killing FPS ──
static std::unordered_map<std::string, int> s_errorCounts;
static constexpr int MAX_ERROR_REPEATS = 3;  // only print first 3 occurrences

static void reportError(const std::string& msg) {
    auto& count = s_errorCounts[msg];
    if (count < MAX_ERROR_REPEATS) {
        std::cerr << msg << std::endl;
        count++;
    }
    // After MAX_ERROR_REPEATS, silently suppress (prevents I/O stall)
}

// Shared scancode map used by all key query functions (string_view keys for zero-alloc lookup)
static const std::unordered_map<std::string_view, SDL_Scancode>& getKeyMap() {
    static const std::unordered_map<std::string_view, SDL_Scancode> keyMap = {
        {"A", SDL_SCANCODE_A}, {"B", SDL_SCANCODE_B}, {"C", SDL_SCANCODE_C},
        {"D", SDL_SCANCODE_D}, {"E", SDL_SCANCODE_E}, {"F", SDL_SCANCODE_F},
        {"G", SDL_SCANCODE_G}, {"H", SDL_SCANCODE_H}, {"I", SDL_SCANCODE_I},
        {"J", SDL_SCANCODE_J}, {"K", SDL_SCANCODE_K}, {"L", SDL_SCANCODE_L},
        {"M", SDL_SCANCODE_M}, {"N", SDL_SCANCODE_N}, {"O", SDL_SCANCODE_O},
        {"P", SDL_SCANCODE_P}, {"Q", SDL_SCANCODE_Q}, {"R", SDL_SCANCODE_R},
        {"S", SDL_SCANCODE_S}, {"T", SDL_SCANCODE_T}, {"U", SDL_SCANCODE_U},
        {"V", SDL_SCANCODE_V}, {"W", SDL_SCANCODE_W}, {"X", SDL_SCANCODE_X},
        {"Y", SDL_SCANCODE_Y}, {"Z", SDL_SCANCODE_Z},
        {"0", SDL_SCANCODE_0}, {"1", SDL_SCANCODE_1}, {"2", SDL_SCANCODE_2},
        {"3", SDL_SCANCODE_3}, {"4", SDL_SCANCODE_4}, {"5", SDL_SCANCODE_5},
        {"6", SDL_SCANCODE_6}, {"7", SDL_SCANCODE_7}, {"8", SDL_SCANCODE_8},
        {"9", SDL_SCANCODE_9},
        {"Space",     SDL_SCANCODE_SPACE},
        {"Enter",     SDL_SCANCODE_RETURN},
        {"Escape",    SDL_SCANCODE_ESCAPE},
        {"Tab",       SDL_SCANCODE_TAB},
        {"Backspace", SDL_SCANCODE_BACKSPACE},
        {"Delete",    SDL_SCANCODE_DELETE},
        {"Up",        SDL_SCANCODE_UP},
        {"Down",      SDL_SCANCODE_DOWN},
        {"Left",      SDL_SCANCODE_LEFT},
        {"Right",     SDL_SCANCODE_RIGHT},
        {"LShift",    SDL_SCANCODE_LSHIFT},
        {"RShift",    SDL_SCANCODE_RSHIFT},
        {"Shift",     SDL_SCANCODE_LSHIFT},
        {"LCtrl",     SDL_SCANCODE_LCTRL},
        {"RCtrl",     SDL_SCANCODE_RCTRL},
        {"Ctrl",      SDL_SCANCODE_LCTRL},
        {"LAlt",      SDL_SCANCODE_LALT},
        {"RAlt",      SDL_SCANCODE_RALT},
        {"Alt",       SDL_SCANCODE_LALT},
        {"F1",  SDL_SCANCODE_F1},  {"F2",  SDL_SCANCODE_F2},
        {"F3",  SDL_SCANCODE_F3},  {"F4",  SDL_SCANCODE_F4},
        {"F5",  SDL_SCANCODE_F5},  {"F6",  SDL_SCANCODE_F6},
        {"F7",  SDL_SCANCODE_F7},  {"F8",  SDL_SCANCODE_F8},
        {"F9",  SDL_SCANCODE_F9},  {"F10", SDL_SCANCODE_F10},
        {"F11", SDL_SCANCODE_F11}, {"F12", SDL_SCANCODE_F12},
        {"Home",     SDL_SCANCODE_HOME},
        {"End",      SDL_SCANCODE_END},
        {"PageUp",   SDL_SCANCODE_PAGEUP},
        {"PageDown", SDL_SCANCODE_PAGEDOWN},
        {"Insert",   SDL_SCANCODE_INSERT},
        {"Numpad0",  SDL_SCANCODE_KP_0}, {"Numpad1", SDL_SCANCODE_KP_1},
        {"Numpad2",  SDL_SCANCODE_KP_2}, {"Numpad3", SDL_SCANCODE_KP_3},
        {"Numpad4",  SDL_SCANCODE_KP_4}, {"Numpad5", SDL_SCANCODE_KP_5},
        {"Numpad6",  SDL_SCANCODE_KP_6}, {"Numpad7", SDL_SCANCODE_KP_7},
        {"Numpad8",  SDL_SCANCODE_KP_8}, {"Numpad9", SDL_SCANCODE_KP_9},
    };
    return keyMap;
}

Engine::Engine()
    : window(nullptr), renderer(nullptr), running(false),
      keyboardState(nullptr), lastTime(0), deltaTime(0.0),
      m_worldId{0}, m_timeStep(1.0f / 60.0f), m_accumulator(0.0f),
      m_scrollX(0.0f), m_scrollY(0.0f),
      m_cameraX(0.0f), m_cameraY(0.0f), m_cameraZoom(1.0f),
      m_escapeQuits(true),
      m_logicalWidth(0), m_logicalHeight(0), m_useLetterbox(false),
      m_letterboxR(0), m_letterboxG(0), m_letterboxB(0),
      m_hasDynamicBodies(false),
      m_cachedLetterboxScale(1.0f), m_cachedLetterboxOX(0.0f), m_cachedLetterboxOY(0.0f),
      m_cachedWindowW(0), m_cachedWindowH(0), m_letterboxDirty(true),
      m_network(nullptr)
{
    memset(prevKeyboardState, 0, sizeof(prevKeyboardState));
    // Initialise platform sockets (Winsock2 on Windows)
    NetworkManager::initSockets();
    m_network = new NetworkManager();
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const std::string& title, int width, int height) {
    std::cout << "Like2D Framework v1.2.1.0" << std::endl;
    std::cout << "Initializing engine..." << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, 0);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity    = {0.0f, 0.0f};
    worldDef.workerCount = 2;
    m_worldId = b2CreateWorld(&worldDef);

    running = true;
    return true;
}

void Engine::run(lua_State* L, Renderer* luaRenderer) {
    lastTime = SDL_GetPerformanceCounter();

    // Call onInit once before the loop
    lua_getglobal(L, "onInit");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != 0) {
            reportError(std::string("Error calling onInit: ") + lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }

    // Reset timer after onInit so startup cost doesn't spike dt
    lastTime = SDL_GetPerformanceCounter();

    while (running) {
        // Snapshot previous keyboard state for just-pressed/released detection
        if (keyboardState) {
            memcpy(prevKeyboardState, keyboardState, sizeof(prevKeyboardState));
        }

        // Reset per-frame scroll
        m_scrollX = 0.0f;
        m_scrollY = 0.0f;

        SDL_PumpEvents();
        keyboardState = SDL_GetKeyboardState(NULL);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (m_escapeQuits && event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                // Fire onKeyPressed callback
                lua_getglobal(L, "onKeyPressed");
                if (lua_isfunction(L, -1)) {
                    lua_pushstring(L, SDL_GetKeyName(event.key.key));
                    if (lua_pcall(L, 1, 0, 0) != 0) {
                        reportError(std::string("Error in onKeyPressed: ") + lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                lua_getglobal(L, "onKeyReleased");
                if (lua_isfunction(L, -1)) {
                    lua_pushstring(L, SDL_GetKeyName(event.key.key));
                    if (lua_pcall(L, 1, 0, 0) != 0) {
                        reportError(std::string("Error in onKeyReleased: ") + lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                const char* cbName = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? "onMousePressed" : "onMouseReleased";
                lua_getglobal(L, cbName);
                if (lua_isfunction(L, -1)) {
                    lua_pushinteger(L, event.button.button - 1); // 0-based
                    lua_pushnumber(L, event.button.x);
                    lua_pushnumber(L, event.button.y);
                    if (lua_pcall(L, 3, 0, 0) != 0) {
                        reportError(std::string("Error in ") + cbName + ": " + lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                m_scrollX += event.wheel.x;
                m_scrollY += event.wheel.y;
                lua_getglobal(L, "onMouseScroll");
                if (lua_isfunction(L, -1)) {
                    lua_pushnumber(L, event.wheel.x);
                    lua_pushnumber(L, event.wheel.y);
                    if (lua_pcall(L, 2, 0, 0) != 0) {
                        reportError(std::string("Error in onMouseScroll: ") + lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            } else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                m_letterboxDirty = true;
                lua_getglobal(L, "onResize");
                if (lua_isfunction(L, -1)) {
                    lua_pushinteger(L, event.window.data1);
                    lua_pushinteger(L, event.window.data2);
                    if (lua_pcall(L, 2, 0, 0) != 0) {
                        reportError(std::string("Error in onResize: ") + lua_tostring(L, -1));
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
            }
        }

        // Delta time — high-resolution using performance counter
        Uint64 currentTime = SDL_GetPerformanceCounter();
        static const double freq = (double)SDL_GetPerformanceFrequency();
        deltaTime = (double)(currentTime - lastTime) / freq;
        if (deltaTime > 0.1) deltaTime = 0.1;   // cap at 100ms
        if (deltaTime < 0.0) deltaTime = 0.0;
        lastTime  = currentTime;

        // ── Hot reload check (dev mode only) ──
        if (m_hotReloadChecker) {
            lua_State* newState = m_hotReloadChecker();
            if (newState) {
                L = newState;
                // Reset timer to avoid dt spike after reload
                lastTime = SDL_GetPerformanceCounter();
                // Skip rest of frame — next frame starts fresh with onInit
                continue;
            }
        }

        // Physics step — only if there are dynamic bodies
        if (m_hasDynamicBodies) {
            m_accumulator += (float)deltaTime;
            while (m_accumulator >= m_timeStep) {
                b2World_Step(m_worldId, m_timeStep, 4);
                m_accumulator -= m_timeStep;
            }
        }

        // ── Poll network events and fire Lua callbacks ──
        if (m_network) {
            std::vector<NetworkEvent> netEvents;
            m_network->poll(netEvents);
            for (const auto& ev : netEvents) {
                switch (ev.type) {
                case NetEventType::Connect:
                    lua_getglobal(L, "onNetConnect");
                    if (lua_isfunction(L, -1)) {
                        lua_pushinteger(L, ev.peerId);
                        lua_pushstring(L, ev.ip.c_str());
                        lua_pushinteger(L, ev.port);
                        if (lua_pcall(L, 3, 0, 0) != 0) {
                            reportError(std::string("Error in onNetConnect: ") + lua_tostring(L, -1));
                            lua_pop(L, 1);
                        }
                    } else {
                        lua_pop(L, 1);
                    }
                    break;
                case NetEventType::Message:
                    lua_getglobal(L, "onNetMessage");
                    if (lua_isfunction(L, -1)) {
                        lua_pushinteger(L, ev.peerId);
                        lua_pushstring(L, ev.data.c_str());
                        lua_pushstring(L, ev.ip.c_str());
                        lua_pushinteger(L, ev.port);
                        if (lua_pcall(L, 4, 0, 0) != 0) {
                            reportError(std::string("Error in onNetMessage: ") + lua_tostring(L, -1));
                            lua_pop(L, 1);
                        }
                    } else {
                        lua_pop(L, 1);
                    }
                    break;
                case NetEventType::Disconnect:
                    lua_getglobal(L, "onNetDisconnect");
                    if (lua_isfunction(L, -1)) {
                        lua_pushinteger(L, ev.peerId);
                        lua_pushstring(L, ev.data.c_str());
                        if (lua_pcall(L, 2, 0, 0) != 0) {
                            reportError(std::string("Error in onNetDisconnect: ") + lua_tostring(L, -1));
                            lua_pop(L, 1);
                        }
                    } else {
                        lua_pop(L, 1);
                    }
                    break;
                }
            }
        }

        // onUpdate callback
        lua_getglobal(L, "onUpdate");
        if (lua_isfunction(L, -1)) {
            lua_pushnumber(L, (float)deltaTime);
            if (lua_pcall(L, 1, 0, 0) != 0) {
                reportError(std::string("Error calling onUpdate: ") + lua_tostring(L, -1));
                lua_pop(L, 1);
            }
        } else {
            lua_pop(L, 1);
        }

    // Calculate letterbox (cached — only recalc when window size changes)
    float scale, offsetX, offsetY;
    int windowW, windowH;
    if (m_letterboxDirty) {
        calculateLetterbox(scale, offsetX, offsetY, windowW, windowH);
        m_cachedLetterboxScale = scale;
        m_cachedLetterboxOX = offsetX;
        m_cachedLetterboxOY = offsetY;
        m_cachedWindowW = windowW;
        m_cachedWindowH = windowH;
        m_letterboxDirty = false;
    } else {
        scale = m_cachedLetterboxScale;
        offsetX = m_cachedLetterboxOX;
        offsetY = m_cachedLetterboxOY;
        windowW = m_cachedWindowW;
        windowH = m_cachedWindowH;
    }

    // Clear screen (letterbox color if enabled)
    if (m_useLetterbox) {
        SDL_SetRenderDrawColor(renderer, 
            (Uint8)m_letterboxR, 
            (Uint8)m_letterboxG, 
            (Uint8)m_letterboxB, 
            255);
    } else {
        // Clear screen with onDraw() handling clear normally
    }
    SDL_RenderClear(this->renderer);

    // Apply letterbox transform
    if (m_useLetterbox) {
        SDL_Rect viewportRect = {
            (int)offsetX, (int)offsetY, 
            (int)(m_logicalWidth * scale), (int)(m_logicalHeight * scale)
        };
        SDL_SetRenderViewport(renderer, &viewportRect);
    }

    // Apply camera transform (single call, not redundant)
    SDL_SetRenderScale(this->renderer, m_cameraZoom * scale, m_cameraZoom * scale);

    // onDraw callback
    lua_getglobal(L, "onDraw");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != 0) {
            reportError(std::string("Error calling onDraw: ") + lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }

    // Reset transforms
    SDL_SetRenderScale(this->renderer, 1.0f, 1.0f);
    if (m_useLetterbox) {
        SDL_SetRenderViewport(renderer, NULL);
    }
    SDL_RenderPresent(this->renderer);
    }

    // Call onCleanup before exit
    lua_getglobal(L, "onCleanup");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != 0) {
            std::cerr << "Error calling onCleanup: " << lua_tostring(L, -1) << std::endl;
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }
}

void Engine::shutdown() {
    // Shut down networking first
    if (m_network) {
        m_network->shutdown();
        delete m_network;
        m_network = nullptr;
    }
    if (b2World_IsValid(m_worldId)) {
        b2DestroyWorld(m_worldId);
        m_worldId = {0};
    }
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

SDL_Window*        Engine::getWindow()        const { return window; }
SDL_Renderer*      Engine::getRenderer()      const { return renderer; }
const std::string& Engine::getProjectFolder() const { return projectFolder; }
void               Engine::setProjectFolder(const std::string& folder) { projectFolder = folder; }

void Engine::setWindowTitle(const std::string& title) {
    if (window) SDL_SetWindowTitle(window, title.c_str());
}

void Engine::setWindowSize(int width, int height) {
    if (window) {
        SDL_SetWindowSize(window, width, height);
        SDL_RenderPresent(renderer);
        m_letterboxDirty = true;
    }
}

void Engine::setFullscreen(bool fullscreen) {
    if (window) {
        SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        m_letterboxDirty = true;
    }
}

void Engine::getWindowSize(int& width, int& height) const {
    if (window) SDL_GetWindowSize(window, &width, &height);
}

void Engine::setEscapeQuits(bool value) {
    m_escapeQuits = value;
}

// ---- Input ----

bool Engine::isKeyDown(const char* keyName) {
    if (!keyboardState) return false;
    auto& keyMap = getKeyMap();
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) return keyboardState[it->second];
    return false;
}

bool Engine::isKeyJustPressed(const char* keyName) {
    if (!keyboardState) return false;
    auto& keyMap = getKeyMap();
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        SDL_Scancode sc = it->second;
        return keyboardState[sc] && !prevKeyboardState[sc];
    }
    return false;
}

bool Engine::isKeyJustReleased(const char* keyName) {
    if (!keyboardState) return false;
    auto& keyMap = getKeyMap();
    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        SDL_Scancode sc = it->second;
        return !keyboardState[sc] && prevKeyboardState[sc];
    }
    return false;
}

void Engine::getMouseScroll(float& x, float& y) const {
    x = m_scrollX;
    y = m_scrollY;
}

// ---- Time ----

float Engine::getDeltaTime() { return deltaTime; }

// ---- Physics ----

b2WorldId Engine::getWorldId() const { return m_worldId; }

void Engine::setGravity(float x, float y) {
    if (b2World_IsValid(m_worldId))
        b2World_SetGravity(m_worldId, {x, y});
}

// ---- Camera ----

void Engine::setCameraPosition(float x, float y) { m_cameraX = x; m_cameraY = y; }
void Engine::setCameraZoom(float zoom)            { m_cameraZoom = (zoom > 0.01f) ? zoom : 0.01f; }
void Engine::getCameraPosition(float& x, float& y) const { x = m_cameraX; y = m_cameraY; }
float Engine::getCameraZoom() const { return m_cameraZoom; }

void Engine::worldToScreen(float wx, float wy, float& sx, float& sy) const {
    sx = (wx - m_cameraX) * m_cameraZoom;
    sy = (wy - m_cameraY) * m_cameraZoom;
}

void Engine::setLogicalSize(int width, int height) {
    m_logicalWidth = width;
    m_logicalHeight = height;
}

void Engine::setUseLetterbox(bool use) {
    m_useLetterbox = use;
}

void Engine::setLetterboxColor(int r, int g, int b) {
    m_letterboxR = r;
    m_letterboxG = g;
    m_letterboxB = b;
}

void Engine::getLogicalSize(int& width, int& height) const {
    width = m_logicalWidth;
    height = m_logicalHeight;
}

void Engine::calculateLetterbox(float& scale, float& offsetX, float& offsetY, int& windowW, int& windowH) const {
    getWindowSize(windowW, windowH);
    
    if (!m_useLetterbox || m_logicalWidth == 0 || m_logicalHeight == 0) {
        scale = 1.0f;
        offsetX = 0.0f;
        offsetY = 0.0f;
        return;
    }
    
    float aspectRatio = (float)m_logicalWidth / (float)m_logicalHeight;
    float windowAspect = (float)windowW / (float)windowH;
    
    if (windowAspect > aspectRatio) {
        // Window is wider: black bars on sides
        scale = (float)windowH / (float)m_logicalHeight;
        offsetX = (windowW - m_logicalWidth * scale) / 2.0f;
        offsetY = 0.0f;
    } else {
        // Window is taller: black bars on top/bottom
        scale = (float)windowW / (float)m_logicalWidth;
        offsetX = 0.0f;
        offsetY = (windowH - m_logicalHeight * scale) / 2.0f;
    }
}
