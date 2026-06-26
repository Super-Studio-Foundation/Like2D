#include "LuaBindings.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include "audio/AudioManager.h"
#include "network/NetworkManager.h"
#include "../platform.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <random>
#include <functional>
#include <cmath>
#include <lua.h>
#include <lualib.h>
#include <box2d/box2d.h>
#include "../../external/nlohmann/json.hpp"
#include "../../external/miniz/miniz.h"

using json = nlohmann::json;

static Engine*           g_engine   = nullptr;
static Renderer*         g_renderer = nullptr;
static AudioManager*     g_audio    = nullptr;
static NetworkManager*   g_network  = nullptr;

static std::vector<b2BodyId>  g_bodyRegistry;
static std::vector<b2ShapeId> g_shapeRegistry;
const float PPM = 32.0f;

// ============================================================
//  WINDOW
// ============================================================
int set_window_title(lua_State* L) {
    const char* title = luaL_checkstring(L, 1);
    if (g_engine) g_engine->setWindowTitle(title);
    return 0;
}
int get_window_size(lua_State* L) {
    int w = 0, h = 0;
    if (g_engine) g_engine->getWindowSize(w, h);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    return 2;
}
int set_window_size(lua_State* L) {
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    if (g_engine) g_engine->setWindowSize(w, h);
    return 0;
}
int set_fullscreen(lua_State* L) {
    bool fs = lua_toboolean(L, 1);
    if (g_engine) g_engine->setFullscreen(fs);
    return 0;
}
int set_escape_quits(lua_State* L) {
    bool v = lua_toboolean(L, 1);
    if (g_engine) g_engine->setEscapeQuits(v);
    return 0;
}
int set_logical_size(lua_State* L) {
    int w = luaL_checkinteger(L, 1);
    int h = luaL_checkinteger(L, 2);
    if (g_engine) g_engine->setLogicalSize(w, h);
    return 0;
}
int get_logical_size(lua_State* L) {
    int w = 0, h = 0;
    if (g_engine) g_engine->getLogicalSize(w, h);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    return 2;
}
int set_use_letterbox(lua_State* L) {
    bool use = lua_toboolean(L, 1);
    if (g_engine) g_engine->setUseLetterbox(use);
    return 0;
}
int set_letterbox_color(lua_State* L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    if (g_engine) g_engine->setLetterboxColor(r, g, b);
    return 0;
}

// ============================================================
//  TIME
// ============================================================
int get_time(lua_State* L) {
    lua_pushinteger(L, SDL_GetTicks());
    return 1;
}
int get_delta_time(lua_State* L) {
    lua_pushnumber(L, g_engine ? g_engine->getDeltaTime() : 0.0f);
    return 1;
}

// ============================================================
//  INPUT — keyboard
// ============================================================
int is_key_down(lua_State* L) {
    const char* k = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_engine && g_engine->isKeyDown(k));
    return 1;
}
int is_key_just_pressed(lua_State* L) {
    const char* k = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_engine && g_engine->isKeyJustPressed(k));
    return 1;
}
int is_key_just_released(lua_State* L) {
    const char* k = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_engine && g_engine->isKeyJustReleased(k));
    return 1;
}

// ============================================================
//  INPUT — mouse
// ============================================================
int is_mouse_pressed(lua_State* L) {
    int button = luaL_checkinteger(L, 1); // 0=left,1=right,2=middle
    Uint32 state = SDL_GetMouseState(NULL, NULL);
    bool pressed = false;
    if      (button == 0) pressed = (state & SDL_BUTTON_LMASK) != 0;
    else if (button == 1) pressed = (state & SDL_BUTTON_RMASK) != 0;
    else if (button == 2) pressed = (state & SDL_BUTTON_MMASK) != 0;
    lua_pushboolean(L, pressed);
    return 1;
}
int get_mouse_pos(lua_State* L) {
    float x, y;
    SDL_GetMouseState(&x, &y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}
int get_mouse_scroll(lua_State* L) {
    float x = 0, y = 0;
    if (g_engine) g_engine->getMouseScroll(x, y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

// ============================================================
//  RENDERING — screen / images
// ============================================================
int clear_screen(lua_State* L) {
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    if (g_renderer) { g_renderer->setDrawColor(r, g, b); g_renderer->clear(); }
    return 0;
}
int load_image(lua_State* L) {
    const char* f = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_renderer && g_renderer->loadImage(f));
    return 1;
}
int render_image(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);
    lua_pushboolean(L, g_renderer && g_renderer->renderImage(key, x, y, w, h));
    return 1;
}
// renderImageEx(key, x, y, w, h, angle, flipH, flipV, alpha, originX, originY)
// angle in degrees, alpha 0-1, origin 0-1 (0.5,0.5 = center)
int render_image_ex(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    float x  = (float)luaL_checknumber(L, 2);
    float y  = (float)luaL_checknumber(L, 3);
    float w  = (float)luaL_checknumber(L, 4);
    float h  = (float)luaL_checknumber(L, 5);
    float angle = (float)luaL_optnumber(L, 6, 0.0);
    bool flipH  = lua_toboolean(L, 7);
    bool flipV  = lua_toboolean(L, 8);
    float alpha = (float)luaL_optnumber(L, 9, 1.0);
    float ox    = (float)luaL_optnumber(L, 10, 0.5);
    float oy    = (float)luaL_optnumber(L, 11, 0.5);
    lua_pushboolean(L, g_renderer &&
        g_renderer->renderImageEx(key, x, y, w, h, angle, flipH, flipV, alpha, ox, oy));
    return 1;
}

// renderImageRegion(key, dx,dy,dw,dh, srcX,srcY,srcW,srcH, angle,flipH,flipV,alpha,ox,oy)
// Renders a sub-rectangle of a sprite sheet texture.
// dx/dy/dw/dh = destination on screen
// srcX/srcY/srcW/srcH = source rectangle in the texture (pixels)
// angle/flipH/flipV/alpha/ox/oy = same as renderImageEx
int render_image_region(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    float dx  = (float)luaL_checknumber(L, 2);
    float dy  = (float)luaL_checknumber(L, 3);
    float dw  = (float)luaL_checknumber(L, 4);
    float dh  = (float)luaL_checknumber(L, 5);
    float sx  = (float)luaL_checknumber(L, 6);
    float sy  = (float)luaL_checknumber(L, 7);
    float sw  = (float)luaL_checknumber(L, 8);
    float sh  = (float)luaL_checknumber(L, 9);
    float angle = (float)luaL_optnumber(L, 10, 0.0);
    bool flipH  = lua_toboolean(L, 11);
    bool flipV  = lua_toboolean(L, 12);
    float alpha = (float)luaL_optnumber(L, 13, 1.0);
    float ox    = (float)luaL_optnumber(L, 14, 0.5);
    float oy    = (float)luaL_optnumber(L, 15, 0.5);
    lua_pushboolean(L, g_renderer &&
        g_renderer->renderImageRegion(key, dx, dy, dw, dh, sx, sy, sw, sh,
                                       angle, flipH, flipV, alpha, ox, oy));
    return 1;
}

// getImageSize(key) -> width, height  (returns 0,0 if not loaded)
int get_image_size(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    int w = 0, h = 0;
    if (g_renderer) g_renderer->getImageSize(key, w, h);
    lua_pushinteger(L, w);
    lua_pushinteger(L, h);
    return 2;
}

// ============================================================
//  RENDERING — primitives
// ============================================================
int draw_rect(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float w = (float)luaL_checknumber(L, 3);
    float h = (float)luaL_checknumber(L, 4);
    int r   = luaL_checkinteger(L, 5);
    int g   = luaL_checkinteger(L, 6);
    int b   = luaL_checkinteger(L, 7);
    int a   = (int)luaL_optinteger(L, 8, 255);
    bool filled = lua_toboolean(L, 9);
    float angle = (float)luaL_optnumber(L, 10, 0.0);
    if (g_renderer) g_renderer->drawRectEx(x, y, w, h, r, g, b, a, filled, angle);
    return 0;
}
int draw_line(lua_State* L) {
    float x1 = (float)luaL_checknumber(L, 1);
    float y1 = (float)luaL_checknumber(L, 2);
    float x2 = (float)luaL_checknumber(L, 3);
    float y2 = (float)luaL_checknumber(L, 4);
    int r = luaL_checkinteger(L, 5);
    int g = luaL_checkinteger(L, 6);
    int b = luaL_checkinteger(L, 7);
    int a = (int)luaL_optinteger(L, 8, 255);
    if (g_renderer) g_renderer->drawLine(x1, y1, x2, y2, r, g, b, a);
    return 0;
}
int draw_circle(lua_State* L) {
    float cx = (float)luaL_checknumber(L, 1);
    float cy = (float)luaL_checknumber(L, 2);
    float rad = (float)luaL_checknumber(L, 3);
    int r = luaL_checkinteger(L, 4);
    int g = luaL_checkinteger(L, 5);
    int b = luaL_checkinteger(L, 6);
    int a = (int)luaL_optinteger(L, 7, 255);
    bool filled = lua_toboolean(L, 8);
    if (g_renderer) g_renderer->drawCircle(cx, cy, rad, r, g, b, a, filled);
    return 0;
}

// ============================================================
//  RENDERING — text
// ============================================================
int load_font(lua_State* L) {
    const char* f = luaL_checkstring(L, 1);
    int size = luaL_checkinteger(L, 2);
    lua_pushboolean(L, g_renderer && g_renderer->loadFont(f, size));
    return 1;
}
int draw_text(lua_State* L) {
    const char* text    = luaL_checkstring(L, 1);
    const char* fontKey = luaL_checkstring(L, 2);
    int x = luaL_checkinteger(L, 3);
    int y = luaL_checkinteger(L, 4);
    int r = luaL_checkinteger(L, 5);
    int g = luaL_checkinteger(L, 6);
    int b = luaL_checkinteger(L, 7);
    lua_pushboolean(L, g_renderer && g_renderer->drawText(text, fontKey, x, y, r, g, b));
    return 1;
}
int draw_text_direct(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int x    = luaL_checkinteger(L, 2);
    int y    = luaL_checkinteger(L, 3);
    int size = luaL_checkinteger(L, 4);
    int r    = luaL_checkinteger(L, 5);
    int g    = luaL_checkinteger(L, 6);
    int b    = luaL_checkinteger(L, 7);
    int a    = (int)luaL_optinteger(L, 8, 255);
    int anchor = (int)luaL_optinteger(L, 9, 0);
    lua_pushboolean(L, g_renderer && g_renderer->drawTextDirect(text, x, y, size, r, g, b, a, anchor));
    return 1;
}
int get_text_size(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int size = luaL_checkinteger(L, 2);
    int w = 0, h = 0;
    if (g_renderer && g_renderer->getTextSize(text, size, w, h)) {
        lua_pushinteger(L, w);
        lua_pushinteger(L, h);
        return 2;
    }
    lua_pushinteger(L, 0);
    lua_pushinteger(L, 0);
    return 2;
}
int set_default_font(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    if (g_renderer) g_renderer->setDefaultFont(path);
    return 0;
}

// ============================================================
//  CAMERA
// ============================================================
int set_camera_position(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    if (g_engine) g_engine->setCameraPosition(x, y);
    return 0;
}
int get_camera_position(lua_State* L) {
    float x = 0, y = 0;
    if (g_engine) g_engine->getCameraPosition(x, y);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}
int set_camera_zoom(lua_State* L) {
    float z = (float)luaL_checknumber(L, 1);
    if (g_engine) g_engine->setCameraZoom(z);
    return 0;
}
int get_camera_zoom(lua_State* L) {
    lua_pushnumber(L, g_engine ? g_engine->getCameraZoom() : 1.0f);
    return 1;
}
int world_to_screen(lua_State* L) {
    float wx = (float)luaL_checknumber(L, 1);
    float wy = (float)luaL_checknumber(L, 2);
    float sx = 0, sy = 0;
    if (g_engine) g_engine->worldToScreen(wx, wy, sx, sy);
    lua_pushnumber(L, sx);
    lua_pushnumber(L, sy);
    return 2;
}

// ============================================================
//  PHYSICS — bodies
// ============================================================
int create_body(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    // type: "dynamic", "static", "kinematic"
    const char* typeStr = luaL_optstring(L, 3, "dynamic");
    bool fixedRotation  = lua_toboolean(L, 4);

    if (!g_engine) { lua_pushinteger(L, -1); return 1; }

    b2BodyDef def = b2DefaultBodyDef();
    def.position  = (b2Vec2){x / PPM, y / PPM};
    if      (strcmp(typeStr, "static")    == 0) def.type = b2_staticBody;
    else if (strcmp(typeStr, "kinematic") == 0) def.type = b2_kinematicBody;
    else                                         def.type = b2_dynamicBody;
    def.fixedRotation = fixedRotation;

    b2BodyId id = b2CreateBody(g_engine->getWorldId(), &def);
    g_bodyRegistry.push_back(id);

    // Flag engine to enable physics stepping when dynamic bodies exist
    if (def.type == b2_dynamicBody && g_engine)
        g_engine->setHasDynamicBodies(true);

    lua_pushinteger(L, (int)g_bodyRegistry.size() - 1);
    return 1;
}
int destroy_body(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushboolean(L, false); return 1; }
    b2BodyId id = g_bodyRegistry[idx];
    if (b2Body_IsValid(id)) b2DestroyBody(id);
    g_bodyRegistry[idx] = {0};
    lua_pushboolean(L, true);
    return 1;
}
int set_body_position(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    if (idx >= 0 && idx < (int)g_bodyRegistry.size()) {
        b2BodyId id = g_bodyRegistry[idx];
        if (b2Body_IsValid(id)) b2Body_SetTransform(id, (b2Vec2){x/PPM, y/PPM}, b2Body_GetRotation(id));
    }
    return 0;
}
int get_body_position(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushnil(L); return 1; }
    b2BodyId id = g_bodyRegistry[idx];
    if (!b2Body_IsValid(id)) { lua_pushnil(L); return 1; }
    b2Vec2 pos = b2Body_GetPosition(id);
    lua_pushnumber(L, pos.x * PPM);
    lua_pushnumber(L, pos.y * PPM);
    return 2;
}
int get_physics_transform(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushnil(L); return 1; }
    b2BodyId id = g_bodyRegistry[idx];
    if (!b2Body_IsValid(id)) { lua_pushnil(L); return 1; }
    b2Vec2 pos   = b2Body_GetPosition(id);
    float  angle = b2Rot_GetAngle(b2Body_GetRotation(id));
    lua_newtable(L);
    lua_pushnumber(L, pos.x * PPM); lua_setfield(L, -2, "x");
    lua_pushnumber(L, pos.y * PPM); lua_setfield(L, -2, "y");
    lua_pushnumber(L, angle * (180.0f / 3.14159265f)); lua_setfield(L, -2, "angle");
    return 1;
}
int set_body_velocity(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    float vx = (float)luaL_checknumber(L, 2);
    float vy = (float)luaL_checknumber(L, 3);
    if (idx >= 0 && idx < (int)g_bodyRegistry.size()) {
        b2BodyId id = g_bodyRegistry[idx];
        if (b2Body_IsValid(id)) b2Body_SetLinearVelocity(id, (b2Vec2){vx / PPM, vy / PPM});
    }
    return 0;
}
int get_body_velocity(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushnumber(L,0); lua_pushnumber(L,0); return 2; }
    b2BodyId id = g_bodyRegistry[idx];
    if (!b2Body_IsValid(id)) { lua_pushnumber(L,0); lua_pushnumber(L,0); return 2; }
    b2Vec2 v = b2Body_GetLinearVelocity(id);
    // Convert back to pixel units for Lua
    lua_pushnumber(L, v.x * PPM);
    lua_pushnumber(L, v.y * PPM);
    return 2;
}
int apply_force(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    float fx = (float)luaL_checknumber(L, 2);
    float fy = (float)luaL_checknumber(L, 3);
    if (idx >= 0 && idx < (int)g_bodyRegistry.size()) {
        b2BodyId id = g_bodyRegistry[idx];
        // Force in Box2D is in N (kg·m/s²). Convert pixel-space force → world-space.
        if (b2Body_IsValid(id)) b2Body_ApplyForceToCenter(id, (b2Vec2){fx / PPM, fy / PPM}, true);
    }
    return 0;
}
int apply_impulse(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    float ix = (float)luaL_checknumber(L, 2);
    float iy = (float)luaL_checknumber(L, 3);
    if (idx >= 0 && idx < (int)g_bodyRegistry.size()) {
        b2BodyId id = g_bodyRegistry[idx];
        // Impulse in Box2D is in kg·m/s. Convert pixel-space impulse → world-space.
        if (b2Body_IsValid(id)) b2Body_ApplyLinearImpulseToCenter(id, (b2Vec2){ix / PPM, iy / PPM}, true);
    }
    return 0;
}
int set_gravity(lua_State* L) {
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    // Gravity from Lua is in pixels/s² — convert to m/s² for Box2D
    if (g_engine) g_engine->setGravity(x / PPM, y / PPM);
    return 0;
}

// ============================================================
//  PHYSICS — shapes
// ============================================================
int add_box_shape(lua_State* L) {
    int idx = luaL_checkinteger(L, 1);
    float hx      = (float)luaL_checknumber(L, 2);
    float hy      = (float)luaL_checknumber(L, 3);
    float density = (float)luaL_optnumber(L, 4, 1.0);
    float friction = (float)luaL_optnumber(L, 5, 0.3);
    float restitution = (float)luaL_optnumber(L, 6, 0.0);
    bool  isSensor = lua_toboolean(L, 7);

    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushinteger(L, -1); return 1; }
    b2BodyId bodyId = g_bodyRegistry[idx];
    if (!b2Body_IsValid(bodyId)) { lua_pushinteger(L, -1); return 1; }

    b2Polygon box = b2MakeBox(hx / PPM, hy / PPM);
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density              = density;
    sd.material.friction    = friction;
    sd.material.restitution = restitution;
    sd.isSensor             = isSensor;

    b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &sd, &box);
    g_shapeRegistry.push_back(shapeId);
    lua_pushinteger(L, (int)g_shapeRegistry.size() - 1);
    return 1;
}
int add_circle_shape(lua_State* L) {
    int idx    = luaL_checkinteger(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    float density = (float)luaL_optnumber(L, 3, 1.0);
    float friction = (float)luaL_optnumber(L, 4, 0.3);
    float restitution = (float)luaL_optnumber(L, 5, 0.0);
    bool  isSensor = lua_toboolean(L, 6);

    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushinteger(L, -1); return 1; }
    b2BodyId bodyId = g_bodyRegistry[idx];
    if (!b2Body_IsValid(bodyId)) { lua_pushinteger(L, -1); return 1; }

    b2Circle circle;
    circle.center = (b2Vec2){0.0f, 0.0f};
    circle.radius = radius / PPM;

    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density              = density;
    sd.material.friction    = friction;
    sd.material.restitution = restitution;
    sd.isSensor             = isSensor;

    b2ShapeId shapeId = b2CreateCircleShape(bodyId, &sd, &circle);
    g_shapeRegistry.push_back(shapeId);
    lua_pushinteger(L, (int)g_shapeRegistry.size() - 1);
    return 1;
}
int add_capsule_shape(lua_State* L) {
    int idx    = luaL_checkinteger(L, 1);
    float halfH  = (float)luaL_checknumber(L, 2); // half-height of the middle segment
    float radius = (float)luaL_checknumber(L, 3);
    float density = (float)luaL_optnumber(L, 4, 1.0);
    float friction = (float)luaL_optnumber(L, 5, 0.3);

    if (idx < 0 || idx >= (int)g_bodyRegistry.size()) { lua_pushinteger(L, -1); return 1; }
    b2BodyId bodyId = g_bodyRegistry[idx];
    if (!b2Body_IsValid(bodyId)) { lua_pushinteger(L, -1); return 1; }

    b2Capsule capsule;
    capsule.center1 = (b2Vec2){0.0f, -halfH / PPM};
    capsule.center2 = (b2Vec2){0.0f,  halfH / PPM};
    capsule.radius  = radius / PPM;

    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density              = density;
    sd.material.friction    = friction;

    b2ShapeId shapeId = b2CreateCapsuleShape(bodyId, &sd, &capsule);
    g_shapeRegistry.push_back(shapeId);
    lua_pushinteger(L, (int)g_shapeRegistry.size() - 1);
    return 1;
}

// ============================================================
//  AUDIO
// ============================================================
int load_sound(lua_State* L) {
    const char* key  = luaL_checkstring(L, 1);
    const char* file = luaL_checkstring(L, 2);
    lua_pushboolean(L, g_audio && g_audio->loadSound(key, file));
    return 1;
}
int play_sound(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    bool loop = lua_toboolean(L, 2);
    lua_pushboolean(L, g_audio && g_audio->playSound(key, loop));
    return 1;
}
int stop_sound(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_audio && g_audio->stopSound(key));
    return 1;
}
int pause_sound(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_audio && g_audio->pauseSound(key));
    return 1;
}
int resume_sound(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_audio && g_audio->resumeSound(key));
    return 1;
}
int is_sound_playing(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_audio && g_audio->isSoundPlaying(key));
    return 1;
}
int set_sound_volume(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    float vol = (float)luaL_checknumber(L, 2);
    if (g_audio) g_audio->setSoundVolume(key, vol);
    return 0;
}
int set_master_volume(lua_State* L) {
    float vol = (float)luaL_checknumber(L, 1);
    if (g_audio) g_audio->setMasterVolume(vol);
    return 0;
}
int set_sound_pitch(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    float pitch = (float)luaL_checknumber(L, 2);
    if (g_audio) g_audio->setSoundPitch(key, pitch);
    return 0;
}
int unload_sound(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    if (g_audio) g_audio->unloadSound(key);
    return 0;
}

// ============================================================
//  JSON
// ============================================================

// Forward declarations
static void jsonToLua(lua_State* L, const json& j);
static json luaToJson(lua_State* L, int idx);

static void jsonToLua(lua_State* L, const json& j) {
    if (j.is_null()) {
        lua_pushnil(L);
    } else if (j.is_boolean()) {
        lua_pushboolean(L, j.get<bool>());
    } else if (j.is_number_integer()) {
        lua_pushinteger(L, j.get<long long>());
    } else if (j.is_number_float()) {
        lua_pushnumber(L, j.get<double>());
    } else if (j.is_string()) {
        lua_pushstring(L, j.get<std::string>().c_str());
    } else if (j.is_array()) {
        lua_newtable(L);
        int i = 1;
        for (const auto& elem : j) {
            jsonToLua(L, elem);
            lua_rawseti(L, -2, i++);
        }
    } else if (j.is_object()) {
        lua_newtable(L);
        for (auto& [key, val] : j.items()) {
            lua_pushstring(L, key.c_str());
            jsonToLua(L, val);
            lua_rawset(L, -3);
        }
    } else {
        lua_pushnil(L);
    }
}

static json luaToJson(lua_State* L, int idx) {
    if (idx < 0) idx = lua_gettop(L) + idx + 1;
    int t = lua_type(L, idx);
    if (t == LUA_TNIL || t == LUA_TNONE) {
        return nullptr;
    } else if (t == LUA_TBOOLEAN) {
        return (bool)lua_toboolean(L, idx);
    } else if (t == LUA_TNUMBER) {
        // Luau: check if the number has no fractional part
        double v = lua_tonumber(L, idx);
        if (v == (long long)v) return (long long)v;
        return v;
    } else if (t == LUA_TSTRING) {
        return std::string(lua_tostring(L, idx));
    } else if (t == LUA_TTABLE) {
        // Detect array vs object: check if key 1 exists
        lua_rawgeti(L, idx, 1);
        bool isArray = !lua_isnil(L, -1);
        lua_pop(L, 1);

        if (isArray) {
            json arr = json::array();
            // lua_objlen is the Luau equivalent of lua_rawlen
            int n = (int)lua_objlen(L, idx);
            for (int i = 1; i <= n; i++) {
                lua_rawgeti(L, idx, i);
                arr.push_back(luaToJson(L, -1));
                lua_pop(L, 1);
            }
            return arr;
        } else {
            json obj = json::object();
            lua_pushnil(L);
            while (lua_next(L, idx) != 0) {
                std::string key;
                if (lua_type(L, -2) == LUA_TSTRING) key = lua_tostring(L, -2);
                else if (lua_type(L, -2) == LUA_TNUMBER) key = std::to_string((int)lua_tonumber(L, -2));
                obj[key] = luaToJson(L, -1);
                lua_pop(L, 1);
            }
            return obj;
        }
    }
    return nullptr;
}

// ── Encryption constants (must match compiler.cpp) ───────────
static const uint8_t LIKE2D_XOR_KEY[] = {
    0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
    0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
};
static const size_t  LIKE2D_XOR_KEY_LEN = sizeof(LIKE2D_XOR_KEY);
static const uint8_t LIKE2D_ENC_MAGIC[] = { 'L', '2', 'D', 'E' };

// ── Universal encryption helpers ────────────────────────────
static std::vector<char> xorTransform(const std::vector<char>& input) {
    std::vector<char> output = input;
    for (size_t i = 0; i < output.size(); i++) {
        output[i] ^= (char)LIKE2D_XOR_KEY[i % LIKE2D_XOR_KEY_LEN];
    }
    return output;
}

static bool hasEncryptionHeader(const std::vector<char>& data) {
    if (data.size() < 4) return false;
    return (uint8_t)data[0] == LIKE2D_ENC_MAGIC[0] &&
           (uint8_t)data[1] == LIKE2D_ENC_MAGIC[1] &&
           (uint8_t)data[2] == LIKE2D_ENC_MAGIC[2] &&
           (uint8_t)data[3] == LIKE2D_ENC_MAGIC[3];
}

// Returns true if the file starts with the L2DE magic header
static bool isEncryptedGameLike(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    uint8_t hdr[4] = {};
    f.read(reinterpret_cast<char*>(hdr), 4);
    return f.gcount() == 4 &&
           hdr[0] == LIKE2D_ENC_MAGIC[0] && hdr[1] == LIKE2D_ENC_MAGIC[1] &&
           hdr[2] == LIKE2D_ENC_MAGIC[2] && hdr[3] == LIKE2D_ENC_MAGIC[3];
}

// Reads game.like, decrypts if needed, returns raw zip bytes in outZipData
static bool readGameLikeBytes(const std::filesystem::path& path, std::vector<char>& outZipData) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::vector<char> raw((std::istreambuf_iterator<char>(f)), {});
    f.close();

    if (raw.size() >= 4 &&
        (uint8_t)raw[0] == LIKE2D_ENC_MAGIC[0] &&
        (uint8_t)raw[1] == LIKE2D_ENC_MAGIC[1] &&
        (uint8_t)raw[2] == LIKE2D_ENC_MAGIC[2] &&
        (uint8_t)raw[3] == LIKE2D_ENC_MAGIC[3])
    {
        // Encrypted: skip 4-byte magic, XOR-decrypt the rest
        outZipData.assign(raw.begin() + 4, raw.end());
        for (size_t i = 0; i < outZipData.size(); i++)
            outZipData[i] ^= (char)LIKE2D_XOR_KEY[i % LIKE2D_XOR_KEY_LEN];
    } else {
        // Plain zip
        outZipData = std::move(raw);
    }
    return true;
}

// ── Global caches for performance ──
static std::vector<char> s_gameLikeZipCache;   // cached decrypted game.like data
static std::string       s_gameLikeZipPath;    // path of cached archive
static std::unordered_map<std::string, std::string> s_fileReadCache;  // filename -> content

// Resolve a filename to full path (dev mode) or extract from game.like (production)
static std::string resolveFileRead(const std::string& filename, std::string& outContent, bool& fromArchive) {
    fromArchive = false;

    // Check file read cache first (avoids all I/O)
    auto cacheIt = s_fileReadCache.find(filename);
    if (cacheIt != s_fileReadCache.end()) {
        outContent = cacheIt->second;
        return "";
    }

    std::filesystem::path exePath     = getExeDir();
    std::filesystem::path selfPath    = getExePath();
    std::filesystem::path gameLikePath = exePath / "game.like";

    // Check if the binary itself contains a valid ZIP at the end (fused mode)
    bool isFused = false;
    std::string selfPathStr = selfPath.string();
    if (s_gameLikeZipPath == selfPathStr && !s_gameLikeZipCache.empty()) {
        isFused = true;
    } else {
        std::vector<char> zipData;
        if (readGameLikeBytes(selfPath, zipData)) {
            mz_zip_archive zip_test = {};
            if (mz_zip_reader_init_mem(&zip_test, zipData.data(), zipData.size(), 0)) {
                mz_zip_reader_end(&zip_test);
                s_gameLikeZipCache = std::move(zipData);
                s_gameLikeZipPath = selfPathStr;
                isFused = true;
            }
        }
    }

    // If not fused, try external game.like (with cache)
    if (!isFused) {
        std::string glPathStr = gameLikePath.string();
        if (s_gameLikeZipPath != glPathStr || s_gameLikeZipCache.empty()) {
            if (std::filesystem::exists(gameLikePath)) {
                std::vector<char> zipData;
                if (!readGameLikeBytes(gameLikePath, zipData))
                    return "Failed to read game.like";
                s_gameLikeZipCache = std::move(zipData);
                s_gameLikeZipPath = glPathStr;
            } else {
                goto dev_mode;
            }
        }
    }

    // Load from cached zip data
    if (!s_gameLikeZipCache.empty()) {
        mz_zip_archive zip = {};
        if (!mz_zip_reader_init_mem(&zip, s_gameLikeZipCache.data(), s_gameLikeZipCache.size(), 0))
            return "Failed to open game archive (invalid format or encryption)";

        std::string norm = filename;
        for (char& c : norm) if (c == '\\') c = '/';

        std::vector<std::string> paths = { norm };
        if (norm.find('/') == std::string::npos) {
            paths.push_back("data/"   + norm);
            paths.push_back("assets/" + norm);
        } else {
            paths.push_back(norm.substr(norm.rfind('/') + 1));
        }

        for (const auto& p : paths) {
            int idx = mz_zip_reader_locate_file(&zip, p.c_str(), nullptr, 0);
            if (idx >= 0) {
                mz_zip_archive_file_stat stat;
                mz_zip_reader_file_stat(&zip, idx, &stat);
                std::vector<char> buf(stat.m_uncomp_size + 1, 0);
                mz_zip_reader_extract_to_mem(&zip, idx, buf.data(), stat.m_uncomp_size, 0);
                outContent = std::string(buf.data(), stat.m_uncomp_size);
                mz_zip_reader_end(&zip);
                fromArchive = true;
                s_fileReadCache[filename] = outContent;
                return "";
            }
        }
        mz_zip_reader_end(&zip);
        return "File not found in game.like: " + filename;
    }

    dev_mode:
    // Dev mode: read from disk (with cache)
    if (g_renderer) {
        std::filesystem::path full = std::filesystem::path(g_renderer->getProjectFolder()) / filename;
        std::ifstream f(full);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf();
            outContent = ss.str();
            s_fileReadCache[filename] = outContent;
            return "";
        }
    }
    {
        std::filesystem::path exeFull = getExeDir() / std::filesystem::path(filename).filename();
        std::ifstream f(exeFull);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf();
            outContent = ss.str();
            s_fileReadCache[filename] = outContent;
            return "";
        }
    }
    std::ifstream f(filename);
    if (f.is_open()) {
        std::ostringstream ss; ss << f.rdbuf();
        outContent = ss.str();
        s_fileReadCache[filename] = outContent;
        return "";
    }
    return "File not found: " + filename;
}

// json.load(filename) -> table, errmsg
int json_load(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    std::string content;
    bool fromArchive = false;
    std::string err = resolveFileRead(filename, content, fromArchive);
    if (!err.empty()) {
        lua_pushnil(L);
        lua_pushstring(L, err.c_str());
        return 2;
    }
    try {
        json j = json::parse(content);
        jsonToLua(L, j);
        lua_pushnil(L);  // no error
        return 2;
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }
}

// json.save(filename, table) -> ok, errmsg
int json_save(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    json j = luaToJson(L, 2);

    std::filesystem::path savePath;

    // In production mode (game.like exists), save next to the executable
    // In dev mode, save relative to the project folder
    std::filesystem::path exeDir      = getExeDir();
    std::filesystem::path gameLikePath = exeDir / "game.like";
    bool productionMode = std::filesystem::exists(gameLikePath);

    if (productionMode) {
        // Save next to the exe so it's always writable
        savePath = exeDir / std::filesystem::path(filename).filename();
    } else if (g_renderer && !std::filesystem::path(filename).is_absolute()) {
        savePath = std::filesystem::path(g_renderer->getProjectFolder()) / filename;
    } else {
        savePath = filename;
    }

    // Create parent directories if needed
    std::error_code ec;
    std::filesystem::create_directories(savePath.parent_path(), ec);

    std::ofstream f(savePath);
    if (!f.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot write file: " + savePath.string()).c_str());
        return 2;
    }
    f << j.dump(2);
    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// json.encode(table) -> string
int json_encode(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    try {
        json j = luaToJson(L, 1);
        lua_pushstring(L, j.dump(2).c_str());
    } catch (const std::exception& e) {
        lua_pushnil(L);
    }
    return 1;
}

// json.decode(string) -> table, errmsg
int json_decode(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    try {
        json j = json::parse(str);
        jsonToLua(L, j);
        lua_pushnil(L);
        return 2;
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }
}

// ============================================================
//  VIDEO
// ============================================================
int load_video(lua_State* L) {
    const char* key  = luaL_checkstring(L, 1);
    const char* file = luaL_checkstring(L, 2);
    lua_pushboolean(L, g_renderer && g_renderer->loadVideo(key, file));
    return 1;
}
int update_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    double dt = luaL_checknumber(L, 2);
    if (g_renderer) g_renderer->updateVideo(key, dt);
    return 0;
}
int render_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float w = (float)luaL_checknumber(L, 4);
    float h = (float)luaL_checknumber(L, 5);
    lua_pushboolean(L, g_renderer && g_renderer->renderVideo(key, x, y, w, h));
    return 1;
}
int play_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    bool loop = lua_toboolean(L, 2);
    if (g_renderer) g_renderer->playVideo(key, loop);
    return 0;
}
int pause_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    if (g_renderer) g_renderer->pauseVideo(key);
    return 0;
}
int stop_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    if (g_renderer) g_renderer->stopVideo(key);
    return 0;
}
int seek_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    double secs = luaL_checknumber(L, 2);
    if (g_renderer) g_renderer->seekVideo(key, secs);
    return 0;
}
int is_video_playing(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_renderer && g_renderer->isVideoPlaying(key));
    return 1;
}
int is_video_ended(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushboolean(L, g_renderer && g_renderer->isVideoEnded(key));
    return 1;
}
int get_video_duration(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushnumber(L, g_renderer ? g_renderer->getVideoDuration(key) : 0.0);
    return 1;
}
int get_video_time(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    lua_pushnumber(L, g_renderer ? g_renderer->getVideoTime(key) : 0.0);
    return 1;
}
int unload_video(lua_State* L) {
    const char* key = luaL_checkstring(L, 1);
    if (g_renderer) g_renderer->unloadVideo(key);
    return 0;
}

// ============================================================
//  MISC
// ============================================================
int print_message(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    std::cout << "Lua: " << msg << std::endl;
    return 0;
}

// quit() — cleanly close the engine window
int lua_quit(lua_State* L) {
    if (g_engine) {
        // Trigger SDL quit event so the main loop exits cleanly
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
    }
    return 0;
}

// ============================================================
//  FILE SYSTEM
// ============================================================

// fileExists(path) -> bool
static std::unordered_map<std::string, bool> s_fileExistsCache;
int lua_file_exists(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    // Check cache first
    auto it = s_fileExistsCache.find(path);
    if (it != s_fileExistsCache.end()) {
        lua_pushboolean(L, it->second);
        return 1;
    }

    std::filesystem::path fullPath;
    bool exists = false;

    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / path;
        exists = std::filesystem::exists(fullPath);
    }
    if (!exists) {
        fullPath = getExeDir() / path;
        exists = std::filesystem::exists(fullPath);
    }

    s_fileExistsCache[path] = exists;
    lua_pushboolean(L, exists);
    return 1;
}

// readFile(path) -> string content, string error
// Read text/binary file from disk (with cache for non-userdata files)
int lua_read_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    // Check file read cache (shared with resolveFileRead)
    auto cacheIt = s_fileReadCache.find(path);
    if (cacheIt != s_fileReadCache.end()) {
        lua_pushstring(L, cacheIt->second.c_str());
        lua_pushnil(L);
        return 2;
    }

    std::filesystem::path fullPath;
    bool isUserdata = false;

    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / path;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / "userdata" / path;
        isUserdata = true;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / path;
        isUserdata = false;
    }

    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        lua_pushnil(L);
        lua_pushstring(L, ("Cannot open file: " + fullPath.string()).c_str());
        return 2;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    // Cache non-userdata files (project assets don't change at runtime)
    if (!isUserdata)
        s_fileReadCache[path] = content;

    lua_pushstring(L, content.c_str());
    lua_pushnil(L);  // no error
    return 2;
}

// writeFile(path, content) -> bool success, string error
// Write file to userdata folder (writable)
int lua_write_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* content = luaL_checkstring(L, 2);

    // Write to userdata folder (separate from game files)
    std::filesystem::path fullPath = getExeDir() / "userdata" / path;

    // Create parent folders if needed
    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);

    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot write to: " + fullPath.string()).c_str());
        return 2;
    }

    file.write(content, strlen(content));
    file.close();

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// listDirectory(path) -> table of filenames
// List all files/folders in a directory
int lua_list_directory(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    std::filesystem::path fullPath;

    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / path;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / "userdata" / path;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / path;
    }

    lua_newtable(L);
    int idx = 1;

    if (std::filesystem::exists(fullPath) && std::filesystem::is_directory(fullPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
            lua_pushstring(L, entry.path().filename().string().c_str());
            lua_rawseti(L, -2, idx++);
        }
    }

    return 1;
}

// createDirectory(path) -> bool success, string error
// Create a new folder inside userdata
int lua_create_directory(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    std::filesystem::path fullPath = getExeDir() / "userdata" / path;

    std::error_code ec;
    bool success = std::filesystem::create_directories(fullPath, ec);

    lua_pushboolean(L, success);
    if (!success) {
        lua_pushstring(L, ec.message().c_str());
    } else {
        lua_pushnil(L);
    }
    return 2;
}

// deleteFile(path) -> bool success, string error
// Delete a file inside userdata
int lua_delete_file(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    std::filesystem::path fullPath = getExeDir() / "userdata" / path;

    std::error_code ec;
    bool success = std::filesystem::remove(fullPath, ec);

    lua_pushboolean(L, success);
    if (!success) {
        lua_pushstring(L, ec.message().c_str());
    } else {
        lua_pushnil(L);
    }
    return 2;
}

// getUserdataPath() -> string
// Returns the path to userdata folder (where saves/configs are stored)
int lua_get_userdata_path(lua_State* L) {
    lua_pushstring(L, (getExeDir() / "userdata").string().c_str());
    return 1;
}

// getGamePath() -> string
// Returns the path where the game executable is located
int lua_get_game_path(lua_State* L) {
    lua_pushstring(L, getExeDir().string().c_str());
    return 1;
}

// ============================================================
//  ENCRYPTION API
// ============================================================

// encryptFile(inputPath, inPlace) -> bool success, string error
// If inPlace=true, overwrites the original file with encrypted data.
// If inPlace=false, creates inputPath.enc
int lua_encrypt_file(lua_State* L) {
    const char* inputPath = luaL_checkstring(L, 1);
    bool inPlace = lua_toboolean(L, 2);

    // Resolve full path (check userdata folder where writeFile saves)
    std::filesystem::path fullPath;
    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / "userdata" / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = inputPath; // try as-is
    }

    std::ifstream inFile(fullPath, std::ios::binary);
    if (!inFile.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot open file: " + fullPath.string()).c_str());
        return 2;
    }
    std::vector<char> raw((std::istreambuf_iterator<char>(inFile)), {});
    inFile.close();

    // Don't double-encrypt
    if (hasEncryptionHeader(raw)) {
        lua_pushboolean(L, true); // already encrypted
        lua_pushnil(L);
        return 2;
    }

    // Encrypt
    std::vector<char> encrypted = xorTransform(raw);

    // Determine output path
    std::filesystem::path outPath = inPlace ? fullPath : std::filesystem::path(fullPath.string() + ".enc");

    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot write file: " + outPath.string()).c_str());
        return 2;
    }
    outFile.write(reinterpret_cast<const char*>(LIKE2D_ENC_MAGIC), sizeof(LIKE2D_ENC_MAGIC));
    outFile.write(encrypted.data(), (std::streamsize)encrypted.size());
    outFile.close();

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// decryptFile(inputPath) -> bool success, string error
// Decrypts an encrypted file and writes the result to inputPath (removes encryption).
int lua_decrypt_file(lua_State* L) {
    const char* inputPath = luaL_checkstring(L, 1);

    std::filesystem::path fullPath;
    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / "userdata" / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = inputPath;
    }

    std::ifstream inFile(fullPath, std::ios::binary);
    if (!inFile.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot open file: " + fullPath.string()).c_str());
        return 2;
    }
    std::vector<char> raw((std::istreambuf_iterator<char>(inFile)), {});
    inFile.close();

    if (!hasEncryptionHeader(raw)) {
        lua_pushboolean(L, false);
        lua_pushstring(L, "File is not encrypted (no L2DE header)");
        return 2;
    }

    // Skip 4-byte magic, XOR-decrypt the rest
    std::vector<char> payload(raw.begin() + 4, raw.end());
    std::vector<char> decrypted = xorTransform(payload);

    // Overwrite original file with decrypted data
    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot write file: " + fullPath.string()).c_str());
        return 2;
    }
    outFile.write(decrypted.data(), (std::streamsize)decrypted.size());
    outFile.close();

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// readEncryptedFile(path) -> string content, string error
// Reads and decrypts a file, returns raw bytes as a Lua string.
int lua_read_encrypted_file(lua_State* L) {
    const char* inputPath = luaL_checkstring(L, 1);

    std::filesystem::path fullPath;
    if (g_renderer) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / "userdata" / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = getExeDir() / inputPath;
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = inputPath;
    }

    std::ifstream inFile(fullPath, std::ios::binary);
    if (!inFile.is_open()) {
        lua_pushnil(L);
        lua_pushstring(L, ("Cannot open file: " + fullPath.string()).c_str());
        return 2;
    }
    std::vector<char> raw((std::istreambuf_iterator<char>(inFile)), {});
    inFile.close();

    if (!hasEncryptionHeader(raw)) {
        lua_pushnil(L);
        lua_pushstring(L, "File is not encrypted (no L2DE header)");
        return 2;
    }

    // Skip 4-byte magic, XOR-decrypt
    std::vector<char> payload(raw.begin() + 4, raw.end());
    std::vector<char> decrypted = xorTransform(payload);

    lua_pushlstring(L, decrypted.data(), decrypted.size());
    lua_pushnil(L);
    return 2;
}

// writeEncryptedFile(path, data) -> bool success, string error
// Encrypts data with L2DE header + XOR and writes to file.
int lua_write_encrypted_file(lua_State* L) {
    const char* inputPath = luaL_checkstring(L, 1);
    size_t dataLen = 0;
    const char* data = luaL_checklstring(L, 2, &dataLen);

    // Write to userdata folder (writable) or absolute path
    std::filesystem::path fullPath = getExeDir() / "userdata" / inputPath;

    // If the path is absolute, use it directly
    std::filesystem::path absTest(inputPath);
    if (absTest.is_absolute()) {
        fullPath = absTest;
    } else if (g_renderer && std::filesystem::exists(
        std::filesystem::path(g_renderer->getProjectFolder()) / inputPath)) {
        fullPath = std::filesystem::path(g_renderer->getProjectFolder()) / inputPath;
    }

    std::error_code ec;
    std::filesystem::create_directories(fullPath.parent_path(), ec);

    std::vector<char> raw(data, data + dataLen);
    std::vector<char> encrypted = xorTransform(raw);

    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open()) {
        lua_pushboolean(L, false);
        lua_pushstring(L, ("Cannot write file: " + fullPath.string()).c_str());
        return 2;
    }
    outFile.write(reinterpret_cast<const char*>(LIKE2D_ENC_MAGIC), sizeof(LIKE2D_ENC_MAGIC));
    outFile.write(encrypted.data(), (std::streamsize)encrypted.size());
    outFile.close();

    lua_pushboolean(L, true);
    lua_pushnil(L);
    return 2;
}

// ============================================================
//  NETWORKING (net.*)
// ============================================================
int net_create_server(lua_State* L) {
    int port = luaL_checkinteger(L, 1);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->createServer(port));
    return 1;
}

int net_connect(lua_State* L) {
    const char* host = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    if (!g_network) { lua_pushinteger(L, -1); return 1; }
    int peerId = g_network->connectTo(host, port);
    lua_pushinteger(L, peerId);
    return 1;
}

int net_send(lua_State* L) {
    int peerId = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->sendTo(peerId, std::string(data, len)));
    return 1;
}

int net_send_all(lua_State* L) {
    size_t len = 0;
    const char* data = luaL_checklstring(L, 1, &len);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->sendToAll(std::string(data, len)));
    return 1;
}

int net_create_udp(lua_State* L) {
    int port = luaL_checkinteger(L, 1);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->createUDP(port));
    return 1;
}

int net_send_udp(lua_State* L) {
    const char* ip = luaL_checkstring(L, 1);
    int port = luaL_checkinteger(L, 2);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 3, &len);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->sendUDP(ip, port, std::string(data, len)));
    return 1;
}

int net_close(lua_State* L) {
    if (g_network) g_network->shutdown();
    return 0;
}

int net_get_peer_count(lua_State* L) {
    if (!g_network) { lua_pushinteger(L, 0); return 1; }
    lua_pushinteger(L, g_network->getPeerCount());
    return 1;
}

int net_get_peer_ip(lua_State* L) {
    int peerId = luaL_checkinteger(L, 1);
    if (!g_network) { lua_pushstring(L, ""); return 1; }
    std::string ip = g_network->getPeerIP(peerId);
    lua_pushstring(L, ip.c_str());
    return 1;
}

int net_send_to_all_except(lua_State* L) {
    int excludeId = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->sendToAllExcept(excludeId, std::string(data, len)));
    return 1;
}

int net_disconnect(lua_State* L) {
    int peerId = luaL_checkinteger(L, 1);
    const char* reason = luaL_optstring(L, 2, "kicked");
    if (!g_network) return 0;
    g_network->disconnectPeer(peerId, std::string(reason));
    return 0;
}

int net_broadcast_udp(lua_State* L) {
    int port = luaL_checkinteger(L, 1);
    size_t len = 0;
    const char* data = luaL_checklstring(L, 2, &len);
    if (!g_network) { lua_pushboolean(L, false); return 1; }
    lua_pushboolean(L, g_network->broadcastUDP(port, std::string(data, len)));
    return 1;
}

int net_http_get(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    if (!g_network) {
        lua_newtable(L);
        lua_pushinteger(L, 0); lua_setfield(L, -2, "status");
        lua_pushstring(L, ""); lua_setfield(L, -2, "body");
        lua_pushstring(L, "network not available"); lua_setfield(L, -2, "error");
        return 1;
    }
    HttpResponse resp = g_network->httpGet(std::string(url));
    lua_newtable(L);
    lua_pushinteger(L, resp.statusCode); lua_setfield(L, -2, "status");
    lua_pushlstring(L, resp.body.c_str(), resp.body.size()); lua_setfield(L, -2, "body");
    lua_pushstring(L, resp.error.c_str()); lua_setfield(L, -2, "error");
    return 1;
}

int net_http_post(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    size_t bodyLen = 0;
    const char* body = luaL_checklstring(L, 2, &bodyLen);
    const char* contentType = luaL_optstring(L, 3, "application/json");
    if (!g_network) {
        lua_newtable(L);
        lua_pushinteger(L, 0); lua_setfield(L, -2, "status");
        lua_pushstring(L, ""); lua_setfield(L, -2, "body");
        lua_pushstring(L, "network not available"); lua_setfield(L, -2, "error");
        return 1;
    }
    HttpResponse resp = g_network->httpPost(std::string(url), std::string(body, bodyLen), std::string(contentType));
    lua_newtable(L);
    lua_pushinteger(L, resp.statusCode); lua_setfield(L, -2, "status");
    lua_pushlstring(L, resp.body.c_str(), resp.body.size()); lua_setfield(L, -2, "body");
    lua_pushstring(L, resp.error.c_str()); lua_setfield(L, -2, "error");
    return 1;
}

int net_set_peer_data(lua_State* L) {
    int peerId = luaL_checkinteger(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* value = luaL_checkstring(L, 3);
    if (!g_network) return 0;
    g_network->setPeerMetadata(peerId, std::string(key), std::string(value));
    return 0;
}

int net_get_peer_data(lua_State* L) {
    int peerId = luaL_checkinteger(L, 1);
    const char* key = luaL_checkstring(L, 2);
    if (!g_network) { lua_pushstring(L, ""); return 1; }
    std::string val = g_network->getPeerMetadata(peerId, std::string(key));
    lua_pushstring(L, val.c_str());
    return 1;
}

int net_get_stats(lua_State* L) {
    if (!g_network) {
        lua_newtable(L);
        lua_pushinteger(L, 0); lua_setfield(L, -2, "bytesSent");
        lua_pushinteger(L, 0); lua_setfield(L, -2, "bytesReceived");
        lua_pushinteger(L, 0); lua_setfield(L, -2, "packetsSent");
        lua_pushinteger(L, 0); lua_setfield(L, -2, "packetsRecv");
        lua_pushinteger(L, 0); lua_setfield(L, -2, "peerCount");
        lua_pushnumber(L, 0.0); lua_setfield(L, -2, "uptime");
        return 1;
    }
    NetStats s = g_network->getStats();
    lua_newtable(L);
    lua_pushinteger(L, (lua_Integer)s.bytesSent); lua_setfield(L, -2, "bytesSent");
    lua_pushinteger(L, (lua_Integer)s.bytesReceived); lua_setfield(L, -2, "bytesReceived");
    lua_pushinteger(L, (lua_Integer)s.packetsSent); lua_setfield(L, -2, "packetsSent");
    lua_pushinteger(L, (lua_Integer)s.packetsRecv); lua_setfield(L, -2, "packetsRecv");
    lua_pushinteger(L, s.peerCount); lua_setfield(L, -2, "peerCount");
    lua_pushnumber(L, s.uptimeSeconds); lua_setfield(L, -2, "uptime");
    return 1;
}

int net_set_keepalive(lua_State* L) {
    float interval = (float)luaL_checknumber(L, 1);
    float timeout = (float)luaL_checknumber(L, 2);
    if (g_network) g_network->setKeepalive(interval, timeout);
    return 0;
}

// ============================================================
//  TILEMAP (tilemap.*)
// ============================================================

// tilemap.create(tilesetKey, tileW, tileH, cols, rows)
// Returns a Luau table: {tileset=string, tileW=int, tileH=int, cols=int, rows=int, tiles={...}}
int tilemap_create(lua_State* L) {
    const char* tilesetKey = luaL_checkstring(L, 1);
    int tileW = luaL_checkinteger(L, 2);
    int tileH = luaL_checkinteger(L, 3);
    int cols  = luaL_checkinteger(L, 4);
    int rows  = luaL_checkinteger(L, 5);

    // Create the map table
    lua_newtable(L);
    lua_pushstring(L, tilesetKey);   lua_setfield(L, -2, "tileset");
    lua_pushinteger(L, tileW);       lua_setfield(L, -2, "tileW");
    lua_pushinteger(L, tileH);       lua_setfield(L, -2, "tileH");
    lua_pushinteger(L, cols);        lua_setfield(L, -2, "cols");
    lua_pushinteger(L, rows);        lua_setfield(L, -2, "rows");

    // Create tiles array filled with 0 (empty)
    int total = cols * rows;
    lua_newtable(L);
    for (int i = 1; i <= total; i++) {
        lua_pushinteger(L, 0);
        lua_rawseti(L, -2, i);
    }
    lua_setfield(L, -2, "tiles");

    return 1;  // return the map table
}

// tilemap.draw(map, x, y, alpha?)
int tilemap_draw(lua_State* L) {
    if (!g_renderer) { lua_pushboolean(L, false); return 1; }

    luaL_checktype(L, 1, LUA_TTABLE);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float alpha = (float)luaL_optnumber(L, 4, 1.0);

    // Read map properties
    lua_getfield(L, 1, "tileset");
    const char* tilesetKey = luaL_checkstring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "tileW");
    int tileW = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "tileH");
    int tileH = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "cols");
    int cols = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "rows");
    int rows = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    // Read tiles array into C++ vector
    lua_getfield(L, 1, "tiles");
    luaL_checktype(L, -1, LUA_TTABLE);
    int total = cols * rows;
    std::vector<int> tiles(total, 0);
    for (int i = 0; i < total; i++) {
        lua_rawgeti(L, -1, i + 1);  // 1-based index in Luau
        if (lua_isnumber(L, -1)) {
            tiles[i] = lua_tointeger(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);  // pop tiles table

    bool ok = g_renderer->drawTileMap(tilesetKey, tileW, tileH, cols, rows, tiles, x, y, alpha);
    lua_pushboolean(L, ok);
    return 1;
}

// tilemap.setTile(map, row, col, tileId)
int tilemap_set_tile(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int row    = luaL_checkinteger(L, 2);
    int col    = luaL_checkinteger(L, 3);
    int tileId = luaL_checkinteger(L, 4);

    lua_getfield(L, 1, "cols");
    int cols = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "rows");
    int rows = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    if (row < 0 || row >= rows || col < 0 || col >= cols) {
        lua_pushboolean(L, false);
        return 1;
    }

    int idx = row * cols + col + 1;  // 1-based for Luau

    lua_getfield(L, 1, "tiles");
    lua_pushinteger(L, tileId);
    lua_rawseti(L, -2, idx);
    lua_pop(L, 1);

    lua_pushboolean(L, true);
    return 1;
}

// tilemap.getTile(map, row, col)
int tilemap_get_tile(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int row = luaL_checkinteger(L, 2);
    int col = luaL_checkinteger(L, 3);

    lua_getfield(L, 1, "cols");
    int cols = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "rows");
    int rows = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    if (row < 0 || row >= rows || col < 0 || col >= cols) {
        lua_pushinteger(L, -1);
        return 1;
    }

    int idx = row * cols + col + 1;

    lua_getfield(L, 1, "tiles");
    lua_rawgeti(L, -1, idx);
    int tileId = lua_tointeger(L, -1);
    lua_pop(L, 2);  // pop value + tiles table

    lua_pushinteger(L, tileId);
    return 1;
}

// tilemap.fill(map, row, col, w, h, tileId)
int tilemap_fill(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int startRow = luaL_checkinteger(L, 2);
    int startCol = luaL_checkinteger(L, 3);
    int fillW    = luaL_checkinteger(L, 4);
    int fillH    = luaL_checkinteger(L, 5);
    int tileId   = luaL_checkinteger(L, 6);

    lua_getfield(L, 1, "cols");
    int cols = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "rows");
    int rows = luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, 1, "tiles");

    for (int r = startRow; r < startRow + fillH; r++) {
        for (int c = startCol; c < startCol + fillW; c++) {
            if (r >= 0 && r < rows && c >= 0 && c < cols) {
                int idx = r * cols + c + 1;
                lua_pushinteger(L, tileId);
                lua_rawseti(L, -2, idx);
            }
        }
    }
    lua_pop(L, 1);  // pop tiles table

    lua_pushboolean(L, true);
    return 1;
}


// ============================================================
//  PARTICLE SYSTEM  (particle.*)
// ============================================================

struct ParticleEntry {
    float x, y, vx, vy;
    float life, maxLife;
    float size, endSize;
    float r, g, b, a;
    float endR, endG, endB, endA;
    float rotation, rotSpeed;
    float gravity;
    float drag;
    float originX, originY; // for relative mode: offset from emitter at spawn
};

struct ParticleSystemState {
    std::vector<ParticleEntry> particles;
    // Emission
    float x = 0, y = 0;
    float rate = 50;
    float lifetime = 2.0f;
    float speedMin = 20, speedMax = 80;
    float angle = -90;        // degrees, -90 = up
    float spread = 180;       // degrees
    float gravity = 50;
    float drag = 0;
    // Size
    float startSize = 8, endSize = 2;
    float sizeVariation = 0;  // random size ± variation
    // Color
    float sR = 1, sG = 0.8f, sB = 0.2f, sA = 1;
    float eR = 1, eG = 0.2f, eB = 0, eA = 0;
    float colorVariation = 0; // random hue shift per particle (0-1)
    // Shape
    int shape = 0;   // 0=square, 1=circle, 2=ring
    int emitShape = 0; // 0=point, 1=line, 2=circle, 3=rect
    float emitW = 0, emitH = 0, emitRadius = 0;
    // Texture (optional)
    std::string texture;
    int texW = 0, texH = 0;
    // Sprite sheet animation
    int spriteFrameW = 0, spriteFrameH = 0;
    float spriteFPS = 0;
    int spriteCols = 1;
    // Blend
    bool additive = false;
    // Limits
    int maxParticles = 5000;
    bool emitting = true;
    bool oneShot = false;
    float burstTimer = 0;
    // Accumulator
    float spawnAccum = 0;
    int aliveCount = 0;
    // ── Advanced customization ──
    float windX = 0, windY = 0;      // constant wind force
    bool followVelocity = false;      // particles rotate to face movement
    bool relative = false;            // particles move with emitter
    float rotSpeedMin = -180, rotSpeedMax = 180;
    bool bounce = false;              // bounce off floor
    float bounceY = 0;                // floor Y position
    float bounceDamping = 0.5f;       // energy retained on bounce (0-1)
    float lifetimeVariation = 0;      // random lifetime ± variation (0-1)
};

static std::unordered_map<int, ParticleSystemState*> g_particleSystems;
static int g_nextParticleId = 1;
static std::mt19937 g_particleRng(12345);
static std::unordered_map<std::string, std::function<void(ParticleSystemState*)>> g_customPresets;

// Pre-rendered circle texture for particle rendering (created once, reused)
static SDL_Texture* g_circleTexture = nullptr;
static SDL_Texture* getCircleTexture(SDL_Renderer* ren) {
    if (g_circleTexture) return g_circleTexture;
    const int SZ = 64;
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, SZ, SZ);
    if (!tex) return nullptr;
    Uint32 pixels[SZ * SZ];
    float center = SZ * 0.5f;
    float rad = SZ * 0.5f - 1.0f;
    for (int y = 0; y < SZ; y++) {
        for (int x = 0; x < SZ; x++) {
            float dx = x + 0.5f - center;
            float dy = y + 0.5f - center;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist <= rad) {
                float edge = rad - dist;
                Uint8 a = (Uint8)(edge < 1.0f ? edge * 255 : 255);
                // RGBA32 = ABGR8888 on little-endian: (A<<24)|(B<<16)|(G<<8)|R
                pixels[y * SZ + x] = ((Uint32)a << 24) | 0x00FFFFFF;
            } else {
                pixels[y * SZ + x] = 0;
            }
        }
    }
    SDL_UpdateTexture(tex, NULL, pixels, SZ * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    g_circleTexture = tex;
    return tex;
}

static float prand(float lo, float hi) {
    std::uniform_real_distribution<float> d(lo, hi);
    return d(g_particleRng);
}

static void spawnParticle(ParticleSystemState* ps) {
    if ((int)ps->particles.size() >= ps->maxParticles) return;
    ParticleEntry p{};
    // Position based on emit shape
    float ox = 0, oy = 0;
    if (ps->emitShape == 1) { // line
        ox = prand(-ps->emitW * 0.5f, ps->emitW * 0.5f);
    } else if (ps->emitShape == 2) { // circle
        float a = prand(0, 6.28318f);
        float r = prand(0, ps->emitRadius);
        ox = cosf(a) * r;
        oy = sinf(a) * r;
    } else if (ps->emitShape == 3) { // rect
        ox = prand(-ps->emitW * 0.5f, ps->emitW * 0.5f);
        oy = prand(-ps->emitH * 0.5f, ps->emitH * 0.5f);
    }
    p.x = ps->x + ox;
    p.y = ps->y + oy;
    p.originX = ox;
    p.originY = oy;
    // Velocity
    float dirRad = ps->angle * 3.14159f / 180.0f;
    float sprRad = ps->spread * 3.14159f / 180.0f;
    float a = dirRad + prand(-sprRad * 0.5f, sprRad * 0.5f);
    float spd = prand(ps->speedMin, ps->speedMax);
    p.vx = cosf(a) * spd;
    p.vy = sinf(a) * spd;
    // Lifetime with variation
    float lt = ps->lifetime;
    if (ps->lifetimeVariation > 0)
        lt *= prand(1.0f - ps->lifetimeVariation, 1.0f + ps->lifetimeVariation);
    if (lt < 0.01f) lt = 0.01f;
    p.life = lt;
    p.maxLife = lt;
    // Size with variation
    p.size = ps->startSize;
    if (ps->sizeVariation > 0)
        p.size *= prand(1.0f - ps->sizeVariation, 1.0f + ps->sizeVariation);
    if (p.size < 0.1f) p.size = 0.1f;
    p.endSize = ps->endSize * (p.size / ps->startSize);
    // Color with variation
    p.r = ps->sR; p.g = ps->sG; p.b = ps->sB; p.a = ps->sA;
    p.endR = ps->eR; p.endG = ps->eG; p.endB = ps->eB; p.endA = ps->eA;
    if (ps->colorVariation > 0) {
        float v = prand(-ps->colorVariation, ps->colorVariation);
        p.r = std::max(0.f, std::min(1.f, p.r + v));
        p.g = std::max(0.f, std::min(1.f, p.g + v * 0.7f));
        p.b = std::max(0.f, std::min(1.f, p.b - v * 0.5f));
        p.endR = std::max(0.f, std::min(1.f, p.endR + v));
        p.endG = std::max(0.f, std::min(1.f, p.endG + v * 0.7f));
        p.endB = std::max(0.f, std::min(1.f, p.endB - v * 0.5f));
    }
    p.rotation = prand(0, 360);
    p.rotSpeed = prand(ps->rotSpeedMin, ps->rotSpeedMax);
    p.gravity = ps->gravity;
    p.drag = ps->drag;
    ps->particles.push_back(p);
}

static void applyPreset(ParticleSystemState* ps, const std::string& preset) {
    if (preset == "explosion") {
        ps->rate = 0; ps->oneShot = true; ps->burstTimer = 0.01f;
        ps->speedMin = 100; ps->speedMax = 350; ps->spread = 360; ps->angle = 0;
        ps->lifetime = 1.0f; ps->gravity = 120; ps->drag = 2;
        ps->startSize = 10; ps->endSize = 1;
        ps->sR = 1; ps->sG = 0.9f; ps->sB = 0.3f; ps->sA = 1;
        ps->eR = 1; ps->eG = 0.1f; ps->eB = 0; ps->eA = 0;
        ps->additive = true; ps->shape = 1;
        ps->maxParticles = 500;
    } else if (preset == "fire") {
        ps->rate = 80; ps->speedMin = 20; ps->speedMax = 60;
        ps->angle = -90; ps->spread = 30; ps->lifetime = 1.2f;
        ps->gravity = -60; ps->drag = 0.5f;
        ps->startSize = 12; ps->endSize = 2;
        ps->sR = 1; ps->sG = 0.8f; ps->sB = 0.1f; ps->sA = 0.9f;
        ps->eR = 0.8f; ps->eG = 0.1f; ps->eB = 0; ps->eA = 0;
        ps->additive = true; ps->shape = 1;
        ps->emitShape = 1; ps->emitW = 30;
    } else if (preset == "smoke") {
        ps->rate = 30; ps->speedMin = 10; ps->speedMax = 30;
        ps->angle = -90; ps->spread = 20; ps->lifetime = 3.0f;
        ps->gravity = -20; ps->drag = 0.3f;
        ps->startSize = 6; ps->endSize = 30;
        ps->sR = 0.5f; ps->sG = 0.5f; ps->sB = 0.5f; ps->sA = 0.5f;
        ps->eR = 0.3f; ps->eG = 0.3f; ps->eB = 0.3f; ps->eA = 0;
        ps->additive = false; ps->shape = 1;
    } else if (preset == "rain") {
        ps->rate = 200; ps->speedMin = 300; ps->speedMax = 400;
        ps->angle = 85; ps->spread = 5; ps->lifetime = 2.0f;
        ps->gravity = 200; ps->drag = 0;
        ps->startSize = 3; ps->endSize = 2;
        ps->sR = 0.6f; ps->sG = 0.7f; ps->sB = 0.9f; ps->sA = 0.7f;
        ps->eR = 0.4f; ps->eG = 0.5f; ps->eB = 0.8f; ps->eA = 0;
        ps->additive = false; ps->shape = 0;
        ps->emitShape = 1; ps->emitW = 800;
        ps->maxParticles = 2000;
    } else if (preset == "snow") {
        ps->rate = 60; ps->speedMin = 20; ps->speedMax = 50;
        ps->angle = 90; ps->spread = 30; ps->lifetime = 5.0f;
        ps->gravity = 15; ps->drag = 1.0f;
        ps->startSize = 4; ps->endSize = 3;
        ps->sR = 1; ps->sG = 1; ps->sB = 1; ps->sA = 0.8f;
        ps->eR = 1; ps->eG = 1; ps->eB = 1; ps->eA = 0;
        ps->additive = false; ps->shape = 1;
        ps->emitShape = 1; ps->emitW = 800;
    } else if (preset == "magic") {
        ps->rate = 60; ps->speedMin = 30; ps->speedMax = 80;
        ps->angle = -90; ps->spread = 360; ps->lifetime = 1.5f;
        ps->gravity = -10; ps->drag = 1;
        ps->startSize = 6; ps->endSize = 1;
        ps->sR = 0.5f; ps->sG = 0.3f; ps->sB = 1; ps->sA = 1;
        ps->eR = 0.8f; ps->eG = 0.4f; ps->eB = 1; ps->eA = 0;
        ps->additive = true; ps->shape = 1;
        ps->emitShape = 2; ps->emitRadius = 20;
    } else if (preset == "sparkle") {
        ps->rate = 40; ps->speedMin = 5; ps->speedMax = 30;
        ps->angle = 0; ps->spread = 360; ps->lifetime = 0.8f;
        ps->gravity = 0; ps->drag = 2;
        ps->startSize = 4; ps->endSize = 0;
        ps->sR = 1; ps->sG = 1; ps->sB = 0.8f; ps->sA = 1;
        ps->eR = 1; ps->eG = 1; ps->eB = 1; ps->eA = 0;
        ps->additive = true; ps->shape = 0;
        ps->emitShape = 2; ps->emitRadius = 30;
    } else if (preset == "confetti") {
        ps->rate = 0; ps->oneShot = true; ps->burstTimer = 0.01f;
        ps->speedMin = 100; ps->speedMax = 300; ps->spread = 120; ps->angle = -90;
        ps->lifetime = 3.0f; ps->gravity = 150; ps->drag = 1;
        ps->startSize = 8; ps->endSize = 4;
        ps->sR = prand(0,1); ps->sG = prand(0,1); ps->sB = prand(0,1); ps->sA = 1;
        ps->eR = ps->sR; ps->eG = ps->sG; ps->eB = ps->sB; ps->eA = 0;
        ps->additive = false; ps->shape = 0;
        ps->maxParticles = 300;
    } else {
        // Check user-registered custom presets
        auto it = g_customPresets.find(preset);
        if (it != g_customPresets.end()) {
            it->second(ps);
        }
    }
}

static float readOptFloat(lua_State* L, const char* key, float def) {
    lua_getfield(L, 1, key);
    float v = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : def;
    lua_pop(L, 1);
    return v;
}
static int readOptInt(lua_State* L, const char* key, int def) {
    lua_getfield(L, 1, key);
    int v = lua_isnumber(L, -1) ? (int)lua_tointeger(L, -1) : def;
    lua_pop(L, 1);
    return v;
}
static bool readOptBool(lua_State* L, const char* key, bool def) {
    lua_getfield(L, 1, key);
    bool v = lua_isboolean(L, -1) ? (lua_toboolean(L, -1) != 0) : def;
    lua_pop(L, 1);
    return v;
}

// particle.create(config) → id
int particle_create(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    auto* ps = new ParticleSystemState();
    ps->x = readOptFloat(L, "x", 0);
    ps->y = readOptFloat(L, "y", 0);
    ps->rate = readOptFloat(L, "rate", 50);
    ps->lifetime = readOptFloat(L, "lifetime", 2.0f);
    ps->speedMin = readOptFloat(L, "speedMin", 20);
    ps->speedMax = readOptFloat(L, "speedMax", 80);
    ps->angle = readOptFloat(L, "angle", -90);
    ps->spread = readOptFloat(L, "spread", 180);
    ps->gravity = readOptFloat(L, "gravity", 50);
    ps->drag = readOptFloat(L, "drag", 0);
    ps->startSize = readOptFloat(L, "startSize", 8);
    ps->endSize = readOptFloat(L, "endSize", 2);
    ps->maxParticles = readOptInt(L, "maxParticles", 5000);
    ps->additive = readOptBool(L, "additive", false);
    ps->emitting = readOptBool(L, "emitting", true);
    ps->emitW = readOptFloat(L, "emitW", 0);
    ps->emitH = readOptFloat(L, "emitH", 0);
    ps->emitRadius = readOptFloat(L, "emitRadius", 0);
    // Advanced customization
    ps->windX = readOptFloat(L, "windX", 0);
    ps->windY = readOptFloat(L, "windY", 0);
    ps->followVelocity = readOptBool(L, "followVelocity", false);
    ps->relative = readOptBool(L, "relative", false);
    ps->rotSpeedMin = readOptFloat(L, "rotSpeedMin", -180);
    ps->rotSpeedMax = readOptFloat(L, "rotSpeedMax", 180);
    ps->bounce = readOptBool(L, "bounce", false);
    ps->bounceY = readOptFloat(L, "bounceY", 0);
    ps->bounceDamping = readOptFloat(L, "bounceDamping", 0.5f);
    ps->sizeVariation = readOptFloat(L, "sizeVariation", 0);
    ps->colorVariation = readOptFloat(L, "colorVariation", 0);
    ps->lifetimeVariation = readOptFloat(L, "lifetimeVariation", 0);
    // Sprite sheet animation
    ps->spriteFrameW = readOptInt(L, "spriteFrameW", 0);
    ps->spriteFrameH = readOptInt(L, "spriteFrameH", 0);
    ps->spriteFPS = readOptFloat(L, "spriteFPS", 0);
    // Shape string
    lua_getfield(L, 1, "shape");
    if (lua_isstring(L, -1)) {
        std::string s = lua_tostring(L, -1);
        if (s == "circle") ps->shape = 1;
        else if (s == "ring") ps->shape = 2;
        else ps->shape = 0;
    }
    lua_pop(L, 1);
    // Emit shape string
    lua_getfield(L, 1, "emitShape");
    if (lua_isstring(L, -1)) {
        std::string s = lua_tostring(L, -1);
        if (s == "line") ps->emitShape = 1;
        else if (s == "circle") ps->emitShape = 2;
        else if (s == "rect") ps->emitShape = 3;
        else ps->emitShape = 0;
    }
    lua_pop(L, 1);
    // Texture
    lua_getfield(L, 1, "texture");
    if (lua_isstring(L, -1)) {
        ps->texture = lua_tostring(L, -1);
        if (g_renderer && g_renderer->loadImage(ps->texture)) {
            g_renderer->getImageSize(ps->texture, ps->texW, ps->texH);
        }
    }
    lua_pop(L, 1);
    // startColor / endColor tables {r,g,b,a} 0-255
    lua_getfield(L, 1, "startColor");
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); ps->sR = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); ps->sG = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); ps->sB = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 4); ps->sA = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 1, "endColor");
    if (lua_istable(L, -1)) {
        lua_rawgeti(L, -1, 1); ps->eR = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 2); ps->eG = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 3); ps->eB = (float)luaL_optnumber(L, -1, 255) / 255.0f; lua_pop(L, 1);
        lua_rawgeti(L, -1, 4); ps->eA = (float)luaL_optnumber(L, -1, 0) / 255.0f; lua_pop(L, 1);
    }
    lua_pop(L, 1);
    // Preset (overrides above if specified)
    lua_getfield(L, 1, "preset");
    if (lua_isstring(L, -1)) {
        applyPreset(ps, lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    ps->particles.reserve(std::min(ps->maxParticles, 500));
    int id = g_nextParticleId++;
    g_particleSystems[id] = ps;
    lua_pushinteger(L, id);
    return 1;
}

// particle.update(id, dt)
int particle_update(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float dt = (float)luaL_checknumber(L, 2);
    auto it = g_particleSystems.find(id);
    if (it == g_particleSystems.end()) return 0;
    auto* ps = it->second;
    float prevX = ps->x, prevY = ps->y; // track emitter movement for relative mode

    // Update existing particles
    int alive = 0;
    for (int i = 0; i < (int)ps->particles.size(); i++) {
        auto& p = ps->particles[i];
        p.life -= dt;
        if (p.life <= 0) continue;
        // Physics
        p.vy += p.gravity * dt;
        // Wind
        if (ps->windX != 0) p.vx += ps->windX * dt;
        if (ps->windY != 0) p.vy += ps->windY * dt;
        // Drag
        if (p.drag > 0) {
            float factor = 1.0f - p.drag * dt;
            if (factor < 0) factor = 0;
            p.vx *= factor;
            p.vy *= factor;
        }
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        // Rotation
        if (ps->followVelocity && (p.vx != 0 || p.vy != 0)) {
            p.rotation = atan2f(p.vy, p.vx) * 180.0f / 3.14159f;
        } else {
            p.rotation += p.rotSpeed * dt;
        }
        // Bounce
        if (ps->bounce && p.y > ps->bounceY) {
            p.y = ps->bounceY;
            p.vy = -p.vy * ps->bounceDamping;
            p.vx *= 0.9f; // friction on bounce
        }
        // Relative mode: move particle with emitter
        if (ps->relative) {
            p.x += (ps->x - prevX);
            p.y += (ps->y - prevY);
        }
        // Move alive particle to front (compact array)
        if (i != alive) ps->particles[alive] = p;
        alive++;
    }
    ps->particles.resize(alive);
    ps->aliveCount = alive;

    // Spawn new particles
    if (ps->emitting && ps->rate > 0) {
        ps->spawnAccum += ps->rate * dt;
        int toSpawn = (int)ps->spawnAccum;
        ps->spawnAccum -= toSpawn;
        for (int i = 0; i < toSpawn; i++) spawnParticle(ps);
    }
    // One-shot burst
    if (ps->oneShot && ps->burstTimer > 0) {
        ps->burstTimer -= dt;
        if (ps->burstTimer <= 0) {
            int count = (int)ps->rate;
            if (count <= 0) count = ps->maxParticles;
            for (int i = 0; i < count; i++) spawnParticle(ps);
            ps->emitting = false;
        }
    }
    return 0;
}

// particle.draw(id)
int particle_draw(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    auto it = g_particleSystems.find(id);
    if (it == g_particleSystems.end()) return 0;
    auto* ps = it->second;
    if (ps->particles.empty()) return 0;

    SDL_Renderer* ren = g_renderer ? g_renderer->getSDLRenderer() : nullptr;
    if (!ren) return 0;

    // Set blend mode
    SDL_BlendMode prevBlend;
    SDL_GetRenderDrawBlendMode(ren, &prevBlend);
    if (ps->additive) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_ADD);
    } else {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    }

    // If texture is set, get it
    SDL_Texture* tex = nullptr;
    if (!ps->texture.empty() && g_renderer) {
        tex = (SDL_Texture*)g_renderer->getTextureHandle(ps->texture);
    }

    for (auto& p : ps->particles) {
        float t = p.life / p.maxLife; // 1.0 = just born, 0.0 = dead
        float it_ = 1.0f - t;
        float cr = p.r + (p.endR - p.r) * it_;
        float cg = p.g + (p.endG - p.g) * it_;
        float cb = p.b + (p.endB - p.b) * it_;
        float ca = p.a + (p.endA - p.a) * it_;
        float sz = p.size + (p.endSize - p.size) * it_;
        int ir = (int)(cr * 255); if (ir < 0) ir = 0; if (ir > 255) ir = 255;
        int ig = (int)(cg * 255); if (ig < 0) ig = 0; if (ig > 255) ig = 255;
        int ib = (int)(cb * 255); if (ib < 0) ib = 0; if (ib > 255) ib = 255;
        int ia = (int)(ca * 255); if (ia < 0) ia = 0; if (ia > 255) ia = 255;

        if (tex) {
            // Render with texture
            SDL_FRect dst = { p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz };
            SDL_SetTextureAlphaMod(tex, (Uint8)ia);
            if (p.rotation != 0) {
                SDL_RenderTextureRotated(ren, tex, nullptr, &dst, p.rotation, nullptr, SDL_FLIP_NONE);
            } else {
                SDL_RenderTexture(ren, tex, nullptr, &dst);
            }
        } else {
            // Render with colored shapes
            SDL_SetRenderDrawColor(ren, (Uint8)ir, (Uint8)ig, (Uint8)ib, (Uint8)ia);
            if (ps->shape == 0) {
                // Square
                SDL_FRect r = { p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz };
                SDL_RenderFillRect(ren, &r);
            } else if (ps->shape == 1) {
                // Circle (pre-rendered texture — no per-pixel sqrt!)
                SDL_Texture* circTex = getCircleTexture(ren);
                if (circTex) {
                    SDL_SetTextureColorMod(circTex, (Uint8)ir, (Uint8)ig, (Uint8)ib);
                    SDL_SetTextureAlphaMod(circTex, (Uint8)ia);
                    SDL_FRect dst = { p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz };
                    SDL_RenderTexture(ren, circTex, nullptr, &dst);
                } else {
                    SDL_FRect r = { p.x - sz * 0.5f, p.y - sz * 0.5f, sz, sz };
                    SDL_RenderFillRect(ren, &r);
                }
            } else if (ps->shape == 2) {
                // Ring (unfilled circle)
                int segments = std::max(8, (int)(sz * 0.8f));
                if (segments > 20) segments = 20;
                float rad = sz * 0.5f;
                for (int s = 0; s < segments; s++) {
                    float a1 = (float)s / segments * 6.28318f;
                    float a2 = (float)(s + 1) / segments * 6.28318f;
                    SDL_RenderLine(ren,
                        p.x + cosf(a1) * rad, p.y + sinf(a1) * rad,
                        p.x + cosf(a2) * rad, p.y + sinf(a2) * rad);
                }
            }
        }
    }

    SDL_SetRenderDrawBlendMode(ren, prevBlend);
    return 0;
}

// particle.destroy(id)
int particle_destroy(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) {
        delete it->second;
        g_particleSystems.erase(it);
    }
    return 0;
}

// particle.burst(id, count)
int particle_burst(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    int count = luaL_checkinteger(L, 2);
    auto it = g_particleSystems.find(id);
    if (it == g_particleSystems.end()) return 0;
    auto* ps = it->second;
    for (int i = 0; i < count; i++) spawnParticle(ps);
    return 0;
}

// particle.setPosition(id, x, y)
int particle_set_position(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) {
        it->second->x = x;
        it->second->y = y;
    }
    return 0;
}

// particle.setEmitting(id, bool)
int particle_set_emitting(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    bool on = lua_toboolean(L, 2) != 0;
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) it->second->emitting = on;
    return 0;
}

// particle.getCount(id) → int
int particle_get_count(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) {
        lua_pushinteger(L, (int)it->second->particles.size());
    } else {
        lua_pushinteger(L, 0);
    }
    return 1;
}

// particle.setRate(id, rate)
int particle_set_rate(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float rate = (float)luaL_checknumber(L, 2);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) it->second->rate = rate;
    return 0;
}

// particle.clear() — destroy all particle systems
int particle_clear_all(lua_State* L) {
    for (auto& [k, v] : g_particleSystems) delete v;
    g_particleSystems.clear();
    return 0;
}

// particle.setGravity(id, gravity) — change gravity at runtime
int particle_set_gravity(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float g = (float)luaL_checknumber(L, 2);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) it->second->gravity = g;
    return 0;
}

// particle.setWind(id, x, y) — change wind at runtime
int particle_set_wind(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float wx = (float)luaL_checknumber(L, 2);
    float wy = (float)luaL_checknumber(L, 3);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) {
        it->second->windX = wx;
        it->second->windY = wy;
    }
    return 0;
}

// particle.setBounce(id, enabled, bounceY, damping) — toggle bounce at runtime
int particle_set_bounce(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    bool enabled = lua_toboolean(L, 2) != 0;
    float bY = (float)luaL_optnumber(L, 3, 0);
    float damping = (float)luaL_optnumber(L, 4, 0.5);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) {
        it->second->bounce = enabled;
        it->second->bounceY = bY;
        it->second->bounceDamping = damping;
    }
    return 0;
}

// particle.setLifetime(id, lifetime) — change particle lifetime at runtime
int particle_set_lifetime(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    float lt = (float)luaL_checknumber(L, 2);
    auto it = g_particleSystems.find(id);
    if (it != g_particleSystems.end()) it->second->lifetime = lt;
    return 0;
}

// particle.registerPreset(name, configFn) — register a custom preset
// configFn is called with a table and should fill it with config values
int particle_register_preset(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    // Store the Lua function reference in the Lua registry so it persists
    lua_pushvalue(L, 2); // push copy of function
    int funcRef = lua_ref(L, -1);
    lua_pop(L, 1);
    // Capture the lua_State and funcRef in a lambda
    lua_State* capturedL = L;
    g_customPresets[std::string(name)] = [capturedL, funcRef](ParticleSystemState* ps) {
        lua_getref(capturedL, funcRef);
        // Create a config table
        lua_newtable(capturedL);
        lua_pushnumber(capturedL, ps->x); lua_setfield(capturedL, -2, "x");
        lua_pushnumber(capturedL, ps->y); lua_setfield(capturedL, -2, "y");
        lua_pcall(capturedL, 1, 1, 0); // call configFn(configTable) -> returns configTable or nil
        // Read back from returned table or from what's on top of stack
        int tbl = lua_gettop(capturedL);
        if (lua_istable(capturedL, tbl)) {
            lua_getfield(capturedL, tbl, "rate");        if (lua_isnumber(capturedL, -1)) ps->rate = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "lifetime");    if (lua_isnumber(capturedL, -1)) ps->lifetime = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "speedMin");    if (lua_isnumber(capturedL, -1)) ps->speedMin = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "speedMax");    if (lua_isnumber(capturedL, -1)) ps->speedMax = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "angle");       if (lua_isnumber(capturedL, -1)) ps->angle = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "spread");      if (lua_isnumber(capturedL, -1)) ps->spread = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "gravity");     if (lua_isnumber(capturedL, -1)) ps->gravity = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "drag");        if (lua_isnumber(capturedL, -1)) ps->drag = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "startSize");   if (lua_isnumber(capturedL, -1)) ps->startSize = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "endSize");     if (lua_isnumber(capturedL, -1)) ps->endSize = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "maxParticles");if (lua_isnumber(capturedL, -1)) ps->maxParticles = (int)lua_tointeger(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "additive");    if (lua_isboolean(capturedL, -1)) ps->additive = lua_toboolean(capturedL, -1) != 0; lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "oneShot");     if (lua_isboolean(capturedL, -1)) { ps->oneShot = lua_toboolean(capturedL, -1) != 0; if (ps->oneShot) { ps->rate = 0; ps->burstTimer = 0.01f; } } lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "windX");       if (lua_isnumber(capturedL, -1)) ps->windX = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "windY");       if (lua_isnumber(capturedL, -1)) ps->windY = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "followVelocity"); if (lua_isboolean(capturedL, -1)) ps->followVelocity = lua_toboolean(capturedL, -1) != 0; lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "relative");    if (lua_isboolean(capturedL, -1)) ps->relative = lua_toboolean(capturedL, -1) != 0; lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "bounce");      if (lua_isboolean(capturedL, -1)) ps->bounce = lua_toboolean(capturedL, -1) != 0; lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "bounceY");     if (lua_isnumber(capturedL, -1)) ps->bounceY = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "bounceDamping");if (lua_isnumber(capturedL, -1)) ps->bounceDamping = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "sizeVariation");if (lua_isnumber(capturedL, -1)) ps->sizeVariation = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "colorVariation");if (lua_isnumber(capturedL, -1)) ps->colorVariation = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "lifetimeVariation");if (lua_isnumber(capturedL, -1)) ps->lifetimeVariation = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            // startColor / endColor
            lua_getfield(capturedL, tbl, "startColor");
            if (lua_istable(capturedL, -1)) {
                lua_rawgeti(capturedL, -1, 1); ps->sR = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 2); ps->sG = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 3); ps->sB = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 4); ps->sA = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
            }
            lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "endColor");
            if (lua_istable(capturedL, -1)) {
                lua_rawgeti(capturedL, -1, 1); ps->eR = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 2); ps->eG = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 3); ps->eB = (float)luaL_optnumber(capturedL, -1, 255) / 255.0f; lua_pop(capturedL, 1);
                lua_rawgeti(capturedL, -1, 4); ps->eA = (float)luaL_optnumber(capturedL, -1, 0) / 255.0f; lua_pop(capturedL, 1);
            }
            lua_pop(capturedL, 1);
            // shape string
            lua_getfield(capturedL, tbl, "shape");
            if (lua_isstring(capturedL, -1)) {
                std::string s = lua_tostring(capturedL, -1);
                if (s == "circle") ps->shape = 1;
                else if (s == "ring") ps->shape = 2;
                else ps->shape = 0;
            }
            lua_pop(capturedL, 1);
            // emitShape string
            lua_getfield(capturedL, tbl, "emitShape");
            if (lua_isstring(capturedL, -1)) {
                std::string s = lua_tostring(capturedL, -1);
                if (s == "line") ps->emitShape = 1;
                else if (s == "circle") ps->emitShape = 2;
                else if (s == "rect") ps->emitShape = 3;
                else ps->emitShape = 0;
            }
            lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "emitW");      if (lua_isnumber(capturedL, -1)) ps->emitW = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "emitH");      if (lua_isnumber(capturedL, -1)) ps->emitH = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
            lua_getfield(capturedL, tbl, "emitRadius"); if (lua_isnumber(capturedL, -1)) ps->emitRadius = (float)lua_tonumber(capturedL, -1); lua_pop(capturedL, 1);
        }
        lua_pop(capturedL, 1); // pop returned table
    };
    lua_pushboolean(L, 1);
    return 1;
}

// particle.getConfig(id) → table — return current particle system config
int particle_get_config(lua_State* L) {
    int id = luaL_checkinteger(L, 1);
    auto it = g_particleSystems.find(id);
    if (it == g_particleSystems.end()) { lua_pushnil(L); return 1; }
    auto* ps = it->second;
    lua_newtable(L);
    lua_pushnumber(L, ps->x); lua_setfield(L, -2, "x");
    lua_pushnumber(L, ps->y); lua_setfield(L, -2, "y");
    lua_pushnumber(L, ps->rate); lua_setfield(L, -2, "rate");
    lua_pushnumber(L, ps->lifetime); lua_setfield(L, -2, "lifetime");
    lua_pushnumber(L, ps->speedMin); lua_setfield(L, -2, "speedMin");
    lua_pushnumber(L, ps->speedMax); lua_setfield(L, -2, "speedMax");
    lua_pushnumber(L, ps->angle); lua_setfield(L, -2, "angle");
    lua_pushnumber(L, ps->spread); lua_setfield(L, -2, "spread");
    lua_pushnumber(L, ps->gravity); lua_setfield(L, -2, "gravity");
    lua_pushnumber(L, ps->drag); lua_setfield(L, -2, "drag");
    lua_pushnumber(L, ps->startSize); lua_setfield(L, -2, "startSize");
    lua_pushnumber(L, ps->endSize); lua_setfield(L, -2, "endSize");
    lua_pushinteger(L, ps->maxParticles); lua_setfield(L, -2, "maxParticles");
    lua_pushboolean(L, ps->additive); lua_setfield(L, -2, "additive");
    lua_pushboolean(L, ps->emitting); lua_setfield(L, -2, "emitting");
    lua_pushnumber(L, ps->windX); lua_setfield(L, -2, "windX");
    lua_pushnumber(L, ps->windY); lua_setfield(L, -2, "windY");
    lua_pushboolean(L, ps->followVelocity); lua_setfield(L, -2, "followVelocity");
    lua_pushboolean(L, ps->relative); lua_setfield(L, -2, "relative");
    lua_pushboolean(L, ps->bounce); lua_setfield(L, -2, "bounce");
    lua_pushnumber(L, ps->bounceY); lua_setfield(L, -2, "bounceY");
    lua_pushnumber(L, ps->bounceDamping); lua_setfield(L, -2, "bounceDamping");
    lua_pushnumber(L, ps->sizeVariation); lua_setfield(L, -2, "sizeVariation");
    lua_pushnumber(L, ps->colorVariation); lua_setfield(L, -2, "colorVariation");
    lua_pushnumber(L, ps->lifetimeVariation); lua_setfield(L, -2, "lifetimeVariation");
    lua_pushinteger(L, ps->aliveCount); lua_setfield(L, -2, "aliveCount");
    // startColor / endColor
    lua_newtable(L);
    lua_pushnumber(L, ps->sR * 255); lua_rawseti(L, -2, 1);
    lua_pushnumber(L, ps->sG * 255); lua_rawseti(L, -2, 2);
    lua_pushnumber(L, ps->sB * 255); lua_rawseti(L, -2, 3);
    lua_pushnumber(L, ps->sA * 255); lua_rawseti(L, -2, 4);
    lua_setfield(L, -2, "startColor");
    lua_newtable(L);
    lua_pushnumber(L, ps->eR * 255); lua_rawseti(L, -2, 1);
    lua_pushnumber(L, ps->eG * 255); lua_rawseti(L, -2, 2);
    lua_pushnumber(L, ps->eB * 255); lua_rawseti(L, -2, 3);
    lua_pushnumber(L, ps->eA * 255); lua_rawseti(L, -2, 4);
    lua_setfield(L, -2, "endColor");
    // shape / emitShape strings
    const char* shapes[] = {"square", "circle", "ring"};
    lua_pushstring(L, shapes[std::min(ps->shape, 2)]); lua_setfield(L, -2, "shape");
    const char* emitShapes[] = {"point", "line", "circle", "rect"};
    lua_pushstring(L, emitShapes[std::min(ps->emitShape, 3)]); lua_setfield(L, -2, "emitShape");
    return 1;
}

// ============================================================
//  REGISTRATION
// ============================================================
void register_lua_functions(lua_State* L, Engine* engine, Renderer* renderer) {
    g_engine   = engine;
    g_renderer = renderer;

    // Clean up old AudioManager on hot reload to prevent leak
    if (g_audio) {
        delete g_audio;
        g_audio = nullptr;
    }

    // Create and initialize AudioManager
    g_audio = new AudioManager();
    if (g_audio->initialize()) {
        g_audio->setProjectFolder(engine->getProjectFolder());
    } else {
        std::cerr << "Warning: Audio system failed to initialize." << std::endl;
    }

    static const luaL_Reg functions[] = {
        // Window
        {"setWindowTitle",    set_window_title},
        {"getWindowSize",     get_window_size},
        {"setWindowSize",     set_window_size},
        {"setFullscreen",     set_fullscreen},
        {"setEscapeQuits",    set_escape_quits},
        {"setLogicalSize",    set_logical_size},
        {"getLogicalSize",    get_logical_size},
        {"setUseLetterbox",   set_use_letterbox},
        {"setLetterboxColor", set_letterbox_color},
        // Time
        {"getTime",           get_time},
        {"getDeltaTime",      get_delta_time},
        // Input — keyboard
        {"isKeyDown",         is_key_down},
        {"isKeyJustPressed",  is_key_just_pressed},
        {"isKeyJustReleased", is_key_just_released},
        // Input — mouse
        {"isMousePressed",    is_mouse_pressed},
        {"getMousePos",       get_mouse_pos},
        {"getMouseScroll",    get_mouse_scroll},
        // Rendering — images
        {"clearScreen",       clear_screen},
        {"loadImage",         load_image},
        {"renderImage",       render_image},
        {"renderImageEx",     render_image_ex},
        {"renderImageRegion", render_image_region},
        {"getImageSize",      get_image_size},
        // Rendering — primitives
        {"drawRect",          draw_rect},
        {"drawLine",          draw_line},
        {"drawCircle",        draw_circle},
        // Rendering — text
        {"loadFont",          load_font},
        {"drawText",          draw_text},
        {"drawTextDirect",    draw_text_direct},
        {"getTextSize",       get_text_size},
        {"setDefaultFont",    set_default_font},
        // Camera
        {"setCameraPosition", set_camera_position},
        {"getCameraPosition", get_camera_position},
        {"setCameraZoom",     set_camera_zoom},
        {"getCameraZoom",     get_camera_zoom},
        {"worldToScreen",     world_to_screen},
        // Physics — bodies
        {"createBody",           create_body},
        {"destroyBody",          destroy_body},
        {"setBodyPosition",      set_body_position},
        {"getBodyPosition",      get_body_position},
        {"getPhysicsTransform",  get_physics_transform},
        {"setBodyVelocity",      set_body_velocity},
        {"getBodyVelocity",      get_body_velocity},
        {"applyForce",           apply_force},
        {"applyImpulse",         apply_impulse},
        {"setGravity",           set_gravity},
        // Physics — shapes
        {"addBoxShape",          add_box_shape},
        {"addCircleShape",       add_circle_shape},
        {"addCapsuleShape",      add_capsule_shape},
        // Audio
        {"loadSound",        load_sound},
        {"playSound",        play_sound},
        {"stopSound",        stop_sound},
        {"pauseSound",       pause_sound},
        {"resumeSound",      resume_sound},
        {"isSoundPlaying",   is_sound_playing},
        {"setSoundVolume",   set_sound_volume},
        {"setMasterVolume",  set_master_volume},
        {"setSoundPitch",    set_sound_pitch},
        {"unloadSound",      unload_sound},
        // Video
        {"loadVideo",        load_video},
        {"updateVideo",      update_video},
        {"renderVideo",      render_video},
        {"playVideo",        play_video},
        {"pauseVideo",       pause_video},
        {"stopVideo",        stop_video},
        {"seekVideo",        seek_video},
        {"isVideoPlaying",   is_video_playing},
        {"isVideoEnded",     is_video_ended},
        {"getVideoDuration", get_video_duration},
        {"getVideoTime",     get_video_time},
        {"unloadVideo",      unload_video},
        // Misc
        {"print",            print_message},
        {"quit",             lua_quit},

        // FileSystem
        {"fileExists",        lua_file_exists},
        {"readFile",          lua_read_file},
        {"writeFile",         lua_write_file},
        {"listDirectory",     lua_list_directory},
        {"createDirectory",   lua_create_directory},
        {"deleteFile",        lua_delete_file},
        {"getUserdataPath",   lua_get_userdata_path},
        {"getGamePath",       lua_get_game_path},
        // Encryption
        {"encryptFile",       lua_encrypt_file},
        {"decryptFile",       lua_decrypt_file},
        {"readEncryptedFile", lua_read_encrypted_file},
        {"writeEncryptedFile",lua_write_encrypted_file},

        {NULL, NULL}
    };
    luaL_register(L, "_G", functions);

    // Register json as a table: json.load, json.save, json.encode, json.decode
    lua_newtable(L);
    lua_pushcfunction(L, json_load,   "json.load");   lua_setfield(L, -2, "load");
    lua_pushcfunction(L, json_save,   "json.save");   lua_setfield(L, -2, "save");
    lua_pushcfunction(L, json_encode, "json.encode"); lua_setfield(L, -2, "encode");
    lua_pushcfunction(L, json_decode, "json.decode"); lua_setfield(L, -2, "decode");
    lua_setglobal(L, "json");

    // Register net as a table: net.createServer, net.connect, net.send, etc.
    g_network = engine->getNetworkManager();
    lua_newtable(L);
    lua_pushcfunction(L, net_create_server,  "net.createServer");  lua_setfield(L, -2, "createServer");
    lua_pushcfunction(L, net_connect,        "net.connect");       lua_setfield(L, -2, "connect");
    lua_pushcfunction(L, net_send,           "net.send");          lua_setfield(L, -2, "send");
    lua_pushcfunction(L, net_send_all,       "net.sendAll");       lua_setfield(L, -2, "sendAll");
    lua_pushcfunction(L, net_send_to_all_except,"net.sendToAllExcept"); lua_setfield(L, -2, "sendToAllExcept");
    lua_pushcfunction(L, net_disconnect,     "net.disconnect");    lua_setfield(L, -2, "disconnect");
    lua_pushcfunction(L, net_create_udp,     "net.createUDP");     lua_setfield(L, -2, "createUDP");
    lua_pushcfunction(L, net_send_udp,       "net.sendUDP");       lua_setfield(L, -2, "sendUDP");
    lua_pushcfunction(L, net_broadcast_udp,  "net.broadcastUDP");  lua_setfield(L, -2, "broadcastUDP");
    lua_pushcfunction(L, net_http_get,       "net.httpGet");       lua_setfield(L, -2, "httpGet");
    lua_pushcfunction(L, net_http_post,      "net.httpPost");      lua_setfield(L, -2, "httpPost");
    lua_pushcfunction(L, net_close,          "net.close");         lua_setfield(L, -2, "close");
    lua_pushcfunction(L, net_get_peer_count, "net.getPeerCount");  lua_setfield(L, -2, "getPeerCount");
    lua_pushcfunction(L, net_get_peer_ip,    "net.getPeerIP");     lua_setfield(L, -2, "getPeerIP");
    lua_pushcfunction(L, net_set_peer_data,  "net.setPeerData");   lua_setfield(L, -2, "setPeerData");
    lua_pushcfunction(L, net_get_peer_data,  "net.getPeerData");   lua_setfield(L, -2, "getPeerData");
    lua_pushcfunction(L, net_get_stats,      "net.getStats");      lua_setfield(L, -2, "getStats");
    lua_pushcfunction(L, net_set_keepalive,  "net.setKeepalive");  lua_setfield(L, -2, "setKeepalive");
    lua_setglobal(L, "net");

    // Register tilemap as a table: tilemap.create, tilemap.draw, tilemap.setTile, etc.
    lua_newtable(L);
    lua_pushcfunction(L, tilemap_create,    "tilemap.create");   lua_setfield(L, -2, "create");
    lua_pushcfunction(L, tilemap_draw,      "tilemap.draw");     lua_setfield(L, -2, "draw");
    lua_pushcfunction(L, tilemap_set_tile,  "tilemap.setTile");  lua_setfield(L, -2, "setTile");
    lua_pushcfunction(L, tilemap_get_tile,  "tilemap.getTile");  lua_setfield(L, -2, "getTile");
    lua_pushcfunction(L, tilemap_fill,      "tilemap.fill");     lua_setfield(L, -2, "fill");
    lua_setglobal(L, "tilemap");

    // Register particle as a table: particle.create, particle.update, particle.draw, etc.
    lua_newtable(L);
    lua_pushcfunction(L, particle_create,        "particle.create");        lua_setfield(L, -2, "create");
    lua_pushcfunction(L, particle_update,        "particle.update");        lua_setfield(L, -2, "update");
    lua_pushcfunction(L, particle_draw,          "particle.draw");          lua_setfield(L, -2, "draw");
    lua_pushcfunction(L, particle_destroy,       "particle.destroy");       lua_setfield(L, -2, "destroy");
    lua_pushcfunction(L, particle_burst,         "particle.burst");         lua_setfield(L, -2, "burst");
    lua_pushcfunction(L, particle_set_position,  "particle.setPosition");   lua_setfield(L, -2, "setPosition");
    lua_pushcfunction(L, particle_set_emitting,  "particle.setEmitting");   lua_setfield(L, -2, "setEmitting");
    lua_pushcfunction(L, particle_set_rate,      "particle.setRate");       lua_setfield(L, -2, "setRate");
    lua_pushcfunction(L, particle_get_count,     "particle.getCount");      lua_setfield(L, -2, "getCount");
    lua_pushcfunction(L, particle_clear_all,     "particle.clearAll");      lua_setfield(L, -2, "clearAll");
    lua_pushcfunction(L, particle_set_gravity,   "particle.setGravity");    lua_setfield(L, -2, "setGravity");
    lua_pushcfunction(L, particle_set_wind,      "particle.setWind");       lua_setfield(L, -2, "setWind");
    lua_pushcfunction(L, particle_set_bounce,    "particle.setBounce");     lua_setfield(L, -2, "setBounce");
    lua_pushcfunction(L, particle_set_lifetime,  "particle.setLifetime");   lua_setfield(L, -2, "setLifetime");
    lua_pushcfunction(L, particle_register_preset,"particle.registerPreset");lua_setfield(L, -2, "registerPreset");
    lua_pushcfunction(L, particle_get_config,    "particle.getConfig");     lua_setfield(L, -2, "getConfig");
    lua_setglobal(L, "particle");
}

void cleanup_lua_audio() {
    if (g_audio) {
        delete g_audio;
        g_audio = nullptr;
    }
}
