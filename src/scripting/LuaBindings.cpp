#include "LuaBindings.h"
#include "core/Engine.h"
#include "renderer/Renderer.h"
#include "audio/AudioManager.h"
#include "../platform.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <box2d/box2d.h>
#include "../../external/nlohmann/json.hpp"
#include "../../external/miniz/miniz.h"

using json = nlohmann::json;

static Engine*       g_engine   = nullptr;
static Renderer*     g_renderer = nullptr;
static AudioManager* g_audio    = nullptr;

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
    lua_pushboolean(L, g_renderer && g_renderer->drawTextDirect(text, x, y, size, r, g, b, a));
    return 1;
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

// Resolve a filename to full path (dev mode) or extract from game.like (production)
static std::string resolveFileRead(const std::string& filename, std::string& outContent, bool& fromArchive) {
    fromArchive = false;

    std::filesystem::path exePath     = getExeDir();
    std::filesystem::path selfPath    = getExePath(); // Path to the binary itself
    std::filesystem::path gameLikePath = exePath / "game.like";

    // 1. Try to load from "Fused" mode (binary + archive)
    bool isFused = false;
    std::vector<char> zipData;

    // Check if the binary itself contains a valid ZIP at the end
    if (readGameLikeBytes(selfPath, zipData)) {
        // Test if this is actually a ZIP (could just be a plain binary with no data)
        mz_zip_archive zip_test = {};
        if (mz_zip_reader_init_mem(&zip_test, zipData.data(), zipData.size(), 0)) {
            mz_zip_reader_end(&zip_test);
            isFused = true;
        }
    }

    // 2. If not fused, try external game.like
    if (!isFused) {
        if (std::filesystem::exists(gameLikePath)) {
            if (!readGameLikeBytes(gameLikePath, zipData))
                return "Failed to read game.like";
        } else {
            // Fallback to dev mode (normal files)
            goto dev_mode;
        }
    }

    // Load assets from zipData (either fused or from game.like)
    {
        mz_zip_archive zip = {};
        if (!mz_zip_reader_init_mem(&zip, zipData.data(), zipData.size(), 0))
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
                return "";
            }
        }
        mz_zip_reader_end(&zip);
        return "File not found in game.like: " + filename;
    }

    dev_mode:
    // Dev mode: read from disk relative to project folder
    if (g_renderer) {
        std::filesystem::path full = std::filesystem::path(g_renderer->getProjectFolder()) / filename;
        std::ifstream f(full);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf();
            outContent = ss.str();
            return "";
        }
    }
    // Fallback: next to exe, then absolute/cwd
    {
        std::filesystem::path exeFull = getExeDir() / std::filesystem::path(filename).filename();
        std::ifstream f(exeFull);
        if (f.is_open()) {
            std::ostringstream ss; ss << f.rdbuf();
            outContent = ss.str();
            return "";
        }
    }
    std::ifstream f(filename);
    if (f.is_open()) {
        std::ostringstream ss; ss << f.rdbuf();
        outContent = ss.str();
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
//  REGISTRATION
// ============================================================
void register_lua_functions(lua_State* L, Engine* engine, Renderer* renderer) {
    g_engine   = engine;
    g_renderer = renderer;

    // Create and initialize AudioManager
    g_audio = new AudioManager();
    if (!g_audio->initialize()) {
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
}
