#pragma once
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
#include <SDL3/SDL.h>
#include "../external/stb/stb_truetype.h"

// Font character structure for stb_truetype
struct FontChar {
    stbtt_packedchar packed;
    float u0, v0, u1, v1;  // Texture coordinates
};

// Font atlas structure
struct FontAtlas {
    SDL_Texture* texture;
    int width, height;
    std::map<char, FontChar> chars;
    float fontSize;
    bool loaded;
};

class Renderer {
private:
    SDL_Renderer* renderer;
    std::map<std::string, SDL_Texture*> textureCache;
    std::map<std::string, FontAtlas> fontCache;
    std::string projectFolder;
    std::unordered_set<std::string> reportedErrors;
    
    // Font rendering helper methods
    bool bakeFontAtlas(const std::string& fontPath, float fontSize, FontAtlas& atlas);
    bool loadFontFile(const std::string& fontPath, std::vector<unsigned char>& fontData);

public:
    Renderer(SDL_Renderer* renderer, const std::string& projectFolder);
    ~Renderer();
    
    bool loadImage(const std::string& filename);
    bool renderImage(const std::string& key, int x, int y, int width, int height);
    bool loadFont(const std::string& filename, int size);
    bool drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b);
    bool drawTextDirect(const std::string& text, int x, int y, int size, int r, int g, int b, int a = 255);
    void setDrawColor(int r, int g, int b);
    void clear();
};
