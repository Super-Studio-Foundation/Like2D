# Like2D Framework - Build System Documentation

## Overview

The Like2D Framework provides two build systems to accommodate different development needs:

1. **Quick Build** (`build.bat`) - Uses pre-built dependencies for faster compilation
2. **Full Build** (`build-full.bat`) - Compiles all dependencies from source

Both systems create two executables:
- **Like2D.exe** - Release version (Windows subsystem, no console)
- **LikeC.exe** - Debug version (Console subsystem, shows output)

## System Requirements

### Required Tools
- **MinGW-w64** with C++20 support
- **CMake** (for full builds only)
- **Git** (optional, for cloning dependencies)

### Windows Environment
- Windows 10 or later
- Visual Studio Build Tools or standalone MinGW-w64
- PATH should include MinGW-w64 bin directory

## Quick Build System

### Overview
The quick build system uses pre-compiled SDL3 and Luau libraries located in the `external/` folder. This is the recommended approach for most users.

### Directory Structure for Quick Build
```
Like2D/
├── build.bat                 # Quick build script
├── src/                      # Source code
├── external/                 # Pre-built dependencies
│   ├── SDL3/
│   │   ├── build/           # Pre-built SDL3 files
│   │   │   ├── libSDL3.dll.a
│   │   │   ├── SDL3.dll
│   │   │   └── include/     # SDL3 headers
│   └── Luau/
│       ├── build/           # Pre-built Luau files
│       │   ├── libLuau.*.a
│       │   └── include/     # Luau headers
└── build/                    # Build output directory
```

### Running Quick Build
```bash
# Navigate to project directory
cd Like2D

# Run quick build
.\build.bat

# Clean build directory
.\build.bat clean

# Create distribution package
.\build.bat dist
```

### Quick Build Process
1. **Dependency Check** - Verifies required files exist
2. **Resource Compilation** - Compiles `resource.rc` for application metadata
3. **Engine DLL Compilation** - Builds `Like.dll` from source files
4. **Launcher Compilation** - Builds main executable source
5. **Linking** - Creates both Like2D.exe (release) and LikeC.exe (debug)
6. **Deployment** - Copies SDL3.dll to build directory

### Quick Build Configuration
The quick build uses these key settings in `build.bat`:

```batch
set RELEASE_NAME=Like2D.exe
set DEBUG_NAME=LikeC.exe
set DLL_NAME=Like.dll
set ENGINE_SOURCES=src\Like2D.cpp src\core\Engine.cpp src\renderer\Renderer.cpp src\scripting\LuaBindings.cpp
set LAUNCHER_SOURCES=src\main.cpp
```

## Full Build System

### Overview
The full build system compiles SDL3 and Luau from source code using CMake and MinGW. This provides maximum control but takes longer.

### Running Full Build
```bash
# Navigate to project directory
cd Like2D

# Run full build (compiles everything from source)
.\build-full.bat
```

### Full Build Process
1. **Directory Setup** - Creates required build directories
2. **SDL3 Compilation** - Downloads and compiles SDL3 from source
3. **Luau Compilation** - Downloads and compiles Luau from source
4. **Resource Compilation** - Compiles application resources
5. **Engine DLL Build** - Compiles Like2D engine as DLL
6. **Launcher Executables** - Builds release and debug versions
7. **Deployment** - Copies runtime dependencies

### Full Build Dependencies
The full build will automatically:
- Clone SDL3 from official repository
- Clone Luau from Roblox repository
- Configure with CMake using MinGW
- Compile in Release mode with optimizations
- Install headers and libraries to `external/`

## Build Outputs

### Generated Files
After successful build, the `build/` directory contains:

```
build/
├── Like2D.exe          # Release executable
├── LikeC.exe           # Debug executable  
├── Like.dll            # Engine DLL
├── SDL3.dll            # SDL3 runtime library
├── resource.o          # Compiled resources
├── *.o                 # Object files
└── scripts/            # Sample scripts (copied from source)
```

### Executable Differences

| Feature | Like2D.exe (Release) | LikeC.exe (Debug) |
|---------|---------------------|-------------------|
| Subsystem | Windows | Console |
| Console Output | No | Yes |
| Debug Info | Minimal | Full |
| File Size | Smaller | Larger |
| Use Case | Distribution | Development |

## Advanced Build Options

### Custom Configuration
You can modify build settings by editing the batch files:

#### Compiler Flags
```batch
set CFLAGS=-std=c++20 -O2 -static-libgcc -static-libstdc++ -c
set DLL_CFLAGS=-std=c++20 -O2 -static-libgcc -static-libstdc++ -c -DLIKE2D_EXPORTS
```

#### Linker Flags
```batch
set RELEASE_LFLAGS=-Wl,-subsystem,windows
set DEBUG_LFLAGS=-Wl,-subsystem,console
```

#### Library Paths
```batch
set SDL_DIR=external\SDL3\build
set LUAU_DIR=external\Luau\build
```

### Incremental Builds
Both build systems support incremental compilation:
- Only modified source files are recompiled
- Object files are timestamp-checked
- DLL and executables are relinked only when necessary

### Clean Build
To force a complete rebuild:
```bash
.\build.bat clean
```
This removes the entire `build/` directory.

## Distribution Package

### Creating Distribution
```bash
.\build.bat dist
```

This creates a `Like2D-Dist/` folder containing:
- Like2D.exe (release executable)
- LikeC.exe (debug executable)
- Like.dll (engine library)
- SDL3.dll (runtime library)
- scripts/ (sample scripts)
- README.md (documentation)

### Distribution Contents
```
Like2D-Dist/
├── Like2D.exe
├── LikeC.exe
├── Like.dll
├── SDL3.dll
├── scripts/
│   └── main.luau
└── README.md
```

## Troubleshooting

### Common Issues

#### "g++ not found"
**Solution**: Install MinGW-w64 and add to PATH
```bash
# Verify installation
g++ --version

# Add to PATH (example)
set PATH=C:\mingw64\bin;%PATH%
```

#### "CMake not found" (Full Build)
**Solution**: Install CMake and add to PATH
```bash
# Verify installation
cmake --version

# Add to PATH (example)
set PATH=C:\cmake\bin;%PATH%
```

#### "SDL3.dll not found"
**Solution**: Ensure SDL3.dll is in build directory
```bash
# Copy manually if needed
copy external\SDL3\build\SDL3.dll build\
```

#### "libSDL3.dll.a not found"
**Solution**: Check SDL3 installation
```bash
# Verify file exists
dir external\SDL3\build\libSDL3.dll.a

# Rebuild SDL3 if missing
.\build-full.bat
```

#### "Luau libraries not found"
**Solution**: Rebuild Luau dependencies
```bash
# Clean and rebuild
rmdir /s /q external\Luau\build
.\build-full.bat
```

### Build Errors

#### Compilation Errors
**Symptoms**: g++ errors during compilation
**Solutions**:
1. Check C++20 compiler support
2. Verify source file paths
3. Check for syntax errors in source code

#### Linking Errors
**Symptoms**: Undefined references during linking
**Solutions**:
1. Verify library paths are correct
2. Check library files exist
3. Ensure proper library order in linking

#### Runtime Errors
**Symptoms**: Application crashes on startup
**Solutions**:
1. Verify SDL3.dll is present
2. Check for missing dependencies
3. Use debug version (LikeC.exe) for error messages

### Debug Mode

#### Using Debug Executable
```bash
# Run with console output
.\build\LikeC.exe "your_project"

# This shows:
# - Asset loading errors
# - Lua compilation errors
# - Runtime error messages
# - print() output from Lua scripts
```

#### Common Debug Output
```
Main Lua Path: scripts/main.luau
Project Folder: C:/Users/Rozak/Documents/Like2D/scripts
Game initialized successfully!
Error: Failed to load texture: assets/player.png
Lua compilation error: syntax error near 'if'
```

## Performance Optimization

### Build Optimization
The build systems use these optimizations:
- `-O2` compiler flag for speed optimization
- `-static-libgcc` and `-static-libstdc++` for static linking
- Incremental builds to reduce compilation time
- Parallel compilation where possible

### Runtime Optimization
For best performance in release builds:
1. Use Like2D.exe (not LikeC.exe) for distribution
2. Ensure all assets are pre-loaded in onInit()
3. Avoid creating objects in onDraw()
4. Use appropriate image sizes

## Custom Build Scenarios

### Building Without Resources
If you don't need Windows resources (icons, metadata):
```batch
# Comment out resource compilation in build.bat
REM windres -i "%RESOURCE_FILE%" -o "%RESOURCE_OBJ%"
```

### Building with Custom Libraries
To add additional libraries:
```batch
# Add to LIBS variable
set LIBS=%LIBS% -lyour_library

# Add to INCLUDES variable  
set INCLUDES=%INCLUDES% -Ipath\to\headers
```

### Building Different Configurations
For development vs release configurations:
```batch
# Debug configuration
set CFLAGS=-std=c++20 -g -O0 -c
set RELEASE_LFLAGS=-Wl,-subsystem,console

# Release configuration  
set CFLAGS=-std=c++20 -O2 -s -c
set RELEASE_LFLAGS=-Wl,-subsystem,windows
```

## Integration with IDEs

### Visual Studio Code
1. Install C/C++ extension
2. Configure `.vscode/tasks.json` for build commands
3. Use integrated terminal for build execution

### Other IDEs
Most IDEs can be configured to run the batch files as external tools or build commands.

## Build Script Customization

### Adding New Source Files
To add new source files to the build:
```batch
# Add to ENGINE_SOURCES or LAUNCHER_SOURCES
set ENGINE_SOURCES=%ENGINE_SOURCES% src\new_file.cpp
```

### Modifying Build Steps
The build scripts are organized into clear sections:
1. Configuration
2. Dependency checking
3. Resource compilation
4. Source compilation
5. Linking
6. Deployment

Each section can be modified independently.

## Conclusion

The Like2D build system provides flexible options for different development scenarios:

- **Use Quick Build** for rapid development and when dependencies are available
- **Use Full Build** for complete control or when dependencies need to be compiled
- **Use Debug Mode** during development for error messages
- **Use Release Mode** for distribution and performance testing

The incremental build system ensures fast rebuilds during development, while the distribution system makes it easy to share your creations.

For additional help, refer to the main documentation or check the console output when running builds in debug mode.
