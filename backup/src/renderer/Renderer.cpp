#include "Renderer.h"
#include <iostream>
#include <unordered_set>

Renderer::Renderer(SDL_Renderer* renderer, const std::string& projectFolder) 
    : renderer(renderer), projectFolder(projectFolder) {
}

Renderer::~Renderer() {
    for (auto& pair : textureCache) {
        SDL_DestroyTexture(pair.second);
    }
}

bool Renderer::loadImage(const std::string& filename) {
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        return it->second != nullptr;
    }
    
    std::string fullPath;
    if (!projectFolder.empty() && projectFolder.back() != '/' && projectFolder.back() != '\\') {
        fullPath = projectFolder + "/" + filename;
    } else {
        fullPath = projectFolder + filename;
    }
    
    SDL_Surface* surface = SDL_LoadBMP(fullPath.c_str());
    if (!surface) {
        std::string errorMsg = "Failed to load image: " + fullPath + " - " + SDL_GetError();
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << errorMsg << std::endl;
        }
        return false;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    
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
        std::string errorMsg = "Image not found in cache with key: '" + key + "'";
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    SDL_Texture* texture = it->second;
    if (texture == nullptr) {
        std::string errorMsg = "Texture pointer is NULL for key: '" + key + "'";
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    SDL_FRect destRect = { (float)x, (float)y, (float)width, (float)height };
    
    if (SDL_RenderTexture(renderer, texture, NULL, &destRect) != 0) {
        std::string errorMsg = "Failed to render texture '" + key + "': " + std::string(SDL_GetError());
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        return false;
    }
    
    return true;
}

bool Renderer::loadFont(const std::string& filename, int size) {
    std::cerr << "SDL3_ttf not yet integrated - loadFont() disabled" << std::endl;
    return false;
}

bool Renderer::drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b) {
    std::cerr << "SDL3_ttf not yet integrated - drawText() disabled" << std::endl;
    return false;
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
