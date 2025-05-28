#include "Engine.h"
#include "Camera.h"
#include "Renderer.h"
#include "GameLoop.h"
#include <iostream>
#include <cstring>
#include <chrono>

Engine* g_Engine = nullptr;

Engine::Engine()
    : m_hwnd(nullptr)
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_isRunning(false)
    , m_device(nullptr)
    , m_deviceContext(nullptr)
    , m_swapChain(nullptr)
    , m_renderTargetView(nullptr)
    , m_depthStencilView(nullptr)
    , m_depthStencilBuffer(nullptr)
    , m_depthStencilState(nullptr)
    , m_rasterizerState(nullptr)
    , m_mouseX(0)
    , m_mouseY(0)
    , m_lastMouseX(0)
    , m_lastMouseY(0)
    , m_isMouseCaptured(false)
    , m_cubeRotationAngle(0.0f)
    , m_triangleRotationAngle(0.0f)
{
    g_Engine = this;

    // Initialize input arrays
    std::memset(m_keys, false, sizeof(m_keys));
    std::memset(m_mouseButtons, false, sizeof(m_mouseButtons));
}

Engine::~Engine()
{
    Shutdown();
}

bool Engine::Initialize(HINSTANCE hInstance, int width, int height, const std::wstring& title)
{
    m_screenWidth = width;
    m_screenHeight = height;

    // Initialize window
    if (!InitializeWindow(hInstance, width, height, title))
    {
        std::wcout << L"Failed to initialize window" << std::endl;
        return false;
    }

    // Initialize DirectX
    if (!InitializeDirectX())
    {
        std::wcout << L"Failed to initialize DirectX" << std::endl;
        return false;
    }

    // Initialize camera
    m_camera = std::make_unique<Camera>();
    m_camera->Initialize(static_cast<float>(width), static_cast<float>(height));

    // Initialize renderer
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->Initialize(m_device, m_deviceContext))
    {
        std::wcout << L"Failed to initialize renderer" << std::endl;
        return false;
    }

    m_isRunning = true;
    return true;
}

bool Engine::InitializeWindow(HINSTANCE hInstance, int width, int height, const std::wstring& title)
{
    WNDCLASSEX wc;

    // Setup window class
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = L"DX11Engine";
    wc.cbSize = sizeof(WNDCLASSEX);

    // Register window class
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    // Calculate window size for desired client area
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Create window
    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        L"DX11Engine",
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!m_hwnd)
    {
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    SetFocus(m_hwnd);
    UpdateWindow(m_hwnd);

    return true;
}

bool Engine::InitializeDirectX()
{
    HRESULT result;
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    D3D_FEATURE_LEVEL featureLevel;

    // Initialize swap chain description
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = m_screenWidth;
    swapChainDesc.BufferDesc.Height = m_screenHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;

    // Create device, device context and swap chain
    result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &m_swapChain,
        &m_device,
        &featureLevel,
        &m_deviceContext
    );

    if (FAILED(result))
    {
        return false;
    }

    // Get back buffer
    ID3D11Texture2D* backBufferPtr;
    result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
    if (FAILED(result))
    {
        return false;
    }

    // Create render target view
    result = m_device->CreateRenderTargetView(backBufferPtr, nullptr, &m_renderTargetView);
    backBufferPtr->Release();
    backBufferPtr = nullptr;

    if (FAILED(result))
    {
        return false;
    }

    // Setup depth buffer description
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
    depthBufferDesc.Width = m_screenWidth;
    depthBufferDesc.Height = m_screenHeight;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    // Create depth buffer
    result = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Setup depth stencil description
    D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
    ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = 0xFF;
    depthStencilDesc.StencilWriteMask = 0xFF;
    depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Create depth stencil state
    result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
    if (FAILED(result))
    {
        return false;
    }

    // Set depth stencil state
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

    // Initialize depth stencil view description
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    // Create depth stencil view
    result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
    if (FAILED(result))
    {
        return false;
    }

    // Bind render target view and depth stencil buffer to output render pipeline
    m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

    // Setup rasterizer description
    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.AntialiasedLineEnable = false;
    rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = true;
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = false;
    rasterDesc.ScissorEnable = false;
    rasterDesc.SlopeScaledDepthBias = 0.0f;

    // Create rasterizer state
    result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterizerState);
    if (FAILED(result))
    {
        return false;
    }

    // Set rasterizer state
    m_deviceContext->RSSetState(m_rasterizerState);

    // Setup viewport
    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<float>(m_screenWidth);
    viewport.Height = static_cast<float>(m_screenHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    // Create viewport
    m_deviceContext->RSSetViewports(1, &viewport);

    return true;
}

void Engine::Run()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));

    // Initialize timing for manual game loop
    auto lastTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> accumulator(0.0);
    const std::chrono::duration<double> fixedTimestep(1.0 / 60.0); // 60 UPS

    while (m_isRunning)
    {
        // Handle Windows messages (non-blocking)
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            m_isRunning = false;
            break;
        }

        // Manual game loop implementation
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto frameTime = currentTime - lastTime;
        lastTime = currentTime;

        accumulator += frameTime;

        // Clamp frame time to prevent spiral of death
        if (frameTime > std::chrono::duration<double>(0.05)) // Max 50ms per frame
        {
            accumulator = fixedTimestep;
        }

        // Process input every frame
        ProcessInput();

        // Fixed timestep updates
        while (accumulator >= fixedTimestep)
        {
            UpdateGame(static_cast<float>(fixedTimestep.count()));
            accumulator -= fixedTimestep;
        }

        // Render with interpolation
        float interpolation = static_cast<float>(accumulator / fixedTimestep);
        RenderFrame(interpolation);
    }
}

void Engine::ProcessInput()
{
    // Update key states
    for (int i = 0; i < 256; ++i)
    {
        m_keys[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
    }

    // Update mouse button states
    m_mouseButtons[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    m_mouseButtons[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    m_mouseButtons[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    // Get current mouse position
    POINT mousePos;
    GetCursorPos(&mousePos);
    ScreenToClient(m_hwnd, &mousePos);
    m_mouseX = mousePos.x;
    m_mouseY = mousePos.y;
}

void Engine::UpdateGame(float deltaTime)
{
    // Update camera based on input state
    if (m_camera)
    {
        m_camera->Update(deltaTime);

        // Camera movement
        if (m_keys['W'])
            m_camera->MoveForward(deltaTime);
        if (m_keys['S'])
            m_camera->MoveBackward(deltaTime);
        if (m_keys['A'])
            m_camera->MoveLeft(deltaTime);
        if (m_keys['D'])
            m_camera->MoveRight(deltaTime);
        if (m_keys['Q'])
            m_camera->MoveUp(deltaTime);
        if (m_keys['E'])
            m_camera->MoveDown(deltaTime);

        // Mouse look
        if (m_isMouseCaptured)
        {
            int deltaX = m_mouseX - m_lastMouseX;
            int deltaY = m_mouseY - m_lastMouseY;

            // Different sensitivity for different camera modes
            float sensitivity = (m_camera->GetCameraMode() == CameraMode::ThirdPerson) ? 0.01f : 0.005f;
            m_camera->Rotate(static_cast<float>(deltaX) * sensitivity, static_cast<float>(deltaY) * sensitivity);

            m_lastMouseX = m_mouseX;
            m_lastMouseY = m_mouseY;
        }
    }

    // Update game objects (fixed timestep)
    m_triangleRotationAngle += 1.0f * deltaTime; // 1 radian per second
    m_cubeRotationAngle += 2.0f * deltaTime;     // 2 radians per second

    // Wrap angles
    if (m_triangleRotationAngle > 6.28318f) // 2π
        m_triangleRotationAngle -= 6.28318f;
    if (m_cubeRotationAngle > 6.28318f)
        m_cubeRotationAngle -= 6.28318f;

    // Camera mode switching with C key (simple toggle without debouncing for now)
    static bool cKeyWasPressed = false;
    if (m_keys['C'] && !cKeyWasPressed)
    {
        if (m_camera->GetCameraMode() == CameraMode::FirstPerson)
            m_camera->SetCameraMode(CameraMode::ThirdPerson);
        else
            m_camera->SetCameraMode(CameraMode::FirstPerson);
        cKeyWasPressed = true;
    }
    else if (!m_keys['C'])
    {
        cKeyWasPressed = false;
    }

    // Check for exit condition
    if (m_keys[VK_ESCAPE])
    {
        m_isRunning = false;
        PostQuitMessage(0);
    }
}

void Engine::RenderFrame(float interpolation)
{
    // Clear render target and depth buffer
    float clearColor[4] = { 0.2f, 0.3f, 0.4f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_renderTargetView, clearColor);
    m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    // Render scene with interpolation
    if (m_renderer && m_camera)
    {
        // Pass interpolated rotation angles to renderer
        m_renderer->SetRotationAngles(m_triangleRotationAngle, m_cubeRotationAngle, interpolation);

        // Use different render method based on camera mode
        if (m_camera->GetCameraMode() == CameraMode::ThirdPerson)
        {
            m_renderer->RenderWithTarget(m_camera->GetViewMatrix(), m_camera->GetProjectionMatrix(), m_camera->GetTarget());
        }
        else
        {
            m_renderer->Render(m_camera->GetViewMatrix(), m_camera->GetProjectionMatrix());
        }
    }

    // Present back buffer
    m_swapChain->Present(1, 0); // VSync enabled
}

void Engine::SetTargetUPS(int updatesPerSecond)
{
    // This would be used if we had the GameLoop class active
    // For now, the fixed timestep is hardcoded to 60 UPS
}

void Engine::SetTargetFPS(int framesPerSecond)
{
    // This would be used if we had frame rate limiting
    // For now, we use VSync
}

void Engine::SetVSyncEnabled(bool enabled)
{
    // VSync is controlled in Present() call
}

int Engine::GetCurrentFPS() const
{
    return m_gameLoop ? m_gameLoop->GetCurrentFPS() : 0;
}

int Engine::GetCurrentUPS() const
{
    return m_gameLoop ? m_gameLoop->GetCurrentUPS() : 0;
}

double Engine::GetFrameTime() const
{
    return m_gameLoop ? m_gameLoop->GetAverageFrameTime() : 0.0;
}

double Engine::GetUpdateTime() const
{
    return m_gameLoop ? m_gameLoop->GetAverageUpdateTime() : 0.0;
}

void Engine::Shutdown()
{
    // Release DirectX objects
    if (m_rasterizerState)
    {
        m_rasterizerState->Release();
        m_rasterizerState = nullptr;
    }

    if (m_depthStencilView)
    {
        m_depthStencilView->Release();
        m_depthStencilView = nullptr;
    }

    if (m_depthStencilState)
    {
        m_depthStencilState->Release();
        m_depthStencilState = nullptr;
    }

    if (m_depthStencilBuffer)
    {
        m_depthStencilBuffer->Release();
        m_depthStencilBuffer = nullptr;
    }

    if (m_renderTargetView)
    {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }

    if (m_deviceContext)
    {
        m_deviceContext->Release();
        m_deviceContext = nullptr;
    }

    if (m_device)
    {
        m_device->Release();
        m_device = nullptr;
    }

    if (m_swapChain)
    {
        m_swapChain->Release();
        m_swapChain = nullptr;
    }

    // Destroy window
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }

    // Unregister class
    UnregisterClass(L"DX11Engine", GetModuleHandle(nullptr));
}

LRESULT Engine::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case WM_DESTROY:
    case WM_CLOSE:
        m_isRunning = false;
        PostQuitMessage(0);
        return 0;

    case WM_LBUTTONDOWN:
        m_isMouseCaptured = true;
        SetCapture(hwnd);
        {
            POINT mousePos;
            GetCursorPos(&mousePos);
            ScreenToClient(hwnd, &mousePos);
            m_lastMouseX = mousePos.x;
            m_lastMouseY = mousePos.y;
        }
        return 0;

    case WM_LBUTTONUP:
        m_isMouseCaptured = false;
        ReleaseCapture();
        return 0;

    case WM_KEYDOWN:
        // Key states are handled in ProcessInput()
        return 0;

    case WM_KEYUP:
        // Key states are handled in ProcessInput()
        return 0;

    case WM_MOUSEMOVE:
        // Mouse movement is handled in ProcessInput() and UpdateGame()
        return 0;

    case WM_MOUSEWHEEL:
        // Handle mouse wheel for zoom in third person mode
        if (m_camera && m_camera->GetCameraMode() == CameraMode::ThirdPerson)
        {
            int wheelDelta = GET_WHEEL_DELTA_WPARAM(wparam);
            float zoomDelta = static_cast<float>(wheelDelta) / 120.0f; // Standard wheel delta is 120
            m_camera->ZoomToTarget(zoomDelta);
        }
        return 0;

    default:
        return DefWindowProc(hwnd, umsg, wparam, lparam);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
    if (g_Engine)
    {
        return g_Engine->MessageHandler(hwnd, umessage, wparam, lparam);
    }
    else
    {
        return DefWindowProc(hwnd, umessage, wparam, lparam);
    }
}
