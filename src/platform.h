#pragma once
// ============================================================
//  platform.h — Cross-platform helpers (Windows + Linux)
// ============================================================
#include <string>
#include <filesystem>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <climits>
#  ifndef MAX_PATH
#    define MAX_PATH 4096
#  endif
#endif

// Returns the directory containing the running executable.
inline std::filesystem::path getExeDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
#else
    char buf[MAX_PATH] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) buf[len] = '\0';
    return std::filesystem::path(buf).parent_path();
#endif
}

// Returns the full path of the running executable.
inline std::filesystem::path getExePath() {
#ifdef _WIN32
    char buf[MAX_PATH];
    GetModuleFileNameA(NULL, buf, MAX_PATH);
    return std::filesystem::path(buf);
#else
    char buf[MAX_PATH] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) buf[len] = '\0';
    return std::filesystem::path(buf);
#endif
}
