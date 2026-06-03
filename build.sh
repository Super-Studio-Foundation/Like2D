#!/bin/bash

# ===========================================
# Like2D Framework Build System (Linux)
# ===========================================

# --- CONFIGURATION ---
RELEASE_NAME="Like2D"
DEBUG_NAME="LikeC"
DLL_NAME="libLike.so"
BUILD_DIR="build"

# Sources
ENGINE_SOURCES="src/Like2D.cpp src/core/Engine.cpp src/renderer/Renderer.cpp src/scripting/LuaBindings.cpp src/compile/compiler.cpp src/audio/AudioManager.cpp external/miniz/miniz.c"
LAUNCHER_SOURCES="src/main.cpp"

# Include paths
INCLUDES="-Iinclude -Iexternal/SDL3/include -Iexternal/Luau/VM/include -Iexternal/Luau/Compiler/include -Iexternal/Luau/Ast/include -Iexternal/Luau/Config/include -Iexternal/Luau/Common/include -Iexternal/box2d/include -Iexternal/miniaudio -Iexternal/nlohmann -Iexternal/pl_mpeg -Isrc"

# Library paths
LIB_PATHS="-Lexternal/lib -L$BUILD_DIR"

# Libraries
LIBS="-lSDL3 -lLuau.Compiler -lLuau.VM -lLuau.Ast -lLuau.Bytecode -lLuau.Common -lbox2d -ldl -lpthread"

# Flags
CFLAGS="-std=c++20 -O2 -fPIC -c"
DLL_LFLAGS="-shared -fPIC"

# ---------------------

mkdir -p $BUILD_DIR

echo "[1/4] Checking dependencies..."
# On Linux, we usually assume SDL3 and others are either in external/ or installed via package manager

echo "[2/4] Compiling engine objects..."
ENGINE_OBJECTS=""
for src in $ENGINE_SOURCES; do
    filename=$(basename "$src")
    obj="$BUILD_DIR/${filename%.*}.o"
    echo "  Compiling $src..."
    g++ $CFLAGS $INCLUDES "$src" -o "$obj"
    if [ $? -ne 0 ]; then
        echo "[ERROR] Failed to compile $src"
        exit 1
    fi
    ENGINE_OBJECTS="$ENGINE_OBJECTS $obj"
done

echo "[3/4] Linking $DLL_NAME..."
g++ $DLL_LFLAGS $ENGINE_OBJECTS -o "$BUILD_DIR/$DLL_NAME" $LIB_PATHS $LIBS
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to link $DLL_NAME"
    exit 1
fi

echo "[4/4] Compiling launcher..."
g++ -std=c++20 -O2 $INCLUDES $LAUNCHER_SOURCES -o "$BUILD_DIR/$RELEASE_NAME" -L"$BUILD_DIR" -lLike -Wl,-rpath,'$ORIGIN'
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to link $RELEASE_NAME"
    exit 1
fi

# Create a debug version (identical on Linux, but we'll follow the name convention)
cp "$BUILD_DIR/$RELEASE_NAME" "$BUILD_DIR/$DEBUG_NAME"

echo ""
echo "[SUCCESS] Build completed! Binary available in $BUILD_DIR/$RELEASE_NAME"
echo "To run, use: cd $BUILD_DIR && ./$RELEASE_NAME"
