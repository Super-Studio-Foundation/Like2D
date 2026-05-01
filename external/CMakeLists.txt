# External dependencies CMakeLists.txt

# SDL3
add_subdirectory(SDL3)

# Create an alias for SDL3
add_library(SDL3::SDL3 ALIAS SDL3-shared)

# Luau
add_subdirectory(Luau)

# Collect Luau libraries
set(LUAU_LIBRARIES
    Luau::Ast
    Luau::Compiler
    Luau::VM
    Luau::Analysis
    Luau::CodeGen
    Luau::CodeGenTypes
    Luau::Common
)

# Create a combined Luau target
add_library(Luau::Luau INTERFACE IMPORTED)
target_link_libraries(Luau::Luau INTERFACE ${LUAU_LIBRARIES})
target_include_directories(Luau::Luau INTERFACE ${CMAKE_SOURCE_DIR}/external/Luau/include)
