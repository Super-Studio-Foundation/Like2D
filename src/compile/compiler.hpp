#pragma once
#include <string>
#include <filesystem>

#ifdef LIKE2D_MONOLITHIC
    #define COMPILER_API
#else
    #ifdef _WIN32
        #ifdef LIKE2D_EXPORTS
            #define COMPILER_API __declspec(dllexport)
        #else
            #define COMPILER_API __declspec(dllimport)
        #endif
    #else
        #define COMPILER_API
    #endif
#endif

namespace Like2D {
namespace Compiler {

struct GameMetadata {
    std::string gameName;
    std::string author;
    std::string version;
    std::string description;
    std::string originalFilename;
    std::string productName;
    std::string copyright;
    bool        encryptArchive = false;  // true = XOR-encrypt game.like
    bool        fuseBinary     = false;  // true = concatenate game.like to the runner binary
};

enum class TargetPlatform {
    Windows,
    Linux
};

class Compiler {
public:
    COMPILER_API static bool runCompilerWizard(const std::string& projectPath, TargetPlatform target = 
#ifdef _WIN32
        TargetPlatform::Windows
#else
        TargetPlatform::Linux
#endif
    );

private:
    static bool validateProject(const std::filesystem::path& projectPath);
    static bool createDistFolder(const std::filesystem::path& projectPath);
    static bool copyExecutableAndDLLs(const std::filesystem::path& projectPath, const std::string& gameName, TargetPlatform target);
    static bool packageAssets(const std::filesystem::path& projectPath, const std::filesystem::path& distPath, bool encrypt);
    static bool injectIcon(const std::filesystem::path& executablePath, const std::filesystem::path& projectPath);
    static bool injectVersionInfo(const std::filesystem::path& executablePath, const GameMetadata& meta);
    static std::string getUserInput(const std::string& prompt);
    static std::string sanitizeFilename(const std::string& filename);
};

}
}
