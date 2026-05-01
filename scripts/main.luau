-- Like2D Framework Main Script v1.1.0
print("Hello from Luau! Like2D Framework v1.0.0.0")

-- Test window control functions
setWindowTitle("Like2D Framework v1.0.0.0")
setWindowSize(800, 600)

-- Load Assets
loadImage("logo.png")

-- Try to load fonts with error checking
local font32Loaded = loadFont("Arial.ttf", 32)
local font24Loaded = loadFont("Arial.ttf", 24)
local font18Loaded = loadFont("Arial.ttf", 18)

if font32Loaded then
    print("Arial.ttf 32px loaded successfully")
else
    print("Warning: Arial.ttf 32px failed to load")
end

if font24Loaded then
    print("Arial.ttf 24px loaded successfully")
else
    print("Warning: Arial.ttf 24px failed to load")
end

if font18Loaded then
    print("Arial.ttf 18px loaded successfully")
else
    print("Warning: Arial.ttf 18px failed to load")
end

print("Window title and size set!")
print("Image and Fonts loaded successfully!")
print("stb_truetype font rendering system active!")

-- onDraw function called every frame
function onDraw()
    -- Clear screen with a nice background color
    clearScreen(30, 30, 50)
    
    -- Check if fonts loaded successfully and render accordingly
    if font32Loaded and font24Loaded and font18Loaded then
        -- All fonts loaded - show full demo
        drawText("Like2D Framework v1.1.0", "Arial.ttf_32", 20, 40, 255, 200, 100)
        drawText("NEW UPDATES: LOAD FONT", "Arial.ttf_24", 20, 80, 200, 255, 200)
        drawText("WITH STB_TRUETYPE", "Arial.ttf_24", 20, 100, 200, 255, 200)
        
        drawText("✓ Professional baseline alignment", "Arial.ttf_18", 20, 145, 255, 255, 255)
        drawText("✓ Ultra-lightweight header-only implementation", "Arial.ttf_18", 20, 170, 255, 255, 255)
        drawText("✓ Texture atlas baking for performance", "Arial.ttf_18", 20, 195, 255, 255, 255)
        drawText("✓ Direct text rendering with drawTextDirect()", "Arial.ttf_18", 20, 215, 255, 255, 255)
        
        -- Direct text rendering demo
        drawTextDirect("Direct Text: No pre-loading needed!", 20, 240, 20, 255, 150, 150)
        drawTextDirect("Size: 16px | Color: Pink", 20, 265, 16, 255, 200, 200)
        drawTextDirect("Size: 14px | Color: Yellow", 20, 285, 14, 255, 255, 100)
        
        local time = getTime()
        drawTextDirect("Runtime: " .. math.floor(time/1000) .. "s", 20, 320, 14, 150, 150, 255)
        
        drawText("LIKE2D - FRAMEWORK FOR GAME DEVELOPMENT", "Arial.ttf_18", 20, 600, 100, 100, 255)
    else
        -- Fonts failed to load - show fallback with direct text only
        drawTextDirect("Like2D Framework v1.1.0", 20, 40, 24, 255, 200, 100)
        drawTextDirect("Font Rendering Demo (Fallback Mode)", 20, 80, 18, 255, 255, 255)
        drawTextDirect("Arial.ttf not found - using direct rendering", 20, 120, 16, 255, 150, 150)
        drawTextDirect("Place Arial.ttf in scripts folder to enable full demo", 20, 150, 14, 200, 200, 255)
        
        local time = getTime()
        drawTextDirect("Runtime: " .. math.floor(time/1000) .. "s", 20, 200, 16, 150, 150, 255)
        
        drawTextDirect("LIKE2D - Game Development", 20, 560, 16, 100, 100, 255)
    end
    
    -- Always render the image if available
    renderImage("logo.png", 500, 200, 200, 200)
end