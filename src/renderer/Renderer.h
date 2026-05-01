#pragma once
#include <map>
#include <string>
#include <unordered_set>
#include <SDL3/SDL.h>

class Renderer {
private:
    SDL_Renderer* renderer;
    std::map<std::string, SDL_Texture*> textureCache;
    std::string projectFolder;
    std::unordered_set<std::string> reportedErrors;

public:
    Renderer(SDL_Renderer* renderer, const std::string& projectFolder);
    ~Renderer();
    
    bool loadImage(const std::string& filename);
    bool renderImage(const std::string& key, int x, int y, int width, int height);
    bool loadFont(const std::string& filename, int size);
    bool drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b);
    void setDrawColor(int r, int g, int b);
    void clear();
};
