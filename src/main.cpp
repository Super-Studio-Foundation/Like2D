#define SDL_MAIN_HANDLED
#include "Like2D.h"
#include "compile/compiler.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#else
#  include <unistd.h>   // readlink on Linux
#  include <climits>
#  ifndef MAX_PATH
#    define MAX_PATH 4096
#  endif
#endif

int main(int argc, char* argv[]) {

    // ── Detect production mode (compiled game) ──
    // If game.like exists next to the exe OR is fused into the exe,
    // this is a shipped game — disable compile mode.
    bool isProduction = false;
    {
        std::filesystem::path exePath;
#ifdef _WIN32
        char buf[MAX_PATH];
        GetModuleFileNameA(NULL, buf, MAX_PATH);
        exePath = buf;
#else
        char buf[MAX_PATH] = {};
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) buf[len] = '\0';
        exePath = buf;
#endif
        std::filesystem::path exeDir = exePath.parent_path();

        // Check for game.like file next to exe
        if (std::filesystem::exists(exeDir / "game.like")) {
            isProduction = true;
        } else {
            // Check if game.like is fused into the exe itself (scan last 64KB for ZIP or L2DE magic)
            std::ifstream self(exePath, std::ios::binary);
            if (self.is_open()) {
                self.seekg(0, std::ios::end);
                std::streamsize size = self.tellg();
                if (size > 8) {
                    std::streamsize scanSize = std::min(size, (std::streamsize)65536);
                    self.seekg(-scanSize, std::ios::end);
                    std::vector<char> tailBuf((size_t)scanSize);
                    self.read(tailBuf.data(), tailBuf.size());
                    for (size_t i = 0; i + 3 < tailBuf.size(); i++) {
                        if (tailBuf[i] == 'L' && tailBuf[i+1] == '2' && tailBuf[i+2] == 'D' && tailBuf[i+3] == 'E') {
                            isProduction = true;
                            break;
                        }
                        if (tailBuf[i] == 'P' && tailBuf[i+1] == 'K' && tailBuf[i+2] == 3 && tailBuf[i+3] == 4) {
                            isProduction = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    bool isCompileMode = (!isProduction && argc > 1 && std::string(argv[1]) == "compile");

#ifdef _WIN32
    if (isCompileMode) {
        AllocConsole();
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        freopen_s(&fDummy, "CONIN$",  "r", stdin);
        std::cout.setf(std::ios::unitbuf);
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
#endif
    // On Linux, stdout/stderr are already connected to the terminal.

    std::cout << "Starting" << std::endl;

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

    std::cout << "Creating Like2D Application..." << std::endl;
    Like2D::Application app;

    std::cout << "Initializing..." << std::endl;
    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize Like2D Application" << std::endl;
#ifdef _WIN32
        MessageBoxA(NULL, "Failed to initialize Like2D.\n\nCheck the console for details.", "Like2D Error", MB_OK | MB_ICONERROR);
#endif
        return 1;
    }

    std::cout << "Running..." << std::endl;
    app.run();
    app.shutdown();
    std::cout << "Exited cleanly." << std::endl;

    return 0;
}
