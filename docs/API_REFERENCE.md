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

## Rendering Functions

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

### `loadImage(key, filename)`
Loads an image from file and stores it with the given key.

**Parameters:**
- `key` (string): Unique identifier for the image
- `filename` (string): Path to the image file

**Returns:**
- `success` (boolean): true if loaded successfully, false otherwise

**Example:**
```lua
if loadImage("player", "assets/player.png") then
    print("Player image loaded successfully")
else
    print("Failed to load player image")
end
```

### `renderImage(key, x, y, width, height)`
Renders a previously loaded image at the specified position.

**Parameters:**
- `key` (string): The image key used when loading
- `x` (integer): X position in pixels
- `y` (integer): Y position in pixels
- `width` (integer): Render width in pixels
- `height` (integer): Render height in pixels

**Returns:**
- `success` (boolean): true if rendered successfully, false otherwise

**Example:**
```lua
renderImage("player", 100, 200, 64, 64)
```

### `loadFont(key, filename, size)`
Loads a font file for text rendering.

**Parameters:**
- `key` (string): Unique identifier for the font
- `filename` (string): Path to the font file
- `size` (integer): Font size in points

**Returns:**
- `success` (boolean): true if loaded successfully, false otherwise

**Example:**
```lua
if loadFont("main", "fonts/arial.ttf", 24) then
    print("Font loaded successfully")
end
```

### `drawText(text, fontKey, x, y, red, green, blue)`
Draws text at the specified position.

**Parameters:**
- `text` (string): The text to render
- `fontKey` (string): The font key used when loading
- `x` (integer): X position in pixels
- `y` (integer): Y position in pixels
- `red` (integer): Red color component (0-255)
- `green` (integer): Green color component (0-255)
- `blue` (integer): Blue color component (0-255)

**Returns:**
- `success` (boolean): true if rendered successfully, false otherwise

**Example:**
```lua
drawText("Hello World!", "main", 10, 10, 255, 255, 255)
drawText("Score: " .. score, "ui", 10, 50, 255, 255, 0)
```

## Input Functions

### `isKeyPressed(keycode)`
Checks if a specific key is currently pressed.

**Parameters:**
- `keycode` (integer): SDL key code constant

**Returns:**
- `pressed` (boolean): true if key is pressed, false otherwise

**Common Key Codes:**
```lua
SDLK_ESCAPE      -- Escape key
SDLK_SPACE       -- Space bar
SDLK_RETURN      -- Enter key
SDLK_UP          -- Up arrow
SDLK_DOWN        -- Down arrow
SDLK_LEFT        -- Left arrow
SDLK_RIGHT       -- Right arrow
SDLK_w           -- W key
SDLK_a           -- A key
SDLK_s           -- S key
SDLK_d           -- D key
SDLK_0 through SDLK_9  -- Number keys
SDLK_a through SDLK_z  -- Letter keys
```

**Example:**
```lua
if isKeyPressed(SDLK_SPACE) then
    player.jump()
end

if isKeyPressed(SDLK_RIGHT) then
    player.x = player.x + 5
end
```

## Utility Functions

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
    
    loadImage("player", "assets/player.png")
    loadImage("background", "assets/bg.jpg")
    
    loadFont("ui", "fonts/arial.ttf", 24)
    
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
    renderImage("background", 0, 0, 800, 600)
    
    -- Handle input
    if isKeyPressed(SDLK_LEFT) then
        player.x = player.x - 5
    end
    if isKeyPressed(SDLK_RIGHT) then
        player.x = player.x + 5
    end
    
    -- Draw player
    renderImage("player", player.x, player.y, 64, 64)
    
    -- Draw UI
    drawText("Health: " .. player.health, "ui", 10, 10, 255, 255, 255)
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
    -- Save high score
    saveHighScore(score)
    
    -- Log shutdown
    print("Game shutting down...")
end
```

## Error Handling

### Checking Return Values
Many functions return boolean values indicating success. Always check these:

```lua
-- Safe asset loading
if not loadImage("texture", "assets/texture.png") then
    print("Error: Failed to load texture")
    return
end

-- Safe font loading
if not loadFont("font", "fonts/game.ttf", 32) then
    print("Error: Failed to load font")
    -- Use fallback font or handle error
end
```

### Common Error Patterns

```lua
-- Pattern 1: Asset loading with fallback
local function loadAssetWithFallback(key, path, fallback)
    if loadImage(key, path) then
        return true
    else
        print("Warning: Failed to load " .. path .. ", using fallback")
        return loadImage(key, fallback)
    end
end

-- Pattern 2: Safe rendering
local function safeRenderImage(key, x, y, w, h)
    if not renderImage(key, x, y, w, h) then
        print("Error: Failed to render image " .. key)
        -- Draw placeholder or skip
    end
end
```

## Performance Considerations

### Asset Loading
- Load all assets in `onInit()`, not in `onDraw()`
- Use descriptive keys for easy debugging
- Check return values for error handling

### Rendering
- Avoid loading assets during rendering
- Minimize text rendering calls
- Use appropriate image sizes

### Input Handling
- Check input state once per frame
- Cache key states if needed multiple times

## Complete Example

```lua
-- main.luau - Complete game example

-- Game state
local player = {
    x = 400,
    y = 300,
    width = 64,
    height = 64,
    speed = 5,
    color = {r = 255, g = 255, b = 255}
}

local game = {
    score = 0,
    running = true,
    lastTime = 0
}

function onInit()
    -- Setup window
    setWindowTitle("Complete Example Game")
    setWindowSize(800, 600)
    
    -- Load assets with error checking
    if not loadImage("player", "assets/player.png") then
        print("Warning: Using colored rectangle for player")
    end
    
    if not loadFont("ui", "fonts/arial.ttf", 24) then
        print("Error: Failed to load UI font")
    end
    
    -- Initialize timing
    game.lastTime = getTime()
    
    print("Game initialized successfully!")
end

function onDraw()
    -- Calculate delta time
    local currentTime = getTime()
    local deltaTime = currentTime - game.lastTime
    game.lastTime = currentTime
    
    -- Clear screen
    clearScreen(30, 30, 40)
    
    -- Handle input
    if isKeyPressed(SDLK_LEFT) then
        player.x = player.x - player.speed
    end
    if isKeyPressed(SDLK_RIGHT) then
        player.x = player.x + player.speed
    end
    if isKeyPressed(SDLK_UP) then
        player.y = player.y - player.speed
    end
    if isKeyPressed(SDLK_DOWN) then
        player.y = player.y + player.speed
    end
    
    -- Keep player in bounds
    player.x = math.max(0, math.min(800 - player.width, player.x))
    player.y = math.max(0, math.min(600 - player.height, player.y))
    
    -- Render player (fallback to colored rectangle if image fails)
    if not renderImage("player", player.x, player.y, player.width, player.height) then
        -- Draw colored rectangle as fallback
        clearScreen(player.color.r, player.color.g, player.color.b)
    end
    
    -- Update score
    game.score = game.score + 1
    
    -- Draw UI
    drawText("Score: " .. game.score, "ui", 10, 10, 255, 255, 255)
    drawText("Position: " .. math.floor(player.x) .. "," .. math.floor(player.y), "ui", 10, 40, 255, 255, 255)
    drawText("FPS: " .. math.floor(1000 / deltaTime), "ui", 10, 70, 255, 255, 255)
    
    -- Exit on ESC
    if isKeyPressed(SDLK_ESCAPE) then
        game.running = false
    end
end

function onCleanup()
    print("Game ended. Final score: " .. game.score)
end
```

## SDL Key Code Reference

Here are the most commonly used SDL key codes:

### Letters and Numbers
- `SDLK_a` through `SDLK_z` - Alphabet keys
- `SDLK_0` through `SDLK_9` - Number keys

### Special Keys
- `SDLK_SPACE` - Space bar
- `SDLK_RETURN` - Enter/Return
- `SDLK_ESCAPE` - Escape
- `SDLK_TAB` - Tab
- `SDLK_BACKSPACE` - Backspace
- `SDLK_DELETE` - Delete

### Arrow Keys
- `SDLK_UP` - Up arrow
- `SDLK_DOWN` - Down arrow
- `SDLK_LEFT` - Left arrow
- `SDLK_RIGHT` - Right arrow

### Modifier Keys
- `SDLK_LSHIFT` - Left Shift
- `SDLK_RSHIFT` - Right Shift
- `SDLK_LCTRL` - Left Control
- `SDLK_RCTRL` - Right Control
- `SDLK_LALT` - Left Alt
- `SDLK_RALT` - Right Alt

### Function Keys
- `SDLK_F1` through `SDLK_F12` - Function keys

For a complete list of SDL key codes, refer to the SDL3 documentation.
