# Like2D Framework - Getting Started Guide

## Overview

Like2D is a lightweight 2D game framework built with SDL3 and Luau scripting. It provides a simple API for creating 2D applications, games, and interactive experiences with the power of Lua scripting.

## System Requirements

- **Operating System**: Windows 10 or later
- **Compiler**: MinGW-w64 with C++20 support
- **Dependencies**: SDL3, Luau (included in external folder)
- **Build Tools**: CMake (for full builds), Git (optional)

## Quick Start

### 1. Building the Framework

#### Option A: Quick Build (Pre-built Dependencies)
```bash
# Navigate to project directory
cd Like2D

# Run the quick build script
.\build.bat
```

#### Option B: Full Build (Compile Dependencies from Source)
```bash
# Navigate to project directory
cd Like2D

# Run the full build script (compiles SDL3 and Luau from source)
.\build-full.bat
```

### 2. Running Your First Application

#### Method 1: Drag and Drop
1. Create a folder with your project files
2. Add a `main.luau` file (see example below)
3. Drag the folder onto `Like2D.exe`

#### Method 2: Command Line
```bash
# Run with specific script folder
.\build\Like2D.exe "path\to\your\project"

# Run with specific script file
.\build\Like2D.exe "path\to\main.luau"
```

#### Method 3: Debug Mode
```bash
# Run debug version with console output
.\build\LikeC.exe "path\to\your\project"
```

## Your First Like2D Application

Create a new folder called `MyFirstGame` and add a `main.luau` file:

```lua
-- main.luau - Your first Like2D application

-- This function is called once when the application starts
function onInit()
    -- Set window title and size
    setWindowTitle("My First Game")
    setWindowSize(800, 600)
    
    -- Load assets
    loadImage("player", "assets/player.png")
    loadImage("background", "assets/background.jpg")
    
    -- Load font for text rendering
    loadFont("main", "fonts/arial.ttf", 24)
    
    print("Game initialized successfully!")
end

-- This function is called every frame (60 FPS)
function onDraw()
    -- Clear screen with blue background
    clearScreen(100, 149, 237)
    
    -- Draw background image
    renderImage("background", 0, 0, 800, 600)
    
    -- Draw player at center
    local playerX = 400 - 32  -- Center X - half width
    local playerY = 300 - 32  -- Center Y - half height
    renderImage("player", playerX, playerY, 64, 64)
    
    -- Draw text
    drawText("Hello Like2D!", "main", 10, 10, 255, 255, 255)
    
    -- Display current time
    local currentTime = getTime()
    drawText("Time: " .. currentTime, "main", 10, 40, 255, 255, 255)
end

-- This function is called when the application is closing
function onCleanup()
    print("Game is shutting down...")
end
```

## Project Structure

A typical Like2D project looks like this:

```
MyGame/
├── main.luau          # Main script file
├── assets/            # Images and resources
│   ├── player.png
│   ├── background.jpg
│   └── items/
│       ├── coin.png
│       └── powerup.png
├── fonts/             # Font files
│   ├── arial.ttf
│   └── game_font.otf
└── scripts/           # Additional Lua modules
    ├── player.lua
    ├── enemies.lua
    └── utils.lua
```

## Essential API Functions

### Window Management
```lua
-- Set window title
setWindowTitle("My Game Title")

-- Set window size
setWindowSize(1024, 768)

-- Get current window size
local width, height = getWindowSize()

-- Toggle fullscreen
setFullscreen(true)   -- Enable fullscreen
setFullscreen(false)  -- Disable fullscreen
```

### Rendering
```lua
-- Clear screen with RGB color
clearScreen(red, green, blue)

-- Load an image (returns true/false)
local success = loadImage("key", "path/to/image.png")

-- Render an image
renderImage("key", x, y, width, height)

-- Load a font
loadFont("font_key", "path/to/font.ttf", size)

-- Draw text
drawText("Hello World", "font_key", x, y, r, g, b)
```

### Input
```lua
-- Check if a key is currently pressed
if isKeyPressed(SDLK_SPACE) then
    -- Space bar is pressed
end

-- Common key codes:
-- SDLK_ESCAPE, SDLK_SPACE, SDLK_RETURN
-- SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT
-- SDLK_w, SDLK_a, SDLK_s, SDLK_d
```

### Utilities
```lua
-- Get current time in milliseconds
local time = getTime()

-- Print to console (debug mode only)
print("Debug message")
```

## Asset Management

### Supported Image Formats
- PNG
- JPEG/JPG
- BMP
- TGA
- And other formats supported by STB Image

### Font Support
- TrueType fonts (.ttf)
- OpenType fonts (.otf)
- Bitmap fonts (limited support)

### Best Practices
1. **Organize assets** in separate folders
2. **Use descriptive keys** when loading assets
3. **Check return values** when loading assets
4. **Optimize image sizes** for better performance

## Debugging

### Using the Debug Executable
The `LikeC.exe` provides console output for debugging:
```bash
.\build\LikeC.exe "your_project"
```

### Common Debug Techniques
```lua
-- Print variable values
print("Player position: " .. playerX .. ", " .. playerY)

-- Check asset loading
if not loadImage("texture", "missing.png") then
    print("Failed to load texture!")
end

-- Time-based debugging
local lastTime = getTime()
-- ... your code ...
local currentTime = getTime()
print("Frame time: " .. (currentTime - lastTime) .. "ms")
```

## Performance Tips

1. **Load assets once** in `onInit()`, not every frame
2. **Avoid creating new objects** in `onDraw()` when possible
3. **Use appropriate image sizes** - don't scale large images down
4. **Limit text rendering** - cache text when possible
5. **Profile with getTime()** to identify bottlenecks

## Next Steps

- Read the [API Reference](API_REFERENCE.md) for complete function documentation
- Check out the [Tutorials](TUTORIALS.md) for step-by-step examples
- Learn about the [Build System](BUILD_SYSTEM.md) for advanced configuration

## Troubleshooting

### Common Issues

**"Failed to load image"**
- Check file path is correct
- Ensure image format is supported
- Verify file exists in the correct location

**"SDL initialization failed"**
- Ensure SDL3.dll is in the build directory
- Check if graphics drivers are up to date
- Try running as administrator

**"Lua compilation error"**
- Check syntax in your .luau files
- Ensure all parentheses are balanced
- Verify function names are spelled correctly

### Getting Help

- Check the console output in debug mode
- Review the error messages carefully
- Ensure all dependencies are properly installed
- Test with minimal code first

## License and Credits

Like2D Framework - Alpha Version
Built with SDL3 and Luau
© 2026 Like2D Development Team
