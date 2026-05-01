# Like2D Game Framework

A lightweight, high-performance 2D game framework built with SDL3 and Luau scripting language.

## Features

- **Modern SDL3 Backend**: Latest SDL3 for graphics, audio, and input handling
- **Luau Scripting**: Fast, efficient Luau VM for game logic and scripting
- **DLL Architecture**: Modular engine (Like.dll) + lightweight launcher (Like2D.exe)
- **Multi-Format Image Support**: PNG, JPG, BMP support via STB Image
- **Incremental Builds**: Fast compilation with object file caching
- **Professional Toolchain**: C++20, CMake, MinGW-w64 support
- **Cross-Platform**: Windows (primary), Linux, macOS ready

## Dependencies

### Required Dependencies
- **SDL3** (v3.0+): Graphics and multimedia framework
  - Source: https://github.com/libsdl-org/SDL
  - Build: CMake + MinGW-w64
- **Luau** (latest): Roblox's Lua interpreter
  - Source: https://github.com/Roblox/luau
  - Build: CMake + MinGW-w64
- **STB Image**: Single-header image loading library
  - Included: `external/stb/stb_image.h`

### Build Tools
- **CMake** 3.20+
- **MinGW-w64** (Windows) or GCC (Linux/macOS)
- **Git** (for dependency management)

## Quick Start

### Option 1: Incremental Build (Recommended)
```bash
# Build with existing dependencies
build.bat

# Clean build directory
build.bat clean

# Create distribution package
build.bat dist
```

### Option 2: Full Build (Builds Dependencies)
```bash
# Build everything from source
build-full.bat
```

## Project Structure

```
Like2D/
├── src/                    # C++ source files
│   ├── core/              # Engine core (Engine.cpp, Engine.h)
│   ├── renderer/          # Rendering system (Renderer.cpp, Renderer.h)
│   ├── scripting/         # Lua bindings (LuaBindings.cpp, LuaBindings.h)
│   ├── Like2D.cpp         # Application class (DLL)
│   └── main.cpp           # Launcher executable
├── include/               # Public headers
│   └── Like2D.h          # Main API header with DLL exports
├── scripts/               # Luau game scripts
│   └── main.luau         # Main entry point
├── external/              # Third-party libraries
│   ├── SDL3/             # SDL3 source/build
│   ├── Luau/             # Luau source/build
│   └── stb/              # STB Image library
├── build/                 # Build output directory
├── docs/                  # Documentation
├── resource.rc            # Windows resources (icon, version info)
├── build.bat              # Incremental build script
└── build-full.bat         # Full build script
```

## Documentation

Comprehensive documentation is available in the `docs/` folder:

- **[Getting Started Guide](docs/GETTING_STARTED.md)** - Complete beginner's tutorial
- **[API Reference](docs/API_REFERENCE.md)** - Detailed function documentation
- **[Tutorials](docs/TUTORIALS.md)** - Step-by-step tutorials from basics to complete games
- **[Build System](docs/BUILD_SYSTEM.md)** - Build configuration and troubleshooting

### Quick Documentation Links
- [📖 Getting Started](docs/GETTING_STARTED.md) - Setup and first application
- [🔧 API Reference](docs/API_REFERENCE.md) - Complete function reference
- [📚 Tutorials](docs/TUTORIALS.md) - 8 comprehensive tutorials
- [🔨 Build System](docs/BUILD_SYSTEM.md) - Build configuration guide

## Auto-Setup

The build system automatically:
1. **Checks Dependencies**: Verifies `external/include` and `external/lib` folders
2. **Creates Structure**: Sets up proper directory layout if missing
3. **Provides Instructions**: Guides users to download missing SDKs
4. **Incremental Compilation**: Only compiles modified files for speed

## API Reference

### C++ API (DLL Export)
```cpp
// Main application class
namespace Like2D {
    class LIKE2D_API Application {
    public:
        bool initialize(int argc, char* argv[]);
        void run();
        void shutdown();
    };
}
```

### Luau Scripting API
```lua
-- Window management
setWindowTitle("Game Title")

-- Asset management
loadImage("logo.png")

-- Rendering (called every frame)
function onDraw()
    renderImage("logo.png", 100, 100, 200, 150)
end
```

## Build System Features

### Incremental Builds
- **Object File Caching**: Compiles only modified `.cpp` files
- **Resource Integration**: Automatic `.rc` compilation with `windres`
- **Dependency Tracking**: Timestamp-based file change detection
- **Smart Linking**: Only links when object files change

### Distribution Package
```bash
build.bat dist
```
Creates `Like2D-Dist/` folder with:
- `Like2D.exe` (launcher)
- `Like.dll` (engine)
- `SDL3.dll` (runtime)
- `scripts/` (sample scripts)
- `license.txt` (project license)
- `updates.txt` (release notes)

## Performance Optimizations

- **C++20 Standards**: Modern C++ features for performance
- **Static Linking**: Reduced runtime dependencies
- **Optimized Flags**: `-O2` for release builds
- **DLL Architecture**: Efficient memory usage and modularity
- **Texture Caching**: Automatic image loading and caching

## Development Workflow

1. **Setup**: Run `build.bat` (auto-creates dependency folders)
2. **Development**: Modify source files, use `build.bat` for fast incremental builds
3. **Testing**: Run from `build/` directory with scripts
4. **Distribution**: Use `build.bat dist` for release package

## License

This project is provided as-is for educational and development purposes by SSOFoundation.

## Support

For issues and questions:
- Check the build logs for detailed error information
- Verify all dependencies are properly installed
- Ensure CMake and MinGW-w64 are in your PATH
- Review the documentation in the `docs/` folder

## Support the Project

If you find Like2D Framework useful, please consider supporting its development:

**Donate:** [saweria.co/omedonation](https://saweria.co/omedonation)

Your support helps maintain and improve the framework for everyone!
