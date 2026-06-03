include(FetchContent)

# --- SDL3 ---
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

# --- Luau ---
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
        )
    else()
        target_include_directories(LuauInternal INTERFACE 
            ${CMAKE_SOURCE_DIR}/external/Luau/VM/include 
            ${CMAKE_SOURCE_DIR}/external/Luau/Common/include
        )
    endif()

    # Now create the alias that the rest of the project expects
    add_library(Luau::Luau ALIAS LuauInternal)
endif()
