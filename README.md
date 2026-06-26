# Like2D Engine

A lightweight 2D game engine for Windows and Linux. Write your game in **Luau**, run it with `Like2D.exe`, ship it as a standalone `.exe` with the built-in LikeC compiler.

## Features

- **Window** – Resize, fullscreen, title, letterbox, logical resolution
- **Input** – Keyboard (down/just-pressed/just-released) and mouse (pos, buttons, scroll)
- **Rendering** – Images, sprite sheets with sub-rectangles, primitives (rect/line/circle), text (TTF), MPEG-1 video
- **Animation** – Built-in sprite-sheet animation system with full playback control
- **Camera** – Position, zoom, world-to-screen coordinate conversion
- **Physics** – Box2D v3: dynamic/static/kinematic bodies, box/circle/capsule shapes, forces, impulses, gravity
- **Audio** – miniaudio: load/play/pause/stop/resume, volume, pitch, master volume
- **JSON** – Save/load Lua tables as JSON files, encode/decode
- **Filesystem** – Read/write files, list directories, create/delete, userdata folder
- **Encryption** – XOR-based file encryption/decryption with L2DE header
- **Networking** – TCP server/client, UDP, HTTP GET/POST, LAN broadcast, keepalive, peer stats
- **Particles** – GPU-friendly particle system with presets, emitters, bounce, wind, textures
- **Tilemaps** – Grid-based tilemap system with sprite sheets
- **Scripting** – Full Luau runtime with `dofile`, hot reload
- **Compiler (LikeC)** – Package your game into a standalone `.exe` with assets, icon, encryption
- **Hot Reload** – Automatically reload `main.luau` on save during development
- **Cross-Platform** – Windows (MinGW/MSVC) and Linux via CMake

---

## Quick Start

```
Like2D.exe path/to/your/project/
```

Or drag your project folder onto `Like2D.exe` in Explorer.

Your project folder **must** contain `main.luau`. That is the only required file.

For the compiler: `Like2D.exe compile path/to/your/project/`

## Build Instructions

See [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) for detailed build instructions.

### Quick Build (Windows - MinGW)
```
build.bat
```
Downloads SDL3, Luau, Box2D automatically via FetchContent.

### Full Build (CMake)
```
build-full.bat          # Windows
mkdir build && cd build && cmake .. && make   # Linux
```

### Dependencies

The engine uses FetchContent for SDL3, Luau, and Box2D. Other dependencies (miniaudio, miniz, stb, nlohmann/json, pl_mpeg) are header-only and included in `external/`:
- `external/miniaudio/`
- `external/miniz/`
- `external/stb/`
- `external/nlohmann/`
- `external/pl_mpeg/`
- `external/imgui/` (optional)

---

## Project Structure

```
my_game/
├── main.luau              ← REQUIRED entry point
├── game.ico               ← optional icon (injected on compile)
├── assets/
│   ├── player.png         ← images loaded with loadImage("assets/player.png")
│   ├── jump.wav           ← audio loaded with loadSound("jump", "assets/jump.wav")
│   ├── font.ttf           ← fonts loaded with loadFont("assets/font.ttf", 24)
│   └── walk.png           ← sprite sheets for animation
├── data/
│   └── level.json         ← JSON data files
└── utils.luau             ← split code with dofile("utils.luau")
```

**Rules:**
- All paths are **relative to the project folder** (not the exe)
- In compiled mode (`game.like`) paths still work the same way
- Never use absolute paths like `C:/Users/...` — they break in compiled builds

---

## Callbacks

Define these **global functions** in `main.luau`. The engine calls them automatically.

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
end

function onDraw()
    -- Called every frame AFTER onUpdate.
    -- Draw everything here. Start with clearScreen().
    clearScreen(20, 20, 30)
end

function onCleanup()
    -- Called once when the window closes. Save data here.
end

-- ── Input Callbacks ──

function onKeyPressed(key)
    -- key = string, e.g. "Space", "A", "Escape"
end

function onKeyReleased(key)
end

function onMousePressed(btn, x, y)
    -- btn: 0=left, 1=right, 2=middle
end

function onMouseReleased(btn, x, y)
end

function onMouseScroll(dx, dy)
    -- dy > 0 = scroll up, dy < 0 = scroll down
end

function onResize(w, h)
    -- Called when the window is resized
end

-- ── Network Callbacks ──

function onNetConnect(peerId, ip, port)
    -- A new TCP client connected (server mode)
end

function onNetMessage(peerId, data, ip, port)
    -- Received data from a peer
end

function onNetDisconnect(peerId, reason)
    -- A peer disconnected
end
```

---

## Window

```lua
setWindowTitle(title)           -- Set title bar text
setWindowSize(w, h)             -- Resize window (call in onInit)
getWindowSize() -> w, h         -- Get current window size in pixels
setFullscreen(bool)             -- Toggle fullscreen
setEscapeQuits(bool)            -- Default: true. Set to false to handle Escape yourself
setLogicalSize(w, h)            -- Set virtual resolution for letterboxing
getLogicalSize() -> w, h        -- Get logical resolution
setUseLetterbox(bool)           -- Enable/disable letterbox pillarboxing
setLetterboxColor(r, g, b)      -- Letterbox bars color (0-255 each)
```

---

## Time

```lua
getTime() -> milliseconds       -- Time since engine start (integer)
getDeltaTime() -> seconds       -- Time since last frame (float, capped at 0.1s)
```

---

## Input — Keyboard

```lua
isKeyDown(key) -> bool          -- True while key is held
isKeyJustPressed(key) -> bool   -- True only on first frame of press
isKeyJustReleased(key) -> bool  -- True only on frame of release
```

See [Key Names Reference](#key-names-reference) at the bottom for all valid key strings.

---

## Input — Mouse

```lua
getMousePos() -> x, y           -- Current cursor position (floats)
isMousePressed(button) -> bool  -- 0=left, 1=right, 2=middle
getMouseScroll() -> dx, dy      -- Scroll delta this frame
```

---

## Rendering — Screen

```lua
clearScreen(r, g, b)            -- Fill screen with color (0-255 each). MUST be first in onDraw.
```

---

## Rendering — Images

```lua
loadImage(path) -> bool                 -- Load image into cache. Call in onInit only.
renderImage(key, x, y, w, h) -> bool    -- Draw image at (x, y) with size (w, h)
renderImageEx(key, x, y, w, h, angle, flipH, flipV, alpha, ox, oy) -> bool
getImageSize(key) -> w, h               -- Get original pixel dimensions
```

`renderImageEx` parameters:
- `angle`: rotation in degrees (clockwise)
- `flipH`/`flipV`: mirror horizontally/vertically
- `alpha`: transparency 0.0–1.0
- `ox`/`oy`: pivot point as fraction 0.0–1.0 (0.5, 0.5 = center)

---

## Rendering — Sprite Sheets

```lua
renderImageRegion(key, dx, dy, dw, dh, srcX, srcY, srcW, srcH, angle, flipH, flipV, alpha, ox, oy) -> bool
```

**Argument order: DESTINATION first, then SOURCE.**

| Param | Description |
|-------|-------------|
| `key` | Image path (same as loadImage) |
| `dx, dy, dw, dh` | Destination rectangle on screen |
| `srcX, srcY, srcW, srcH` | Source rectangle in the texture (pixels) |
| `angle, flipH, flipV, alpha, ox, oy` | Same as renderImageEx (optional) |

---

## Rendering — Primitives

```lua
drawRect(x, y, w, h, r, g, b, a, filled, angle)     -- Rectangle (a=255 default, filled=true default)
drawLine(x1, y1, x2, y2, r, g, b, a)                  -- Line (a=255 default)
drawCircle(cx, cy, radius, r, g, b, a, filled)        -- Circle (a=255 default, filled=true default)
```

---

## Rendering — Text

```lua
loadFont(path, size) -> bool                       -- Load a TTF font
drawText(text, fontKey, x, y, r, g, b) -> bool     -- Draw text with a loaded font
drawTextDirect(text, x, y, size, r, g, b, a, anchor) -> bool   -- Draw text directly (no fontKey needed)
getTextSize(text, size) -> w, h                    -- Get text dimensions
setDefaultFont(path)                                -- Set default font for drawTextDirect
```

`drawTextDirect` anchor values: 0=default, or see SDL_ttf anchor constants.

---

## Rendering — Video (MPEG-1)

```lua
loadVideo(key, path) -> bool                -- Load an MPEG-1 video file
updateVideo(key, dt)                        -- Advance video playback
renderVideo(key, x, y, w, h) -> bool        -- Draw current video frame
playVideo(key, loop)                        -- Start playback
pauseVideo(key)                             -- Pause playback
stopVideo(key)                              -- Stop and reset
seekVideo(key, seconds)                     -- Seek to position
isVideoPlaying(key) -> bool                 -- Check if playing
isVideoEnded(key) -> bool                   -- Check if ended
getVideoDuration(key) -> seconds            -- Total duration
getVideoTime(key) -> seconds                -- Current playback position
unloadVideo(key)                            -- Free video resources
```

---

## Animation System

```lua
loadAnimation(filename, frameWidth, frameHeight, frameCount, frameDuration) -> animation
```

Creates an animation object from a horizontal sprite strip. `loadAnimation` automatically calls `loadImage` for you.

### Animation Methods

| Method | Description |
|--------|-------------|
| `anim:update(dt)` | Advance the timer (call in onUpdate) |
| `anim:draw(x, y, w, h)` | Draw current frame (w/h default to frame size) |
| `anim:drawEx(x, y, w, h, angle, flipH, flipV, alpha, ox, oy)` | Draw with full transforms |
| `anim:play()` | Start/resume playback |
| `anim:pause()` | Pause at current frame |
| `anim:resume()` | Resume after pausing |
| `anim:stop()` | Stop and reset to frame 0 |
| `anim:reset()` | Same as stop() |
| `anim:setFrame(n)` | Jump to frame n (0-indexed) |
| `anim:isPlaying()` | Returns true if playing |
| `anim:isFinished()` | Returns true if non-looping anim ended |
| `anim:getCurrentFrame()` | Returns current frame index |
| `anim:getFrameCount()` | Returns total frame count |
| `anim:setLooping(bool)` | Enable/disable looping (default: true) |
| `anim:setSpeed(multiplier)` | Playback speed (1=normal, 2=double) |
| `anim:setPingPong(bool)` | Forward→backward→forward mode |
| `anim:setReverse(bool)` | Play in reverse |
| `anim:setAlpha(alpha)` | Default opacity for drawing (0–1) |
| `anim:setDuration(seconds)` | Change per-frame duration at runtime |

---

## Camera

```lua
setCameraPosition(x, y)             -- Move camera to world position
getCameraPosition() -> x, y         -- Get camera position
setCameraZoom(zoom)                 -- Set zoom level (1.0 = normal)
getCameraZoom() -> zoom             -- Get current zoom
worldToScreen(wx, wy) -> sx, sy     -- Convert world coords to screen coords
```

---

## Physics (Box2D v3)

All positions and velocities are in **pixels** and **pixels/second**. The engine automatically converts to/from Box2D's meter scale (32 pixels = 1 meter).

### Bodies

```lua
createBody(x, y, type, fixedRotation) -> bodyId     -- type: "dynamic" (default), "static", "kinematic"
destroyBody(bodyId) -> bool                          -- Remove a body
setBodyPosition(bodyId, x, y)                        -- Teleport body
getBodyPosition(bodyId) -> x, y                      -- Get body position (or nil if invalid)
getPhysicsTransform(bodyId) -> {x, y, angle}         -- Get position and rotation as a table
setBodyVelocity(bodyId, vx, vy)                      -- Set linear velocity (pixels/s)
getBodyVelocity(bodyId) -> vx, vy                    -- Get linear velocity (pixels/s)
applyForce(bodyId, fx, fy)                           -- Apply force at center (pixels/s²)
applyImpulse(bodyId, ix, iy)                         -- Apply impulse at center (pixels/s)
setGravity(x, y)                                     -- Set world gravity (pixels/s²)
```

### Shapes

```lua
addBoxShape(bodyId, halfWidth, halfHeight, density, friction, restitution, isSensor) -> shapeId
addCircleShape(bodyId, radius, density, friction, restitution, isSensor) -> shapeId
addCapsuleShape(bodyId, halfHeight, radius, density, friction) -> shapeId
```

---

## Audio

```lua
loadSound(key, filepath) -> bool                -- Load an audio file (WAV, OGG, MP3, FLAC)
playSound(key, loop) -> bool                    -- Play a loaded sound
stopSound(key) -> bool                          -- Stop playback
pauseSound(key) -> bool                         -- Pause playback
resumeSound(key) -> bool                        -- Resume playback
isSoundPlaying(key) -> bool                     -- Check if currently playing
setSoundVolume(key, volume)                     -- Set per-sound volume (0.0–1.0)
setMasterVolume(volume)                         -- Set master volume (0.0–1.0)
setSoundPitch(key, pitch)                       -- Set pitch multiplier (1.0 = normal)
unloadSound(key)                                -- Unload from memory
```

---

## JSON

```lua
json.load(filename) -> data, error              -- Load JSON file → Lua table
json.save(filename, table) -> ok, error          -- Save Lua table → JSON file
json.encode(table) -> string                    -- Encode Lua table to JSON string
json.decode(string) -> data, error              -- Decode JSON string to Lua table
```

Supports nested tables, arrays, numbers, strings, booleans, and nil.

---

## Filesystem

```lua
fileExists(path) -> bool                        -- Check if file exists (with caching)
readFile(path) -> content, error                -- Read text/binary file
writeFile(path, content) -> ok, error           -- Write file to userdata folder
listDirectory(path) -> {filenames}              -- List files/folders in a directory
createDirectory(path) -> ok, error              -- Create folder in userdata
deleteFile(path) -> ok, error                   -- Delete file in userdata
getUserdataPath() -> path                       -- Get path to userdata folder (exe's dir/userdata/)
getGamePath() -> path                           -- Get path to game executable directory
```

`writeFile`, `createDirectory`, `deleteFile` operate inside the `userdata/` folder (next to the exe) for security.

---

## Encryption

```lua
encryptFile(path, inPlace) -> ok, error         -- XOR-encrypt file with L2DE header
decryptFile(path) -> ok, error                  -- Decrypt a file (removes encryption)
readEncryptedFile(path) -> content, error       -- Read and decrypt a file in memory
writeEncryptedFile(path, data) -> ok, error     -- Encrypt and write data to file
```

Encryption uses XOR with a 16-byte key and prepends an `L2DE` magic header.

---

## Networking

### TCP

```lua
net.createServer(port) -> bool                  -- Start TCP server
net.connect(host, port) -> peerId               -- Connect to TCP server
net.send(peerId, data) -> bool                  -- Send data to a specific peer
net.sendAll(data) -> bool                       -- Broadcast to all connected peers
net.sendToAllExcept(excludePeerId, data) -> bool-- Send to all except one
net.disconnect(peerId, reason)                  -- Disconnect a peer
```

### UDP

```lua
net.createUDP(port) -> bool                     -- Open UDP socket
net.sendUDP(ip, port, data) -> bool             -- Send UDP packet
net.broadcastUDP(port, data) -> bool            -- LAN broadcast
```

### HTTP

```lua
net.httpGet(url) -> {status, body, error}       -- HTTP GET request
net.httpPost(url, body, contentType) -> {status, body, error}
```

### Management

```lua
net.close()                                     -- Shutdown all networking
net.getPeerCount() -> count                     -- Number of connected peers
net.getPeerIP(peerId) -> ip                     -- Get peer's IP address
net.setPeerData(peerId, key, value)             -- Set peer metadata
net.getPeerData(peerId, key) -> value           -- Get peer metadata
net.getStats() -> {bytesSent, bytesReceived, packetsSent, packetsRecv, peerCount, uptime}
net.setKeepalive(interval, timeout)             -- Set TCP keepalive
```

### Network Callbacks

```lua
function onNetConnect(peerId, ip, port)         -- New connection
function onNetMessage(peerId, data, ip, port)   -- Data received
function onNetDisconnect(peerId, reason)        -- Peer disconnected
```

---

## Tilemaps

```lua
tilemap.create(tilesetKey, tileW, tileH, cols, rows) -> map
    -- Create a tilemap. tilesetKey must be loaded with loadImage() first.
    -- Returns a table: {tileset, tileW, tileH, cols, rows, tiles={...}}

tilemap.draw(map, x, y, alpha) -> bool          -- Draw the tilemap at position
tilemap.setTile(map, row, col, tileId) -> bool  -- Set a tile (0 = empty)
tilemap.getTile(map, row, col) -> tileId        -- Get tile at position
tilemap.fill(map, startRow, startCol, w, h, tileId) -> bool  -- Fill a rectangular area
```

Tile IDs are 1-based indices into the sprite sheet (1 = first tile at top-left). The tiles are indexed row-first: `tileId = row * cols + col + 1`.

---

## Particles

```lua
particle.create(configTable) -> particleId
    -- Create a particle system. See config keys below.

particle.update(id, dt)                         -- Update particles
particle.draw(id)                               -- Render particles
particle.destroy(id)                            -- Destroy particle system
particle.burst(id, count)                       -- Burst-emit N particles instantly
particle.setPosition(id, x, y)                  -- Move emitter
particle.setEmitting(id, bool)                  -- Enable/disable emission
particle.setRate(id, rate)                      -- Change emission rate
particle.setGravity(id, gravity)                -- Change particle gravity
particle.setWind(id, windX, windY)              -- Change wind force
particle.setBounce(id, enabled, bounceY, damping)  -- Toggle floor bounce
particle.setLifetime(id, lifetime)              -- Change particle lifetime
particle.getCount(id) -> count                  -- Number of alive particles
particle.getConfig(id) -> configTable           -- Get current config
particle.clearAll()                             -- Destroy all particle systems
particle.registerPreset(name, configFn)         -- Register a custom preset function
```

### Particle Config Table

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `x, y` | number | 0 | Emitter position |
| `rate` | number | 50 | Particles emitted per second |
| `lifetime` | number | 2.0 | Base particle lifetime (seconds) |
| `speedMin, speedMax` | number | 20, 80 | Spawn speed range |
| `angle` | number | -90 | Emission direction (degrees, -90 = up) |
| `spread` | number | 180 | Emission spread (degrees) |
| `gravity` | number | 50 | Downward acceleration |
| `drag` | number | 0 | Air resistance (0 = none) |
| `startSize, endSize` | number | 8, 2 | Size at birth/death |
| `sizeVariation` | number | 0 | Random size variation (± fraction) |
| `colorVariation` | number | 0 | Random color variation |
| `lifetimeVariation` | number | 0 | Random lifetime variation (± fraction) |
| `startColor` | table | {255,204,51,255} | RGBA 0-255 at birth |
| `endColor` | table | {255,51,0,0} | RGBA 0-255 at death |
| `maxParticles` | number | 5000 | Maximum alive particles |
| `shape` | string | "square" | Particle shape: "square", "circle", "ring" |
| `emitShape` | string | "point" | Emitter shape: "point", "line", "circle", "rect" |
| `emitW, emitH` | number | 0 | Emitter dimensions (for line/rect) |
| `emitRadius` | number | 0 | Emitter radius (for circle) |
| `additive` | bool | false | Additive blending (glow effect) |
| `emitting` | bool | true | Whether particles are being emitted |
| `texture` | string | "" | Optional texture key (loaded with loadImage) |
| `windX, windY` | number | 0 | Constant wind force |
| `followVelocity` | bool | false | Particles rotate to face movement |
| `relative` | bool | false | Particles move with emitter |
| `bounce` | bool | false | Particles bounce off floor |
| `bounceY` | number | 0 | Floor Y position |
| `bounceDamping` | number | 0.5 | Energy retained on bounce |
| `rotSpeedMin, rotSpeedMax` | number | -180, 180 | Rotation speed range |
| `preset` | string | "" | Apply preset: "explosion", "fire", "smoke", "rain", "snow", "magic", "sparkle", "confetti" |

---

## Compiler (LikeC)

Package your game into a standalone executable:

```
Like2D.exe compile path/to/project/ [--linux] [--windows]
```

The compiler:
1. Scans the project folder for all required files
2. Packages them into a zip archive (optionally encrypted)
3. Fuses the archive with a copy of Like2D.exe
4. Injects a custom icon from `game.ico`
5. Applies version info from `version.txt`

The output is a self-contained `.exe` that runs without any dependencies.

---

## Hot Reload

In development mode (no `game.like` present), the engine watches `main.luau` for changes. When you save, the engine:
1. Re-initializes the Lua state
2. Calls all `onInit` functions fresh
3. Continues the game loop seamlessly

Files loaded via `dofile()` are also watched for changes.

---

## Modules / dofile

Split your code across multiple files:

```lua
dofile("utils.luau")        -- Load and execute another Luau file
dofile("data/config.luau")  -- Paths are relative to project folder
```

---

## Common Errors & Fixes

| Error | Likely Cause | Fix |
|-------|-------------|-----|
| `missing argument #8` | Wrong argument count in renderImageRegion | Remember: destination first, then source |
| Nothing drawn | Key mismatch | Key must exactly match the path in loadImage |
| Extreme lag | loadImage called in onDraw | Move to onInit |
| Game crashes on start | Missing main.luau | Ensure main.luau exists in the project folder |
| Smearing effect | No clearScreen in onDraw | Add clearScreen() as first line in onDraw |
| Physics body not moving | Called createBody with wrong type | Use "dynamic" (default) for moving bodies |
| Sound not playing | Audio file format unsupported | Use WAV or OGG Vorbis |
| JSON error | Table has cycles or functions | Only serialize plain data |

---

## Key Names Reference

```
A-Z, 0-9
Space, Enter, Escape, Tab, Backspace, Delete
Up, Down, Left, Right
LShift, RShift, Shift
LCtrl, RCtrl, Ctrl
LAlt, RAlt, Alt
F1-F12
Home, End, PageUp, PageDown, Insert
Numpad0-Numpad9
```

---

## Full Example

See [docs/TUTORIALS.md](docs/TUTORIALS.md) for 8 step-by-step tutorials from Hello World to a complete mini-game.

```lua
-- main.luau — Complete working example
local player = {}
local walkAnim, idleAnim
local currentAnim

function onInit()
    setWindowTitle("Like2D Demo")
    setWindowSize(800, 600)
    setLogicalSize(800, 600)
    setUseLetterbox(true)
    setLetterboxColor(20, 20, 30)

    loadImage("assets/player.png")
    loadImage("assets/bg.png")
    loadSound("bgm", "assets/bgm.ogg")
    loadFont("assets/font.ttf", 24)
    setDefaultFont("assets/font.ttf")

    walkAnim = loadAnimation("assets/walk.png", 64, 64, 4, 0.1)
    idleAnim = loadAnimation("assets/idle.png", 64, 64, 2, 0.5)
    currentAnim = idleAnim

    player = { x = 400, y = 300, speed = 200 }
    playSound("bgm", true)
end

function onUpdate(dt)
    local dx, dy = 0, 0
    if isKeyDown("Left")  then dx = dx - 1 end
    if isKeyDown("Right") then dx = dx + 1 end
    if isKeyDown("Up")    then dy = dy - 1 end
    if isKeyDown("Down")  then dy = dy + 1 end

    player.x = player.x + dx * player.speed * dt
    player.y = player.y + dy * player.speed * dt

    if dx ~= 0 or dy ~= 0 then
        currentAnim = walkAnim
        if not walkAnim:isPlaying() then walkAnim:play() end
    else
        currentAnim = idleAnim
    end

    currentAnim:update(dt)
end

function onDraw()
    clearScreen(20, 20, 30)
    renderImage("assets/bg.png", 0, 0, 800, 600)
    currentAnim:draw(player.x, player.y, 64, 64)
    drawTextDirect("X: " .. math.floor(player.x) .. " Y: " .. math.floor(player.y), 10, 10, 20, 255, 255, 255)
end

function onKeyPressed(key)
    if key == "Space" then
        print("Jump!")
    end
end

function onCleanup()
    json.save("save.json", { x = player.x, y = player.y })
end
```
