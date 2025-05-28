@echo off
echo DirectX 11 Engine Build Script
echo ================================

REM Check if build directory exists
if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build

echo Generating Visual Studio project files...
cmake .. -G "Visual Studio 16 2019" -A x64

if %ERRORLEVEL% NEQ 0 (
    echo CMake generation failed!
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable location: build\Release\DX11Engine.exe

echo.
echo To run the engine:
echo   cd build\Release
echo   DX11Engine.exe

pause
