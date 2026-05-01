# Source directory CMakeLists.txt

# Collect source files
file(GLOB_RECURSE SOURCES 
    "*.cpp"
    "*.hpp"
    "*.h"
)

# Create the main library
add_library(Like2DCore STATIC ${SOURCES})

target_include_directories(Like2DCore
    PUBLIC 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/SDL3/include
        ${CMAKE_SOURCE_DIR}/external/Luau/include
)

target_link_libraries(Like2DCore
    PUBLIC SDL3::SDL3 Luau::Luau
)
