include(FetchContent)

# ============================================================
# SDL3
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/SDL3/CMakeLists.txt")
    add_subdirectory(SDL3)
else()
    message(STATUS "SDL3 folder missing, fetching from GitHub...")
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG main
    )
    FetchContent_MakeAvailable(SDL3)
endif()

# Create an alias for SDL3
if(NOT TARGET SDL3::SDL3)
    if(TARGET SDL3-shared)
        add_library(SDL3::SDL3 ALIAS SDL3-shared)
    elseif(TARGET SDL3)
        add_library(SDL3::SDL3 ALIAS SDL3)
    endif()
endif()

# ============================================================
# Luau
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Luau/CMakeLists.txt")
    add_subdirectory(Luau)
else()
    message(STATUS "Luau folder missing, fetching from GitHub...")
    FetchContent_Declare(
        Luau
        GIT_REPOSITORY https://github.com/luau-lang/luau.git
        GIT_TAG master
    )
    # Disable building tests/fuzzing to avoid Clang requirement
    set(LUAU_BUILD_TESTS OFF CACHE BOOL "Build Luau tests")
    set(LUAU_BUILD_WEB OFF CACHE BOOL "Build Luau web")
    FetchContent_MakeAvailable(Luau)
endif()

# Create a combined Luau target using an ALIAS to satisfy CMake rules
if(NOT TARGET Luau::Luau)
    set(LUAU_LIBS Luau.Ast Luau.Compiler Luau.VM Luau.Analysis Luau.CodeGen Luau.Common)

    # Create an internal interface library first
    add_library(LuauInternal INTERFACE)
    target_link_libraries(LuauInternal INTERFACE ${LUAU_LIBS})

    if(Luau_SOURCE_DIR)
        target_include_directories(LuauInternal INTERFACE
            ${Luau_SOURCE_DIR}/VM/include
            ${Luau_SOURCE_DIR}/Common/include
            ${Luau_SOURCE_DIR}/Compiler/include
            ${Luau_SOURCE_DIR}/Ast/include
            ${Luau_SOURCE_DIR}/Config/include
        )
    else()
        target_include_directories(LuauInternal INTERFACE
            ${CMAKE_SOURCE_DIR}/external/Luau/VM/include
            ${CMAKE_SOURCE_DIR}/external/Luau/Common/include
            ${CMAKE_SOURCE_DIR}/external/Luau/Compiler/include
            ${CMAKE_SOURCE_DIR}/external/Luau/Ast/include
            ${CMAKE_SOURCE_DIR}/external/Luau/Config/include
        )
    endif()

    # Now create the alias that the rest of the project expects
    add_library(Luau::Luau ALIAS LuauInternal)
endif()

# ============================================================
# miniaudio (audio library)
# ============================================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/miniaudio/miniaudio.h")
    message(STATUS "miniaudio missing, fetching from GitHub...")
    FetchContent_Declare(
        miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(miniaudio)
endif()

# Create miniaudio interface library
if(NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio_interface INTERFACE)
    target_include_directories(miniaudio_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio)
    add_library(miniaudio::miniaudio ALIAS miniaudio_interface)
endif()

# ============================================================
# miniz (zip compression)
# ============================================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/miniz/miniz.h")
    message(STATUS "miniz missing, fetching from GitHub...")
    FetchContent_Declare(
        miniz
        GIT_REPOSITORY https://github.com/richgel999/miniz.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(miniz)
endif()

if(NOT TARGET miniz::miniz)
    add_library(miniz_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/miniz/miniz.c)
    target_include_directories(miniz_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/miniz)
    add_library(miniz::miniz ALIAS miniz_lib)
endif()

# ============================================================
# stb (single-header libraries)
# ============================================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_image.h")
    message(STATUS "stb missing, fetching from GitHub...")
    FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(stb)
endif()

if(NOT TARGET stb::stb)
    add_library(stb_interface INTERFACE)
    target_include_directories(stb_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)
    add_library(stb::stb ALIAS stb_interface)
endif()

# ============================================================
# pl_mpeg (MPEG-1 video decoder)
# ============================================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pl_mpeg/pl_mpeg.h")
    message(STATUS "pl_mpeg missing, fetching from GitHub...")
    FetchContent_Declare(
        pl_mpeg
        GIT_REPOSITORY https://github.com/phoboslab/pl_mpeg.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(pl_mpeg)
endif()

if(NOT TARGET pl_mpeg::pl_mpeg)
    add_library(pl_mpeg_interface INTERFACE)
    target_include_directories(pl_mpeg_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/pl_mpeg)
    add_library(pl_mpeg::pl_mpeg ALIAS pl_mpeg_interface)
endif()

# ============================================================
# nlohmann/json
# ============================================================
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/nlohmann/json.hpp")
    message(STATUS "nlohmann/json missing, fetching from GitHub...")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG develop
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ============================================================
# box2d (physics engine)
# ============================================================
# Force static build to avoid shared library path issues on Linux
set(BOX2D_OLD_SHARED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static" FORCE)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/box2d/CMakeLists.txt")
    set(BOX2D_SAMPLES OFF CACHE BOOL "" FORCE)
    set(BOX2D_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(BOX2D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    add_subdirectory(box2d)
else()
    message(STATUS "box2d missing, fetching from GitHub...")
    FetchContent_Declare(
        box2d
        GIT_REPOSITORY https://github.com/erincatto/box2d.git
        GIT_TAG main
    )
    set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Build Box2D unit tests")
    set(BOX2D_BUILD_BENCHMARKS OFF CACHE BOOL "Build Box2D benchmarks")
    set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Build Box2D docs")
    FetchContent_MakeAvailable(box2d)
endif()

set(BUILD_SHARED_LIBS ${BOX2D_OLD_SHARED} CACHE BOOL "Restore" FORCE)

# ============================================================
# IMGUI (Dear ImGui) - NEW!
# ============================================================
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/CMakeLists.txt")
    add_subdirectory(imgui)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp")
    message(STATUS "Using existing imgui from external/imgui")
    # Create imgui library from existing source files
    file(GLOB IMGUI_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
    )
    file(GLOB IMGUI_BACKEND_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdl3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_sdlrenderer3.cpp"
    )

    add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
    target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends
    )
    target_link_libraries(imgui PUBLIC SDL3::SDL3)
    target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)

    # For MSVC, suppress noisy warnings
    if(MSVC)
        target_compile_options(imgui PRIVATE /W0)
    endif()
else()
    message(STATUS "imgui folder missing, fetching from GitHub...")
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
    )
    FetchContent_GetProperties(imgui)
    if(NOT imgui_POPULATED)
        FetchContent_Populate(imgui)

        file(GLOB IMGUI_SOURCES
            "${imgui_SOURCE_DIR}/imgui.cpp"
            "${imgui_SOURCE_DIR}/imgui_draw.cpp"
            "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
            "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        )
        file(GLOB IMGUI_BACKEND_SOURCES
            "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
            "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp"
        )

        add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
        target_include_directories(imgui PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()
endif()
