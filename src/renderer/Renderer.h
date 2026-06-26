#pragma once
#include <unordered_map>
#include <map>
#include <string>
#include <unordered_set>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>
#include "../external/stb/stb_truetype.h"

struct FontChar {
    stbtt_packedchar packed;
    float u0, v0, u1, v1;
};

struct FontData {
    std::vector<unsigned char> fontData;
    stbtt_fontinfo fontInfo;
    int size;
    bool loaded;
};

struct FontAtlas {
    SDL_Texture* texture;
    int width, height;
    std::map<char, FontChar> chars;
    float fontSize;
    float ascent;     // pixels from baseline to top of tallest glyph
    float descent;    // pixels from baseline to bottom of lowest descender (positive = below)
    float lineHeight; // total line spacing in pixels
    bool loaded;
};

// VideoState is defined in Renderer.cpp (opaque to the header)
// so pl_mpeg.h is never pulled into any other translation unit.
struct VideoState;

class Renderer {
private:
    SDL_Renderer* renderer;
    std::unordered_map<std::string, SDL_Texture*>          textureCache;
    std::unordered_map<std::string, FontAtlas>             fontCache;
    std::unordered_map<std::string, FontData>              ttfFontCache;
    std::unordered_map<std::string, std::unique_ptr<VideoState>> videoCache;
    std::string projectFolder;
    std::unordered_set<std::string> reportedErrors;
    std::string m_defaultFontPath;

    // Negative cache: remember files that don't exist to avoid repeated filesystem probes
    std::unordered_set<std::string> m_failedPaths;

    bool bakeFontAtlas(const std::string& fontPath, float fontSize, FontAtlas& atlas);
    bool bakeFontAtlasFromMemory(const std::vector<unsigned char>& fontData, float fontSize, FontAtlas& atlas);
    bool loadFontFile(const std::string& fontPath, std::vector<unsigned char>& fontData);
    bool loadFontFromGameLike(const std::string& filename, int size);

public:
    Renderer(SDL_Renderer* renderer, const std::string& projectFolder);
    ~Renderer();

    bool loadImage(const std::string& filename);
    bool loadImageFromGameLike(const std::string& filename);
    bool loadImageFromEncryptedFile(const std::string& filename);
    bool renderImage(const std::string& key, int x, int y, int width, int height);
    bool renderImageEx(const std::string& key, float x, float y, float width, float height,
                       float angle, bool flipH, bool flipV, float alpha,
                       float originX, float originY);
    bool renderImageExColor(const std::string& key, float x, float y, float width, float height,
                            float angle, bool flipH, bool flipV, float alpha,
                            float originX, float originY, int r, int g, int b);
    bool renderImageRegion(const std::string& key,
                           float dx, float dy, float dw, float dh,
                           float srcX, float srcY, float srcW, float srcH,
                           float angle, bool flipH, bool flipV, float alpha,
                           float originX, float originY);
    bool getImageSize(const std::string& key, int& w, int& h);

    // ── TileMap API ────────────────────────────────────────────
    bool drawTileMap(const std::string& tilesetKey,
                     int tileW, int tileH, int cols, int rows,
                     const std::vector<int>& tiles,
                     float x, float y, float alpha);
    void* getTextureHandle(const std::string& key);
    bool loadFont(const std::string& filename, int size);
    bool drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b);
    bool drawTextDirect(const std::string& text, int x, int y, int size, int r, int g, int b, int a = 255, int anchor = 0);
    bool getTextSize(const std::string& text, int size, int& outW, int& outH);
    void setDrawColor(int r, int g, int b, int a = 255);
    void clear();
    void setDefaultFont(const std::string& path);
    SDL_Renderer* getRenderer() const;
    SDL_Renderer* getSDLRenderer() const { return renderer; }
    FontData* getFont(const std::string& fontKey) const;
    const std::string& getProjectFolder() const;

    void drawLine(float x1, float y1, float x2, float y2, int r, int g, int b, int a);
    void drawCircle(float cx, float cy, float radius, int r, int g, int b, int a, bool filled);
    void drawRectEx(float x, float y, float w, float h, int r, int g, int b, int a, bool filled, float angle);

    // ── Video API ─────────────────────────────────────────────
    bool   loadVideo(const std::string& key, const std::string& filename);
    void   updateVideo(const std::string& key, double dt);
    bool   renderVideo(const std::string& key, float x, float y, float w, float h);
    void   playVideo(const std::string& key, bool loop);
    void   pauseVideo(const std::string& key);
    void   stopVideo(const std::string& key);
    void   seekVideo(const std::string& key, double seconds);
    bool   isVideoPlaying(const std::string& key);
    bool   isVideoEnded(const std::string& key);
    double getVideoDuration(const std::string& key);
    double getVideoTime(const std::string& key);
    void   unloadVideo(const std::string& key);
};
