#include "PostProcess.h"
#include "Shader.h"
#include "../Resources/Texture.h"
#include <iostream>
#include <algorithm>

// Post-process effect shaders as strings
namespace PostProcessShaders
{
    const char* FULLSCREEN_QUAD_VS = R"(
struct VSInput
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = input.position;
    output.texCoord = input.texCoord;
    return output;
}
)";

    const char* COPY_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    return inputTexture.Sample(linearSampler, input.texCoord);
}
)";

    const char* GRAYSCALE_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer PostProcessParams : register(b0)
{
    float intensity;
    float3 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = inputTexture.Sample(linearSampler, input.texCoord);
    float gray = dot(color.rgb, float3(0.299, 0.587, 0.114));
    color.rgb = lerp(color.rgb, float3(gray, gray, gray), intensity);
    return color;
}
)";

    const char* BLOOM_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer BloomParams : register(b0)
{
    float bloomThreshold;
    float bloomIntensity;
    float2 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = inputTexture.Sample(linearSampler, input.texCoord);
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));

    if (brightness > bloomThreshold)
    {
        color.rgb *= bloomIntensity;
    }
    else
    {
        color.rgb = float3(0, 0, 0);
    }

    return color;
}
)";

    const char* GAUSSIAN_BLUR_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer BlurParams : register(b0)
{
    float2 texelSize;
    float blurRadius;
    float padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 0);
    float totalWeight = 0;

    for (int i = -4; i <= 4; ++i)
    {
        for (int j = -4; j <= 4; ++j)
        {
            float2 offset = float2(i, j) * texelSize * blurRadius;
            float weight = exp(-(i*i + j*j) / (2.0 * 2.0 * 2.0));

            color += inputTexture.Sample(linearSampler, input.texCoord + offset) * weight;
            totalWeight += weight;
        }
    }

    return color / totalWeight;
}
)";

    const char* TONE_MAPPING_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer ToneMappingParams : register(b0)
{
    float exposure;
    float whitePoint;
    float2 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = inputTexture.Sample(linearSampler, input.texCoord);

    // Apply exposure
    color.rgb *= exposure;

    // Reinhard tone mapping
    color.rgb = color.rgb / (color.rgb + whitePoint);

    // Gamma correction
    color.rgb = pow(color.rgb, 1.0 / 2.2);

    return color;
}
)";

    const char* VIGNETTE_PS = R"(
Texture2D inputTexture : register(t0);
SamplerState linearSampler : register(s0);

cbuffer VignetteParams : register(b0)
{
    float vignetteRadius;
    float vignetteSoftness;
    float2 padding;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 color = inputTexture.Sample(linearSampler, input.texCoord);

    float2 center = float2(0.5, 0.5);
    float dist = distance(input.texCoord, center);

    float vignette = 1.0 - smoothstep(vignetteRadius, vignetteRadius + vignetteSoftness, dist);
    color.rgb *= vignette;

    return color;
}
)";
}

// PostProcessEffect_Base implementation
PostProcessEffect_Base::PostProcessEffect_Base(PostProcessEffect type)
    : m_type(type)
    , m_enabled(true)
    , m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_parameterBuffer(nullptr)
{
}

PostProcessEffect_Base::~PostProcessEffect_Base()
{
    Shutdown();
}

bool PostProcessEffect_Base::Initialize(ID3D11Device* device, int width, int height)
{
    if (!device)
        return false;

    m_width = width;
    m_height = height;

    // Create fullscreen quad vertex shader (shared by all effects)
    auto vsLayout = ShaderUtils::CreatePositionColorLayout();
    m_vertexShader = ShaderUtils::CreateVertexShaderFromString(device, PostProcessShaders::FULLSCREEN_QUAD_VS, vsLayout);

    if (!m_vertexShader)
    {
        std::cerr << "PostProcess: Failed to create vertex shader" << std::endl;
        return false;
    }

    // Create effect-specific pixel shader
    std::string pixelShaderCode = GetPixelShaderCode();
    m_pixelShader = ShaderUtils::CreatePixelShaderFromString(device, pixelShaderCode);

    if (!m_pixelShader)
    {
        std::cerr << "PostProcess: Failed to create pixel shader for effect " << (int)m_type << std::endl;
        return false;
    }

    // Create parameter constant buffer
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = GetParameterBufferSize();
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &m_parameterBuffer);
    if (FAILED(hr))
    {
        std::cerr << "PostProcess: Failed to create parameter buffer" << std::endl;
        return false;
    }

    return true;
}

void PostProcessEffect_Base::Shutdown()
{
    if (m_parameterBuffer)
    {
        m_parameterBuffer->Release();
        m_parameterBuffer = nullptr;
    }

    m_vertexShader.reset();
    m_pixelShader.reset();
}

void PostProcessEffect_Base::Apply(ID3D11DeviceContext* context,
                                  ID3D11ShaderResourceView* inputTexture,
                                  ID3D11RenderTargetView* outputTarget,
                                  const PostProcessParams& params)
{
    if (!context || !inputTexture || !outputTarget || !m_enabled)
        return;

    // Update parameter buffer
    UpdateParameterBuffer(context, params);

    // Set render target
    context->OMSetRenderTargets(1, &outputTarget, nullptr);

    // Set viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = (FLOAT)m_width;
    viewport.Height = (FLOAT)m_height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    context->RSSetViewports(1, &viewport);

    // Bind shaders
    m_vertexShader->Bind(context);
    m_pixelShader->Bind(context);

    // Bind input texture
    context->PSSetShaderResources(0, 1, &inputTexture);

    // Bind parameter buffer
    context->PSSetConstantBuffers(0, 1, &m_parameterBuffer);

    // Draw fullscreen quad
    DrawFullscreenQuad(context);

    // Clear bindings
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);
}

std::string PostProcessEffect_Base::GetPixelShaderCode() const
{
    switch (m_type)
    {
        case PostProcessEffect::Grayscale:
            return PostProcessShaders::GRAYSCALE_PS;
        case PostProcessEffect::Bloom:
            return PostProcessShaders::BLOOM_PS;
        case PostProcessEffect::GaussianBlur:
            return PostProcessShaders::GAUSSIAN_BLUR_PS;
        case PostProcessEffect::ToneMapping:
            return PostProcessShaders::TONE_MAPPING_PS;
        case PostProcessEffect::Vignette:
            return PostProcessShaders::VIGNETTE_PS;
        default:
            return PostProcessShaders::COPY_PS;
    }
}

UINT PostProcessEffect_Base::GetParameterBufferSize() const
{
    // Align to 16 bytes for constant buffer requirements
    return ((sizeof(PostProcessParams) + 15) / 16) * 16;
}

void PostProcessEffect_Base::UpdateParameterBuffer(ID3D11DeviceContext* context, const PostProcessParams& params)
{
    if (!m_parameterBuffer)
        return;

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(m_parameterBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (SUCCEEDED(hr))
    {
        memcpy(mappedResource.pData, &params, sizeof(PostProcessParams));
        context->Unmap(m_parameterBuffer, 0);
    }
}

void PostProcessEffect_Base::DrawFullscreenQuad(ID3D11DeviceContext* context)
{
    // Note: In a production implementation, this vertex buffer should be cached
    // and created once during initialization for better performance

    struct QuadVertex
    {
        XMFLOAT3 position;
        XMFLOAT2 texCoord;
    };

    QuadVertex vertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) }
    };

    // Create temporary vertex buffer (should be cached in production)
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    ID3D11Buffer* vertexBuffer = nullptr;
    ID3D11Device* device = nullptr;
    context->GetDevice(&device);

    if (device && SUCCEEDED(device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer)))
    {
        UINT stride = sizeof(QuadVertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);

        vertexBuffer->Release();
    }

    if (device) device->Release();
}

// PostProcessManager implementation
PostProcessManager::PostProcessManager()
    : m_device(nullptr)
    , m_width(0)
    , m_height(0)
    , m_renderTarget1(nullptr)
    , m_renderTarget2(nullptr)
    , m_renderTargetView1(nullptr)
    , m_renderTargetView2(nullptr)
    , m_shaderResourceView1(nullptr)
    , m_shaderResourceView2(nullptr)
    , m_samplerState(nullptr)
{
}

PostProcessManager::~PostProcessManager()
{
    Shutdown();
}

bool PostProcessManager::Initialize(ID3D11Device* device, int width, int height)
{
    if (!device)
        return false;

    m_device = device;
    m_width = width;
    m_height = height;

    // Create render targets for ping-pong rendering
    if (!CreateRenderTargets())
    {
        std::cerr << "PostProcessManager: Failed to create render targets" << std::endl;
        return false;
    }

    // Create sampler state
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = device->CreateSamplerState(&samplerDesc, &m_samplerState);
    if (FAILED(hr))
    {
        std::cerr << "PostProcessManager: Failed to create sampler state" << std::endl;
        return false;
    }

    std::cout << "PostProcessManager: Initialized for " << width << "x" << height << std::endl;
    return true;
}

void PostProcessManager::Shutdown()
{
    for (auto& effect : m_effects)
    {
        if (effect.second)
        {
            effect.second->Shutdown();
        }
    }
    m_effects.clear();

    if (m_samplerState)
    {
        m_samplerState->Release();
        m_samplerState = nullptr;
    }

    if (m_shaderResourceView2)
    {
        m_shaderResourceView2->Release();
        m_shaderResourceView2 = nullptr;
    }

    if (m_shaderResourceView1)
    {
        m_shaderResourceView1->Release();
        m_shaderResourceView1 = nullptr;
    }

    if (m_renderTargetView2)
    {
        m_renderTargetView2->Release();
        m_renderTargetView2 = nullptr;
    }

    if (m_renderTargetView1)
    {
        m_renderTargetView1->Release();
        m_renderTargetView1 = nullptr;
    }

    if (m_renderTarget2)
    {
        m_renderTarget2->Release();
        m_renderTarget2 = nullptr;
    }

    if (m_renderTarget1)
    {
        m_renderTarget1->Release();
        m_renderTarget1 = nullptr;
    }

    m_device = nullptr;
}

void PostProcessManager::AddEffect(PostProcessEffect effectType)
{
    if (m_effects.find(effectType) != m_effects.end())
        return; // Effect already exists

    auto effect = std::make_unique<PostProcessEffect_Base>(effectType);
    if (effect->Initialize(m_device, m_width, m_height))
    {
        m_effectOrder.push_back(effectType);
        m_effects[effectType] = std::move(effect);

        std::cout << "PostProcessManager: Added effect " << (int)effectType << std::endl;
    }
    else
    {
        std::cerr << "PostProcessManager: Failed to add effect " << (int)effectType << std::endl;
    }
}

void PostProcessManager::RemoveEffect(PostProcessEffect effectType)
{
    auto it = m_effects.find(effectType);
    if (it != m_effects.end())
    {
        it->second->Shutdown();
        m_effects.erase(it);

        auto orderIt = std::find(m_effectOrder.begin(), m_effectOrder.end(), effectType);
        if (orderIt != m_effectOrder.end())
        {
            m_effectOrder.erase(orderIt);
        }
    }
}

void PostProcessManager::SetEffectEnabled(PostProcessEffect effectType, bool enabled)
{
    auto it = m_effects.find(effectType);
    if (it != m_effects.end())
    {
        it->second->SetEnabled(enabled);
    }
}

void PostProcessManager::Process(ID3D11DeviceContext* context,
                                ID3D11ShaderResourceView* inputTexture,
                                ID3D11RenderTargetView* outputTarget)
{
    if (!context || !inputTexture || !outputTarget || m_effectOrder.empty())
    {
        // No effects, just copy input to output
        CopyTexture(context, inputTexture, outputTarget);
        return;
    }

    // Set sampler state
    context->PSSetSamplers(0, 1, &m_samplerState);

    ID3D11ShaderResourceView* currentInput = inputTexture;
    ID3D11RenderTargetView* currentOutput = nullptr;

    bool useRT1 = true;

    // Apply effects in order
    for (size_t i = 0; i < m_effectOrder.size(); ++i)
    {
        PostProcessEffect effectType = m_effectOrder[i];
        auto it = m_effects.find(effectType);

        if (it == m_effects.end() || !it->second->IsEnabled())
            continue;

        // Determine output target
        if (i == m_effectOrder.size() - 1)
        {
            // Last effect, render to final output
            currentOutput = outputTarget;
        }
        else
        {
            // Render to ping-pong buffer
            currentOutput = useRT1 ? m_renderTargetView1 : m_renderTargetView2;
        }

        // Apply effect
        it->second->Apply(context, currentInput, currentOutput, m_parameters);

        // Setup for next effect
        if (i < m_effectOrder.size() - 1)
        {
            currentInput = useRT1 ? m_shaderResourceView1 : m_shaderResourceView2;
            useRT1 = !useRT1;
        }
    }

    // Clear sampler binding
    ID3D11SamplerState* nullSampler = nullptr;
    context->PSSetSamplers(0, 1, &nullSampler);
}

bool PostProcessManager::CreateRenderTargets()
{
    if (!m_device)
        return false;

    // Create texture description
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = m_width;
    textureDesc.Height = m_height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    // Create first render target
    HRESULT hr = m_device->CreateTexture2D(&textureDesc, nullptr, &m_renderTarget1);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateRenderTargetView(m_renderTarget1, nullptr, &m_renderTargetView1);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateShaderResourceView(m_renderTarget1, nullptr, &m_shaderResourceView1);
    if (FAILED(hr))
        return false;

    // Create second render target
    hr = m_device->CreateTexture2D(&textureDesc, nullptr, &m_renderTarget2);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateRenderTargetView(m_renderTarget2, nullptr, &m_renderTargetView2);
    if (FAILED(hr))
        return false;

    hr = m_device->CreateShaderResourceView(m_renderTarget2, nullptr, &m_shaderResourceView2);
    if (FAILED(hr))
        return false;

    return true;
}

void PostProcessManager::CopyTexture(ID3D11DeviceContext* context,
                                    ID3D11ShaderResourceView* input,
                                    ID3D11RenderTargetView* output)
{
    if (!context || !input || !output)
        return;

    // Create a simple copy effect if needed
    static std::unique_ptr<PostProcessEffect_Base> copyEffect = nullptr;
    if (!copyEffect)
    {
        copyEffect = std::make_unique<PostProcessEffect_Base>(PostProcessEffect::None);
        copyEffect->Initialize(m_device, m_width, m_height);
    }

    // Use the copy effect to transfer the texture
    PostProcessParams emptyParams; // Default parameters
    copyEffect->Apply(context, input, output, emptyParams);
}
