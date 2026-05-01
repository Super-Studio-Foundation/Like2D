#include "Renderer.h"
#include <iostream>
#include <unordered_set>
#include <fstream>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../external/stb/stb_truetype.h"

Renderer::Renderer(SDL_Renderer* renderer, const std::string& projectFolder) 
    : renderer(renderer), projectFolder(projectFolder) {
}

Renderer::~Renderer() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
    for (auto& pair : fontCache) {
        if (pair.second.texture) {
            SDL_DestroyTexture(pair.second.texture);
        }
    }
}

bool Renderer::loadImage(const std::string& filename) {
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        return it->second != nullptr;
    }
    
    std::string fullPath;
    if (!projectFolder.empty()) {
        if (projectFolder.back() != '/' && projectFolder.back() != '\\') {
            fullPath = projectFolder + "/" + filename;
        } else {
            fullPath = projectFolder + filename;
        }
    } else {
        fullPath = filename;
    }
    
    // Debug output
    std::cerr << "Trying to load image: " << fullPath << std::endl;
    
    int width, height, channels;
    unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::string errorMsg = "Failed to load image: " + fullPath + " - stb_image error";
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << errorMsg << std::endl;
        }
        return false;
    }
    
    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, data, width * 4);
    if (!surface) {
        std::string errorMsg = "Failed to create surface from image data: " + fullPath + " - " + SDL_GetError();
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << errorMsg << std::endl;
        }
        stbi_image_free(data);
        return false;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(data);
    
    if (!texture) {
        std::string errorMsg = "Failed to create texture: " + std::string(SDL_GetError());
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << errorMsg << std::endl;
        }
        return false;
    }
    
    textureCache[filename] = texture;
    return true;
}

bool Renderer::renderImage(const std::string& key, int x, int y, int width, int height) {
    auto it = textureCache.find(key);
    if (it == textureCache.end()) {
        return false;
    }
    
    SDL_Texture* texture = it->second;
    if (!texture || !renderer) {
        return false;
    }
    
    SDL_FRect destRect = { (float)x, (float)y, (float)width, (float)height };
    
    // Simple render call - no blend mode setting to avoid errors
    return SDL_RenderTexture(renderer, texture, NULL, &destRect) == 0;
}

bool Renderer::loadFont(const std::string& filename, int size) {
    std::string fontKey = filename + "_" + std::to_string(size);
    
    // Check if font is already loaded
    auto it = fontCache.find(fontKey);
    if (it != fontCache.end()) {
        return it->second.loaded;
    }
    
    std::string fullPath;
    if (!projectFolder.empty()) {
        if (projectFolder.back() != '/' && projectFolder.back() != '\\') {
            fullPath = projectFolder + "/" + filename;
        } else {
            fullPath = projectFolder + filename;
        }
    } else {
        fullPath = filename;
    }
    
    // Check if font file exists before trying to load
    std::ifstream testFile(fullPath);
    if (!testFile.is_open()) {
        std::string errorMsg = "Font file not found: " + fullPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    testFile.close();
    
    FontAtlas atlas;
    atlas.fontSize = (float)size;
    atlas.loaded = false;
    atlas.texture = nullptr;
    
    try {
        if (bakeFontAtlas(fullPath, (float)size, atlas)) {
            fontCache[fontKey] = atlas;
            return true;
        }
    } catch (const std::exception& e) {
        std::string errorMsg = "Exception during font loading: " + std::string(e.what());
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        // Clean up any partial data
        if (atlas.texture) {
            SDL_DestroyTexture(atlas.texture);
            atlas.texture = nullptr;
        }
    } catch (...) {
        std::string errorMsg = "Unknown exception during font loading: " + fullPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        // Clean up any partial data
        if (atlas.texture) {
            SDL_DestroyTexture(atlas.texture);
            atlas.texture = nullptr;
        }
    }
    
    return false;
}

bool Renderer::drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b) {
    if (!renderer) return false;
    
    auto it = fontCache.find(fontKey);
    if (it == fontCache.end()) return false;
    
    const FontAtlas& atlas = it->second;
    if (!atlas.loaded || !atlas.texture) return false;
    
    SDL_SetTextureColorMod(atlas.texture, r, g, b);
    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    
    float currentX = (float)x;
    float currentY = (float)y;
    
    for (char c : text) {
        if (c < 32 || c >= 127) continue;
        
        auto charIt = atlas.chars.find(c);
        if (charIt == atlas.chars.end()) continue;
        
        const FontChar& fontChar = charIt->second;
        
        float charWidth = (float)(fontChar.packed.x1 - fontChar.packed.x0);
        float charHeight = (float)(fontChar.packed.y1 - fontChar.packed.y0);
        
        if (charWidth <= 0 || charHeight <= 0) {
            currentX += (float)fontChar.packed.xadvance;
            continue;
        }
        
        SDL_FRect srcRect = {
            (float)fontChar.packed.x0,
            (float)fontChar.packed.y0,
            charWidth,
            charHeight
        };
        
        float destX = currentX + (float)fontChar.packed.xoff;
        float destY = currentY + (float)fontChar.packed.yoff;
        
        SDL_FRect destRect = {
            destX,
            destY,
            charWidth,
            charHeight
        };
        
        if (srcRect.w > 0 && srcRect.h > 0 && destRect.w > 0 && destRect.h > 0) {
            SDL_RenderTexture(renderer, atlas.texture, &srcRect, &destRect);
        }
        
        currentX += (float)fontChar.packed.xadvance;
    }
    
    return true;
}

void Renderer::setDrawColor(int r, int g, int b) {
    if (renderer) {
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    }
}

void Renderer::clear() {
    if (renderer) {
        SDL_RenderClear(renderer);
    }
}

bool Renderer::loadFontFile(const std::string& fontPath, std::vector<unsigned char>& fontData) {
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::string errorMsg = "Failed to open font file: " + fontPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    fontData.resize(size);
    if (!file.read(reinterpret_cast<char*>(fontData.data()), size)) {
        std::string errorMsg = "Failed to read font file: " + fontPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    return true;
}

bool Renderer::bakeFontAtlas(const std::string& fontPath, float fontSize, FontAtlas& atlas) {
    // Validate renderer pointer
    if (!renderer) {
        std::string errorMsg = "Renderer pointer is null in bakeFontAtlas";
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    std::vector<unsigned char> fontData;
    if (!loadFontFile(fontPath, fontData)) {
        return false;
    }
    
    // Character range: ASCII 32-127 (space to ~)
    const int firstChar = 32;
    const int charCount = 96;
    
    // Atlas dimensions
    atlas.width = 512;
    atlas.height = 512;
    
    // Create bitmap for atlas
    std::vector<unsigned char> atlasBitmap(atlas.width * atlas.height, 0);
    
    // Initialize font packer
    stbtt_pack_context packContext;
    if (!stbtt_PackBegin(&packContext, atlasBitmap.data(), atlas.width, atlas.height, 0, 1, nullptr)) {
        std::string errorMsg = "Failed to initialize font packer: " + fontPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    // Store packed character data
    std::vector<stbtt_packedchar> packedChars(charCount);
    
    // Pack font characters - this fills both the bitmap and packedChars array
    if (!stbtt_PackFontRange(&packContext, fontData.data(), 0, fontSize, firstChar, charCount, packedChars.data())) {
        std::string errorMsg = "Failed to pack font range: " + fontPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        stbtt_PackEnd(&packContext);
        return false;
    }
    
    stbtt_PackEnd(&packContext);
    
    // Store character info with proper alignment data
    for (int i = 0; i < charCount; i++) {
        char c = firstChar + i;
        FontChar fontChar;
        fontChar.packed = packedChars[i];
        
        // Store UV coordinates (normalized 0.0-1.0)
        fontChar.u0 = (float)fontChar.packed.x0 / atlas.width;
        fontChar.v0 = (float)fontChar.packed.y0 / atlas.height;
        fontChar.u1 = (float)fontChar.packed.x1 / atlas.width;
        fontChar.v1 = (float)fontChar.packed.y1 / atlas.height;
        
        atlas.chars[c] = fontChar;
    }
    
    // Convert alpha8 bitmap to RGBA32 for SDL3
    std::vector<unsigned char> rgbaBitmap(atlas.width * atlas.height * 4);
    for (int y = 0; y < atlas.height; y++) {
        for (int x = 0; x < atlas.width; x++) {
            int srcIndex = y * atlas.width + x;
            int dstIndex = srcIndex * 4;
            
            unsigned char alpha = atlasBitmap[srcIndex];
            rgbaBitmap[dstIndex + 0] = 255; // R
            rgbaBitmap[dstIndex + 1] = 255; // G
            rgbaBitmap[dstIndex + 2] = 255; // B
            rgbaBitmap[dstIndex + 3] = alpha; // A
        }
    }
    
    // Create SDL surface and texture
    SDL_Surface* surface = SDL_CreateSurfaceFrom(atlas.width, atlas.height, SDL_PIXELFORMAT_RGBA32, rgbaBitmap.data(), atlas.width * 4);
    if (!surface) {
        std::string errorMsg = "Failed to create font atlas surface: " + std::string(SDL_GetError());
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    // Validate renderer before creating texture
    if (!renderer) {
        std::string errorMsg = "Renderer became null during font atlas creation";
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        SDL_DestroySurface(surface);
        return false;
    }
    
    atlas.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    
    if (!atlas.texture) {
        std::string errorMsg = "Failed to create font atlas texture: " + std::string(SDL_GetError());
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    // Skip blend mode setting to avoid SDL errors - alpha works fine without it
    
    atlas.loaded = true;
    return true;
}

bool Renderer::drawTextDirect(const std::string& text, int x, int y, int size, int r, int g, int b, int a) {
    if (!renderer) return false;
    
    std::string fontKey = "Arial.ttf_" + std::to_string(size);
    
    if (fontCache.find(fontKey) == fontCache.end()) {
        if (!loadFont("Arial.ttf", size)) {
            return false;
        }
    }
    
    return drawText(text, fontKey, x, y, r, g, b);
}
