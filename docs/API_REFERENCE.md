# Like2D Framework - Complete API Reference

## Overview

This document provides comprehensive documentation for all Like2D Framework API functions. All functions are available globally in Luau scripts and are called directly without any prefixes.

## Window Management Functions

### `setWindowTitle(title)`
Sets the window title text.

**Parameters:**
- `title` (string): The new window title

**Example:**
```lua
setWindowTitle("My Awesome Game")
```

### `setWindowSize(width, height)`
Sets the window dimensions in pixels.

**Parameters:**
- `width` (integer): Window width in pixels
- `height` (integer): Window height in pixels

**Example:**
```lua
setWindowSize(1024, 768)
```

### `getWindowSize()`
Gets the current window dimensions.

**Returns:**
- `width` (integer): Current window width
- `height` (integer): Current window height

**Example:**
```lua
local w, h = getWindowSize()
print("Window size: " .. w .. "x" .. h)
```

### `setFullscreen(fullscreen)`
Toggles fullscreen mode.

**Parameters:**
- `fullscreen` (boolean): true for fullscreen, false for windowed

**Example:**
```lua
setFullscreen(true)   -- Enable fullscreen
setFullscreen(false)  -- Disable fullscreen
```

### `setEscapeQuits(escapeQuits)`
Sets whether the Escape key should quit the application.

**Parameters:**
- `escapeQuits` (boolean): true to make Escape quit, false otherwise

## Time Functions

### `getTime()`
Gets the current time in milliseconds since application start.

**Returns:**
- `time` (integer): Current time in milliseconds

**Example:**
```lua
local currentTime = getTime()
local deltaTime = currentTime - lastTime
lastTime = currentTime

print("Frame time: " .. deltaTime .. "ms")
```

### `getDeltaTime()`
Gets the time elapsed since the last frame in seconds.

**Returns:**
- `deltaTime` (number): Time since last frame in seconds

## Input Functions - Keyboard

### `isKeyDown(keyName)`
Checks if a specific key is currently held down.

**Parameters:**
- `keyName` (string): Name of the key (e.g., "Escape", "Space", "Return", "Up", "Down", "Left", "Right", "a", "b", etc.)

**Returns:**
- `held` (boolean): true if key is held down, false otherwise

### `isKeyJustPressed(keyName)`
Checks if a specific key was just pressed this frame.

**Parameters:**
- `keyName` (string): Name of the key

**Returns:**
- `pressed` (boolean): true if key was just pressed, false otherwise

### `isKeyJustReleased(keyName)`
Checks if a specific key was just released this frame.

**Parameters:**
- `keyName` (string): Name of the key

**Returns:**
- `released` (boolean): true if key was just released, false otherwise

## Input Functions - Mouse

### `isMousePressed(button)`
Checks if a mouse button is currently pressed.

**Parameters:**
- `button` (integer): Mouse button (0 = left, 1 = right, 2 = middle)

**Returns:**
- `pressed` (boolean): true if button is pressed, false otherwise

### `getMousePos()`
Gets the current mouse position.

**Returns:**
- `x` (number): Mouse X position
- `y` (number): Mouse Y position

### `getMouseScroll()`
Gets the mouse scroll delta since last frame.

**Returns:**
- `dx` (number): Horizontal scroll
- `dy` (number): Vertical scroll

## Rendering Functions - Screen and Images

### `clearScreen(red, green, blue)`
Clears the screen with the specified RGB color.

**Parameters:**
- `red` (integer): Red component (0-255)
- `green` (integer): Green component (0-255)
- `blue` (integer): Blue component (0-255)

**Example:**
```lua
clearScreen(0, 0, 0)        -- Black
clearScreen(255, 255, 255)  -- White
clearScreen(100, 149, 237)  -- Cornflower blue
```

### `loadImage(filename)`
Loads an image from file. The filename is used as the key.

**Parameters:**
- `filename` (string): Path to the image file

**Returns:**
- `success` (boolean): true if loaded successfully, false otherwise

**Example:**
```lua
if loadImage("assets/player.png") then
    print("Player image loaded successfully")
else
    print("Failed to load player image")
end
```

### `renderImage(key, x, y, width, height)`
Renders a previously loaded image at the specified position.

**Parameters:**
- `key` (string): The image key (usually the filename used when loading)
- `x` (integer): X position in pixels
- `y` (integer): Y position in pixels
- `width` (integer): Render width in pixels
- `height` (integer): Render height in pixels

**Returns:**
- `success` (boolean): true if rendered successfully, false otherwise

**Example:**
```lua
renderImage("assets/player.png", 100, 200, 64, 64)
```

### `renderImageEx(key, x, y, width, height, angle, flipH, flipV, alpha, originX, originY)`
Renders an image with advanced parameters.

**Parameters:**
- `key` (string): The image key
- `x` (number): X position
- `y` (number): Y position
- `width` (number): Render width
- `height` (number): Render height
- `angle` (number): Rotation angle in degrees (optional, default = 0)
- `flipH` (boolean): Flip horizontally (optional, default = false)
- `flipV` (boolean): Flip vertically (optional, default = false)
- `alpha` (number): Opacity from 0 to 1 (optional, default = 1)
- `originX` (number): X origin as factor (0-1, 0.5 = center, optional, default = 0.5)
- `originY` (number): Y origin as factor (0-1, 0.5 = center, optional, default = 0.5)

### `renderImageRegion(key, destX, destY, destW, destH, srcX, srcY, srcW, srcH, angle, flipH, flipV, alpha, originX, originY)`
Renders a sub-rectangle of an image.

### `getImageSize(key)`
Gets the dimensions of a loaded image.

**Parameters:**
- `key` (string): The image key

**Returns:**
- `width` (integer): Image width
- `height` (integer): Image height

## Rendering Functions - Primitives

### `drawRect(x, y, w, h, r, g, b, a, filled, angle)`
Draws a rectangle.

**Parameters:**
- `x` (number): X position
- `y` (number): Y position
- `w` (number): Width
- `h` (number): Height
- `r` (integer): Red (0-255)
- `g` (integer): Green (0-255)
- `b` (integer): Blue (0-255)
- `a` (integer): Alpha (0-255, optional, default = 255)
- `filled` (boolean): Fill the rectangle (optional, default = false)
- `angle` (number): Rotation angle in degrees (optional, default = 0)

### `drawLine(x1, y1, x2, y2, r, g, b, a)`
Draws a line.

### `drawCircle(cx, cy, radius, r, g, b, a, filled)`
Draws a circle.

## Rendering Functions - Text

### `loadFont(filename, size)`
Loads a font file for text rendering. The filename is used as the key.

**Parameters:**
- `filename` (string): Path to the font file
- `size` (integer): Font size in points

**Returns:**
- `success` (boolean): true if loaded successfully, false otherwise

**Example:**
```lua
if loadFont("fonts/arial.ttf", 24) then
    print("Font loaded successfully")
end
```

### `drawText(text, fontKey, x, y, red, green, blue)`
Draws text using a loaded font.

**Parameters:**
- `text` (string): The text to render
- `fontKey` (string): The font key (usually the filename used when loading)
- `x` (integer): X position in pixels
- `y` (integer): Y position in pixels
- `red` (integer): Red color component (0-255)
- `green` (integer): Green color component (0-255)
- `blue` (integer): Blue color component (0-255)

**Returns:**
- `success` (boolean): true if rendered successfully, false otherwise

**Example:**
```lua
drawText("Hello World!", "fonts/arial.ttf", 10, 10, 255, 255, 255)
```

### `drawTextDirect(text, x, y, size, red, green, blue, alpha)`
Draws text without needing to preload a font (uses default font).

**Parameters:**
- `text` (string): The text to render
- `x` (integer): X position in pixels
- `y` (integer): Y position in pixels
- `size` (integer): Font size in points
- `red` (integer): Red color component (0-255)
- `green` (integer): Green color component (0-255)
- `blue` (integer): Blue color component (0-255)
- `alpha` (integer): Alpha component (0-255, optional, default = 255)

**Example:**
```lua
drawTextDirect("Hello World!", 10, 10, 24, 255, 255, 255, 255)
```

### `setDefaultFont(filename)`
Sets the default font to use for `drawTextDirect`.

## Camera Functions

### `setCameraPosition(x, y)`
Sets the camera position.

### `getCameraPosition()`
Gets the camera position.

### `setCameraZoom(zoom)`
Sets the camera zoom.

### `getCameraZoom()`
Gets the camera zoom.

### `worldToScreen(x, y)`
Converts world coordinates to screen coordinates.

## Physics Functions - Bodies

### `createBody(x, y, type, fixedRotation)`
Creates a physics body.

**Parameters:**
- `x` (number): Initial X position
- `y` (number): Initial Y position
- `type` (string): Body type ("dynamic", "static", "kinematic", optional, default = "dynamic")
- `fixedRotation` (boolean): Whether to lock rotation (optional, default = false)

**Returns:**
- `bodyIndex` (integer): Index of the created body

### `destroyBody(index)`
Destroys a physics body.

### `setBodyPosition(index, x, y)`
Sets a body's position.

### `getBodyPosition(index)`
Gets a body's position.

### `getPhysicsTransform(index)`
Gets a body's position and angle as a table with "x", "y", "angle" fields.

### `setBodyVelocity(index, vx, vy)`
Sets a body's velocity.

### `getBodyVelocity(index)`
Gets a body's velocity.

### `applyForce(index, fx, fy)`
Applies a force to a body.

### `applyImpulse(index, ix, iy)`
Applies an impulse to a body.

### `setGravity(x, y)`
Sets the world gravity.

## Physics Functions - Shapes

### `addBoxShape(bodyIndex, halfWidth, halfHeight, density, friction, restitution, isSensor)`
Adds a box shape to a body.

### `addCircleShape(bodyIndex, radius, density, friction, restitution, isSensor)`
Adds a circle shape to a body.

### `addCapsuleShape(bodyIndex, halfHeight, radius, density, friction)`
Adds a capsule shape to a body.

## Audio Functions

### `loadSound(key, filename)`
Loads a sound file.

### `playSound(key, loop)`
Plays a sound.

### `stopSound(key)`
Stops a sound.

### `pauseSound(key)`
Pauses a sound.

### `resumeSound(key)`
Resumes a sound.

### `isSoundPlaying(key)`
Checks if a sound is playing.

### `setSoundVolume(key, volume)`
Sets a sound's volume (0 to 1).

### `setMasterVolume(volume)`
Sets the master volume (0 to 1).

### `setSoundPitch(key, pitch)`
Sets a sound's pitch.

### `unloadSound(key)`
Unloads a sound.

## JSON Functions

### `json.load(filename)`
Loads JSON data from a file.

### `json.save(filename, data)`
Saves a table to a JSON file.

### `json.encode(data)`
Encodes a table to a JSON string.

### `json.decode(str)`
Decodes a JSON string to a table.

## Video Functions

### `loadVideo(key, filename)`
Loads a video file.

### `updateVideo(key, deltaTime)`
Updates video playback.

### `renderVideo(key, x, y, width, height)`
Renders a video.

### `playVideo(key, loop)`
Plays a video.

### `pauseVideo(key)`
Pauses a video.

### `stopVideo(key)`
Stops a video.

### `seekVideo(key, seconds)`
Seeks to a position in a video.

### `isVideoPlaying(key)`
Checks if a video is playing.

### `isVideoEnded(key)`
Checks if a video has ended.

### `getVideoDuration(key)`
Gets a video's total duration.

### `getVideoTime(key)`
Gets a video's current time.

### `unloadVideo(key)`
Unloads a video.

## Utility Functions

### `print(message)`
Outputs a message to the console (debug mode only).

**Parameters:**
- `message` (string): The message to print

**Note:** This function only works when running with `LikeC.exe` (debug mode)

**Example:**
```lua
print("Debug: Player position updated")
print("Score: " .. score)
```

### `quit()`
Quits the application.

### `dofile(filename)`
Loads and executes a Luau script file. This is used to split your code across multiple files.

**Parameters:**
- `filename` (string): Path to the script file (relative to the project root)

**Example:**
```lua
-- In main.luau
dofile("player.luau")
dofile("scripts/utils.luau")

function onInit()
    player_init() -- function defined in player.luau
end
```

## Callback Functions

These functions are called by the framework at specific times. You should implement them in your main.luau file.

### `onInit()`
Called once when the application starts. Use this to:
- Load assets
- Initialize variables
- Set up the window
- Prepare game state

**Example:**
```lua
function onInit()
    setWindowTitle("My Game")
    setWindowSize(800, 600)
    
    loadImage("assets/player.png")
    loadImage("assets/bg.jpg")
    
    loadFont("fonts/arial.ttf", 24)
    
    player.x = 400
    player.y = 300
    player.health = 100
    
    print("Game initialized!")
end
```

### `onDraw()`
Called every frame (approximately 60 FPS). Use this to:
- Clear the screen
- Render graphics
- Update game logic
- Handle input

**Example:**
```lua
function onDraw()
    -- Clear screen
    clearScreen(50, 50, 50)
    
    -- Draw background
    renderImage("assets/bg.jpg", 0, 0, 800, 600)
    
    -- Handle input
    if isKeyDown("Left") then
        player.x = player.x - 5
    end
    if isKeyDown("Right") then
        player.x = player.x + 5
    end
    
    -- Draw player
    renderImage("assets/player.png", player.x, player.y, 64, 64)
    
    -- Draw UI
    drawTextDirect("Health: " .. player.health, 10, 10, 24, 255, 255, 255, 255)
end
```

### `onCleanup()`
Called when the application is closing. Use this to:
- Save game state
- Clean up resources
- Log shutdown information

**Example:**
```lua
function onCleanup()
    print("Game shutting down...")
end
```
