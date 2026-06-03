cmake_minimum_required(VERSION 3.20)
project(Like2D VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/external)

add_subdirectory(external)
add_subdirectory(src)

if(WIN32)
    # Windows: GUI subsystem (no console), embed resource for icon/version
    add_executable(Like2D WIN32 src/main.cpp resource.rc)
    # Debug build with console
    add_executable(LikeC src/main.cpp resource.rc)
    target_compile_definitions(LikeC PRIVATE LIKEC_DEBUG_BUILD)
else()
    # Linux / macOS: plain executable, no resource file
    add_executable(Like2D src/main.cpp)
endif()

target_link_libraries(Like2D PRIVATE Like2DCore SDL3::SDL3 Luau::Luau)

if(NOT WIN32)
    # Linux needs these system libs
    target_link_libraries(Like2D PRIVATE dl pthread)
    # Strip release binary
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(TARGET Like2D POST_BUILD
            COMMAND strip $<TARGET_FILE:Like2D>
            COMMENT "Stripping binary")
    endif()
endif()

file(COPY ${CMAKE_SOURCE_DIR}/scripts DESTINATION ${CMAKE_BINARY_DIR}/bin)
