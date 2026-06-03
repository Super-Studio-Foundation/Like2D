@echo off
setlocal enabledelayedexpansion

:: ===========================================
:: Like2D Framework Build System
:: SSOFoundation - International Edition
:: ===========================================

:: --- CONFIGURATION ---
set RELEASE_NAME=Like2D.exe
set DEBUG_NAME=LikeC.exe
set DLL_NAME=Like.dll
set BUILD_DIR=build
set ENGINE_SOURCES=src\Like2D.cpp src\core\Engine.cpp src\renderer\Renderer.cpp src\scripting\LuaBindings.cpp src\compile\compiler.cpp src\audio\AudioManager.cpp external\miniz\miniz.c
set LAUNCHER_SOURCES=src\main.cpp
set RESOURCE_FILE=resource.rc

:: Library locations
set SDL_DIR=external\SDL3\build
set LUAU_DIR=external\Luau\build

set INCLUDES=-Iinclude -Iexternal\SDL3\include -Iexternal\Luau\VM\include -Iexternal\Luau\Compiler\include -Iexternal\Luau\Ast\include -Iexternal\Luau\Config\include -Iexternal\Luau\Common\include -Iexternal\box2d\include -Iexternal\miniaudio -Iexternal\nlohmann -Iexternal\pl_mpeg -Isrc
set LIB_PATHS=-L"%SDL_DIR%" -L"%LUAU_DIR%" -L"external\lib" -L"%BUILD_DIR%"

:: Libraries for dynamic linking to SDL3.dll
set LIBS=-lSDL3 -lLuau.Compiler -lLuau.VM -lLuau.Ast -lLuau.Bytecode -lLuau.Common -lbox2d -lgdi32 -luser32 -lkernel32 -lwinmm -lole32 -loleaut32 -lsetupapi -lversion -luuid

:: Compilation flags for performance and small size
set CFLAGS=-std=c++20 -O2 -static-libgcc -static-libstdc++ -c
set DLL_CFLAGS=-std=c++20 -O2 -static-libgcc -static-libstdc++ -c -DLIKE2D_EXPORTS
set MONOLITHIC_CFLAGS=-std=c++20 -O2 -static-libgcc -static-libstdc++ -c -DLIKE2D_MONOLITHIC
set DLL_LFLAGS=-Wl,-subsystem,windows -shared
set RELEASE_LFLAGS=-Wl,-subsystem,windows
set DEBUG_LFLAGS=-Wl,-subsystem,console
:: ---------------------

:: Check for dist command
if "%1"=="dist" goto :dist
if "%1"=="clean" goto :clean

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

:: Delete old Like2D.exe to force relink with correct GUI subsystem
if exist "%BUILD_DIR%\%RELEASE_NAME%" del /q "%BUILD_DIR%\%RELEASE_NAME%"

echo [1/6] Checking dependencies...
:: Auto-dependency check
call :checkDependencies

echo [2/6] Verifying build materials...
:: Check SDL3 import library
if not exist "%SDL_DIR%\libSDL3.dll.a" (
    echo [ERROR] libSDL3.dll.a not found in %SDL_DIR%!
    echo Please ensure this file exists for linking to SDL3.dll.
    pause
    exit /b 1
)

echo [3/6] Compiling resources...
set RESOURCE_OBJ=%BUILD_DIR%\resource.o
set NEED_COMPILE_RESOURCE=1

if exist "%RESOURCE_OBJ%" (
    if exist "%RESOURCE_FILE%" (
        for %%F in ("%RESOURCE_FILE%") do set RESOURCE_TIME=%%~tF
        for %%F in ("%RESOURCE_OBJ%") do set OBJ_TIME=%%~tF
        if "!RESOURCE_TIME!" leq "!OBJ_TIME!" set NEED_COMPILE_RESOURCE=0
    )
)

if %NEED_COMPILE_RESOURCE%==1 (
    echo   Compiling %RESOURCE_FILE%...
    windres -i "%RESOURCE_FILE%" -o "%RESOURCE_OBJ%"
    if errorlevel 1 (
        echo [ERROR] Failed to compile resource!
        pause
        exit /b 1
    )
) else (
    echo   Resource is up-to-date, skipping compilation.
)

echo [4/6] Compiling engine DLL (incremental)...
set ENGINE_OBJECT_FILES=
set NEED_DLL_LINK=0

for %%S in (%ENGINE_SOURCES%) do (
    set SOURCE_FILE=%%S
    set OBJ_FILE=%BUILD_DIR%\%%~nS.o
    
    :: Check if object file needs compilation
    set NEED_COMPILE=1
    if exist "!OBJ_FILE!" (
        for %%F in ("!SOURCE_FILE!") do set SOURCE_TIME=%%~tF
        for %%F in ("!OBJ_FILE!") do set OBJ_TIME=%%~tF
        if "!SOURCE_TIME!" leq "!OBJ_TIME!" set NEED_COMPILE=0
    )
    
    if !NEED_COMPILE!==1 (
        echo   Compiling !SOURCE_FILE!...
        g++ %DLL_CFLAGS% %INCLUDES% "!SOURCE_FILE!" -o "!OBJ_FILE!"
        if errorlevel 1 (
            echo [ERROR] Failed to compile !SOURCE_FILE!
            pause
            exit /b 1
        )
        set NEED_DLL_LINK=1
    ) else (
        echo   !SOURCE_FILE! is up-to-date, skipping compilation.
    )
    
    set ENGINE_OBJECT_FILES=!ENGINE_OBJECT_FILES! "!OBJ_FILE!"
)

:: Check if DLL exists and needs linking
if exist "%BUILD_DIR%\%DLL_NAME%" (
    if %NEED_DLL_LINK%==0 (
        for %%F in ("%RESOURCE_OBJ%") do set RES_TIME=%%~tF
        for %%F in ("%BUILD_DIR%\%DLL_NAME%") do set DLL_TIME=%%~tF
        if "!RES_TIME!" gtr "!DLL_TIME!" set NEED_DLL_LINK=1
    )
) else (
    set NEED_DLL_LINK=1
)

echo [5/6] Linking %DLL_NAME%...
if %NEED_DLL_LINK%==1 (
    echo   Linking engine DLL...
    g++ %DLL_LFLAGS% %ENGINE_OBJECT_FILES% "%RESOURCE_OBJ%" -o "%BUILD_DIR%\%DLL_NAME%" %LIB_PATHS% %LIBS%
    if errorlevel 1 (
        echo.
        echo [ERROR] Failed to link DLL! Check error messages above.
        pause
        exit /b 1
    )
) else (
    echo   DLL is up-to-date, skipping linking.
)

echo [6/6] Compiling launcher executables (Release + Debug)...
set LAUNCHER_OBJECT_FILES=
set NEED_EXE_LINK=0

for %%S in (%LAUNCHER_SOURCES%) do (
    set SOURCE_FILE=%%S
    set OBJ_FILE=%BUILD_DIR%\%%~nS.o
    
    :: Check if object file needs compilation
    set NEED_COMPILE=1
    if exist "!OBJ_FILE!" (
        for %%F in ("!SOURCE_FILE!") do set SOURCE_TIME=%%~tF
        for %%F in ("!OBJ_FILE!") do set OBJ_TIME=%%~tF
        if "!SOURCE_TIME!" leq "!OBJ_TIME!" set NEED_COMPILE=0
    )
    
    if !NEED_COMPILE!==1 (
        echo   Compiling !SOURCE_FILE!...
        g++ %CFLAGS% %INCLUDES% "!SOURCE_FILE!" -o "!OBJ_FILE!"
        if errorlevel 1 (
            echo [ERROR] Failed to compile !SOURCE_FILE!
            pause
            exit /b 1
        )
        set NEED_EXE_LINK=1
    ) else (
        echo   !SOURCE_FILE! is up-to-date, skipping compilation.
    )
    
    set LAUNCHER_OBJECT_FILES=!LAUNCHER_OBJECT_FILES! "!OBJ_FILE!"
)

:: Check if executables exist and need linking
set NEED_RELEASE_LINK=0
set NEED_DEBUG_LINK=0

:: Always force relink of Like2D.exe to guarantee correct subsystem
:: (incremental skip could leave an old console-subsystem binary)
set NEED_RELEASE_LINK=1

if exist "%BUILD_DIR%\%RELEASE_NAME%" (
    if %NEED_EXE_LINK%==0 (
        for %%F in ("%BUILD_DIR%\%DLL_NAME%") do set DLL_TIME=%%~tF
        for %%F in ("%BUILD_DIR%\%RELEASE_NAME%") do set RELEASE_TIME=%%~tF
        if "!DLL_TIME!" gtr "!RELEASE_TIME!" set NEED_RELEASE_LINK=1
    )
) else (
    set NEED_RELEASE_LINK=1
)

if exist "%BUILD_DIR%\%DEBUG_NAME%" (
    if %NEED_EXE_LINK%==0 (
        for %%F in ("%BUILD_DIR%\%DLL_NAME%") do set DLL_TIME=%%~tF
        for %%F in ("%BUILD_DIR%\%DEBUG_NAME%") do set DEBUG_TIME=%%~tF
        if "!DLL_TIME!" gtr "!DEBUG_TIME!" set NEED_DEBUG_LINK=1
    )
) else (
    set NEED_DEBUG_LINK=1
)

echo   Linking %RELEASE_NAME% (Release)...
if %NEED_RELEASE_LINK%==1 (
    echo   Building release executable with Windows subsystem...
    g++ %RELEASE_LFLAGS% %LAUNCHER_OBJECT_FILES% %BUILD_DIR%\resource.o -o "%BUILD_DIR%\%RELEASE_NAME%" -L"%BUILD_DIR%" -lLike
    if errorlevel 1 (
        echo.
        echo [ERROR] Failed to link release executable! Check error messages above.
        pause
        exit /b 1
    )
) else (
    echo   Release executable is up-to-date, skipping linking.
)

echo   Linking %DEBUG_NAME% (Debug)...
if %NEED_DEBUG_LINK%==1 (
    echo   Building debug executable with Console subsystem...
    g++ %DEBUG_LFLAGS% %LAUNCHER_OBJECT_FILES% %BUILD_DIR%\resource.o -o "%BUILD_DIR%\%DEBUG_NAME%" -L"%BUILD_DIR%" -lLike
    if errorlevel 1 (
        echo.
        echo [ERROR] Failed to link debug executable! Check error messages above.
        pause
        exit /b 1
    )
) else (
    echo   Debug executable is up-to-date, skipping linking.
)

echo.
echo [SUCCESS] Build completed successfully!
echo Deploying SDL3.dll...

:: Copy SDL3.dll to be alongside Like2D.exe
if exist "%SDL_DIR%\SDL3.dll" (
    copy "%SDL_DIR%\SDL3.dll" "%BUILD_DIR%\" /Y >nul
    echo [+] SDL3.dll deployed to %BUILD_DIR% folder!
) else (
    echo [!] WARNING: SDL3.dll not found. Please place it manually next to the executable!
)

if exist "%BUILD_DIR%\SDL3_ttf.dll" (
    echo [+] SDL3_ttf.dll already available in %BUILD_DIR% folder!
) else (
    echo [!] WARNING: SDL3_ttf.dll not found in build directory. Please copy it manually!
)

goto :end

:checkDependencies
:: Auto-dependency check and setup
if not exist "external\include" (
    echo   Setting up dependencies...
    if not exist "external" mkdir external
    if not exist "external\include" mkdir external\include
    if not exist "external\lib" mkdir external\lib
    
    echo   Dependencies folder structure created.
    echo   Please download required SDKs or run setup script.
    echo.
    echo   Required dependencies:
    echo   - SDL3 (https://github.com/libsdl-org/SDL)
    echo   - Luau (https://github.com/Roblox/luau)
    echo   - STB Image (included in external/stb)
    echo.
    echo   Place SDL3 and Luau in external/ folder with proper structure.
)
goto :eof

:clean
echo Cleaning build directory...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
echo Build directory cleaned.
goto :end

:dist
echo Creating distribution package...
if not exist "%BUILD_DIR%" (
    echo [ERROR] Build directory not found. Run build.bat first.
    pause
    exit /b 1
)

set DIST_DIR=Like2D-Dist
if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"

:: Copy everything from build directory except .o files
echo   Copying build files...
xcopy "%BUILD_DIR%\*.exe" "%DIST_DIR%\" /Q >nul
xcopy "%BUILD_DIR%\*.dll" "%DIST_DIR%\" /Q >nul
xcopy "%BUILD_DIR%\*.rc" "%DIST_DIR%\" /Q >nul
xcopy "%BUILD_DIR%\scripts" "%DIST_DIR%\scripts\" /E /I /Q >nul

echo   Cleaning object files...
del /q "%DIST_DIR%\*.o" 2>nul

:: Copy SDL3.dll if available
copy "%SDL_DIR%\SDL3.dll" "%DIST_DIR%\" >nul 2>nul

:: Copy sample scripts
if exist "scripts" xcopy "scripts" "%DIST_DIR%\scripts\" /E /I /Q >nul

:: Copy documentation and project files
if exist "docs" xcopy "docs" "%DIST_DIR%\docs\" /E /I /Q >nul
if exist "CREDITS.md" copy "CREDITS.md" "%DIST_DIR%\" >nul
if exist "CONTRIBUTING.md" copy "CONTRIBUTING.md" "%DIST_DIR%\" >nul
if exist "contributors.txt" copy "contributors.txt" "%DIST_DIR%\" >nul

:: Copy license and updates files
if exist "license.txt" copy "license.txt" "%DIST_DIR%\" >nul
if exist "updates.txt" copy "updates.txt" "%DIST_DIR%\" >nul
if exist "README.md" copy "README.md" "%DIST_DIR%\" >nul

echo Distribution package created in %DIST_DIR% folder.
goto :end

:end
echo Build process completed.
pause