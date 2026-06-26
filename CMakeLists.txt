cmake_minimum_required(VERSION 3.20)
project(Like2D VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

add_subdirectory(external)
add_subdirectory(src)

# ============================================================
# GAME RUNNER (Like2D) — the main game executable
# ============================================================
if(WIN32)
    add_executable(Like2D WIN32 src/main.cpp resource.rc)
else()
    add_executable(Like2D src/main.cpp)
endif()
target_link_libraries(Like2D PRIVATE Like2DCore)

# ============================================================
# EDITOR (Like2DEditor) — the visual editor
# ============================================================
if(WIN32)
    add_executable(Like2DEditor WIN32 src/main.cpp src/editor/EditorApp.cpp resource.rc)
else()
    add_executable(Like2DEditor src/main.cpp src/editor/EditorApp.cpp)
endif()
target_link_libraries(Like2DEditor PRIVATE Like2DCore)

# ============================================================
# COMPILER (LikeC) — standalone game compiler
# ============================================================
if(WIN32)
    add_executable(LikeC src/compile/CompilerApp.cpp resource.rc)
else()
    add_executable(LikeC src/compile/CompilerApp.cpp)
endif()
target_link_libraries(LikeC PRIVATE Like2DCore)

# ============================================================
# Platform-specific post-build steps
# ============================================================
if(NOT WIN32)
    target_link_libraries(Like2D PRIVATE dl pthread)
    target_link_libraries(Like2DEditor PRIVATE dl pthread)
    target_link_libraries(LikeC PRIVATE dl pthread)

    # Strip release binaries
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET Like2D POST_BUILD
            COMMAND strip $<TARGET_FILE:Like2D>
            COMMENT "Stripping Like2D binary")
        add_custom_command(TARGET LikeC POST_BUILD
            COMMAND strip $<TARGET_FILE:LikeC>
            COMMENT "Stripping LikeC binary")
    endif()
endif()
