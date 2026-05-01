#include "LuaBindings.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include <iostream>
#include <sstream>
#include <lua.h>
#include <lualib.h>

static Engine* g_engine = nullptr;
static Renderer* g_renderer = nullptr;

int set_window_title(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    if (g_engine) {
        g_engine->setWindowTitle(title);
    }
    return 0;
}

int get_window_size(lua_State* L) {
    int width, height;
    g_engine->getWindowSize(width, height);
    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    return 2;
}

int set_window_size(lua_State* L) {
    int width = luaL_checkinteger(L, 1);
    int height = luaL_checkinteger(L, 2);
    if (g_engine) {
        g_engine->setWindowSize(width, height);
    }
    return 0;
}

int set_fullscreen(lua_State* L) {
    bool fullscreen = lua_toboolean(L, 1);
    if (g_engine) {
        g_engine->setFullscreen(fullscreen);
    }
    return 0;
}

int clear_screen(lua_State* L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    if (g_renderer) {
        g_renderer->setDrawColor(r, g, b);
    }
    return 0;
}

int print_message(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    std::cout << "Lua: " << message << std::endl;
    return 0;
}

int get_time(lua_State* L) {
    lua_pushinteger(L, SDL_GetTicks());
    return 1;
}

int is_key_pressed(lua_State* L) {
    int keycode = luaL_checkinteger(L, 1);
    const bool* keyboardState = SDL_GetKeyboardState(NULL);
    bool pressed = keyboardState[keycode];
    lua_pushboolean(L, pressed);
    return 1;
}

int load_image(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    if (g_renderer) {
        bool success = g_renderer->loadImage(filename);
        lua_pushboolean(L, success);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int render_image(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int width = luaL_checkinteger(L, 4);
    int height = luaL_checkinteger(L, 5);
    if (g_renderer) {
        bool success = g_renderer->renderImage(key, x, y, width, height);
        lua_pushboolean(L, success);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int load_font(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    int size = luaL_checkinteger(L, 2);
    if (g_renderer) {
        bool success = g_renderer->loadFont(filename, size);
        lua_pushboolean(L, success);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int draw_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    const char* fontKey = luaL_checkstring(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int r = luaL_checkinteger(L, 5);
    int g = luaL_checkinteger(L, 6);
    int b = luaL_checkinteger(L, 7);
    if (g_renderer) {
        bool success = g_renderer->drawText(text, fontKey, x, y, r, g, b);
        lua_pushboolean(L, success);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

int draw_text_direct(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int size = luaL_checkinteger(L, 4);
    int r = luaL_checkinteger(L, 5);
    int g = luaL_checkinteger(L, 6);
    int b = luaL_checkinteger(L, 7);
    int a = luaL_optinteger(L, 8, 255);
    if (g_renderer) {
        bool success = g_renderer->drawTextDirect(text, x, y, size, r, g, b, a);
        lua_pushboolean(L, success);
    } else {
        lua_pushboolean(L, false);
    }
    return 1;
}

void register_lua_functions(lua_State* L, Engine* engine, Renderer* renderer) {
    g_engine = engine;
    g_renderer = renderer;
    
    static const luaL_Reg functions[] = {
        {"setWindowTitle", set_window_title},
        {"getWindowSize", get_window_size},
        {"setWindowSize", set_window_size},
        {"setFullscreen", set_fullscreen},
        {"clearScreen", clear_screen},
        {"print", print_message},
        {"getTime", get_time},
        {"isKeyPressed", is_key_pressed},
        {"loadImage", load_image},
        {"renderImage", render_image},
        {"loadFont", load_font},
        {"drawText", draw_text},
        {"drawTextDirect", draw_text_direct},
        {NULL, NULL}
    };
    luaL_register(L, "_G", functions);
}
