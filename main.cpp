#include "Engine/Engine.h"
#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
    // Create engine instance
    Engine engine;

    // Initialize engine
    if (!engine.Initialize(hInstance, 1280, 720, L"DirectX 11 Engine with Camera System"))
    {
        std::wcout << L"Failed to initialize engine" << std::endl;
        return -1;
    }

    // Run main loop
    engine.Run();

    // Cleanup
    engine.Shutdown();

    return 0;
}

// Alternative console entry point for debugging
int main()
{
    std::wcout << L"DirectX 11 Engine with Separated Game Loop" << std::endl;
    std::wcout << L"==========================================" << std::endl;
    std::wcout << L"Architecture:" << std::endl;
    std::wcout << L"  - Fixed timestep for game logic (60 UPS)" << std::endl;
    std::wcout << L"  - Variable framerate for rendering" << std::endl;
    std::wcout << L"  - Separated input/update/render loops" << std::endl;
    std::wcout << L"  - Frame interpolation for smooth visuals" << std::endl;
    std::wcout << L"" << std::endl;
    std::wcout << L"Controls:" << std::endl;
    std::wcout << L"  WASD - Move character/camera" << std::endl;
    std::wcout << L"  QE - Move character/camera up/down" << std::endl;
    std::wcout << L"  Hold left mouse button and move mouse - Camera control" << std::endl;
    std::wcout << L"  Mouse wheel - Zoom in/out (third person mode)" << std::endl;
    std::wcout << L"  C - Toggle camera mode (First/Third Person)" << std::endl;
    std::wcout << L"  ESC - Exit" << std::endl;
    std::wcout << L"" << std::endl;
    std::wcout << L"Features:" << std::endl;
    std::wcout << L"  - Triangle rotates at 1 rad/sec (game logic)" << std::endl;
    std::wcout << L"  - Cube rotates at 2 rad/sec (game logic)" << std::endl;
    std::wcout << L"  - Smooth visual interpolation between updates" << std::endl;
    std::wcout << L"Starting engine..." << std::endl;

    return WinMain(GetModuleHandle(nullptr), nullptr, nullptr, SW_SHOWNORMAL);
}
