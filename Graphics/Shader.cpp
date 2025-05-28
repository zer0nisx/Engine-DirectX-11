#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

Shader::Shader()
    : m_type(ShaderType::Vertex)
    , m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_geometryShader(nullptr)
    , m_hullShader(nullptr)
    , m_domainShader(nullptr)
    , m_computeShader(nullptr)
    , m_inputLayout(nullptr)
    , m_shaderBlob(nullptr)
    , m_isCompiled(false)
{
}

Shader::~Shader()
{
    Shutdown();
}

bool Shader::CompileFromString(ID3D11Device* device,
                              const std::string& shaderCode,
                              const std::string& entryPoint,
                              ShaderType type,
                              const std::vector<InputLayoutElement>& layoutElements)
{
    if (!device || shaderCode.empty() || entryPoint.empty())
    {
        return false;
    }

    Shutdown(); // Clean up existing resources

    m_type = type;
    m_entryPoint = entryPoint;

    // Compile shader
    ID3DBlob* errorBlob = nullptr;
    std::string profile = GetShaderProfile(type);

    HRESULT hr = D3DCompile(
        shaderCode.c_str(),
        shaderCode.length(),
        nullptr,                // Source name
        nullptr,                // Defines
        nullptr,                // Include
        entryPoint.c_str(),
        profile.c_str(),
        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        &m_shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::cout << "Shader compilation error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
            errorBlob->Release();
        }
        return false;
    }

    if (errorBlob)
    {
        errorBlob->Release();
    }

    // Create the appropriate shader object
    switch (type)
    {
    case ShaderType::Vertex:
        hr = device->CreateVertexShader(
            m_shaderBlob->GetBufferPointer(),
            m_shaderBlob->GetBufferSize(),
            nullptr,
            &m_vertexShader
        );

        // Create input layout for vertex shader
        if (SUCCEEDED(hr) && !layoutElements.empty())
        {
            if (!CreateInputLayout(device, layoutElements))
            {
                std::cout << "Failed to create input layout for vertex shader" << std::endl;
                return false;
            }
        }
        break;

    case ShaderType::Pixel:
        hr = device->CreatePixelShader(
            m_shaderBlob->GetBufferPointer(),
            m_shaderBlob->GetBufferSize(),
            nullptr,
            &m_pixelShader
        );
        break;

    // Add other shader types as needed
    default:
        std::cout << "Unsupported shader type" << std::endl;
        return false;
    }

    if (FAILED(hr))
    {
        std::cout << "Failed to create shader object" << std::endl;
        return false;
    }

    m_isCompiled = true;
    return true;
}

bool Shader::CompileFromFile(ID3D11Device* device,
                            const std::string& filepath,
                            const std::string& entryPoint,
                            ShaderType type,
                            const std::vector<InputLayoutElement>& layoutElements)
{
    // Read file
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cout << "Failed to open shader file: " << filepath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    m_filepath = filepath;
    return CompileFromString(device, buffer.str(), entryPoint, type, layoutElements);
}

void Shader::Bind(ID3D11DeviceContext* context)
{
    if (!context || !m_isCompiled)
    {
        return;
    }

    switch (m_type)
    {
    case ShaderType::Vertex:
        context->VSSetShader(m_vertexShader, nullptr, 0);
        if (m_inputLayout)
        {
            context->IASetInputLayout(m_inputLayout);
        }
        break;

    case ShaderType::Pixel:
        context->PSSetShader(m_pixelShader, nullptr, 0);
        break;

    // Add other shader types as needed
    }
}

void Shader::Unbind(ID3D11DeviceContext* context)
{
    if (!context)
    {
        return;
    }

    switch (m_type)
    {
    case ShaderType::Vertex:
        context->VSSetShader(nullptr, nullptr, 0);
        context->IASetInputLayout(nullptr);
        break;

    case ShaderType::Pixel:
        context->PSSetShader(nullptr, nullptr, 0);
        break;

    // Add other shader types as needed
    }
}

void Shader::Shutdown()
{
    ReleaseShaderResources();

    if (m_inputLayout)
    {
        m_inputLayout->Release();
        m_inputLayout = nullptr;
    }

    if (m_shaderBlob)
    {
        m_shaderBlob->Release();
        m_shaderBlob = nullptr;
    }

    m_isCompiled = false;
    m_entryPoint.clear();
    m_filepath.clear();
}

bool Shader::IsValid() const
{
    return m_isCompiled && (
        m_vertexShader != nullptr ||
        m_pixelShader != nullptr ||
        m_geometryShader != nullptr ||
        m_hullShader != nullptr ||
        m_domainShader != nullptr ||
        m_computeShader != nullptr
    );
}

void Shader::PrintShaderInfo() const
{
    std::cout << "Shader Info:" << std::endl;
    std::cout << "  Type: ";

    switch (m_type)
    {
    case ShaderType::Vertex:   std::cout << "Vertex"; break;
    case ShaderType::Pixel:    std::cout << "Pixel"; break;
    case ShaderType::Geometry: std::cout << "Geometry"; break;
    case ShaderType::Hull:     std::cout << "Hull"; break;
    case ShaderType::Domain:   std::cout << "Domain"; break;
    case ShaderType::Compute:  std::cout << "Compute"; break;
    }

    std::cout << std::endl;
    std::cout << "  Entry Point: " << m_entryPoint << std::endl;
    std::cout << "  File Path: " << m_filepath << std::endl;
    std::cout << "  Is Compiled: " << (m_isCompiled ? "Yes" : "No") << std::endl;
    std::cout << "  Has Input Layout: " << (m_inputLayout ? "Yes" : "No") << std::endl;
}

bool Shader::CreateInputLayout(ID3D11Device* device, const std::vector<InputLayoutElement>& layoutElements)
{
    if (!device || layoutElements.empty() || !m_shaderBlob)
    {
        return false;
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> d3dElements;
    d3dElements.reserve(layoutElements.size());

    for (const auto& element : layoutElements)
    {
        D3D11_INPUT_ELEMENT_DESC d3dElement = {};
        d3dElement.SemanticName = element.semanticName.c_str();
        d3dElement.SemanticIndex = element.semanticIndex;
        d3dElement.Format = element.format;
        d3dElement.InputSlot = element.inputSlot;
        d3dElement.AlignedByteOffset = element.alignedByteOffset;
        d3dElement.InputSlotClass = element.inputSlotClass;
        d3dElement.InstanceDataStepRate = element.instanceDataStepRate;

        d3dElements.push_back(d3dElement);
    }

    HRESULT hr = device->CreateInputLayout(
        d3dElements.data(),
        static_cast<UINT>(d3dElements.size()),
        m_shaderBlob->GetBufferPointer(),
        m_shaderBlob->GetBufferSize(),
        &m_inputLayout
    );

    return SUCCEEDED(hr);
}

std::string Shader::GetShaderProfile(ShaderType type) const
{
    switch (type)
    {
    case ShaderType::Vertex:   return "vs_5_0";
    case ShaderType::Pixel:    return "ps_5_0";
    case ShaderType::Geometry: return "gs_5_0";
    case ShaderType::Hull:     return "hs_5_0";
    case ShaderType::Domain:   return "ds_5_0";
    case ShaderType::Compute:  return "cs_5_0";
    default:                   return "vs_5_0";
    }
}

void Shader::ReleaseShaderResources()
{
    if (m_vertexShader)
    {
        m_vertexShader->Release();
        m_vertexShader = nullptr;
    }

    if (m_pixelShader)
    {
        m_pixelShader->Release();
        m_pixelShader = nullptr;
    }

    if (m_geometryShader)
    {
        m_geometryShader->Release();
        m_geometryShader = nullptr;
    }

    if (m_hullShader)
    {
        m_hullShader->Release();
        m_hullShader = nullptr;
    }

    if (m_domainShader)
    {
        m_domainShader->Release();
        m_domainShader = nullptr;
    }

    if (m_computeShader)
    {
        m_computeShader->Release();
        m_computeShader = nullptr;
    }
}

// ShaderProgram implementation
ShaderProgram::ShaderProgram()
    : m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
{
}

ShaderProgram::~ShaderProgram()
{
    Shutdown();
}

void ShaderProgram::SetVertexShader(std::shared_ptr<Shader> vertexShader)
{
    m_vertexShader = vertexShader;
}

void ShaderProgram::SetPixelShader(std::shared_ptr<Shader> pixelShader)
{
    m_pixelShader = pixelShader;
}

void ShaderProgram::Bind(ID3D11DeviceContext* context)
{
    if (!context)
    {
        return;
    }

    if (m_vertexShader)
    {
        m_vertexShader->Bind(context);
    }

    if (m_pixelShader)
    {
        m_pixelShader->Bind(context);
    }
}

void ShaderProgram::Unbind(ID3D11DeviceContext* context)
{
    if (!context)
    {
        return;
    }

    if (m_vertexShader)
    {
        m_vertexShader->Unbind(context);
    }

    if (m_pixelShader)
    {
        m_pixelShader->Unbind(context);
    }
}

bool ShaderProgram::IsValid() const
{
    return (m_vertexShader && m_vertexShader->IsValid()) &&
           (m_pixelShader && m_pixelShader->IsValid());
}

void ShaderProgram::Shutdown()
{
    m_vertexShader.reset();
    m_pixelShader.reset();
}

// Utility functions implementation
namespace ShaderUtils
{
    std::vector<InputLayoutElement> CreateBasicInputLayout()
    {
        std::vector<InputLayoutElement> layout;

        // Position
        layout.push_back({
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        });

        // Normal
        layout.push_back({
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        });

        // Texture coordinates
        layout.push_back({
            "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        });

        return layout;
    }

    std::vector<InputLayoutElement> CreatePositionColorLayout()
    {
        std::vector<InputLayoutElement> layout;

        // Position
        layout.push_back({
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        });

        // Color
        layout.push_back({
            "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
            D3D11_INPUT_PER_VERTEX_DATA, 0
        });

        return layout;
    }

    std::shared_ptr<Shader> CreateVertexShaderFromString(ID3D11Device* device,
                                                        const std::string& shaderCode,
                                                        const std::vector<InputLayoutElement>& layout)
    {
        auto shader = std::make_shared<Shader>();
        if (shader->CompileFromString(device, shaderCode, "main", ShaderType::Vertex, layout))
        {
            return shader;
        }
        return nullptr;
    }

    std::shared_ptr<Shader> CreatePixelShaderFromString(ID3D11Device* device,
                                                       const std::string& shaderCode)
    {
        auto shader = std::make_shared<Shader>();
        if (shader->CompileFromString(device, shaderCode, "main", ShaderType::Pixel))
        {
            return shader;
        }
        return nullptr;
    }

    // Default shader source code
    const char* DEFAULT_VERTEX_SHADER = R"(
        cbuffer MatrixBuffer : register(b0)
        {
            matrix worldMatrix;
            matrix viewMatrix;
            matrix projectionMatrix;
        };

        struct VertexInput
        {
            float4 position : POSITION;
            float4 color : COLOR;
        };

        struct PixelInput
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        PixelInput main(VertexInput input)
        {
            PixelInput output;

            input.position.w = 1.0f;

            output.position = mul(input.position, worldMatrix);
            output.position = mul(output.position, viewMatrix);
            output.position = mul(output.position, projectionMatrix);

            output.color = input.color;

            return output;
        }
    )";

    const char* DEFAULT_PIXEL_SHADER = R"(
        struct PixelInput
        {
            float4 position : SV_POSITION;
            float4 color : COLOR;
        };

        float4 main(PixelInput input) : SV_TARGET
        {
            return input.color;
        }
    )";

    const char* MATERIAL_VERTEX_SHADER = R"(
        cbuffer MatrixBuffer : register(b0)
        {
            matrix worldMatrix;
            matrix viewMatrix;
            matrix projectionMatrix;
        };

        cbuffer MaterialBuffer : register(b1)
        {
            float4 diffuseColor;
            float4 specularColor;
            float4 emissiveColor;
            float shininess;
            float transparency;
            float reflectivity;
            float padding;
        };

        struct VertexInput
        {
            float4 position : POSITION;
            float3 normal : NORMAL;
            float2 texCoord : TEXCOORD0;
        };

        struct PixelInput
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL;
            float2 texCoord : TEXCOORD0;
            float3 worldPos : TEXCOORD1;
        };

        PixelInput main(VertexInput input)
        {
            PixelInput output;

            input.position.w = 1.0f;

            output.worldPos = mul(input.position, worldMatrix).xyz;
            output.position = mul(input.position, worldMatrix);
            output.position = mul(output.position, viewMatrix);
            output.position = mul(output.position, projectionMatrix);

            output.normal = mul(input.normal, (float3x3)worldMatrix);
            output.texCoord = input.texCoord;

            return output;
        }
    )";

    const char* MATERIAL_PIXEL_SHADER = R"(
        Texture2D diffuseTexture : register(t0);
        Texture2D specularTexture : register(t1);
        SamplerState textureSampler : register(s0);

        cbuffer MaterialBuffer : register(b1)
        {
            float4 diffuseColor;
            float4 specularColor;
            float4 emissiveColor;
            float shininess;
            float transparency;
            float reflectivity;
            float padding;
        };

        struct PixelInput
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL;
            float2 texCoord : TEXCOORD0;
            float3 worldPos : TEXCOORD1;
        };

        float4 main(PixelInput input) : SV_TARGET
        {
            float4 textureColor = diffuseTexture.Sample(textureSampler, input.texCoord);
            float4 finalColor = diffuseColor * textureColor;

            // Simple lighting calculation
            float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
            float3 normal = normalize(input.normal);
            float NdotL = max(dot(normal, lightDir), 0.0f);

            finalColor.rgb *= NdotL;
            finalColor.rgb += emissiveColor.rgb;
            finalColor.a = transparency;

            return finalColor;
        }
    )";
}
