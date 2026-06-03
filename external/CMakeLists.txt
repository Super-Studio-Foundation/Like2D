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

# Create an alias for SDL3 (matching your project structure)
if(NOT TARGET SDL3::SDL3)
    if(TARGET SDL3-shared)
        add_library(SDL3::SDL3 ALIAS SDL3-shared)
    else()
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
    FetchContent_MakeAvailable(Luau)
endif()

# Collect Luau libraries
set(LUAU_LIBRARIES
    Luau.Ast
    Luau.Compiler
    Luau.VM
    Luau.Analysis
    Luau.CodeGen
    Luau.Common
)

# Create a combined Luau target
if(NOT TARGET Luau::Luau)
    add_library(Luau::Luau INTERFACE IMPORTED)
    target_link_libraries(Luau::Luau INTERFACE ${LUAU_LIBRARIES})
    # Handle include path depending on if it's fetched or local
    if(Luau_SOURCE_DIR)
        target_include_directories(Luau::Luau INTERFACE ${Luau_SOURCE_DIR}/VM/include ${Luau_SOURCE_DIR}/Common/include ${Luau_SOURCE_DIR}/Compiler/include ${Luau_SOURCE_DIR}/Ast/include)
    else()
        target_include_directories(Luau::Luau INTERFACE ${CMAKE_SOURCE_DIR}/external/Luau/VM/include ${CMAKE_SOURCE_DIR}/external/Luau/Common/include)
    endif()
endif()
