#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <memory>
#include <string>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

class Camera;
class Renderer;
class GameLoop;

class Engine
{
public:
    Engine();
    ~Engine();

    bool Initialize(HINSTANCE hInstance, int width, int height, const std::wstring& title);
    void Run();
    void Shutdown();

    LRESULT MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

    // Configuration methods
    void SetTargetUPS(int updatesPerSecond);
    void SetTargetFPS(int framesPerSecond);
    void SetVSyncEnabled(bool enabled);

    // Performance getters
    int GetCurrentFPS() const;
    int GetCurrentUPS() const;
    double GetFrameTime() const;
    double GetUpdateTime() const;

private:
    bool InitializeWindow(HINSTANCE hInstance, int width, int height, const std::wstring& title);
    bool InitializeDirectX();

    // Separated update functions
    void ProcessInput();
    void UpdateGame(float deltaTime);
    void RenderFrame(float interpolation);

    HWND m_hwnd;
    int m_screenWidth;
    int m_screenHeight;
    bool m_isRunning;

    // DirectX 11 components
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_deviceContext;
    IDXGISwapChain* m_swapChain;
    ID3D11RenderTargetView* m_renderTargetView;
    ID3D11DepthStencilView* m_depthStencilView;
    ID3D11Texture2D* m_depthStencilBuffer;
    ID3D11DepthStencilState* m_depthStencilState;
    ID3D11RasterizerState* m_rasterizerState;

    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<GameLoop> m_gameLoop;

    // Input state tracking
    bool m_keys[256];
    bool m_mouseButtons[3];
    int m_mouseX, m_mouseY;
    int m_lastMouseX, m_lastMouseY;
    bool m_isMouseCaptured;

    // Game objects (for interpolation example)
    float m_cubeRotationAngle;
    float m_triangleRotationAngle;
};

// Global engine instance
extern Engine* g_Engine;

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam);
