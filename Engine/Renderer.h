#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>

using namespace DirectX;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct ConstantBuffer
{
    XMMATRIX world;
    XMMATRIX view;
    XMMATRIX projection;
};

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
    void Render(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    void RenderWithTarget(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMFLOAT3& targetPosition);
    void SetRotationAngles(float triangleAngle, float cubeAngle, float interpolation);
    void Shutdown();

private:
    bool InitializeShaders(ID3D11Device* device);
    bool InitializeBuffers(ID3D11Device* device);
    void RenderTriangle(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    void RenderCube(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix);
    void RenderTarget(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMFLOAT3& position);

    ID3D11Device* m_device;
    ID3D11DeviceContext* m_deviceContext;

    // Shaders
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;

    // Buffers
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    ID3D11Buffer* m_constantBuffer;

    // Cube buffers
    ID3D11Buffer* m_cubeVertexBuffer;
    ID3D11Buffer* m_cubeIndexBuffer;

    int m_vertexCount;
    int m_indexCount;
    int m_cubeVertexCount;
    int m_cubeIndexCount;

    // Interpolated rotation angles
    float m_interpolatedTriangleAngle;
    float m_interpolatedCubeAngle;
};
