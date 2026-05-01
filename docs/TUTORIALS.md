# Like2D Framework - Tutorials

## Table of Contents

1. [Tutorial 1: Hello World](#tutorial-1-hello-world)
2. [Tutorial 2: Basic Movement](#tutorial-2-basic-movement)
3. [Tutorial 3: Image Rendering](#tutorial-3-image-rendering)
4. [Tutorial 4: Text and UI](#tutorial-4-text-and-ui)
5. [Tutorial 5: Simple Game Loop](#tutorial-5-simple-game-loop)
6. [Tutorial 6: Asset Management](#tutorial-6-asset-management)
7. [Tutorial 7: Window Controls](#tutorial-7-window-controls)
8. [Tutorial 8: Complete Mini Game](#tutorial-8-complete-mini-game)

---

## Tutorial 1: Hello World

**Objective**: Create your first Like2D application that displays text on screen.

### Step 1: Create Project Structure
```
HelloWorld/
└── main.luau
```

### Step 2: Write the Code

Create `main.luau`:

```lua
-- main.luau - Hello World Tutorial

function onInit()
    -- Set window properties
    setWindowTitle("Hello World Tutorial")
    setWindowSize(800, 600)
    
    -- Load a font for text rendering
    if loadFont("main", "C:/Windows/Fonts/arial.ttf", 48) then
        print("Font loaded successfully")
    else
        print("Failed to load font - using default")
    end
end

function onDraw()
    -- Clear screen with a nice blue color
    clearScreen(100, 149, 237)  -- Cornflower blue
    
    -- Draw "Hello World" text in the center
    local text = "Hello, Like2D World!"
    local centerX = 400 - (string.len(text) * 12)  -- Rough centering
    local centerY = 300
    
    drawText(text, "main", centerX, centerY, 255, 255, 255)
    
    -- Draw smaller text below
    drawText("Press ESC to exit", "main", 320, 350, 255, 255, 100)
end

function onCleanup()
    print("Hello World tutorial ended")
end
```

### Step 3: Run the Application
```bash
.\build\Like2D.exe "HelloWorld"
```

**Expected Result**: A blue window with "Hello, Like2D World!" text in white.

---

## Tutorial 2: Basic Movement

**Objective**: Add keyboard-controlled movement to a simple colored rectangle.

### Step 1: Create Project
```
Movement/
└── main.luau
```

### Step 2: Write the Code

```lua
-- main.luau - Basic Movement Tutorial

-- Player object
local player = {
    x = 400,
    y = 300,
    width = 50,
    height = 50,
    speed = 5,
    color = {r = 255, g = 100, b = 100}
}

function onInit()
    setWindowTitle("Movement Tutorial")
    setWindowSize(800, 600)
    
    -- Load font for UI
    loadFont("ui", "C:/Windows/Fonts/arial.ttf", 20)
    
    print("Movement tutorial started!")
end

function onDraw()
    -- Clear screen
    clearScreen(40, 40, 50)
    
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
    
    -- Draw player (as colored rectangle using clearScreen hack)
    -- Note: In a real scenario, you'd use renderImage with a sprite
    clearScreen(player.color.r, player.color.g, player.color.b)
    
    -- Draw UI text
    drawText("Use Arrow Keys to Move", "ui", 10, 10, 255, 255, 255)
    drawText("Position: " .. math.floor(player.x) .. ", " .. math.floor(player.y), "ui", 10, 35, 255, 255, 255)
    drawText("Press ESC to exit", "ui", 10, 60, 200, 200, 200)
end

function onCleanup()
    print("Movement tutorial ended")
end
```

### Step 3: Test the Movement
Run the application and use arrow keys to move the red square around the screen.

---

## Tutorial 3: Image Rendering

**Objective**: Load and display images in your application.

### Step 1: Create Project with Assets
```
ImageDemo/
├── main.luau
└── assets/
    ├── player.png
    └── background.jpg
```

### Step 2: Prepare Assets
- Create or download a simple player sprite (64x64 pixels recommended)
- Create or download a background image (800x600 pixels recommended)
- Save them in the `assets/` folder

### Step 3: Write the Code

```lua
-- main.luau - Image Rendering Tutorial

local player = {
    x = 400,
    y = 300,
    width = 64,
    height = 64,
    speed = 3
}

function onInit()
    setWindowTitle("Image Rendering Tutorial")
    setWindowSize(800, 600)
    
    -- Load assets with error checking
    local playerLoaded = loadImage("player", "assets/player.png")
    local bgLoaded = loadImage("background", "assets/background.jpg")
    
    if not playerLoaded then
        print("Warning: Failed to load player image")
    end
    
    if not bgLoaded then
        print("Warning: Failed to load background image")
    end
    
    -- Load UI font
    loadFont("ui", "C:/Windows/Fonts/arial.ttf", 18)
    
    print("Image tutorial started!")
end

function onDraw()
    -- Draw background first
    if not renderImage("background", 0, 0, 800, 600) then
        -- Fallback: draw colored background
        clearScreen(50, 100, 150)
    end
    
    -- Handle movement
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
    
    -- Draw player
    if not renderImage("player", player.x, player.y, player.width, player.height) then
        -- Fallback: draw colored rectangle
        clearScreen(255, 100, 100)
    end
    
    -- Draw UI
    drawText("Arrow keys to move, ESC to exit", "ui", 10, 10, 255, 255, 255)
    drawText("Player: " .. math.floor(player.x) .. ", " .. math.floor(player.y), "ui", 10, 30, 255, 255, 255)
end

function onCleanup()
    print("Image tutorial ended")
end
```

---

## Tutorial 4: Text and UI

**Objective**: Create a complete UI system with multiple text elements.

### Step 1: Create Project
```
UITutorial/
└── main.luau
```

### Step 2: Write the Code

```lua
-- main.luau - Text and UI Tutorial

local game = {
    score = 0,
    health = 100,
    level = 1,
    fps = 60,
    lastTime = 0
}

function onInit()
    setWindowTitle("UI Tutorial")
    setWindowSize(800, 600)
    
    -- Load multiple fonts for different UI elements
    loadFont("title", "C:/Windows/Fonts/arial.ttf", 36)
    loadFont("normal", "C:/Windows/Fonts/arial.ttf", 20)
    loadFont("small", "C:/Windows/Fonts/arial.ttf", 14)
    
    game.lastTime = getTime()
    print("UI tutorial started!")
end

function onDraw()
    -- Calculate FPS
    local currentTime = getTime()
    local deltaTime = currentTime - game.lastTime
    game.fps = math.floor(1000 / deltaTime)
    game.lastTime = currentTime
    
    -- Clear screen with gradient-like effect
    clearScreen(20, 25, 40)
    
    -- Update game state
    game.score = game.score + 1
    if game.score % 100 == 0 then
        game.level = game.level + 1
    end
    
    -- Simulate health changes
    if isKeyPressed(SDLK_h) then
        game.health = math.max(0, game.health - 1)
    end
    if isKeyPressed(SDLK_j) then
        game.health = math.min(100, game.health + 1)
    end
    
    -- Draw title
    drawText("Like2D UI Demo", "title", 250, 30, 255, 255, 255)
    
    -- Draw game stats
    drawText("Score: " .. game.score, "normal", 20, 100, 255, 255, 100)
    drawText("Level: " .. game.level, "normal", 20, 130, 100, 255, 255)
    drawText("Health: " .. game.health .. "%", "normal", 20, 160, 255, 100, 100)
    
    -- Draw health bar (using text characters)
    local healthBarWidth = math.floor(game.health * 2)
    local healthBar = string.rep("█", healthBarWidth) .. string.rep("░", 200 - healthBarWidth)
    drawText(healthBar, "small", 20, 185, 255, 100, 100)
    
    -- Draw FPS counter
    drawText("FPS: " .. game.fps, "small", 700, 10, 255, 255, 255)
    
    -- Draw controls
    drawText("Controls:", "normal", 600, 100, 255, 255, 255)
    drawText("H - Decrease Health", "small", 600, 130, 200, 200, 200)
    drawText("J - Increase Health", "small", 600, 150, 200, 200, 200)
    drawText("ESC - Exit", "small", 600, 170, 200, 200, 200)
    
    -- Draw status messages
    if game.health < 30 then
        drawText("WARNING: Low Health!", "normal", 250, 500, 255, 50, 50)
    end
    
    if game.level > 5 then
        drawText("You're doing great!", "normal", 280, 530, 100, 255, 100)
    end
    
    -- Draw footer
    drawText("Like2D Framework - UI Tutorial", "small", 250, 580, 150, 150, 150)
end

function onCleanup()
    print("UI tutorial ended")
    print("Final score: " .. game.score)
    print("Final level: " .. game.level)
end
```

---

## Tutorial 5: Simple Game Loop

**Objective**: Create a complete game loop with game states and logic.

### Step 1: Create Project
```
GameLoop/
└── main.luau
```

### Step 2: Write the Code

```lua
-- main.luau - Simple Game Loop Tutorial

-- Game states
local GameState = {
    MENU = "menu",
    PLAYING = "playing",
    PAUSED = "paused",
    GAME_OVER = "game_over"
}

-- Game data
local game = {
    state = GameState.MENU,
    score = 0,
    highScore = 0,
    player = {
        x = 400,
        y = 500,
        width = 40,
        height = 40,
        speed = 6
    },
    enemies = {},
    lastEnemySpawn = 0,
    enemySpawnInterval = 2000
}

function onInit()
    setWindowTitle("Game Loop Tutorial")
    setWindowSize(800, 600)
    
    loadFont("large", "C:/Windows/Fonts/arial.ttf", 48)
    loadFont("medium", "C:/Windows/Fonts/arial.ttf", 24)
    loadFont("small", "C:/Windows/Fonts/arial.ttf", 16)
    
    print("Game loop tutorial started!")
end

function spawnEnemy()
    local enemy = {
        x = math.random(50, 750),
        y = -50,
        width = 30,
        height = 30,
        speed = math.random(2, 5)
    }
    table.insert(game.enemies, enemy)
end

function updateGame()
    local currentTime = getTime()
    
    -- Spawn enemies
    if currentTime - game.lastEnemySpawn > game.enemySpawnInterval then
        spawnEnemy()
        game.lastEnemySpawn = currentTime
    end
    
    -- Update player
    if isKeyPressed(SDLK_LEFT) then
        game.player.x = game.player.x - game.player.speed
    end
    if isKeyPressed(SDLK_RIGHT) then
        game.player.x = game.player.x + game.player.speed
    end
    
    -- Keep player in bounds
    game.player.x = math.max(0, math.min(800 - game.player.width, game.player.x))
    
    -- Update enemies
    for i = #game.enemies, 1, -1 do
        local enemy = game.enemies[i]
        enemy.y = enemy.y + enemy.speed
        
        -- Remove enemies that go off screen
        if enemy.y > 650 then
            table.remove(game.enemies, i)
            game.score = game.score + 10
        end
        
        -- Check collision with player
        if checkCollision(game.player, enemy) then
            game.state = GameState.GAME_OVER
            if game.score > game.highScore then
                game.highScore = game.score
            end
        end
    end
end

function checkCollision(rect1, rect2)
    return rect1.x < rect2.x + rect2.width and
           rect1.x + rect1.width > rect2.x and
           rect1.y < rect2.y + rect2.height and
           rect1.y + rect1.height > rect2.y
end

function renderGame()
    clearScreen(20, 20, 40)
    
    -- Draw player (blue square)
    clearScreen(100, 150, 255)
    
    -- Draw enemies (red squares - simplified)
    for _, enemy in ipairs(game.enemies) do
        -- In real implementation, you'd use renderImage
        clearScreen(255, 50, 50)
    end
    
    -- Draw UI
    drawText("Score: " .. game.score, "medium", 10, 10, 255, 255, 255)
    drawText("High Score: " .. game.highScore, "medium", 10, 40, 255, 255, 100)
end

function renderMenu()
    clearScreen(40, 40, 80)
    
    drawText("Like2D Game", "large", 250, 150, 255, 255, 255)
    drawText("Press SPACE to Start", "medium", 250, 250, 255, 255, 255)
    drawText("Press ESC to Quit", "medium", 250, 280, 200, 200, 200)
    drawText("Arrow Keys to Move", "small", 250, 350, 150, 150, 150)
    drawText("Avoid the red enemies!", "small", 250, 370, 150, 150, 150)
end

function renderGameOver()
    clearScreen(80, 20, 20)
    
    drawText("GAME OVER", "large", 250, 200, 255, 50, 50)
    drawText("Final Score: " .. game.score, "medium", 280, 280, 255, 255, 255)
    drawText("High Score: " .. game.highScore, "medium", 280, 310, 255, 255, 100)
    drawText("Press SPACE to Play Again", "medium", 220, 380, 255, 255, 255)
    drawText("Press ESC to Quit", "medium", 250, 410, 200, 200, 200)
end

function resetGame()
    game.score = 0
    game.state = GameState.PLAYING
    game.enemies = {}
    game.player.x = 400
    game.lastEnemySpawn = getTime()
end

function onDraw()
    if game.state == GameState.MENU then
        renderMenu()
        
        if isKeyPressed(SDLK_SPACE) then
            resetGame()
        end
        
    elseif game.state == GameState.PLAYING then
        updateGame()
        renderGame()
        
        if isKeyPressed(SDLK_ESCAPE) then
            game.state = GameState.MENU
        end
        
    elseif game.state == GameState.GAME_OVER then
        renderGameOver()
        
        if isKeyPressed(SDLK_SPACE) then
            resetGame()
        elseif isKeyPressed(SDLK_ESCAPE) then
            game.state = GameState.MENU
        end
    end
end

function onCleanup()
    print("Game loop tutorial ended")
    print("Final high score: " .. game.highScore)
end
```

---

## Tutorial 6: Asset Management

**Objective**: Learn proper asset loading and management techniques.

### Step 1: Create Project Structure
```
AssetDemo/
├── main.luau
├── assets/
│   ├── images/
│   │   ├── player.png
│   │   ├── enemy.png
│   │   └── background.jpg
│   ├── fonts/
│   │   ├── main.ttf
│   │   └── ui.ttf
│   └── sounds/  (for future audio support)
└── config/
    └── assets.lua
```

### Step 2: Create Asset Configuration
Create `config/assets.lua`:

```lua
-- config/assets.lua - Asset configuration

return {
    images = {
        player = "assets/images/player.png",
        enemy = "assets/images/enemy.png",
        background = "assets/images/background.jpg",
        ui = {
            button = "assets/images/button.png",
            panel = "assets/images/panel.png"
        }
    },
    
    fonts = {
        main = {
            path = "assets/fonts/main.ttf",
            sizes = {16, 24, 32, 48}
        },
        ui = {
            path = "assets/fonts/ui.ttf",
            sizes = {12, 14, 18}
        }
    },
    
    -- Future: sounds, music, etc.
}
```

### Step 3: Write Main Code

```lua
-- main.luau - Asset Management Tutorial

local assets = require("config.assets")
local loadedAssets = {}
local assetErrors = {}

function onInit()
    setWindowTitle("Asset Management Tutorial")
    setWindowSize(800, 600)
    
    -- Load all assets systematically
    loadAllAssets()
    
    -- Display loading results
    print("Asset loading completed:")
    print("Loaded: " .. countLoadedAssets() .. " assets")
    print("Errors: " .. #assetErrors .. " assets failed to load")
    
    -- Show any loading errors
    for _, error in ipairs(assetErrors) do
        print("Error: " .. error)
    end
end

function loadAllAssets()
    -- Load images
    for key, path in pairs(assets.images) do
        if type(path) == "string" then
            loadImageAsset(key, path)
        elseif type(path) == "table" then
            -- Handle nested image groups
            for subKey, subPath in pairs(path) do
                loadImageAsset(key .. "_" .. subKey, subPath)
            end
        end
    end
    
    -- Load fonts with multiple sizes
    for fontKey, fontData in pairs(assets.fonts) do
        for _, size in ipairs(fontData.sizes) do
            loadFontAsset(fontKey .. "_" .. size, fontData.path, size)
        end
    end
end

function loadImageAsset(key, path)
    if loadImage(key, path) then
        loadedAssets[key] = "image"
        print("Loaded image: " .. key)
    else
        table.insert(assetErrors, "Failed to load image: " .. key .. " from " .. path)
    end
end

function loadFontAsset(key, path, size)
    local fontKey = key .. "_" .. size
    if loadFont(fontKey, path, size) then
        loadedAssets[fontKey] = "font"
        print("Loaded font: " .. fontKey)
    else
        table.insert(assetErrors, "Failed to load font: " .. fontKey .. " from " .. path)
    end
end

function countLoadedAssets()
    local count = 0
    for _ in pairs(loadedAssets) do
        count = count + 1
    end
    return count
end

function onDraw()
    clearScreen(30, 30, 50)
    
    -- Draw loaded assets demonstration
    local y = 50
    
    -- Draw title
    drawText("Asset Management Demo", "main_32", 200, y, 255, 255, 255)
    y = y + 60
    
    -- Draw asset status
    drawText("Loaded Assets: " .. countLoadedAssets(), "main_24", 50, y, 100, 255, 100)
    y = y + 30
    
    drawText("Errors: " .. #assetErrors, "main_24", 50, y, 255, 100, 100)
    y = y + 50
    
    -- Demonstrate loaded images
    if loadedAssets.player then
        renderImage("player", 100, y, 64, 64)
        drawText("Player Image", "ui_18", 180, y + 20, 255, 255, 255)
    end
    
    if loadedAssets.enemy then
        renderImage("enemy", 300, y, 32, 32)
        drawText("Enemy Image", "ui_18", 340, y + 8, 255, 255, 255)
    end
    
    y = y + 100
    
    -- Demonstrate different font sizes
    drawText("Font Size 16", "main_16", 50, y, 255, 255, 255)
    drawText("Font Size 24", "main_24", 50, y + 25, 255, 255, 255)
    drawText("Font Size 32", "main_32", 50, y + 55, 255, 255, 255)
    
    -- Draw asset errors if any
    if #assetErrors > 0 then
        y = y + 100
        drawText("Asset Loading Errors:", "main_24", 50, y, 255, 100, 100)
        y = y + 30
        
        for i, error in ipairs(assetErrors) do
            if i <= 5 then  -- Show max 5 errors
                drawText(error, "ui_14", 50, y, 255, 150, 150)
                y = y + 20
            end
        end
        
        if #assetErrors > 5 then
            drawText("... and " .. (#assetErrors - 5) .. " more errors", "ui_14", 50, y, 255, 150, 150)
        end
    end
    
    -- Instructions
    drawText("Press ESC to exit", "ui_14", 600, 570, 150, 150, 150)
end

function onCleanup()
    print("Asset management tutorial ended")
    print("Final asset count: " .. countLoadedAssets())
end
```

---

## Tutorial 7: Window Controls

**Objective**: Master window management features.

### Step 1: Create Project
```
WindowControls/
└── main.luau
```

### Step 2: Write the Code

```lua
-- main.luau - Window Controls Tutorial

local window = {
    presets = {
        {name = "800x600", width = 800, height = 600},
        {name = "1024x768", width = 1024, height = 768},
        {name = "1280x720", width = 1280, height = 720},
        {name = "1920x1080", width = 1920, height = 1080}
    },
    currentPreset = 1,
    isFullscreen = false,
    lastToggle = 0
}

function onInit()
    setWindowTitle("Window Controls Tutorial")
    setWindowSize(800, 600)
    
    loadFont("title", "C:/Windows/Fonts/arial.ttf", 32)
    loadFont("normal", "C:/Windows/Fonts/arial.ttf", 20)
    loadFont("small", "C:/Windows/Fonts/arial.ttf", 16)
    
    print("Window controls tutorial started!")
end

function onDraw()
    clearScreen(40, 45, 60)
    
    local currentTime = getTime()
    
    -- Draw title
    drawText("Window Controls Demo", "title", 200, 30, 255, 255, 255)
    
    -- Get current window size
    local width, height = getWindowSize()
    
    -- Display current window info
    drawText("Current Size: " .. width .. "x" .. height, "normal", 50, 100, 255, 255, 255)
    drawText("Fullscreen: " .. (window.isFullscreen and "Yes" or "No"), "normal", 50, 130, 255, 255, 255)
    drawText("Current Preset: " .. window.presets[window.currentPreset].name, "normal", 50, 160, 255, 255, 255)
    
    -- Draw controls
    drawText("Controls:", "normal", 50, 220, 255, 255, 255)
    drawText("1-4 - Change window size preset", "small", 50, 250, 200, 200, 200)
    drawText("F - Toggle fullscreen", "small", 50, 270, 200, 200, 200)
    drawText("T - Change window title", "small", 50, 290, 200, 200, 200)
    drawText("ESC - Exit", "small", 50, 310, 200, 200, 200)
    
    -- Handle window size presets
    local presetChanged = false
    
    if isKeyPressed(SDLK_1) then
        window.currentPreset = 1
        presetChanged = true
    elseif isKeyPressed(SDLK_2) then
        window.currentPreset = 2
        presetChanged = true
    elseif isKeyPressed(SDLK_3) then
        window.currentPreset = 3
        presetChanged = true
    elseif isKeyPressed(SDLK_4) then
        window.currentPreset = 4
        presetChanged = true
    end
    
    if presetChanged then
        local preset = window.presets[window.currentPreset]
        setWindowSize(preset.width, preset.height)
        print("Changed to preset: " .. preset.name)
    end
    
    -- Handle fullscreen toggle (with debounce)
    if isKeyPressed(SDLK_f) and currentTime - window.lastToggle > 500 then
        window.isFullscreen = not window.isFullscreen
        setFullscreen(window.isFullscreen)
        window.lastToggle = currentTime
        print("Fullscreen: " .. (window.isFullscreen and "ON" or "OFF"))
    end
    
    -- Handle title change
    if isKeyPressed(SDLK_t) then
        local titles = {
            "Like2D Framework",
            "Window Controls Tutorial",
            "Awesome Game Engine",
            "My Cool Application"
        }
        local randomTitle = titles[math.random(#titles)]
        setWindowTitle(randomTitle)
        print("Title changed to: " .. randomTitle)
    end
    
    -- Draw visual representation
    local centerX = width / 2
    local centerY = height / 2
    
    -- Draw window representation
    drawText("Window Representation", "normal", centerX - 100, centerY - 100, 255, 255, 100)
    
    -- Draw border
    drawText("+------------------+", "small", centerX - 80, centerY - 50, 255, 255, 255)
    drawText("|                  |", "small", centerX - 80, centerY - 30, 255, 255, 255)
    drawText("|    Like2D App    |", "small", centerX - 80, centerY - 10, 255, 255, 255)
    drawText("|                  |", "small", centerX - 80, centerY + 10, 255, 255, 255)
    drawText("+------------------+", "small", centerX - 80, centerY + 30, 255, 255, 255)
    
    -- Draw size info
    drawText("Actual: " .. width .. "x" .. height, "small", centerX - 60, centerY + 60, 150, 150, 255)
    
    -- Draw preset list on the right
    drawText("Available Presets:", "normal", width - 250, 100, 255, 255, 255)
    
    for i, preset in ipairs(window.presets) do
        local color = (i == window.currentPreset) and {255, 255, 100} or {200, 200, 200}
        drawText(i .. ". " .. preset.name .. " (" .. preset.width .. "x" .. preset.height .. ")", 
                "small", width - 250, 130 + (i-1) * 25, color[1], color[2], color[3])
    end
    
    -- Draw fullscreen indicator
    if window.isFullscreen then
        drawText("FULLSCREEN MODE", "normal", centerX - 80, 50, 255, 100, 100)
    end
end

function onCleanup()
    print("Window controls tutorial ended")
    print("Final window state:")
    local w, h = getWindowSize()
    print("  Size: " .. w .. "x" .. h)
    print("  Fullscreen: " .. (window.isFullscreen and "Yes" or "No"))
end
```

---

## Tutorial 8: Complete Mini Game

**Objective**: Combine all concepts into a complete mini game.

### Step 1: Create Project Structure
```
MiniGame/
├── main.luau
├── assets/
│   ├── player.png
│   ├── enemy.png
│   ├── bullet.png
│   ├── background.jpg
│   └── fonts/
│       └── game.ttf
└── scripts/
    ├── player.lua
    ├── enemy.lua
    └── game.lua
```

### Step 2: Create Game Modules

Create `scripts/player.lua`:
```lua
-- scripts/player.lua

local Player = {}
Player.__index = Player

function Player.new(x, y)
    local self = setmetatable({}, Player)
    self.x = x
    self.y = y
    self.width = 40
    self.height = 40
    self.speed = 5
    self.health = 100
    self.score = 0
    return self
end

function Player:update()
    if isKeyPressed(SDLK_LEFT) then
        self.x = self.x - self.speed
    end
    if isKeyPressed(SDLK_RIGHT) then
        self.x = self.x + self.speed
    end
    if isKeyPressed(SDLK_UP) then
        self.y = self.y - self.speed
    end
    if isKeyPressed(SDLK_DOWN) then
        self.y = self.y + self.speed
    end
    
    -- Keep player in bounds
    self.x = math.max(0, math.min(800 - self.width, self.x))
    self.y = math.max(0, math.min(600 - self.height, self.y))
end

function Player:takeDamage(amount)
    self.health = self.health - amount
    return self.health <= 0
end

function Player:addScore(points)
    self.score = self.score + points
end

return Player
```

Create `scripts/enemy.lua`:
```lua
-- scripts/enemy.lua

local Enemy = {}
Enemy.__index = Enemy

function Enemy.new(x, y, speed)
    local self = setmetatable({}, Enemy)
    self.x = x
    self.y = y
    self.width = 30
    self.height = 30
    self.speed = speed or math.random(2, 4)
    self.health = 1
    self.points = 10
    return self
end

function Enemy:update()
    self.y = self.y + self.speed
end

function Enemy:isOffScreen()
    return self.y > 650
end

function Enemy:checkCollision(other)
    return self.x < other.x + other.width and
           self.x + self.width > other.x and
           self.y < other.y + other.height and
           self.y + self.height > other.y
end

return Enemy
```

### Step 3: Write Main Game Code

```lua
-- main.luau - Complete Mini Game

-- Import modules
local Player = require("scripts.player")
local Enemy = require("scripts.enemy")

-- Game state
local GameState = {
    MENU = "menu",
    PLAYING = "playing",
    PAUSED = "paused",
    GAME_OVER = "game_over"
}

local game = {
    state = GameState.MENU,
    player = nil,
    enemies = {},
    particles = {},
    lastEnemySpawn = 0,
    enemySpawnInterval = 1500,
    difficulty = 1,
    highScore = 0
}

function onInit()
    setWindowTitle("Space Defender - Like2D Mini Game")
    setWindowSize(800, 600)
    
    -- Load assets
    loadGameAssets()
    
    -- Initialize high score (in real game, you'd load from file)
    game.highScore = 0
    
    print("Space Defender initialized!")
end

function loadGameAssets()
    -- Load images
    loadImage("player", "assets/player.png")
    loadImage("enemy", "assets/enemy.png")
    loadImage("bullet", "assets/bullet.png")
    loadImage("background", "assets/background.jpg")
    
    -- Load fonts
    loadFont("title", "assets/fonts/game.ttf", 48)
    loadFont("large", "assets/fonts/game.ttf", 32)
    loadFont("medium", "assets/fonts/game.ttf", 24)
    loadFont("small", "assets/fonts/game.ttf", 16)
end

function startNewGame()
    game.state = GameState.PLAYING
    game.player = Player.new(400, 500)
    game.enemies = {}
    game.particles = {}
    game.lastEnemySpawn = getTime()
    game.difficulty = 1
end

function spawnEnemy()
    local x = math.random(50, 750)
    local speed = math.random(2, 4 + game.difficulty)
    local enemy = Enemy.new(x, -50, speed)
    table.insert(game.enemies, enemy)
end

function createParticle(x, y, color)
    local particle = {
        x = x,
        y = y,
        vx = math.random(-3, 3),
        vy = math.random(-3, 3),
        life = 30,
        color = color or {255, 255, 255}
    }
    table.insert(game.particles, particle)
end

function updateGame()
    if game.state ~= GameState.PLAYING then
        return
    end
    
    local currentTime = getTime()
    
    -- Update difficulty
    game.difficulty = math.floor(game.player.score / 100) + 1
    
    -- Spawn enemies
    if currentTime - game.lastEnemySpawn > game.enemySpawnInterval / game.difficulty then
        spawnEnemy()
        game.lastEnemySpawn = currentTime
    end
    
    -- Update player
    game.player:update()
    
    -- Update enemies
    for i = #game.enemies, 1, -1 do
        local enemy = game.enemies[i]
        enemy:update()
        
        -- Check if enemy is off screen
        if enemy:isOffScreen() then
            table.remove(game.enemies, i)
            game.player:addScore(enemy.points)
        elseif enemy:checkCollision(game.player) then
            -- Player hit by enemy
            if game.player:takeDamage(20) then
                game.state = GameState.GAME_OVER
                if game.player.score > game.highScore then
                    game.highScore = game.player.score
                end
            end
            -- Create explosion particles
            for j = 1, 10 do
                createParticle(enemy.x + 15, enemy.y + 15, {255, 100, 0})
            end
            table.remove(game.enemies, i)
        end
    end
    
    -- Update particles
    for i = #game.particles, 1, -1 do
        local particle = game.particles[i]
        particle.x = particle.x + particle.vx
        particle.y = particle.y + particle.vy
        particle.life = particle.life - 1
        
        if particle.life <= 0 then
            table.remove(game.particles, i)
        end
    end
end

function renderGame()
    -- Draw background
    if not renderImage("background", 0, 0, 800, 600) then
        clearScreen(10, 10, 30)
    end
    
    -- Draw game objects
    if game.state == GameState.PLAYING then
        -- Draw player
        renderImage("player", game.player.x, game.player.y, game.player.width, game.player.height)
        
        -- Draw enemies
        for _, enemy in ipairs(game.enemies) do
            renderImage("enemy", enemy.x, enemy.y, enemy.width, enemy.height)
        end
        
        -- Draw particles (simplified - in real game use proper particle system)
        for _, particle in ipairs(game.particles) do
            clearScreen(particle.color[1], particle.color[2], particle.color[3])
        end
    end
    
    -- Draw UI
    renderUI()
end

function renderUI()
    local y = 10
    
    -- Score and stats
    drawText("Score: " .. game.player.score, "medium", 10, y, 255, 255, 255)
    y = y + 30
    
    drawText("Health: " .. game.player.health, "medium", 10, y, 
             game.player.health > 30 and 255 or 255, 
             game.player.health > 30 and 255 or 100, 
             game.player.health > 30 and 255 or 100)
    y = y + 30
    
    drawText("Level: " .. game.difficulty, "medium", 10, y, 255, 255, 100)
    y = y + 30
    
    drawText("High Score: " .. game.highScore, "medium", 10, y, 255, 255, 100)
    
    -- Controls hint
    drawText("Arrow Keys: Move | ESC: Menu", "small", 10, 570, 150, 150, 150)
end

function renderMenu()
    clearScreen(20, 20, 40)
    
    drawText("SPACE DEFENDER", "title", 200, 100, 255, 255, 255)
    drawText("A Like2D Mini Game", "large", 250, 160, 200, 200, 200)
    
    drawText("High Score: " .. game.highScore, "medium", 300, 250, 255, 255, 100)
    
    drawText("Press SPACE to Start", "medium", 250, 350, 255, 255, 255)
    drawText("Press ESC to Quit", "medium", 270, 380, 200, 200, 200)
    
    drawText("Arrow Keys to Move", "small", 250, 450, 150, 150, 150)
    drawText("Avoid the enemies!", "small", 280, 470, 150, 150, 150)
end

function renderGameOver()
    clearScreen(40, 10, 10)
    
    drawText("GAME OVER", "title", 250, 150, 255, 50, 50)
    
    drawText("Final Score: " .. game.player.score, "large", 280, 250, 255, 255, 255)
    drawText("High Score: " .. game.highScore, "large", 290, 290, 255, 255, 100)
    
    if game.player.score == game.highScore and game.player.score > 0 then
        drawText("NEW HIGH SCORE!", "medium", 270, 340, 255, 255, 0)
    end
    
    drawText("Press SPACE to Play Again", "medium", 220, 400, 255, 255, 255)
    drawText("Press ESC for Menu", "medium", 250, 430, 200, 200, 200)
end

function onDraw()
    if game.state == GameState.MENU then
        renderMenu()
        
        if isKeyPressed(SDLK_SPACE) then
            startNewGame()
        end
        
    elseif game.state == GameState.PLAYING then
        updateGame()
        renderGame()
        
        if isKeyPressed(SDLK_ESCAPE) then
            game.state = GameState.MENU
        end
        
    elseif game.state == GameState.GAME_OVER then
        renderGame()  -- Keep showing the game scene
        
        if isKeyPressed(SDLK_SPACE) then
            startNewGame()
        elseif isKeyPressed(SDLK_ESCAPE) then
            game.state = GameState.MENU
        end
    end
end

function onCleanup()
    print("Space Defender ended")
    print("Final high score: " .. game.highScore)
end
```

---

## Conclusion

These tutorials cover the complete range of Like2D Framework features:

1. **Hello World** - Basic setup and text rendering
2. **Basic Movement** - Input handling and simple graphics
3. **Image Rendering** - Asset loading and display
4. **Text and UI** - Complete user interface systems
5. **Simple Game Loop** - Game states and logic
6. **Asset Management** - Professional asset organization
7. **Window Controls** - Window management features
8. **Complete Mini Game** - Full game implementation

You now have all the knowledge needed to create complete 2D games and applications with the Like2D Framework!

### Next Steps

- Experiment with your own game ideas
- Add more complex features (particle systems, sound, etc.)
- Optimize performance for larger games
- Share your creations with the Like2D community!

Happy coding with Like2D! 🎮
