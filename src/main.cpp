#define SDL_MAIN_HANDLED
#include "Like2D.h"
#include "compile/compiler.hpp"
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#else
#  include <unistd.h>   // readlink on Linux
#endif

int main(int argc, char* argv[]) {
    bool isCompileMode = (argc > 1 && std::string(argv[1]) == "compile");

#ifdef _WIN32
    // Only attach a console for compile mode (Windows only)
    if (isCompileMode) {
        if (GetConsoleWindow() == NULL) {
            AllocConsole();
        }
        
        // Redirect standard streams
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$",  "r", stdin);
        
        // Ensure streams are unbuffered for interactive use
        std::cout.setf(std::ios::unitbuf);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
#endif
    // On Linux, stdout/stderr are already connected to the terminal.
    
    std::cout << "Engine starting..." << std::endl;
    
    if (isCompileMode) {
        std::filesystem::path projectPath;
        Like2D::Compiler::TargetPlatform targetPlatform = 
#ifdef _WIN32
            Like2D::Compiler::TargetPlatform::Windows;
#else
            Like2D::Compiler::TargetPlatform::Linux;
#endif

        // Simple argument parsing for compile mode
        for (int i = 2; i < argc; i++) {
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
        
        try {
            bool success = Like2D::Compiler::Compiler::runCompilerWizard(projectPath.string(), targetPlatform);
            
            if (success) {
                std::cout << "Compilation completed successfully!" << std::endl;
            } else {
                std::cerr << "Compilation failed!" << std::endl;
            }

            std::cout << std::endl << "Press Enter to exit..." << std::endl;
            std::string dummy;
            std::getline(std::cin, dummy);
            
            return success ? 0 : 1;
        } catch (const std::exception& e) {
            std::cerr << "Exception during compilation: " << e.what() << std::endl;
            return 1;
        } catch (...) {
            std::cerr << "Unknown exception during compilation!" << std::endl;
            return 1;
        }
    }
    
    Like2D::Application app;
    
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize Like2D Application" << std::endl;
        return 1;
    }
    
    app.run();
    app.shutdown();
    
    return 0;
}
