# Like2D Framework - Build System Documentation

## Overview

Like2D uses a single **Quick Build** system (`build.bat`) that compiles the engine using pre-built dependencies. All external libraries (SDL3, Luau, Box2D, etc.) are downloaded as a single package from GitHub releases.

The build creates two executables:
- **Like2D.exe** - Release version (Windows subsystem, no console)
- **LikeC.exe** - Debug version (Console subsystem, shows output)

## System Requirements

### Required Tools
- **MinGW-w64** with C++20 support
- **Git** (optional)

### Windows Environment
- Windows 10 or later
- PATH should include MinGW-w64 bin directory

## Quick Build

### Step 1: Download Dependencies

Download the pre-built external package for version 1.3.0.0:

[Click Here to Download 1.3.0.0_External.zip][https://github.com/Super-Studio-Foundation/Like2D/releases/download/1.3.0.0_External/1.3.0.0_External.zip]

Extract the contents into the `external/` folder at the project root. The final structure should look like:

```
Like2D/
├── build.bat
├── src/
├── external/
│   ├── SDL3/
│   │   ├── build/
│   │   │   ├── libSDL3.dll.a
│   │   │   ├── SDL3.dll
│   │   │   └── include/
│   ├── Luau/
│   │   ├── build/
│   │   │   ├── libLuau.*.a
│   │   │   └── include/
│   ├── box2d/
│   │   ├── build/
│   │   │   ├── libbox2d.a
│   │   │   └── include/
│   ├── miniaudio/
│   ├── miniz/
│   ├── stb/
│   ├── nlohmann/
│   ├── pl_mpeg/
│   └── imgui/
└── build/
```

### Step 2: Run Build

```bash
cd Like2D
.\build.bat

# Clean build
.\build.bat clean

# Create distribution package
.\build.bat dist
```

### Build Process

1. **Dependency Check** - Verifies required files in `external/`
2. **Resource Compilation** - Compiles `resource.rc` for application metadata
3. **Engine DLL Compilation** - Builds `Like.dll` from source files
4. **Launcher Compilation** - Builds main executable source
5. **Linking** - Creates both Like2D.exe (release) and LikeC.exe (debug)
6. **Deployment** - Copies SDL3.dll to build directory

### Build Configuration

The quick build uses these key settings in `build.bat`:

```batch
set RELEASE_NAME=Like2D.exe
set DEBUG_NAME=LikeC.exe
set DLL_NAME=Like.dll
set ENGINE_SOURCES=src\Like2D.cpp src\core\Engine.cpp src\renderer\Renderer.cpp src\scripting\LuaBindings.cpp
set LAUNCHER_SOURCES=src\main.cpp
```

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

Modify build settings by editing `build.bat`:

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
set BOX2D_DIR=external\box2d\build
```

### Incremental Builds
- Only modified source files are recompiled
- Object files are timestamp-checked
- DLL and executables are relinked only when necessary

### Clean Build
```bash
.\build.bat clean
```
Removes the entire `build/` directory.

## Distribution Package

### Creating Distribution
```bash
.\build.bat dist
```

Creates a `Like2D-Dist/` folder containing:
- Like2D.exe (release executable)
- LikeC.exe (debug executable)
- Like.dll (engine library)
- SDL3.dll (runtime library)
- scripts/ (sample scripts)
- README.md (documentation)

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
g++ --version
set PATH=C:\mingw64\bin;%PATH%
```

#### "SDL3.dll not found"
**Solution**: Ensure external dependencies are properly extracted
```bash
# Verify SDL3 exists
dir external\SDL3\build\SDL3.dll
```

#### "external/ directory missing or incomplete"
**Solution**: Download and extract the external package for your version:
```
https://github.com/Super-Studio-Foundation/Like2D/releases/download/untagged-9c3a4ab8027a421f2c0d/external_1.3.0.0.zip
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
1. Verify external/ directory has all required libraries
2. Re-download and re-extract the external package
3. Ensure proper library order in linking

#### Runtime Errors
**Symptoms**: Application crashes on startup
**Solutions**:
1. Verify SDL3.dll is present in the build directory
2. Check for missing dependencies
3. Use debug version (LikeC.exe) for error messages

### Debug Mode

```bash
.\build\LikeC.exe "your_project"
```

Shows:
- Asset loading errors
- Lua compilation errors
- Runtime error messages
- print() output from Lua scripts

## Performance Optimization

### Build Optimization
- `-O2` compiler flag for speed optimization
- `-static-libgcc` and `-static-libstdc++` for static linking
- Incremental builds to reduce compilation time

### Runtime Optimization
1. Use Like2D.exe (not LikeC.exe) for distribution
2. Ensure all assets are pre-loaded in onInit()
3. Avoid creating objects in onDraw()
4. Use appropriate image sizes

## Custom Build Scenarios

### Building Without Resources
```batch
REM Comment out resource compilation in build.bat
REM windres -i "%RESOURCE_FILE%" -o "%RESOURCE_OBJ%"
```

### Building with Custom Libraries
```batch
set LIBS=%LIBS% -lyour_library
set INCLUDES=%INCLUDES% -Ipath\to\headers
```

### Building Different Configurations
```batch
REM Debug configuration
set CFLAGS=-std=c++20 -g -O0 -c
set RELEASE_LFLAGS=-Wl,-subsystem,console

REM Release configuration
set CFLAGS=-std=c++20 -O2 -s -c
set RELEASE_LFLAGS=-Wl,-subsystem,windows
```

## Build Script Customization

### Adding New Source Files
```batch
set ENGINE_SOURCES=%ENGINE_SOURCES% src\new_file.cpp
```

The build script is organized into clear sections:
1. Configuration
2. Dependency checking
3. Resource compilation
4. Source compilation
5. Linking
6. Deployment

Each section can be modified independently.
