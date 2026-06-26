# Source directory CMakeLists.txt
# NOTE: Executable targets (Like2D, Like2DEditor, LikeC) are defined in root CMakeLists.txt

# Collect source files for core library
file(GLOB_RECURSE SOURCES
    "*.cpp"
    "*.hpp"
    "*.h"
)
list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER SOURCES EXCLUDE REGEX ".*/CompilerApp\\.cpp$")

list(APPEND SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Like2D.cpp)

# Create core library
add_library(Like2DCore STATIC ${SOURCES})

target_include_directories(Like2DCore
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/external/SDL3/include
        ${CMAKE_SOURCE_DIR}/external/Luau/VM/include
        ${CMAKE_SOURCE_DIR}/external/Luau/Common/include
        ${CMAKE_SOURCE_DIR}/external/Luau/Compiler/include
        ${CMAKE_SOURCE_DIR}/external/Luau/Ast/include
        ${CMAKE_SOURCE_DIR}/external/Luau/Config/include
        ${CMAKE_SOURCE_DIR}/external/box2d/include
        ${CMAKE_SOURCE_DIR}/external/nlohmann
        ${CMAKE_SOURCE_DIR}/external/miniaudio
        ${CMAKE_SOURCE_DIR}/external/miniz
        ${CMAKE_SOURCE_DIR}/external/stb
        ${CMAKE_SOURCE_DIR}/external/pl_mpeg
        ${CMAKE_SOURCE_DIR}/external/imgui
        ${CMAKE_SOURCE_DIR}/external/imgui/backends
)

target_link_libraries(Like2DCore
    PUBLIC
        SDL3::SDL3
        Luau::Luau
        box2d
        miniz::miniz
        miniaudio::miniaudio
        stb::stb
        pl_mpeg::pl_mpeg
        imgui
)

if(WIN32)
    target_link_libraries(Like2DCore PUBLIC winmm ole32 user32 gdi32 shell32 ws2_32)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Like2DCore PUBLIC dl pthread m)
endif()
