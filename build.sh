#!/bin/bash

# ===========================================
# Like2D Framework Build System (Linux)
# Uses CMake to build all dependencies + engine
# ===========================================

set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"

echo "============================================"
echo " Like2D Linux Build System"
echo " Build Type: $BUILD_TYPE"
echo "============================================"
echo ""

# Check for required tools
echo "[1/4] Checking dependencies..."
MISSING_TOOLS=""
for tool in cmake g++ make pkg-config; do
    if ! command -v $tool &> /dev/null; then
        MISSING_TOOLS="$MISSING_TOOLS $tool"
    fi
done

if [ -n "$MISSING_TOOLS" ]; then
    echo "[ERROR] Missing required tools:$MISSING_TOOLS"
    echo ""
    echo "Install with:"
    echo "  sudo apt install build-essential cmake pkg-config"
    echo "  # or for Fedora:"
    echo "  sudo dnf install gcc-c++ cmake make pkgconf-pkg-config"
    exit 1
fi

# Check for SDL3 system libraries (required for SDL3 to build from source)
MISSING_LIBS=""
check_pkg() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        MISSING_LIBS="$MISSING_LIBS $2"
    fi
}

check_pkg "alsa"       "libasound2-dev"
check_pkg "x11"        "libx11-dev"
check_pkg "xext"       "libxext-dev"
check_pkg "xrandr"     "libxrandr-dev"
check_pkg "xcursor"    "libxcursor-dev"
check_pkg "xfixes"     "libxfixes-dev"
check_pkg "xi"         "libxi-dev"
check_pkg "xinerama"   "libxinerama-dev"
check_pkg "xscrnsaver" "libxss-dev"
check_pkg "gl"         "libgl1-mesa-dev"
check_pkg "libudev"    "libudev-dev"
check_pkg "dbus-1"     "libdbus-1-dev"

# Optional but recommended (won't block build)
OPTIONAL_MISSING=""
check_opt() {
    if ! pkg-config --exists "$1" 2>/dev/null; then
        OPTIONAL_MISSING="$OPTIONAL_MISSING $2"
    fi
}
check_opt "libpulse"   "libpulse-dev"
check_opt "wayland-client" "libwayland-dev"
check_opt "libdrm"     "libdrm-dev"
check_opt "gbm"        "libgbm-dev"

if [ -n "$OPTIONAL_MISSING" ]; then
    echo "  [!] Optional libs not found (non-fatal):$OPTIONAL_MISSING"
fi

if [ -n "$MISSING_LIBS" ]; then
    echo "[ERROR] Missing SDL3 system libraries:"
    echo "$MISSING_LIBS"
    echo ""
    echo "Install all with:"
    echo "  sudo apt install build-essential cmake pkg-config \\"
    echo "      libasound2-dev libpulse-dev \\"
    echo "      libx11-dev libxext-dev libxrandr-dev libxrender-dev \\"
    echo "      libxcursor-dev libxfixes-dev libxi-dev libxinerama-dev \\"
    echo "      libxss-dev libxtst-dev libgl1-mesa-dev libglu1-mesa-dev \\"
    echo "      libusb-1.0-0-dev libwayland-dev libdrm-dev libgbm-dev \\"
    echo "      libudev-dev libdbus-1-dev"
    echo ""
    echo "  # or for Fedora:"
    echo "  sudo dnf install alsa-lib-devel pulseaudio-libs-devel \\"
    echo "      libX11-devel libXext-devel libXrandr-devel libXrender-devel \\"
    echo "      libXcursor-devel libXfixes-devel libXi-devel libXinerama-devel \\"
    echo "      libXScrnSaver-devel mesa-libGL-devel mesa-libGLU-devel \\"
    echo "      wayland-devel libdrm-devel mesa-libgbm-devel \\"
    echo "      systemd-devel dbus-devel"
    exit 1
fi

echo "  cmake: $(cmake --version | head -1)"
echo "  g++: $(g++ --version | head -1)"
echo ""

# Configure with CMake
echo "[2/4] Configuring with CMake..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DSDL_SHARED=ON \
    -DSDL_STATIC=OFF \
    -DSDL_TEST_LIBRARY=OFF \
    -DSDL_TESTS=OFF \
    -DSDL_EXAMPLES=OFF \
    -DLUAU_BUILD_TESTS=OFF \
    -DLUAU_BUILD_WEB=OFF \
    -DBOX2D_UNIT_TESTS=OFF \
    -DBOX2D_BENCHMARKS=OFF \
    -DBOX2D_SAMPLES=OFF

echo ""

# Build
echo "[3/4] Building (this may take a while on first run)..."
NPROC=$(nproc 2>/dev/null || echo 4)
make -j"$NPROC"

echo ""

# Post-build
echo "[4/4] Post-build setup..."

BIN_DIR="bin"
if [ -d "$BIN_DIR" ]; then
    # Copy scripts if not already done by CMake
    if [ -d "../scripts" ] && [ ! -d "$BIN_DIR/scripts" ]; then
        cp -r ../scripts "$BIN_DIR/"
    fi

    echo ""
    echo "Build artifacts:"
    echo "  Game Runner:  $BIN_DIR/Like2D"
    echo "  Editor:       $BIN_DIR/Like2DEditor"
    echo "  Compiler:     $BIN_DIR/LikeC"

    # Check for shared libs
    if [ -d "lib" ]; then
        echo "  Libraries:    lib/"
        ls -la lib/*.so* 2>/dev/null || true
    fi

    # Show binary sizes
    echo ""
    for bin in Like2D Like2DEditor LikeC; do
        if [ -f "$BIN_DIR/$bin" ]; then
            SIZE=$(du -h "$BIN_DIR/$bin" | cut -f1)
            echo "  $bin: $SIZE"
        fi
    done
fi

echo ""
echo "============================================"
echo " [SUCCESS] Build completed!"
echo "============================================"
echo ""
echo "To run the game runner:"
echo "  cd $BUILD_DIR/$BIN_DIR && ./Like2D"
echo ""
echo "To run the editor:"
echo "  cd $BUILD_DIR/$BIN_DIR && ./Like2DEditor --editor"
echo ""
echo "To compile a game:"
echo "  cd $BUILD_DIR/$BIN_DIR && ./LikeC /path/to/project"
echo ""
echo "To clean:"
echo "  rm -rf $BUILD_DIR"
