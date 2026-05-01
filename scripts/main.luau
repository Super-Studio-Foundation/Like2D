-- Like2D Framework Main Script
print("Hello from Luau! Like2D Framework is running!")

-- Test window control functions
setWindowTitle("Like2D Framework - Professional Window Controls")

setWindowSize(1024, 768)

-- Load an image (PNG/JPG/BMP supported)
loadImage("logo.png")

print("Window title and size set!")
print("Image loaded successfully!")
print("Use setFullscreen(true) in your script to enable fullscreen mode.")

-- onDraw function called every frame
function onDraw()
    -- Clear screen with a nice background color
    clearScreen(50, 50, 80)
    
    -- Render the loaded image centered in the window
    renderImage("logo.png", 412, 284, 200, 200)
end
