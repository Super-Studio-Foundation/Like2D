#include "Renderer.h"
#include "../platform.h"
#include <iostream>
#include <unordered_set>
#include <fstream>
#include <cmath>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../external/stb/stb_truetype.h"
#include "../external/miniz/miniz.h"

// pl_mpeg — MPEG-1 video decoder
#define PL_MPEG_IMPLEMENTATION
#include "../external/pl_mpeg/pl_mpeg.h"

// ── VideoState — defined here so pl_mpeg types are complete ──
// Must be before Renderer constructor/destructor.
struct VideoState {
    plm_t*        plm     = nullptr;
    SDL_Texture*  texture = nullptr;
    int           width   = 0;
    int           height  = 0;
    double        time    = 0.0;
    bool          playing = false;
    bool          looping = false;
    bool          ended   = false;
    std::vector<uint8_t> rgbBuf;
};

static void uploadVideoFrame(SDL_Renderer* sdlRenderer, VideoState& vs, plm_frame_t* frame) {
    int w = frame->width;
    int h = frame->height;
    vs.rgbBuf.resize((size_t)w * h * 4);
    plm_frame_to_rgba(frame, vs.rgbBuf.data(), w * 4);
    if (!vs.texture || vs.width != w || vs.height != h) {
        if (vs.texture) SDL_DestroyTexture(vs.texture);
        vs.texture = SDL_CreateTexture(sdlRenderer,
            SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w, h);
        vs.width = w; vs.height = h;
    }
    if (vs.texture) {
        void* pixels = nullptr; int pitch = 0;
        if (SDL_LockTexture(vs.texture, nullptr, &pixels, &pitch) == 0) {
            for (int row = 0; row < h; row++)
                memcpy(static_cast<uint8_t*>(pixels) + row * pitch,
                       vs.rgbBuf.data() + row * w * 4, (size_t)w * 4);
            SDL_UnlockTexture(vs.texture);
        }
    }
}

Renderer::Renderer(SDL_Renderer* renderer, const std::string& projectFolder) 
    : renderer(renderer), projectFolder(projectFolder), m_defaultFontPath("") {
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
    // Destroy all loaded videos
    for (auto& pair : videoCache) {
        if (pair.second && pair.second->texture) SDL_DestroyTexture(pair.second->texture);
        if (pair.second && pair.second->plm)     plm_destroy(pair.second->plm);
    }
}

bool Renderer::loadImage(const std::string& filename) {
    auto it = textureCache.find(filename);
    if (it != textureCache.end()) {
        return it->second != nullptr;
    }
    
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
    std::filesystem::path gameLikePath = exePath / "game.like";
    
    if (std::filesystem::exists(gameLikePath)) {
        if (loadImageFromGameLike(filename)) {
            return true;
        }
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
    
    // Debug output removed
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

FontData* Renderer::getFont(const std::string& fontKey) const {
    auto it = ttfFontCache.find(fontKey);
    if (it != ttfFontCache.end()) {
        return const_cast<FontData*>(&it->second);
    }
    return nullptr;
}

SDL_Renderer* Renderer::getRenderer() const {
    return renderer;
}

bool Renderer::renderImage(const std::string& key, int x, int y, int width, int height) {
    if (!renderer) return false;
    
    auto it = textureCache.find(key);
    if (it == textureCache.end()) {
        return false;
    }
    
    SDL_Texture* texture = it->second;
    if (!texture) {
        return false;
    }
    
    SDL_FRect destRect = { (float)x, (float)y, (float)width, (float)height };
    
    return SDL_RenderTexture(renderer, texture, NULL, &destRect) == 0;
}

bool Renderer::loadFont(const std::string& filename, int size) {
    std::string fontKey = filename + "_" + std::to_string(size);
    
    // Check if font is already loaded
    auto it = fontCache.find(fontKey);
    if (it != fontCache.end()) {
        return it->second.loaded;
    }

    // Production mode: load from game.like
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
    std::filesystem::path gameLikePath = exePath / "game.like";

    if (std::filesystem::exists(gameLikePath)) {
        if (loadFontFromGameLike(filename, size)) {
            return true;
        }
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
        if (atlas.texture) { SDL_DestroyTexture(atlas.texture); atlas.texture = nullptr; }
    } catch (...) {
        std::string errorMsg = "Unknown exception during font loading: " + fullPath;
        if (reportedErrors.insert(errorMsg).second) {
            std::cerr << "ERROR: " << errorMsg << std::endl;
        }
        if (atlas.texture) { SDL_DestroyTexture(atlas.texture); atlas.texture = nullptr; }
    }
    
    return false;
}

bool Renderer::drawText(const std::string& text, const std::string& fontKey, int x, int y, int r, int g, int b) {
    if (!renderer) return false;
    
    auto it = fontCache.find(fontKey);
    if (it == fontCache.end()) return false;
    
    FontAtlas& atlas = it->second;
    if (!atlas.loaded || !atlas.texture) return false;
    
    SDL_SetTextureColorMod(atlas.texture, r, g, b);
    
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

void Renderer::setDrawColor(int r, int g, int b, int a) {
    if (renderer) {
        SDL_SetRenderDrawColor(renderer, r, g, b, a);
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
    
    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    
    atlas.loaded = true;
    return true;
}

bool Renderer::drawTextDirect(const std::string& text, int x, int y, int size, int r, int g, int b, int a) {
    if (!renderer) return false;
    if (m_defaultFontPath.empty()) return false;  // no default font set — fail silently

    std::string fontKey = m_defaultFontPath + "_" + std::to_string(size);
    
    if (fontCache.find(fontKey) == fontCache.end()) {
        if (!loadFont(m_defaultFontPath, size)) {
            return false;
        }
    }
    
    return drawText(text, fontKey, x, y, r, g, b);
}

void Renderer::setDefaultFont(const std::string& path) {
    m_defaultFontPath = path;
}

const std::string& Renderer::getProjectFolder() const {
    return projectFolder;
}

// Shared helper: build the list of paths to try inside game.like for a given filename
static std::vector<std::string> buildSearchPaths(const std::string& filename) {
    // Normalise backslashes
    std::string norm = filename;
    for (char& c : norm) if (c == '\\') c = '/';

    std::vector<std::string> paths;
    paths.push_back(norm);  // exact match first (most common case)

    // If the caller passed a bare name like "Sprite.png", also try common subdirs
    if (norm.find('/') == std::string::npos) {
        paths.push_back("assets/"  + norm);
        paths.push_back("fonts/"   + norm);
        paths.push_back("audio/"   + norm);
        paths.push_back("sounds/"  + norm);
        paths.push_back("data/"    + norm);
    }
    // If the caller passed "assets/Sprite.png", also try bare name as fallback
    else {
        auto slash = norm.rfind('/');
        std::string bare = norm.substr(slash + 1);
        paths.push_back(bare);
    }
    return paths;
}

bool Renderer::loadImageFromGameLike(const std::string& filename) {
    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
    std::filesystem::path gameLikePath = exePath / "game.like";

    // ── Read + decrypt game.like (same logic as LuaBindings) ──
    static const uint8_t ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
    static const uint8_t XOR_KEY[]   = {
        0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
        0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
    };
    static const size_t XOR_KEY_LEN = sizeof(XOR_KEY);

    // Use a static cache so we only decrypt once per process lifetime
    static std::vector<char> s_zipCache;
    static std::string       s_zipCachePath;
    std::string pathStr = gameLikePath.string();

    if (s_zipCachePath != pathStr || s_zipCache.empty()) {
        std::ifstream rf(gameLikePath, std::ios::binary);
        if (!rf.is_open()) {
            std::cerr << "ERROR: Cannot open game.like for image loading" << std::endl;
            return false;
        }
        std::vector<char> raw((std::istreambuf_iterator<char>(rf)), {});
        rf.close();

        if (raw.size() >= 4 &&
            (uint8_t)raw[0] == ENC_MAGIC[0] && (uint8_t)raw[1] == ENC_MAGIC[1] &&
            (uint8_t)raw[2] == ENC_MAGIC[2] && (uint8_t)raw[3] == ENC_MAGIC[3])
        {
            s_zipCache.assign(raw.begin() + 4, raw.end());
            for (size_t i = 0; i < s_zipCache.size(); i++)
                s_zipCache[i] ^= (char)XOR_KEY[i % XOR_KEY_LEN];
        } else {
            s_zipCache = std::move(raw);
        }
        s_zipCachePath = pathStr;
    }

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, s_zipCache.data(), s_zipCache.size(), 0)) {
        std::cerr << "ERROR: Failed to open game.like archive for image loading" << std::endl;
        return false;
    }

    for (const std::string& searchPath : buildSearchPaths(filename)) {
        int fileIndex = mz_zip_reader_locate_file(&zip, searchPath.c_str(), nullptr, 0);
        if (fileIndex < 0) continue;

        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, fileIndex, &stat)) continue;

        std::vector<unsigned char> fileData(stat.m_uncomp_size);
        if (!mz_zip_reader_extract_to_mem(&zip, fileIndex, fileData.data(), stat.m_uncomp_size, 0)) continue;

        int width, height, channels;
        unsigned char* imageData = stbi_load_from_memory(fileData.data(), (int)stat.m_uncomp_size,
                                                          &width, &height, &channels, 4);
        if (imageData) {
            SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32,
                                                          imageData, width * 4);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                SDL_DestroySurface(surface);
                if (texture) {
                    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    textureCache[filename] = texture;
                    stbi_image_free(imageData);
                    mz_zip_reader_end(&zip);
                    return true;
                }
            }
            stbi_image_free(imageData);
        }
    }

    std::cerr << "ERROR: Could not find image '" << filename << "' in game.like" << std::endl;
    mz_zip_reader_end(&zip);
    return false;
}

bool Renderer::loadFontFromGameLike(const std::string& filename, int size) {
    std::string fontKey = filename + "_" + std::to_string(size);

    char exePathBuf[MAX_PATH];
    GetModuleFileNameA(NULL, exePathBuf, MAX_PATH);
    std::filesystem::path exePath = std::filesystem::path(exePathBuf).parent_path();
    std::filesystem::path gameLikePath = exePath / "game.like";

    // Reuse the same static decrypt cache as loadImageFromGameLike
    static const uint8_t ENC_MAGIC[] = { 'L', '2', 'D', 'E' };
    static const uint8_t XOR_KEY[]   = {
        0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
        0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
    };
    static const size_t XOR_KEY_LEN = sizeof(XOR_KEY);
    static std::vector<char> s_zipCache;
    static std::string       s_zipCachePath;
    std::string pathStr = gameLikePath.string();

    if (s_zipCachePath != pathStr || s_zipCache.empty()) {
        std::ifstream rf(gameLikePath, std::ios::binary);
        if (!rf.is_open()) return false;
        std::vector<char> raw((std::istreambuf_iterator<char>(rf)), {});
        rf.close();
        if (raw.size() >= 4 &&
            (uint8_t)raw[0] == ENC_MAGIC[0] && (uint8_t)raw[1] == ENC_MAGIC[1] &&
            (uint8_t)raw[2] == ENC_MAGIC[2] && (uint8_t)raw[3] == ENC_MAGIC[3])
        {
            s_zipCache.assign(raw.begin() + 4, raw.end());
            for (size_t i = 0; i < s_zipCache.size(); i++)
                s_zipCache[i] ^= (char)XOR_KEY[i % XOR_KEY_LEN];
        } else {
            s_zipCache = std::move(raw);
        }
        s_zipCachePath = pathStr;
    }

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, s_zipCache.data(), s_zipCache.size(), 0)) {
        std::cerr << "ERROR: Failed to open game.like for font loading" << std::endl;
        return false;
    }

    for (const std::string& searchPath : buildSearchPaths(filename)) {
        int fileIndex = mz_zip_reader_locate_file(&zip, searchPath.c_str(), nullptr, 0);
        if (fileIndex < 0) continue;

        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, fileIndex, &stat)) continue;

        size_t fileSize = stat.m_uncomp_size;
        std::vector<unsigned char> fontData(fileSize);
        if (!mz_zip_reader_extract_to_mem(&zip, fileIndex, fontData.data(), fileSize, 0)) continue;

        mz_zip_reader_end(&zip);

        FontAtlas atlas;
        atlas.fontSize = (float)size;
        atlas.loaded   = false;
        atlas.texture  = nullptr;

        if (bakeFontAtlasFromMemory(fontData, (float)size, atlas)) {
            fontCache[fontKey] = atlas;
            return true;
        }

        std::string errorMsg = "Failed to bake font atlas from game.like: " + filename;
        if (reportedErrors.insert(errorMsg).second)
            std::cerr << "ERROR: " << errorMsg << std::endl;
        return false;
    }

    mz_zip_reader_end(&zip);
    std::string errorMsg = "Font not found in game.like: " + filename;
    if (reportedErrors.insert(errorMsg).second)
        std::cerr << "ERROR: " << errorMsg << std::endl;
    return false;
}

bool Renderer::bakeFontAtlasFromMemory(const std::vector<unsigned char>& fontData, float fontSize, FontAtlas& atlas) {
    if (!renderer) return false;

    const int firstChar = 32;
    const int charCount = 96;

    atlas.width  = 512;
    atlas.height = 512;

    std::vector<unsigned char> atlasBitmap(atlas.width * atlas.height, 0);

    stbtt_pack_context packContext;
    if (!stbtt_PackBegin(&packContext, atlasBitmap.data(), atlas.width, atlas.height, 0, 1, nullptr)) {
        std::cerr << "ERROR: Failed to initialize font packer (from memory)" << std::endl;
        return false;
    }

    std::vector<stbtt_packedchar> packedChars(charCount);
    if (!stbtt_PackFontRange(&packContext, fontData.data(), 0, fontSize, firstChar, charCount, packedChars.data())) {
        std::cerr << "ERROR: Failed to pack font range (from memory)" << std::endl;
        stbtt_PackEnd(&packContext);
        return false;
    }
    stbtt_PackEnd(&packContext);

    for (int i = 0; i < charCount; i++) {
        char c = firstChar + i;
        FontChar fontChar;
        fontChar.packed = packedChars[i];
        fontChar.u0 = (float)fontChar.packed.x0 / atlas.width;
        fontChar.v0 = (float)fontChar.packed.y0 / atlas.height;
        fontChar.u1 = (float)fontChar.packed.x1 / atlas.width;
        fontChar.v1 = (float)fontChar.packed.y1 / atlas.height;
        atlas.chars[c] = fontChar;
    }

    // Convert alpha8 -> RGBA32
    std::vector<unsigned char> rgbaBitmap(atlas.width * atlas.height * 4);
    for (int y = 0; y < atlas.height; y++) {
        for (int x = 0; x < atlas.width; x++) {
            int src = y * atlas.width + x;
            int dst = src * 4;
            rgbaBitmap[dst + 0] = 255;
            rgbaBitmap[dst + 1] = 255;
            rgbaBitmap[dst + 2] = 255;
            rgbaBitmap[dst + 3] = atlasBitmap[src];
        }
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(atlas.width, atlas.height, SDL_PIXELFORMAT_RGBA32,
                                                  rgbaBitmap.data(), atlas.width * 4);
    if (!surface) {
        std::cerr << "ERROR: Failed to create font atlas surface: " << SDL_GetError() << std::endl;
        return false;
    }

    atlas.texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);

    if (!atlas.texture) {
        std::cerr << "ERROR: Failed to create font atlas texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(atlas.texture, SDL_BLENDMODE_BLEND);
    atlas.loaded = true;
    return true;
}

// ---- renderImageEx: rotation, flip, alpha, custom origin ----
bool Renderer::renderImageEx(const std::string& key,
                              float x, float y, float width, float height,
                              float angle, bool flipH, bool flipV,
                              float alpha, float originX, float originY) {
    if (!renderer) return false;

    auto it = textureCache.find(key);
    if (it == textureCache.end() || !it->second) return false;

    SDL_Texture* texture = it->second;

    SDL_SetTextureAlphaMod(texture, (Uint8)(alpha * 255.0f));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_FRect destRect = { x, y, width, height };
    SDL_FPoint center  = { originX * width, originY * height };

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flipH && flipV) flip = (SDL_FlipMode)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    else if (flipH)     flip = SDL_FLIP_HORIZONTAL;
    else if (flipV)     flip = SDL_FLIP_VERTICAL;

    bool ok = SDL_RenderTextureRotated(renderer, texture, NULL, &destRect, angle, &center, flip) == 0;

    // Restore full alpha so other renders aren't affected
    SDL_SetTextureAlphaMod(texture, 255);
    return ok;
}

// ---- renderImageRegion: sprite sheet sub-rect with full Ex params ----
bool Renderer::renderImageRegion(const std::string& key,
                                  float dx, float dy, float dw, float dh,
                                  float srcX, float srcY, float srcW, float srcH,
                                  float angle, bool flipH, bool flipV, float alpha,
                                  float originX, float originY) {
    if (!renderer) return false;
    auto it = textureCache.find(key);
    if (it == textureCache.end() || !it->second) return false;

    SDL_Texture* texture = it->second;
    SDL_SetTextureAlphaMod(texture, (Uint8)(alpha * 255.0f));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    SDL_FRect srcRect = { srcX, srcY, srcW, srcH };
    SDL_FRect dstRect = { dx, dy, dw, dh };
    SDL_FPoint center = { originX * dw, originY * dh };

    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flipH && flipV) flip = (SDL_FlipMode)(SDL_FLIP_HORIZONTAL | SDL_FLIP_VERTICAL);
    else if (flipH)     flip = SDL_FLIP_HORIZONTAL;
    else if (flipV)     flip = SDL_FLIP_VERTICAL;

    bool ok = SDL_RenderTextureRotated(renderer, texture, &srcRect, &dstRect,
                                        angle, &center, flip) == 0;
    SDL_SetTextureAlphaMod(texture, 255);
    return ok;
}

// ---- getImageSize ----
bool Renderer::getImageSize(const std::string& key, int& w, int& h) {
    auto it = textureCache.find(key);
    if (it == textureCache.end() || !it->second) return false;
    float fw, fh;
    if (SDL_GetTextureSize(it->second, &fw, &fh) != 0) return false;
    w = (int)fw;
    h = (int)fh;
    return true;
}

// ---- Primitives ----

void Renderer::drawLine(float x1, float y1, float x2, float y2, int r, int g, int b, int a) {
    if (!renderer) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
    SDL_RenderLine(renderer, x1, y1, x2, y2);
}

void Renderer::drawCircle(float cx, float cy, float radius, int r, int g, int b, int a, bool filled) {
    if (!renderer) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);

    // Midpoint circle algorithm
    int ri = (int)radius;
    if (filled) {
        for (int dy = -ri; dy <= ri; dy++) {
            int dx = (int)sqrtf((float)(ri * ri - dy * dy));
            SDL_RenderLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    } else {
        int x = ri, y = 0, err = 0;
        while (x >= y) {
            SDL_RenderPoint(renderer, cx + x, cy + y);
            SDL_RenderPoint(renderer, cx + y, cy + x);
            SDL_RenderPoint(renderer, cx - y, cy + x);
            SDL_RenderPoint(renderer, cx - x, cy + y);
            SDL_RenderPoint(renderer, cx - x, cy - y);
            SDL_RenderPoint(renderer, cx - y, cy - x);
            SDL_RenderPoint(renderer, cx + y, cy - x);
            SDL_RenderPoint(renderer, cx + x, cy - y);
            y++;
            err += 2 * y + 1;
            if (2 * (err - x) + 1 > 0) { x--; err += 2 * (1 - x); }
        }
    }
}

void Renderer::drawRectEx(float x, float y, float w, float h,
                           int r, int g, int b, int a, bool filled, float angle) {
    if (!renderer) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);

    if (angle == 0.0f) {
        SDL_FRect rect = { x, y, w, h };
        if (filled) SDL_RenderFillRect(renderer, &rect);
        else        SDL_RenderRect(renderer, &rect);
        return;
    }

    // Rotated rect: compute 4 corners around center
    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;
    float rad = angle * 3.14159265f / 180.0f;
    float cosA = cosf(rad), sinA = sinf(rad);

    float hw = w * 0.5f, hh = h * 0.5f;
    float corners[4][2] = {
        { -hw, -hh }, {  hw, -hh },
        {  hw,  hh }, { -hw,  hh }
    };
    float rotated[4][2];
    for (int i = 0; i < 4; i++) {
        rotated[i][0] = cx + corners[i][0] * cosA - corners[i][1] * sinA;
        rotated[i][1] = cy + corners[i][0] * sinA + corners[i][1] * cosA;
    }

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;
        SDL_RenderLine(renderer, rotated[i][0], rotated[i][1], rotated[j][0], rotated[j][1]);
    }

    if (filled) {
        // Fill using horizontal scanlines between edges
        float minY = rotated[0][1], maxY = rotated[0][1];
        for (int i = 1; i < 4; i++) {
            if (rotated[i][1] < minY) minY = rotated[i][1];
            if (rotated[i][1] > maxY) maxY = rotated[i][1];
        }
        for (float sy = minY; sy <= maxY; sy += 1.0f) {
            float xIntersects[2]; int count = 0;
            for (int i = 0; i < 4 && count < 2; i++) {
                int j = (i + 1) % 4;
                float y0 = rotated[i][1], y1 = rotated[j][1];
                if ((y0 <= sy && sy < y1) || (y1 <= sy && sy < y0)) {
                    float t = (sy - y0) / (y1 - y0);
                    xIntersects[count++] = rotated[i][0] + t * (rotated[j][0] - rotated[i][0]);
                }
            }
            if (count == 2) {
                if (xIntersects[0] > xIntersects[1]) {
                    float tmp = xIntersects[0]; xIntersects[0] = xIntersects[1]; xIntersects[1] = tmp;
                }
                SDL_RenderLine(renderer, xIntersects[0], sy, xIntersects[1], sy);
            }
        }
    }
}


// ── loadVideo ─────────────────────────────────────────────────// ── loadVideo ─────────────────────────────────────────────────
bool Renderer::loadVideo(const std::string& key, const std::string& filename) {
    if (videoCache.count(key)) return true;

    std::string fullPath;
    if (!projectFolder.empty()) {
        fullPath = projectFolder;
        if (fullPath.back() != '/' && fullPath.back() != '\\') fullPath += '/';
        fullPath += filename;
    } else {
        fullPath = filename;
    }

    plm_t* plm = plm_create_with_filename(fullPath.c_str());
    if (!plm) {
        std::cerr << "loadVideo: failed to open '" << fullPath << "'" << std::endl;
        return false;
    }
    plm_set_audio_enabled(plm, FALSE);

    auto vs       = std::make_unique<VideoState>();
    vs->plm       = plm;
    vs->width     = plm_get_width(plm);
    vs->height    = plm_get_height(plm);
    vs->texture   = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, vs->width, vs->height);

    videoCache[key] = std::move(vs);
    return true;
}

// ── updateVideo ───────────────────────────────────────────────
void Renderer::updateVideo(const std::string& key, double dt) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second) return;
    VideoState& vs = *it->second;
    if (!vs.plm || !vs.playing || vs.ended) return;

    double frameDuration = 1.0 / plm_get_framerate(vs.plm);
    vs.time += dt;

    plm_frame_t* frame = nullptr;
    while (vs.time >= frameDuration) {
        vs.time -= frameDuration;
        frame = plm_decode_video(vs.plm);
        if (!frame) {
            if (vs.looping) {
                plm_rewind(vs.plm);
                vs.time = 0.0;
                frame = plm_decode_video(vs.plm);
            } else {
                vs.playing = false;
                vs.ended   = true;
                break;
            }
        }
    }
    if (frame) uploadVideoFrame(renderer, vs, frame);
}

// ── renderVideo ───────────────────────────────────────────────
bool Renderer::renderVideo(const std::string& key, float x, float y, float w, float h) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second || !it->second->texture) return false;
    SDL_FRect dst = { x, y, w, h };
    SDL_RenderTexture(renderer, it->second->texture, nullptr, &dst);
    return true;
}

// ── playVideo ─────────────────────────────────────────────────
void Renderer::playVideo(const std::string& key, bool loop) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second) return;
    VideoState& vs = *it->second;
    if (vs.ended) { if (vs.plm) plm_rewind(vs.plm); vs.ended = false; vs.time = 0.0; }
    vs.playing = true;
    vs.looping = loop;
}

// ── pauseVideo ────────────────────────────────────────────────
void Renderer::pauseVideo(const std::string& key) {
    auto it = videoCache.find(key);
    if (it != videoCache.end() && it->second) it->second->playing = false;
}

// ── stopVideo ─────────────────────────────────────────────────
void Renderer::stopVideo(const std::string& key) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second) return;
    VideoState& vs = *it->second;
    vs.playing = false; vs.ended = false; vs.time = 0.0;
    if (vs.plm) plm_rewind(vs.plm);
}

// ── seekVideo ─────────────────────────────────────────────────
void Renderer::seekVideo(const std::string& key, double seconds) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second || !it->second->plm) return;
    plm_seek(it->second->plm, seconds, FALSE);
    it->second->time  = 0.0;
    it->second->ended = false;
}

// ── isVideoPlaying ────────────────────────────────────────────
bool Renderer::isVideoPlaying(const std::string& key) {
    auto it = videoCache.find(key);
    return it != videoCache.end() && it->second && it->second->playing;
}

// ── isVideoEnded ──────────────────────────────────────────────
bool Renderer::isVideoEnded(const std::string& key) {
    auto it = videoCache.find(key);
    return it != videoCache.end() && it->second && it->second->ended;
}

// ── getVideoDuration ──────────────────────────────────────────
double Renderer::getVideoDuration(const std::string& key) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second || !it->second->plm) return 0.0;
    return plm_get_duration(it->second->plm);
}

// ── getVideoTime ──────────────────────────────────────────────
double Renderer::getVideoTime(const std::string& key) {
    auto it = videoCache.find(key);
    if (it == videoCache.end() || !it->second || !it->second->plm) return 0.0;
    return plm_get_time(it->second->plm);
}

// ── unloadVideo ───────────────────────────────────────────────
void Renderer::unloadVideo(const std::string& key) {
    auto it = videoCache.find(key);
    if (it == videoCache.end()) return;
    if (it->second) {
        if (it->second->texture) SDL_DestroyTexture(it->second->texture);
        if (it->second->plm)     plm_destroy(it->second->plm);
    }
    videoCache.erase(it);
}
