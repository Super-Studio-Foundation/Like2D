cmake_minimum_required(VERSION 3.20)
project(Like2D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

# Add subdirectories
add_subdirectory(external)
add_subdirectory(src)

# Main executable
add_executable(Like2D src/main.cpp)

target_link_libraries(Like2D PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf Luau::Luau)

# Copy scripts to build directory
file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_BINARY_DIR})
