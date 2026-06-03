#include "compiler.hpp"
#include "../external/miniz/miniz.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <cstdint>

#ifdef _WIN32
#  include <windows.h>
#  include <shlobj.h>
#else
#  include <unistd.h>
#endif

namespace Like2D {
namespace Compiler {

COMPILER_API bool Compiler::runCompilerWizard(const std::string& projectPath, TargetPlatform target) {
    std::filesystem::path normalizedPath = std::filesystem::absolute(projectPath);
    normalizedPath = normalizedPath.make_preferred();
    
    std::cout << "Target Path: " << normalizedPath.string() << std::endl;
    
    if (!std::filesystem::exists(normalizedPath)) {
        std::cerr << "ERROR: Project directory does not exist: " << normalizedPath.string() << std::endl;
        return false;
    }
    
    if (!validateProject(normalizedPath)) {
        std::cerr << "Error: Invalid project structure." << std::endl;
        return false;
    }
    
    std::cout << "=== Like2D Compiler Wizard ===" << std::endl;
    std::cout << "Project Path: " << normalizedPath.string() << std::endl << std::endl;

    // ── Collect metadata ──────────────────────────────────────
    GameMetadata meta;

    // Platform-specific exe suffix
    const std::string exeSuffix = (target == TargetPlatform::Windows) ? ".exe" : "";

    meta.gameName = getUserInput("Game Name (used as executable filename): ");
    if (meta.gameName.empty()) {
        std::cerr << "Error: Game name cannot be empty." << std::endl;
        return false;
    }

    meta.author = getUserInput("Author / Company Name: ");
    meta.version = getUserInput("Version (e.g. 1.0.0) [default: 1.0.0]: ");
    if (meta.version.empty()) meta.version = "1.0.0";

    if (target == TargetPlatform::Windows) {
        meta.description = getUserInput("File Description (shown in Properties) [default: " + meta.gameName + "]: ");
        if (meta.description.empty()) meta.description = meta.gameName;

        meta.originalFilename = getUserInput("Original Filename [default: " + sanitizeFilename(meta.gameName) + ".exe]: ");
        if (meta.originalFilename.empty()) meta.originalFilename = sanitizeFilename(meta.gameName) + ".exe";

        meta.productName = getUserInput("Product Name [default: " + meta.gameName + "]: ");
        if (meta.productName.empty()) meta.productName = meta.gameName;

        std::string defaultCopyright = "Copyright (C) 2025 " + (meta.author.empty() ? meta.gameName : meta.author);
        meta.copyright = getUserInput("Legal Copyright [default: " + defaultCopyright + "]: ");
        if (meta.copyright.empty()) meta.copyright = defaultCopyright;
    } else {
        meta.description     = meta.gameName;
        meta.originalFilename= sanitizeFilename(meta.gameName);
        meta.productName     = meta.gameName;
        meta.copyright       = "Copyright (C) 2025 " + meta.author;
    }

    // Encryption option
    std::string encryptChoice = getUserInput("Encrypt game.like? (y/n) [default: n]: ");
    meta.encryptArchive = (!encryptChoice.empty() && (encryptChoice[0] == 'y' || encryptChoice[0] == 'Y'));

    // Fusion option (ONLY for Linux)
    if (target == TargetPlatform::Linux) {
        std::string fuseChoice = getUserInput("Fuse game into a single executable? (y/n) [default: n]: ");
        meta.fuseBinary = (!fuseChoice.empty() && (fuseChoice[0] == 'y' || fuseChoice[0] == 'Y'));
    } else {
        meta.fuseBinary = false;
    }

    std::cout << std::endl;
    std::cout << "=== Build Summary ===" << std::endl;
    std::cout << "  Executable:  " << sanitizeFilename(meta.gameName) << exeSuffix << std::endl;
    std::cout << "  Author:      " << meta.author   << std::endl;
    std::cout << "  Version:     " << meta.version  << std::endl;
    std::cout << "  Encryption:  " << (meta.encryptArchive ? "YES (XOR)" : "NO (plain zip)") << std::endl;
    if (target == TargetPlatform::Linux) {
        std::cout << "  Single-File: " << (meta.fuseBinary ? "YES (Fused)" : "NO (Separate game.like)") << std::endl;
    }
    std::cout << "  Platform:    " << (target == TargetPlatform::Windows ? "Windows" : "Linux") << std::endl << std::endl;

    if (!createDistFolder(normalizedPath)) {
        std::cerr << "Error: Failed to create dist folder." << std::endl;
        return false;
    }
    
    if (!copyExecutableAndDLLs(normalizedPath, meta.gameName, target)) {
        std::cerr << "Error: Failed to copy executable and libraries." << std::endl;
        return false;
    }
    
    std::filesystem::path distPath = normalizedPath / "dist";
    if (!packageAssets(normalizedPath, distPath, meta.encryptArchive)) {
        std::cerr << "Error: Failed to package assets." << std::endl;
        return false;
    }

    std::filesystem::path executablePath = distPath / (sanitizeFilename(meta.gameName) + exeSuffix);

    // ── Perform Fusion if requested ─────────────────────────────
    if (meta.fuseBinary) {
        std::cout << "Performing Fusion (Stitching binary + game data)..." << std::endl;
        std::filesystem::path gameLikePath = distPath / "game.like";
        
        if (std::filesystem::exists(gameLikePath)) {
            // Append game.like to the executable
            std::ofstream out(executablePath, std::ios::binary | std::ios::app);
            std::ifstream in(gameLikePath, std::ios::binary);
            if (out.is_open() && in.is_open()) {
                out << in.rdbuf();
                out.close();
                in.close();
                
                // Cleanup separate game.like since it's now inside the binary
                std::filesystem::remove(gameLikePath);
                std::cout << "Successfully fused game data into " << executablePath.filename().string() << std::endl;
            } else {
                std::cerr << "Warning: Failed to open files for fusion stitching." << std::endl;
            }
        }
    }

    if (target == TargetPlatform::Windows) {
#ifdef _WIN32
        // Inject icon (Windows host targeting Windows only)
        if (!injectIcon(executablePath, normalizedPath)) {
            std::cout << "Warning: Failed to inject icon (optional)." << std::endl;
        }
        // Inject version info (Windows host targeting Windows only)
        if (!injectVersionInfo(executablePath, meta)) {
            std::cout << "Warning: Failed to inject version info (optional)." << std::endl;
        }
#endif
    }

    std::cout << std::endl << "=== Compilation Complete ===" << std::endl;
    std::cout << "Output:  " << executablePath.string() << std::endl;
    std::cout << "Assets:  " << (distPath / "game.like").string() << std::endl;

    return true;
}

bool Compiler::validateProject(const std::filesystem::path& projectPath) {
    std::filesystem::path mainLuau = projectPath / "main.luau";
    std::filesystem::path mainLua  = projectPath / "main.lua";
    std::filesystem::path assetsDir = projectPath / "assets";

    bool hasMainLuau = std::filesystem::exists(mainLuau);
    bool hasMainLua  = std::filesystem::exists(mainLua);
    bool hasAssets   = std::filesystem::exists(assetsDir) && std::filesystem::is_directory(assetsDir);

    std::cout << "Project files:" << std::endl;
    std::cout << "  main.luau:    " << (hasMainLuau ? "Found" : "Not found") << std::endl;
    std::cout << "  main.lua:     " << (hasMainLua  ? "Found" : "Not found") << std::endl;
    std::cout << "  assets folder:" << (hasAssets   ? "Found" : "Not found") << std::endl;

    return hasMainLuau || hasMainLua || hasAssets;
}

bool Compiler::createDistFolder(const std::filesystem::path& projectPath) {
    std::filesystem::path distPath = projectPath / "dist";
    std::error_code ec;
    
    if (std::filesystem::exists(distPath)) {
        std::cout << "Cleaning existing dist folder..." << std::endl;
        std::filesystem::remove_all(distPath, ec);
        if (ec) {
            std::cerr << "Error cleaning dist folder: " << ec.message() << std::endl;
            std::cerr << "Please close any running instances of the game and try again." << std::endl;
            return false;
        }
    }
    
    std::cout << "Creating dist folder: " << distPath.string() << std::endl;
    
    int retryCount = 0;
    const int maxRetries = 3;
    
    while (retryCount < maxRetries) {
        std::filesystem::create_directories(distPath, ec);
        if (!ec) {
            return true;
        }
        
        retryCount++;
        if (retryCount < maxRetries) {
            std::cout << "Retry " << retryCount << "/" << maxRetries << "..." << std::endl;
#ifdef _WIN32
            Sleep(500);
#else
            usleep(500000);
#endif
        }
    }
    
    std::cerr << "Error creating dist folder after " << maxRetries << " attempts: " << ec.message() << std::endl;
    return false;
}

bool Compiler::copyExecutableAndDLLs(const std::filesystem::path& projectPath, const std::string& gameName, TargetPlatform target) {
    // ── Resolve current host exe path ───────────────────────────────
    std::filesystem::path currentExe;
#ifdef _WIN32
    char currentHostPath[MAX_PATH];
    GetModuleFileNameA(NULL, currentHostPath, MAX_PATH);
    currentExe = std::filesystem::path(currentHostPath);
#else
    char currentHostPath[4096] = {};
    ssize_t len = readlink("/proc/self/exe", currentHostPath, sizeof(currentHostPath)-1);
    if (len >= 0) {
        currentHostPath[len] = '\0';
        currentExe = std::filesystem::path(currentHostPath);
    }
#endif

    if (currentExe.empty()) {
         std::cerr << "Error: Failed to resolve current executable path." << std::endl;
         return false;
    }

    std::filesystem::path exeDir   = currentExe.parent_path();
    std::filesystem::path distPath = projectPath / "dist";

    // ── Resolve source binary based on target platform ──────────
    std::filesystem::path sourceExe;
    std::string targetExeSuffix = (target == TargetPlatform::Windows) ? ".exe" : "";
    std::string targetLibExt = (target == TargetPlatform::Windows) ? ".dll" : ".so";

    bool crossCompiling = false;
#ifdef _WIN32
    if (target == TargetPlatform::Linux) crossCompiling = true;
#else
    if (target == TargetPlatform::Windows) crossCompiling = true;
#endif

    if (!crossCompiling) {
        sourceExe = currentExe;
    } else {
        // Look for the target binary in the same directory as the host binary
        std::string targetBaseName = currentExe.stem().string(); // Use current runner's name (e.g. Like2D or LikeC)
        sourceExe = exeDir / (targetBaseName + targetExeSuffix);
        
        if (!std::filesystem::exists(sourceExe)) {
            // Try looking in a platform-specific subfolder
            std::string platformFold = (target == TargetPlatform::Windows) ? "windows" : "linux";
            sourceExe = exeDir / platformFold / (targetBaseName + targetExeSuffix);
        }

        if (!std::filesystem::exists(sourceExe)) {
            std::cerr << "\n[!] Error: Target runner binary not found: " << (target == TargetPlatform::Windows ? "Like2D.exe" : "Like2D") << std::endl;
            std::cerr << "To package for " << (target == TargetPlatform::Windows ? "Windows" : "Linux") << " from this host, you need to provide the engine binary for that platform." << std::endl;
            std::cerr << "Please place the " << (target == TargetPlatform::Windows ? "Windows" : "Linux") << " version of the engine (and its libraries) in:" << std::endl;
            std::cerr << "  -> " << exeDir.string() << std::endl;
            std::cerr << "  OR in a " << (target == TargetPlatform::Windows ? "windows" : "linux") << "/ subfolder." << std::endl;
            std::cerr << "\nTIP: You don't need WSL to use the packager, just the binaries themselves." << std::endl;
            std::cerr << "If you don't have them, build the engine on the target platform first and copy them over." << std::endl;
            return false;
        }
    }

    std::filesystem::path newExe = distPath / (sanitizeFilename(gameName) + targetExeSuffix);

    std::cout << "Copying executable to: " << newExe.string() << std::endl;
    try {
        std::filesystem::copy_file(sourceExe, newExe,
            std::filesystem::copy_options::overwrite_existing);
        std::cout << "Successfully copied executable." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy executable: " << e.what() << std::endl;
        return false;
    }

    // ── Apply platform-specific post-processing ─────────────────
    if (target == TargetPlatform::Windows) {
        // Patch PE subsystem to WINDOWS GUI (no console window)
        std::fstream exeFile(newExe, std::ios::in | std::ios::out | std::ios::binary);
        if (exeFile.is_open()) {
            uint32_t peOffset = 0;
            exeFile.seekg(0x3C);
            exeFile.read(reinterpret_cast<char*>(&peOffset), 4);
            uint32_t subsystemOffset = peOffset + 4 + 20 + 68;
            uint16_t subsystem = 2; // IMAGE_SUBSYSTEM_WINDOWS_GUI
            exeFile.seekp(subsystemOffset);
            exeFile.write(reinterpret_cast<const char*>(&subsystem), 2);
            exeFile.close();
            std::cout << "Patched executable subsystem to GUI (no console window)." << std::endl;
        } else {
            std::cout << "Warning: Could not patch subsystem (non-critical)." << std::endl;
        }
    } else {
        // chmod +x on Linux
        std::error_code ec;
        std::filesystem::permissions(newExe,
            std::filesystem::perms::owner_exec |
            std::filesystem::perms::group_exec |
            std::filesystem::perms::others_exec,
            std::filesystem::perm_options::add, ec);
        if (!ec) std::cout << "Set executable permissions (chmod +x)." << std::endl;
    }

    // ── Copy shared libraries ──────────────────────────────────
    int libCount = 0;
    std::filesystem::path libSourceDir = crossCompiling ? sourceExe.parent_path() : exeDir;
    
    for (const auto& entry : std::filesystem::directory_iterator(libSourceDir)) {
        if (!entry.is_regular_file()) continue;
        std::string fname = entry.path().filename().string();
        std::string ext   = entry.path().extension().string();

        bool isLib = false;
        if (target == TargetPlatform::Windows) {
            isLib = (ext == ".dll");
        } else {
            // Linux libraries usually start with 'lib' and end with '.so' or '.so.X'
            isLib = (ext == ".so") || (fname.find(".so.") != std::string::npos) || (fname.find("lib") == 0 && ext == ".so");
        }

        if (!isLib) continue;

        // Special case: if we are on Windows targeting Linux, and we found 'Like.dll', 
        // we should actually be looking for 'libLike.so' in the linux/ subfolder.
        // But the current loop handles files found in libSourceDir.

        std::filesystem::path libDest = distPath / entry.path().filename();
        try {
            std::filesystem::copy_file(entry.path(), libDest,
                std::filesystem::copy_options::overwrite_existing);
            std::cout << "Copied lib: " << fname << std::endl;
            libCount++;
        } catch (const std::exception& e) {
            std::cerr << "Failed to copy lib " << fname << ": " << e.what() << std::endl;
        }
    }
    std::cout << "Copied " << libCount << " library file(s)." << std::endl;
    return true;
}

// XOR key — must match the key used in LuaBindings.cpp for decryption
static const uint8_t LIKE2D_XOR_KEY[] = {
    0x4C, 0x69, 0x6B, 0x65, 0x32, 0x44, 0x53, 0x65,
    0x63, 0x72, 0x65, 0x74, 0x4B, 0x65, 0x79, 0x21
};
static const size_t LIKE2D_XOR_KEY_LEN = sizeof(LIKE2D_XOR_KEY);

// Magic header written at the start of an encrypted game.like
// so the runtime knows to decrypt before treating it as a zip.
static const uint8_t LIKE2D_ENC_MAGIC[] = { 'L', '2', 'D', 'E' };

static void xorEncrypt(std::vector<char>& data) {
    for (size_t i = 0; i < data.size(); i++) {
        data[i] ^= (char)LIKE2D_XOR_KEY[i % LIKE2D_XOR_KEY_LEN];
    }
}

bool Compiler::packageAssets(const std::filesystem::path& projectPath,
                              const std::filesystem::path& distPath,
                              bool encrypt) {
    std::filesystem::path gameLikePath = distPath / "game.like";

    if (encrypt) {
        // Build the zip in memory first, then XOR-encrypt and write with magic header
        std::cout << "Creating encrypted game.like archive..." << std::endl;

        mz_zip_archive zip = {};
        if (!mz_zip_writer_init_heap(&zip, 0, 0)) {
            std::cerr << "Failed to initialize in-memory ZIP archive." << std::endl;
            return false;
        }

        int fileCount = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
                if (!entry.is_regular_file()) continue;
                std::filesystem::path entryPath    = entry.path();
                std::filesystem::path relativePath = std::filesystem::relative(entryPath, projectPath);
                std::string relativeStr = relativePath.generic_string();
                if (relativeStr.find("dist/") == 0 || relativeStr.find("dist\\") == 0) continue;

                std::ifstream file(entryPath, std::ios::binary);
                if (!file.is_open()) { std::cerr << "Failed to open: " << entryPath << std::endl; continue; }
                std::vector<char> fileData((std::istreambuf_iterator<char>(file)), {});
                file.close();

                if (!mz_zip_writer_add_mem(&zip, relativeStr.c_str(),
                                            fileData.data(), fileData.size(), MZ_BEST_COMPRESSION)) {
                    std::cerr << "Failed to add to archive: " << relativeStr << std::endl; continue;
                }
                std::cout << "PACKER: " << relativeStr << std::endl;
                fileCount++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during asset packaging: " << e.what() << std::endl;
            mz_zip_writer_end(&zip);
            return false;
        }

        // Finalise to heap buffer
        void*  heapBuf  = nullptr;
        size_t heapSize = 0;
        if (!mz_zip_writer_finalize_heap_archive(&zip, &heapBuf, &heapSize)) {
            std::cerr << "Failed to finalise in-memory archive." << std::endl;
            mz_zip_writer_end(&zip);
            return false;
        }
        mz_zip_writer_end(&zip);

        // XOR-encrypt the buffer
        std::vector<char> encData(static_cast<char*>(heapBuf),
                                   static_cast<char*>(heapBuf) + heapSize);
        mz_free(heapBuf);
        xorEncrypt(encData);

        // Write: 4-byte magic + encrypted zip data
        std::ofstream out(gameLikePath, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Failed to write encrypted game.like." << std::endl;
            return false;
        }
        out.write(reinterpret_cast<const char*>(LIKE2D_ENC_MAGIC), sizeof(LIKE2D_ENC_MAGIC));
        out.write(encData.data(), (std::streamsize)encData.size());
        out.close();

        std::cout << "Created encrypted game.like with " << fileCount << " files." << std::endl;
        return true;

    } else {
        // Plain zip (original behaviour)
        std::cout << "Creating game.like archive: " << gameLikePath.string() << std::endl;

        mz_zip_archive zip = {};
        if (!mz_zip_writer_init_file(&zip, gameLikePath.string().c_str(), 0)) {
            std::cerr << "Failed to initialize ZIP archive." << std::endl;
            return false;
        }

        int fileCount = 0;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
                if (!entry.is_regular_file()) continue;
                std::filesystem::path entryPath    = entry.path();
                std::filesystem::path relativePath = std::filesystem::relative(entryPath, projectPath);
                std::string relativeStr = relativePath.generic_string();
                if (relativeStr.find("dist/") == 0 || relativeStr.find("dist\\") == 0) continue;

                std::ifstream file(entryPath, std::ios::binary);
                if (!file.is_open()) { std::cerr << "Failed to open: " << entryPath << std::endl; continue; }
                std::vector<char> fileData((std::istreambuf_iterator<char>(file)), {});
                file.close();

                if (!mz_zip_writer_add_mem(&zip, relativeStr.c_str(),
                                            fileData.data(), fileData.size(), MZ_BEST_COMPRESSION)) {
                    std::cerr << "Failed to add to archive: " << relativeStr << std::endl; continue;
                }
                std::cout << "PACKER: " << relativeStr << std::endl;
                fileCount++;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during asset packaging: " << e.what() << std::endl;
            mz_zip_writer_end(&zip);
            return false;
        }

        mz_zip_writer_finalize_archive(&zip);
        mz_zip_writer_end(&zip);
        std::cout << "Created game.like with " << fileCount << " files." << std::endl;
        return true;
    }
}

bool Compiler::injectIcon(const std::filesystem::path& executablePath, const std::filesystem::path& projectPath) {
#ifdef _WIN32
    std::filesystem::path iconPath = projectPath / "game.ico";
    
    if (!std::filesystem::exists(iconPath)) {
        std::cout << "No game.ico found, skipping icon injection." << std::endl;
        return true;
    }
    
    std::cout << "Injecting icon: " << iconPath.string() << std::endl;
    
    // Read the entire .ico file
    std::ifstream iconFile(iconPath, std::ios::binary);
    if (!iconFile.is_open()) {
        std::cerr << "Failed to open icon file." << std::endl;
        return false;
    }
    iconFile.seekg(0, std::ios::end);
    size_t iconSize = iconFile.tellg();
    iconFile.seekg(0, std::ios::beg);
    std::vector<unsigned char> iconData(iconSize);
    iconFile.read(reinterpret_cast<char*>(iconData.data()), iconSize);
    iconFile.close();

    // .ico file layout:
    //   ICONDIR  { idReserved(2), idType(2), idCount(2) }
    //   ICONDIRENTRY[idCount] { bWidth(1), bHeight(1), bColorCount(1), bReserved(1),
    //                           wPlanes(2), wBitCount(2), dwBytesInRes(4), dwImageOffset(4) }
    //
    // Windows resource format for RT_GROUP_ICON uses GRPICONDIR / GRPICONDIRENTRY,
    // which is identical except the last field is a WORD resource ID instead of a DWORD offset.

    if (iconSize < 6) {
        std::cerr << "Icon file too small." << std::endl;
        return false;
    }

    WORD idReserved = *reinterpret_cast<WORD*>(&iconData[0]);
    WORD idType     = *reinterpret_cast<WORD*>(&iconData[2]);
    WORD idCount    = *reinterpret_cast<WORD*>(&iconData[4]);

    if (idReserved != 0 || idType != 1 || idCount == 0) {
        std::cerr << "Invalid .ico file format." << std::endl;
        return false;
    }

    // Each ICONDIRENTRY is 16 bytes; GRPICONDIRENTRY is 14 bytes (replaces DWORD offset with WORD id)
    const size_t ICO_HEADER_SIZE   = 6;
    const size_t ICO_ENTRY_SIZE    = 16;
    const size_t GRP_ENTRY_SIZE    = 14;

    if (iconSize < ICO_HEADER_SIZE + (size_t)idCount * ICO_ENTRY_SIZE) {
        std::cerr << "Icon file truncated." << std::endl;
        return false;
    }

    // Build the RT_GROUP_ICON resource blob
    size_t grpSize = ICO_HEADER_SIZE + (size_t)idCount * GRP_ENTRY_SIZE;
    std::vector<unsigned char> grpData(grpSize);

    // Copy the 6-byte ICONDIR header as-is
    memcpy(grpData.data(), iconData.data(), ICO_HEADER_SIZE);

    HANDLE hExe = BeginUpdateResourceA(executablePath.string().c_str(), FALSE);
    if (hExe == NULL) {
        DWORD error = GetLastError();
        std::cerr << "Failed to open executable for icon injection. Error: " << error << std::endl;
        return false;
    }

    bool allOk = true;
    for (WORD i = 0; i < idCount; i++) {
        size_t entryOffset = ICO_HEADER_SIZE + (size_t)i * ICO_ENTRY_SIZE;
        unsigned char* srcEntry = &iconData[entryOffset];

        // Read image data location from the .ico entry
        DWORD imageOffset = *reinterpret_cast<DWORD*>(srcEntry + 12);
        DWORD imageSize   = *reinterpret_cast<DWORD*>(srcEntry + 8);

        if (imageOffset + imageSize > iconSize) {
            std::cerr << "Icon entry " << i << " data out of bounds." << std::endl;
            allOk = false;
            continue;
        }

        // Write RT_ICON resource (raw image data — DIB or PNG)
        WORD resourceId = i + 1;
        BOOL ok = UpdateResourceA(hExe, RT_ICON, MAKEINTRESOURCEA(resourceId),
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                  &iconData[imageOffset], imageSize);
        if (!ok) {
            std::cerr << "Failed to write RT_ICON " << resourceId << ". Error: " << GetLastError() << std::endl;
            allOk = false;
        }

        // Build the GRPICONDIRENTRY (14 bytes): copy first 12 bytes then append the WORD resource ID
        unsigned char* dstEntry = &grpData[ICO_HEADER_SIZE + (size_t)i * GRP_ENTRY_SIZE];
        memcpy(dstEntry, srcEntry, 12);                                    // bWidth..dwBytesInRes
        *reinterpret_cast<WORD*>(dstEntry + 12) = resourceId;             // nId (replaces dwImageOffset)
    }

    // Write RT_GROUP_ICON resource
    BOOL ok = UpdateResourceA(hExe, RT_GROUP_ICON, MAKEINTRESOURCEA(1),
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                              grpData.data(), static_cast<DWORD>(grpData.size()));
    if (!ok) {
        std::cerr << "Failed to write RT_GROUP_ICON. Error: " << GetLastError() << std::endl;
        allOk = false;
    }

    EndUpdateResource(hExe, allOk ? FALSE : TRUE);

    if (allOk) {
        std::cout << "Successfully injected icon (" << idCount << " image(s))." << std::endl;
    } else {
        std::cerr << "Icon injection completed with errors." << std::endl;
    }
    return allOk;

#else
    // Linux — no PE icon injection
    (void)executablePath; (void)projectPath;
    std::cout << "Icon injection skipped (Linux)." << std::endl;
    return true;
#endif // _WIN32 injectIcon
}

bool Compiler::injectVersionInfo(const std::filesystem::path& executablePath, const GameMetadata& meta) {
#ifdef _WIN32
    // Parse version string "major.minor.patch.build"
    unsigned short vMaj = 1, vMin = 0, vPatch = 0, vBuild = 0;
    sscanf_s(meta.version.c_str(), "%hu.%hu.%hu.%hu", &vMaj, &vMin, &vPatch, &vBuild);

    // Helper: convert UTF-8 string to UTF-16LE with null terminator
    auto toWide = [](const std::string& s) -> std::vector<wchar_t> {
        int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::vector<wchar_t> buf(len);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, buf.data(), len);
        return buf;
    };

    // Build VS_VERSION_INFO binary blob manually.
    // Layout: VS_FIXEDFILEINFO + StringFileInfo block + VarFileInfo block
    // All strings are UTF-16LE, all lengths/offsets are in bytes, padded to DWORD.

    std::vector<BYTE> blob;

    auto align4 = [&]() {
        while (blob.size() % 4 != 0) blob.push_back(0);
    };

    auto writeW = [&](WORD v) {
        blob.push_back(v & 0xFF);
        blob.push_back((v >> 8) & 0xFF);
    };

    auto writeDW = [&](DWORD v) {
        blob.push_back(v & 0xFF);
        blob.push_back((v >> 8) & 0xFF);
        blob.push_back((v >> 16) & 0xFF);
        blob.push_back((v >> 24) & 0xFF);
    };

    auto writeWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) {
            blob.push_back(c & 0xFF);
            blob.push_back((c >> 8) & 0xFF);
        }
        // null terminator
        blob.push_back(0); blob.push_back(0);
    };

    auto patchWord = [&](size_t offset, WORD v) {
        blob[offset]   = v & 0xFF;
        blob[offset+1] = (v >> 8) & 0xFF;
    };

    // ── VS_FIXEDFILEINFO ──────────────────────────────────────
    DWORD fileVerMS = ((DWORD)vMaj << 16) | vMin;
    DWORD fileVerLS = ((DWORD)vPatch << 16) | vBuild;

    std::vector<BYTE> ffi;
    auto appendDW = [&](DWORD v) {
        ffi.push_back(v & 0xFF); ffi.push_back((v>>8)&0xFF);
        ffi.push_back((v>>16)&0xFF); ffi.push_back((v>>24)&0xFF);
    };
    appendDW(0xFEEF04BD);  // dwSignature
    appendDW(0x00010000);  // dwStrucVersion
    appendDW(fileVerMS);   // dwFileVersionMS
    appendDW(fileVerLS);   // dwFileVersionLS
    appendDW(fileVerMS);   // dwProductVersionMS
    appendDW(fileVerLS);   // dwProductVersionLS
    appendDW(0x3F);        // dwFileFlagsMask
    appendDW(0x0);         // dwFileFlags
    appendDW(0x00040004);  // dwFileOS (VOS_NT_WINDOWS32)
    appendDW(0x00000001);  // dwFileType (VFT_APP)
    appendDW(0x0);         // dwFileSubtype
    appendDW(0x0);         // dwFileDateMS
    appendDW(0x0);         // dwFileDateLS

    // ── String pairs ─────────────────────────────────────────
    struct StrPair { std::wstring key; std::wstring val; };
    std::vector<StrPair> strings = {
        { L"CompanyName",      std::wstring(meta.author.begin(), meta.author.end()) },
        { L"FileDescription",  std::wstring(meta.description.begin(), meta.description.end()) },
        { L"FileVersion",      std::wstring(meta.version.begin(), meta.version.end()) },
        { L"InternalName",     std::wstring(meta.gameName.begin(), meta.gameName.end()) },
        { L"LegalCopyright",   std::wstring(meta.copyright.begin(), meta.copyright.end()) },
        { L"OriginalFilename", std::wstring(meta.originalFilename.begin(), meta.originalFilename.end()) },
        { L"ProductName",      std::wstring(meta.productName.begin(), meta.productName.end()) },
        { L"ProductVersion",   std::wstring(meta.version.begin(), meta.version.end()) },
    };

    // Build StringTable block
    std::vector<BYTE> strTable;
    auto stAppendW = [&](WORD v) {
        strTable.push_back(v & 0xFF); strTable.push_back((v>>8)&0xFF);
    };
    auto stAlign = [&]() { while (strTable.size()%4!=0) strTable.push_back(0); };
    auto stAppendWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) { strTable.push_back(c&0xFF); strTable.push_back((c>>8)&0xFF); }
        strTable.push_back(0); strTable.push_back(0);
    };

    for (auto& sp : strings) {
        stAlign();
        size_t strOff = strTable.size();
        stAppendW(0); // length placeholder
        stAppendW((WORD)((sp.val.size()+1)));  // value length in WCHARs
        stAppendW(1); // type = string
        stAppendWStr(sp.key);
        stAlign();
        stAppendWStr(sp.val);
        stAlign();
        WORD strLen = (WORD)(strTable.size() - strOff);
        strTable[strOff]   = strLen & 0xFF;
        strTable[strOff+1] = (strLen>>8) & 0xFF;
    }

    // StringTable header: "040904B0" (English US, Unicode)
    std::vector<BYTE> stHeader;
    auto shAppendW = [&](WORD v) { stHeader.push_back(v&0xFF); stHeader.push_back((v>>8)&0xFF); };
    auto shAppendWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) { stHeader.push_back(c&0xFF); stHeader.push_back((c>>8)&0xFF); }
        stHeader.push_back(0); stHeader.push_back(0);
    };
    shAppendW(0); // length placeholder
    shAppendW(0); // value length = 0
    shAppendW(1); // type = string
    shAppendWStr(L"040904B0");
    while (stHeader.size()%4!=0) stHeader.push_back(0);
    // append string entries
    stHeader.insert(stHeader.end(), strTable.begin(), strTable.end());
    WORD stLen = (WORD)stHeader.size();
    stHeader[0] = stLen & 0xFF; stHeader[1] = (stLen>>8)&0xFF;

    // StringFileInfo block
    std::vector<BYTE> sfi;
    auto sfiAppendW = [&](WORD v) { sfi.push_back(v&0xFF); sfi.push_back((v>>8)&0xFF); };
    auto sfiAppendWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) { sfi.push_back(c&0xFF); sfi.push_back((c>>8)&0xFF); }
        sfi.push_back(0); sfi.push_back(0);
    };
    sfiAppendW(0); // length placeholder
    sfiAppendW(0); // value length = 0
    sfiAppendW(1); // type = string
    sfiAppendWStr(L"StringFileInfo");
    while (sfi.size()%4!=0) sfi.push_back(0);
    sfi.insert(sfi.end(), stHeader.begin(), stHeader.end());
    WORD sfiLen = (WORD)sfi.size();
    sfi[0] = sfiLen&0xFF; sfi[1] = (sfiLen>>8)&0xFF;

    // VarFileInfo block
    std::vector<BYTE> vfi;
    auto vfiAppendW = [&](WORD v) { vfi.push_back(v&0xFF); vfi.push_back((v>>8)&0xFF); };;
    auto vfiAppendWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) { vfi.push_back(c&0xFF); vfi.push_back((c>>8)&0xFF); }
        vfi.push_back(0); vfi.push_back(0);
    };
    // Translation entry
    std::vector<BYTE> transEntry;
    auto teAppendW = [&](WORD v) { transEntry.push_back(v&0xFF); transEntry.push_back((v>>8)&0xFF); };
    teAppendW(0); // length placeholder
    teAppendW(4); // value length = 4 bytes
    teAppendW(0); // type = binary
    // "Translation" key
    std::wstring transKey = L"Translation";
    for (wchar_t c : transKey) { transEntry.push_back(c&0xFF); transEntry.push_back((c>>8)&0xFF); }
    transEntry.push_back(0); transEntry.push_back(0);
    while (transEntry.size()%4!=0) transEntry.push_back(0);
    // value: 0x0409 (English), 0x04B0 (Unicode)
    transEntry.push_back(0x09); transEntry.push_back(0x04);
    transEntry.push_back(0xB0); transEntry.push_back(0x04);
    WORD teLen = (WORD)transEntry.size();
    transEntry[0] = teLen&0xFF; transEntry[1] = (teLen>>8)&0xFF;

    vfiAppendW(0); vfiAppendW(0); vfiAppendW(1);
    vfiAppendWStr(L"VarFileInfo");
    while (vfi.size()%4!=0) vfi.push_back(0);
    vfi.insert(vfi.end(), transEntry.begin(), transEntry.end());
    WORD vfiLen = (WORD)vfi.size();
    vfi[0] = vfiLen&0xFF; vfi[1] = (vfiLen>>8)&0xFF;

    // ── Root VS_VERSION_INFO block ────────────────────────────
    std::vector<BYTE> root;
    auto rAppendW = [&](WORD v) { root.push_back(v&0xFF); root.push_back((v>>8)&0xFF); };
    auto rAppendWStr = [&](const std::wstring& s) {
        for (wchar_t c : s) { root.push_back(c&0xFF); root.push_back((c>>8)&0xFF); }
        root.push_back(0); root.push_back(0);
    };
    rAppendW(0);  // length placeholder
    rAppendW((WORD)ffi.size()); // value length = sizeof(VS_FIXEDFILEINFO)
    rAppendW(0);  // type = binary
    rAppendWStr(L"VS_VERSION_INFO");
    while (root.size()%4!=0) root.push_back(0);
    root.insert(root.end(), ffi.begin(), ffi.end());
    while (root.size()%4!=0) root.push_back(0);
    root.insert(root.end(), sfi.begin(), sfi.end());
    while (root.size()%4!=0) root.push_back(0);
    root.insert(root.end(), vfi.begin(), vfi.end());
    WORD rootLen = (WORD)root.size();
    root[0] = rootLen&0xFF; root[1] = (rootLen>>8)&0xFF;

    // ── Inject into executable ────────────────────────────────
    HANDLE hExe = BeginUpdateResourceA(executablePath.string().c_str(), FALSE);
    if (!hExe) {
        std::cerr << "injectVersionInfo: BeginUpdateResource failed. Error: " << GetLastError() << std::endl;
        return false;
    }

    BOOL ok = UpdateResourceA(hExe, RT_VERSION, MAKEINTRESOURCEA(1),
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                               root.data(), (DWORD)root.size());
    EndUpdateResource(hExe, ok ? FALSE : TRUE);

    if (ok) {
        std::cout << "Version info injected successfully." << std::endl;
    } else {
        std::cerr << "injectVersionInfo: UpdateResource failed. Error: " << GetLastError() << std::endl;
    }
    return ok != 0;

#else
    // Not implemented on Linux — icon/version injection is Windows PE-only
    (void)executablePath; (void)meta;
    return true;
#endif
}

std::string Compiler::getUserInput(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string input;
    if (!std::getline(std::cin, input)) {
        std::cin.clear();
        return "";
    }
    
    size_t start = input.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = input.find_last_not_of(" \t\n\r");
    return input.substr(start, end - start + 1);
}

std::string Compiler::sanitizeFilename(const std::string& filename) {
    std::string result = filename;
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char c : invalidChars) {
        size_t pos = 0;
        while ((pos = result.find(c, pos)) != std::string::npos) {
            result.replace(pos, 1, "_");
            pos += 1;
        }
    }
    return result;
}

} // namespace Compiler
} // namespace Like2D
