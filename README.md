# Like2D Engine — Complete Reference

A lightweight 2D game engine for Windows. Write your game in **Luau**, run it with `Like2D.exe`, ship it as a standalone `.exe` with the built-in compiler.

> **Read this fully before coding.** Most errors come from wrong argument order, wrong types, or calling functions at the wrong time.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Project Structure](#project-structure)
3. [Callbacks](#callbacks)
4. [Window](#window)
5. [Time](#time)
6. [Input — Keyboard](#input--keyboard)
7. [Input — Mouse](#input--mouse)
8. [Rendering — Screen](#rendering--screen)
9. [Rendering — Images](#rendering--images)
10. [Rendering — Sprite Sheets](#rendering--sprite-sheets)
11. [Rendering — Primitives](#rendering--primitives)
12. [Rendering — Text](#rendering--text)
13. [Camera](#camera)
14. [Physics](#physics)
15. [Audio](#audio)
16. [JSON Save/Load](#json-saveload)
17. [Video (MPEG-1)](#video-mpeg-1)
18. [Misc](#misc)
19. [Compiling](#compiling)
20. [Common Errors & Fixes](#common-errors--fixes)
21. [Key Names Reference](#key-names-reference)
22. [Full Example](#full-example)

---

## Quick Start

```
Like2D.exe path/to/your/project/
```

Or drag your project folder onto `Like2D.exe` in Explorer.

Your project folder **must** contain `main.luau`. That is the only required file.

---

## Project Structure

```
my_game/
├── main.luau          ← REQUIRED entry point
├── game.ico           ← optional icon (injected on compile)
├── assets/
│   ├── player.png     ← images loaded with loadImage("assets/player.png")
│   ├── jump.wav       ← audio loaded with loadSound("jump", "assets/jump.wav")
│   └── font.ttf       ← fonts loaded with loadFont("assets/font.ttf", 24)
└── utils.luau         ← split code with dofile("utils.luau")
```

**Rules:**
- All paths are **relative to the project folder** (not the exe).
- In compiled mode (`game.like`) paths still work the same way.
- Never use absolute paths like `C:/Users/...` — they break in compiled builds.

---

## Callbacks

Define these **global functions** in `main.luau`. The engine calls them automatically. You do not call them yourself.

```lua
function onInit()
    -- Called ONCE at startup, before the first frame.
    -- Load all assets here. NEVER load assets in onDraw or onUpdate.
    setWindowTitle("My Game")
    setWindowSize(800, 600)
    loadImage("assets/player.png")
    loadSound("jump", "assets/jump.wav")
end

function onUpdate(dt)
    -- Called every frame BEFORE drawing.
    -- dt = delta time in seconds (e.g. 0.016 at 60fps).
    -- ALWAYS multiply movement/speed by dt for frame-rate independence.
    -- Example: x = x + speed * dt
end

function onDraw()
    -- Called every frame AFTER onUpdate.
    -- Draw everything here. Start with clearScreen().
    -- Do NOT load assets here — only draw.
    clearScreen(20, 20, 30)
end

function onCleanup()
    -- Called once when the window closes.
    -- Save data here if needed.
end

-- ── Optional callbacks ────────────────────────────────────────

function onKeyPressed(key)
    -- Fires once when a key is first pressed down.
    -- key = string, e.g. "Space", "A", "Escape"
    -- Use this for single-press actions (jump, shoot, menu select).
    -- For held movement, use isKeyDown() in onUpdate instead.
end

function onKeyReleased(key)
    -- Fires once when a key is released.
end

function onMousePressed(btn, x, y)
    -- btn: 0=left, 1=right, 2=middle
    -- x, y: cursor position in pixels at time of click
end

function onMouseReleased(btn, x, y)
    -- Same args as onMousePressed
end

function onMouseScroll(dx, dy)
    -- dy > 0 = scroll up, dy < 0 = scroll down
end

function onResize(w, h)
    -- Called when the window is resized.
end
```

> ⚠️ **Common mistake:** Calling `loadImage` inside `onDraw` every frame. This is extremely slow and will cause stuttering. Always load in `onInit`.

---

## Window

```lua
setWindowTitle(title: string)
-- Sets the title bar text.
-- Call in onInit or whenever you want to update it.

setWindowSize(width: number, height: number)
-- Resizes the window. Call in onInit.
-- Example: setWindowSize(1280, 720)

setFullscreen(fullscreen: bool)
-- true = fullscreen, false = windowed.

setEscapeQuits(enabled: bool)
-- Default: true. Set to false if you want to handle Escape yourself.
-- If false, pressing Escape does nothing unless you call quit() manually.

getWindowSize() -> width: number, height: number
-- Returns current window size in pixels.
-- Example: local W, H = getWindowSize()
```

---

## Time

```lua
getTime() -> milliseconds: number
-- Time since engine started, in milliseconds (integer).
-- Useful for animations: math.floor(getTime() / 500) % 2 gives a 0/1 blink.

getDeltaTime() -> seconds: number
-- Time since the last frame, in seconds (float).
-- Capped at 0.1s to prevent physics explosions on lag spikes.
-- This is the same value passed to onUpdate(dt).
-- Use this when you need dt outside of onUpdate.
```

> ⚠️ **Common mistake:** Using `getTime()` for movement instead of `dt`.
> `getTime()` grows forever — use `dt` for movement, `getTime()` for timers/animations.

---

## Input — Keyboard

```lua
isKeyDown(key: string) -> bool
-- Returns true every frame the key is held down.
-- Use in onUpdate for continuous movement.
-- Example: if isKeyDown("Right") then x = x + speed * dt end

isKeyJustPressed(key: string) -> bool
-- Returns true only on the FIRST frame the key is pressed.
-- Use in onUpdate for single-press actions (alternative to onKeyPressed).
-- Example: if isKeyJustPressed("Space") then jump() end

isKeyJustReleased(key: string) -> bool
-- Returns true only on the frame the key is released.
```

**When to use which:**

| Situation | Use |
|---|---|
| Player walking (held) | `isKeyDown` in `onUpdate` |
| Jump (single press) | `onKeyPressed` callback OR `isKeyJustPressed` in `onUpdate` |
| Menu navigation | `onKeyPressed` callback |
| Charge attack (hold duration) | `isKeyDown` + timer in `onUpdate` |

> ⚠️ **Common mistake:** Using `onKeyPressed` for movement. This fires once per keypress, causing choppy movement. Use `isKeyDown` in `onUpdate` instead.

---

## Input — Mouse

```lua
getMousePos() -> x: number, y: number
-- Current cursor position in screen pixels.
-- Returns floats. Use math.floor() if you need integers.

isMousePressed(button: number) -> bool
-- Returns true while the button is held.
-- 0 = left, 1 = right, 2 = middle

getMouseScroll() -> dx: number, dy: number
-- Scroll delta this frame. dy > 0 = up, dy < 0 = down.
-- Only non-zero on the frame the user scrolls.
```

---

## Rendering — Screen

```lua
clearScreen(r: number, g: number, b: number)
-- Fills the entire screen with a solid colour.
-- r, g, b = 0–255 each.
-- MUST be the first call in onDraw, otherwise previous frame bleeds through.
-- Example: clearScreen(0, 0, 0)       -- black
-- Example: clearScreen(135, 206, 235) -- sky blue
```

> ⚠️ **Common mistake:** Forgetting `clearScreen` in `onDraw`. The previous frame will show through, causing a smearing effect.

---

## Rendering — Images

### Loading

```lua
loadImage(path: string) -> bool
-- Loads an image into the engine's texture cache.
-- path is relative to the project folder.
-- Returns true on success, false on failure.
-- CALL IN onInit ONLY. Never call in onDraw or onUpdate.
-- Supported formats: PNG, JPG, BMP, TGA

-- Example:
loadImage("assets/player.png")
loadImage("assets/background.jpg")
```

### Drawing

```lua
renderImage(key: string, x: number, y: number, w: number, h: number) -> bool
-- Draws a loaded image at position (x, y) with size (w, h).
-- key = the SAME path string used in loadImage().
-- x, y = top-left corner of the image on screen.
-- w, h = drawn size in pixels (can differ from original image size).
-- Returns false if the image was not loaded.

-- Example:
renderImage("assets/player.png", 100, 200, 64, 64)
```

```lua
renderImageEx(
    key: string,
    x: number, y: number,
    w: number, h: number,
    angle: number,
    flipH: bool, flipV: bool,
    alpha: number,
    originX: number, originY: number
) -> bool
-- Extended version with rotation, flip, transparency, and custom pivot.
-- angle: rotation in degrees (clockwise). 0 = no rotation.
-- flipH: true = mirror horizontally.
-- flipV: true = mirror vertically.
-- alpha: transparency 0.0 (invisible) to 1.0 (fully opaque).
-- originX, originY: pivot point as fraction 0.0–1.0.
--   (0.5, 0.5) = rotate around centre (most common).
--   (0.0, 0.0) = rotate around top-left corner.

-- Example: draw rotated 45 degrees around centre, 50% transparent:
renderImageEx("assets/coin.png", 200, 300, 32, 32, 45, false, false, 0.5, 0.5, 0.5)
```

```lua
getImageSize(key: string) -> w: number, h: number
-- Returns the original pixel dimensions of a loaded image.
-- Returns 0, 0 if the image is not loaded.
-- Useful for calculating frame sizes in sprite sheets.

-- Example:
local w, h = getImageSize("assets/spritesheet.png")
local frameW = w / 8  -- if sheet has 8 columns
```

---

## Rendering — Sprite Sheets

This is the most error-prone area. Read carefully.

```lua
renderImageRegion(
    key:    string,   -- image key (same path as loadImage)
    dx:     number,   -- destination X on screen (top-left)
    dy:     number,   -- destination Y on screen (top-left)
    dw:     number,   -- destination width  (how wide to draw on screen)
    dh:     number,   -- destination height (how tall to draw on screen)
    srcX:   number,   -- source X in the texture (pixels from left)
    srcY:   number,   -- source Y in the texture (pixels from top)
    srcW:   number,   -- source width  in the texture (one frame's width)
    srcH:   number,   -- source height in the texture (one frame's height)
    angle:  number,   -- rotation degrees (optional, default 0)
    flipH:  bool,     -- mirror horizontally (optional, default false)
    flipV:  bool,     -- mirror vertically   (optional, default false)
    alpha:  number,   -- transparency 0.0–1.0 (optional, default 1.0)
    ox:     number,   -- pivot X 0.0–1.0 (optional, default 0.5)
    oy:     number    -- pivot Y 0.0–1.0 (optional, default 0.5)
) -> bool
```

**Argument order: DESTINATION first, then SOURCE.**

This is the #1 source of errors. The screen position comes before the texture position.

### Sprite Sheet Animation Example

```lua
-- Setup (in onInit or at top of file):
local SHEET   = "assets/run.png"
local COLS    = 8        -- number of frames horizontally
local ROWS    = 1        -- number of frames vertically
local FRAME_W = 0        -- calculated below
local FRAME_H = 0

function onInit()
    loadImage(SHEET)
    local totalW, totalH = getImageSize(SHEET)
    FRAME_W = totalW / COLS
    FRAME_H = totalH / ROWS
end

-- Animation state:
local frame     = 0
local animTimer = 0
local FRAME_SPEED = 0.08  -- seconds per frame

function onUpdate(dt)
    animTimer = animTimer + dt
    if animTimer >= FRAME_SPEED then
        animTimer = 0
        frame = (frame + 1) % COLS
    end
end

function onDraw()
    clearScreen(30, 30, 40)

    local srcX = frame * FRAME_W   -- X position of current frame in sheet
    local srcY = 0                 -- Y position (row 0)

    local destX = 200              -- where to draw on screen
    local destY = 300

    -- CORRECT argument order: dest first, then src
    renderImageRegion(
        SHEET,
        destX, destY, FRAME_W, FRAME_H,   -- destination (screen)
        srcX,  srcY,  FRAME_W, FRAME_H    -- source (texture)
    )
end
```

### Common renderImageRegion Mistakes

| Mistake | Error | Fix |
|---|---|---|
| Swapping src/dest order | `missing argument #8` | Put `dx,dy,dw,dh` BEFORE `srcX,srcY,srcW,srcH` |
| Missing `srcW` or `srcH` | `missing argument #8` or `#9` | Always pass all 9 required args |
| Using total sheet size as srcW/srcH | Draws entire sheet | Use single frame size: `totalW / cols` |
| Calling `loadImage` in `onDraw` | Extreme lag | Move to `onInit` |
| Wrong key string | Returns false, nothing drawn | Key must exactly match the path in `loadImage` |

---
