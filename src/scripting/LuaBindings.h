#pragma once
#include <lua.h>

class Engine;
class Renderer;
class AudioManager;

void register_lua_functions(lua_State* L, Engine* engine, Renderer* renderer);
