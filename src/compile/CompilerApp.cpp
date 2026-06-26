// src/compile/CompilerApp.cpp
#define SDL_MAIN_HANDLED
#include "compiler.hpp"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::filesystem::path projectPath;
    Like2D::Compiler::TargetPlatform targetPlatform =
#ifdef _WIN32
        Like2D::Compiler::TargetPlatform::Windows;
#else
        Like2D::Compiler::TargetPlatform::Linux;
#endif

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--linux") {
            targetPlatform = Like2D::Compiler::TargetPlatform::Linux;
        } else if (arg == "--windows") {
            targetPlatform = Like2D::Compiler::TargetPlatform::Windows;
        } else if (projectPath.empty()) {
            projectPath = std::filesystem::path(arg);
        }
    }

    if (projectPath.empty()) {
        projectPath = std::filesystem::current_path();
    }

    std::cout << "=== Like2D Compiler ===" << std::endl;
    std::cout << "Project: " << projectPath.string() << std::endl;
    std::cout << "Target:  " << (targetPlatform == Like2D::Compiler::TargetPlatform::Windows ? "Windows" : "Linux") << std::endl;
    std::cout << std::endl;

    bool success = Like2D::Compiler::Compiler::runCompilerWizard(projectPath.string(), targetPlatform);
    return success ? 0 : 1;
}
