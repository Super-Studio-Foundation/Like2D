@echo off
setlocal enabledelayedexpansion

:: ===========================================
:: Like2D Framework Full Build System
:: SSOFoundation - International Edition
:: Builds all dependencies from source
:: ===========================================

echo Building Like2D Framework with all dependencies...

:: --- CONFIGURATION ---
set CXX=g++
set FLAGS=-std=c++20 -O2 -s -Wall -Wextra -DNDEBUG -static-libgcc -static-libstdc++
set BUILD_DIR=build
set RELEASE_NAME=Like2D.exe
set DEBUG_NAME=LikeC.exe
set DLL_NAME=Like.dll

:: --- SOURCE SEPARATION ---
set ENGINE_SOURCES=src\Like2D.cpp src\core\Engine.cpp src\renderer\Renderer.cpp src\scripting\LuaBindings.cpp
set LAUNCHER_SOURCES=src\main.cpp
set RESOURCE_FILE=resource.rc

:: Create build directories
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist external\SDL3\build mkdir external\SDL3\build
if not exist external\Luau\build mkdir external\Luau\build
if not exist external\include mkdir external\include
if not exist external\lib mkdir external\lib

echo.
echo [1/6] Setting up dependency folders...
call :setupDependencies

echo.
echo [2/6] Building SDL3 from source...
cd external\SDL3\build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_INSTALL_PREFIX=../..
if errorlevel 1 goto error
mingw32-make -j4
if errorlevel 1 goto error
mingw32-make install
cd ..\..\..

echo.
echo [3/6] Building Luau from source...
cd external\Luau\build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../..
if errorlevel 1 goto error
mingw32-make -j4
if errorlevel 1 goto error
mingw32-make install
cd ..\..\..

echo.
echo [4/6] Compiling resources...
windres -i resource.rc -o %BUILD_DIR%\resource.o
if errorlevel 1 goto error

echo.
echo [5/6] Building Like2D Engine DLL...
set INCLUDES=-Iinclude -Iexternal\SDL3\include -Iexternal\Luau\VM\include -Iexternal\Luau\Compiler\include -Iexternal\Luau\Ast\include -Iexternal\Luau\Config\include -Iexternal\Luau\Common\include -Isrc
set LIBS=-Lexternal\SDL3\build -lSDL3 -Lexternal\Luau\build -lLuau.Compiler -lLuau.VM -lLuau.Ast -lLuau.Bytecode -lLuau.Common -lgdi32 -luser32 -lkernel32 -lwinmm -lole32 -loleaut32 -lsetupapi -lversion -luuid

g++ %FLAGS% -shared -DLIKE2D_EXPORTS %INCLUDES% %ENGINE_SOURCES% %BUILD_DIR%\resource.o -o %BUILD_DIR%\%DLL_NAME% %LIBS%
if errorlevel 1 goto error

echo.
echo [6/6] Building Like2D Launchers (Release + Debug)...
echo   Building release executable with Windows subsystem...
g++ %FLAGS% -Wl,-subsystem,windows %INCLUDES% %LAUNCHER_SOURCES% %BUILD_DIR%\resource.o -o %BUILD_DIR%\%RELEASE_NAME% -L%BUILD_DIR% -lLike
if errorlevel 1 goto error

echo   Building debug executable with Console subsystem...
g++ %FLAGS% -Wl,-subsystem,console %INCLUDES% %LAUNCHER_SOURCES% %BUILD_DIR%\resource.o -o %BUILD_DIR%\%DEBUG_NAME% -L%BUILD_DIR% -lLike
if errorlevel 1 goto error

:: Copy runtime dependencies
echo.
echo Deploying runtime dependencies...
if exist external\SDL3\build\SDL3.dll copy external\SDL3\build\SDL3.dll %BUILD_DIR%\ >nul

:: Copy sample scripts
if not exist %BUILD_DIR%\scripts xcopy scripts %BUILD_DIR%\scripts /E /I /Q >nul

echo.
echo [SUCCESS] Full build completed successfully!
echo.
echo Build artifacts:
dir %BUILD_DIR%\%RELEASE_NAME% | find "Like2D.exe"
dir %BUILD_DIR%\%DEBUG_NAME% | find "LikeC.exe"
dir %BUILD_DIR%\%DLL_NAME% | find "Like.dll"
if exist %BUILD_DIR%\SDL3.dll dir %BUILD_DIR%\SDL3.dll | find "SDL3.dll"

echo.
echo To run the applications:
echo   Release: cd %BUILD_DIR% && %RELEASE_NAME%
echo   Debug:    cd %BUILD_DIR% && %DEBUG_NAME%
goto end

:setupDependencies
:: Create proper directory structure
if not exist external\include mkdir external\include
if not exist external\lib mkdir external\lib
if not exist external\bin mkdir external\bin

echo   Dependency folders created/verified.
goto :eof

:error
echo.
echo [ERROR] Build failed! Check error messages above.
echo Please ensure:
echo - CMake and MinGW-w64 are installed and in PATH
echo - Git is available for cloning dependencies
echo - All required tools are accessible
pause
exit /b 1

:end
echo.
echo Full build process completed.
pause
