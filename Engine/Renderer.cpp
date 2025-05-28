#include "Renderer.h"
#include <d3dcompiler.h>
#include <iostream>

Renderer::Renderer()
    : m_device(nullptr)
    , m_deviceContext(nullptr)
    , m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_layout(nullptr)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_constantBuffer(nullptr)
    , m_cubeVertexBuffer(nullptr)
    , m_cubeIndexBuffer(nullptr)
    , m_vertexCount(0)
    , m_indexCount(0)
    , m_cubeVertexCount(0)
    , m_cubeIndexCount(0)
    , m_interpolatedTriangleAngle(0.0f)
    , m_interpolatedCubeAngle(0.0f)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    m_device = device;
    m_deviceContext = deviceContext;

    // Initialize shaders
    if (!InitializeShaders(device))
    {
        return false;
    }

    // Initialize buffers
    if (!InitializeBuffers(device))
    {
        return false;
    }

    return true;
}

bool Renderer::InitializeShaders(ID3D11Device* device)
{
    HRESULT result;
    ID3D10Blob* errorMessage = nullptr;
    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;

    // Vertex shader source
    const char* vsSource = R"(
        cbuffer ConstantBuffer : register(b0)
        {
            matrix worldMatrix;
            matrix viewMatrix;
            matrix projectionMatrix;
        }

        struct VertexInputType
        {
            float4 position : POSITION;
            float4 color : COLOR;
        };

        struct PixelInputType
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PixelInputType main(VertexInputType input)
        {
            PixelInputType output;

            input.position.w = 1.0f;

            output.position = mul(input.position, worldMatrix);
            output.position = mul(output.position, viewMatrix);
            output.position = mul(output.position, projectionMatrix);

            output.color = input.color;

            return output;
        }
    )";

    // Pixel shader source
    const char* psSource = R"(
        struct PixelInputType
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(PixelInputType input) : SV_TARGET
        {
            return input.color;
        }
    )";

    // Compile vertex shader
    result = D3DCompile(vsSource, strlen(vsSource), nullptr, nullptr, nullptr, "main", "vs_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);

    if (FAILED(result))
    {
        if (errorMessage)
        {
            char* compileErrors = (char*)(errorMessage->GetBufferPointer());
            std::cout << "Vertex shader compile error: " << compileErrors << std::endl;
            errorMessage->Release();
        }
        return false;
    }

    // Compile pixel shader
    result = D3DCompile(psSource, strlen(psSource), nullptr, nullptr, nullptr, "main", "ps_5_0",
        D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);

    if (FAILED(result))
    {
        if (errorMessage)
        {
            char* compileErrors = (char*)(errorMessage->GetBufferPointer());
            std::cout << "Pixel shader compile error: " << compileErrors << std::endl;
            errorMessage->Release();
        }
        return false;
    }

    // Create vertex shader
    result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(),
        vertexShaderBuffer->GetBufferSize(), nullptr, &m_vertexShader);
    if (FAILED(result))
    {
        return false;
    }

    // Create pixel shader
    result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),
        pixelShaderBuffer->GetBufferSize(), nullptr, &m_pixelShader);
    if (FAILED(result))
    {
        return false;
    }

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "COLOR";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

    result = device->CreateInputLayout(polygonLayout, numElements,
        vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_layout);

    vertexShaderBuffer->Release();
    pixelShaderBuffer->Release();

    if (FAILED(result))
    {
        return false;
    }

    return true;
}

bool Renderer::InitializeBuffers(ID3D11Device* device)
{
    HRESULT result;

    // Triangle vertices
    Vertex triangleVertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
    };

    unsigned long triangleIndices[] = { 0, 1, 2 };

    m_vertexCount = 3;
    m_indexCount = 3;

    // Create triangle vertex buffer
    D3D11_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(Vertex) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
    vertexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vertexData;
    vertexData.pSysMem = triangleVertices;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Create triangle index buffer
    D3D11_BUFFER_DESC indexBufferDesc;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
    indexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA indexData;
    indexData.pSysMem = triangleIndices;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Cube vertices
    Vertex cubeVertices[] = {
        // Front face
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
        { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) },

        // Back face
        { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f) }
    };

    unsigned long cubeIndices[] = {
        // Front face
        0, 1, 2,
        0, 2, 3,

        // Back face
        4, 6, 5,
        4, 7, 6,

        // Left face
        4, 1, 0,
        4, 7, 1,

        // Right face
        3, 2, 6,
        3, 6, 5,

        // Top face
        1, 7, 6,
        1, 6, 2,

        // Bottom face
        4, 0, 3,
        4, 3, 5
    };

    m_cubeVertexCount = 8;
    m_cubeIndexCount = 36;

    // Create cube vertex buffer
    D3D11_BUFFER_DESC cubeVertexBufferDesc;
    cubeVertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    cubeVertexBufferDesc.ByteWidth = sizeof(Vertex) * m_cubeVertexCount;
    cubeVertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    cubeVertexBufferDesc.CPUAccessFlags = 0;
    cubeVertexBufferDesc.MiscFlags = 0;
    cubeVertexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA cubeVertexData;
    cubeVertexData.pSysMem = cubeVertices;
    cubeVertexData.SysMemPitch = 0;
    cubeVertexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&cubeVertexBufferDesc, &cubeVertexData, &m_cubeVertexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Create cube index buffer
    D3D11_BUFFER_DESC cubeIndexBufferDesc;
    cubeIndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    cubeIndexBufferDesc.ByteWidth = sizeof(unsigned long) * m_cubeIndexCount;
    cubeIndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    cubeIndexBufferDesc.CPUAccessFlags = 0;
    cubeIndexBufferDesc.MiscFlags = 0;
    cubeIndexBufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA cubeIndexData;
    cubeIndexData.pSysMem = cubeIndices;
    cubeIndexData.SysMemPitch = 0;
    cubeIndexData.SysMemSlicePitch = 0;

    result = device->CreateBuffer(&cubeIndexBufferDesc, &cubeIndexData, &m_cubeIndexBuffer);
    if (FAILED(result))
    {
        return false;
    }

    // Create constant buffer
    D3D11_BUFFER_DESC constantBufferDesc;
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.ByteWidth = sizeof(ConstantBuffer);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantBufferDesc.MiscFlags = 0;
    constantBufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&constantBufferDesc, nullptr, &m_constantBuffer);
    if (FAILED(result))
    {
        return false;
    }

    return true;
}

void Renderer::SetRotationAngles(float triangleAngle, float cubeAngle, float interpolation)
{
    // Store interpolated angles for smooth rendering
    m_interpolatedTriangleAngle = triangleAngle;
    m_interpolatedCubeAngle = cubeAngle;

    // Note: interpolation factor could be used here for even smoother motion
    // For example: m_interpolatedTriangleAngle = lastAngle + (triangleAngle - lastAngle) * interpolation;
}

void Renderer::Render(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    // Set input assembler
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;

    m_deviceContext->IASetInputLayout(m_layout);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders
    m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);

    // Render triangle
    RenderTriangle(viewMatrix, projectionMatrix);

    // Render cube
    RenderCube(viewMatrix, projectionMatrix);
}

void Renderer::RenderWithTarget(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMFLOAT3& targetPosition)
{
    // Set common render state
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;

    m_deviceContext->IASetInputLayout(m_layout);
    m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders
    m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
    m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);

    // Render triangle
    RenderTriangle(viewMatrix, projectionMatrix);

    // Render cube
    RenderCube(viewMatrix, projectionMatrix);

    // Render target (character representation)
    RenderTarget(viewMatrix, projectionMatrix, targetPosition);
}

void Renderer::RenderTriangle(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    // Set triangle buffers
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Create world matrix for triangle (position to the left)
    XMMATRIX worldMatrix = XMMatrixTranslation(-3.0f, 0.0f, 0.0f);
    worldMatrix = XMMatrixMultiply(XMMatrixRotationY(m_interpolatedTriangleAngle), worldMatrix);

    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ConstantBuffer* dataPtr;

    HRESULT result = m_deviceContext->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        dataPtr = (ConstantBuffer*)mappedResource.pData;
        dataPtr->world = XMMatrixTranspose(worldMatrix);
        dataPtr->view = XMMatrixTranspose(viewMatrix);
        dataPtr->projection = XMMatrixTranspose(projectionMatrix);

        m_deviceContext->Unmap(m_constantBuffer, 0);
    }

    // Set constant buffer
    m_deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);

    // Draw triangle
    m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderCube(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix)
{
    // Set cube buffers
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &m_cubeVertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(m_cubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Create world matrix for cube (position to the right)
    XMMATRIX worldMatrix = XMMatrixTranslation(3.0f, 0.0f, 0.0f);
    worldMatrix = XMMatrixMultiply(XMMatrixRotationRollPitchYaw(m_interpolatedCubeAngle, m_interpolatedCubeAngle, 0.0f), worldMatrix);

    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ConstantBuffer* dataPtr;

    HRESULT result = m_deviceContext->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        dataPtr = (ConstantBuffer*)mappedResource.pData;
        dataPtr->world = XMMatrixTranspose(worldMatrix);
        dataPtr->view = XMMatrixTranspose(viewMatrix);
        dataPtr->projection = XMMatrixTranspose(projectionMatrix);

        m_deviceContext->Unmap(m_constantBuffer, 0);
    }

    // Set constant buffer
    m_deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);

    // Draw cube
    m_deviceContext->DrawIndexed(m_cubeIndexCount, 0, 0);
}

void Renderer::RenderTarget(const XMMATRIX& viewMatrix, const XMMATRIX& projectionMatrix, const XMFLOAT3& position)
{
    // Set cube buffers (reuse cube geometry for target)
    unsigned int stride = sizeof(Vertex);
    unsigned int offset = 0;
    m_deviceContext->IASetVertexBuffers(0, 1, &m_cubeVertexBuffer, &stride, &offset);
    m_deviceContext->IASetIndexBuffer(m_cubeIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Create world matrix for target (small cube at target position)
    XMMATRIX scaleMatrix = XMMatrixScaling(0.3f, 0.3f, 0.3f); // Small cube
    XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX worldMatrix = XMMatrixMultiply(scaleMatrix, translationMatrix);

    // Update constant buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    ConstantBuffer* dataPtr;

    HRESULT result = m_deviceContext->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(result))
    {
        dataPtr = (ConstantBuffer*)mappedResource.pData;
        dataPtr->world = XMMatrixTranspose(worldMatrix);
        dataPtr->view = XMMatrixTranspose(viewMatrix);
        dataPtr->projection = XMMatrixTranspose(projectionMatrix);

        m_deviceContext->Unmap(m_constantBuffer, 0);
    }

    // Set constant buffer
    m_deviceContext->VSSetConstantBuffers(0, 1, &m_constantBuffer);

    // Draw target cube
    m_deviceContext->DrawIndexed(m_cubeIndexCount, 0, 0);
}

void Renderer::Shutdown()
{
    // Release buffers
    if (m_cubeIndexBuffer)
    {
        m_cubeIndexBuffer->Release();
        m_cubeIndexBuffer = nullptr;
    }

    if (m_cubeVertexBuffer)
    {
        m_cubeVertexBuffer->Release();
        m_cubeVertexBuffer = nullptr;
    }

    if (m_constantBuffer)
    {
        m_constantBuffer->Release();
        m_constantBuffer = nullptr;
    }

    if (m_indexBuffer)
    {
        m_indexBuffer->Release();
        m_indexBuffer = nullptr;
    }

    if (m_vertexBuffer)
    {
        m_vertexBuffer->Release();
        m_vertexBuffer = nullptr;
    }

    if (m_layout)
    {
        m_layout->Release();
        m_layout = nullptr;
    }

    if (m_pixelShader)
    {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }

    if (m_vertexShader)
    {
        m_vertexShader->Release();
        m_vertexShader = nullptr;
    }
}
