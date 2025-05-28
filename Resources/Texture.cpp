#include "Texture.h"
#include <iostream>
#include <fstream>
#include <unordered_map>

// For texture loading - simplified implementation
// In a real implementation, you'd use DirectXTK or similar library
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

Texture::Texture()
    : m_texture(nullptr)
    , m_shaderResourceView(nullptr)
    , m_renderTargetView(nullptr)
    , m_depthStencilView(nullptr)
    , m_width(0)
    , m_height(0)
    , m_format(TextureFormat::Unknown)
    , m_usage(TextureUsage::Default)
    , m_isInitialized(false)
{
}

Texture::~Texture()
{
    Shutdown();
}

bool Texture::LoadFromFile(ID3D11Device* device, const std::string& filepath)
{
    if (!device || filepath.empty())
    {
        return false;
    }

    m_filepath = filepath;

    // Determine file format based on extension
    std::string extension = filepath.substr(filepath.find_last_of('.'));
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (extension == ".dds")
    {
        return LoadDDS(device, filepath);
    }
    else
    {
        return LoadWIC(device, filepath);
    }
}

bool Texture::CreateFromMemory(ID3D11Device* device, const void* data, size_t dataSize)
{
    if (!device || !data || dataSize == 0)
    {
        return false;
    }

    // Create WIC factory
    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );

    if (FAILED(hr))
    {
        return false;
    }

    // Create stream from memory
    IWICStream* stream = nullptr;
    hr = factory->CreateStream(&stream);
    if (SUCCEEDED(hr))
    {
        hr = stream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(const_cast<void*>(data)),
            static_cast<DWORD>(dataSize)
        );
    }

    // Create decoder
    IWICBitmapDecoder* decoder = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = factory->CreateDecoderFromStream(
            stream,
            nullptr,
            WICDecodeMetadataCacheOnDemand,
            &decoder
        );
    }

    // Get first frame
    IWICBitmapFrameDecode* frame = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = decoder->GetFrame(0, &frame);
    }

    // Convert to RGBA format
    IWICFormatConverter* converter = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = factory->CreateFormatConverter(&converter);
        if (SUCCEEDED(hr))
        {
            hr = converter->Initialize(
                frame,
                GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone,
                nullptr,
                0.0f,
                WICBitmapPaletteTypeCustom
            );
        }
    }

    // Get image dimensions
    UINT width, height;
    if (SUCCEEDED(hr))
    {
        hr = converter->GetSize(&width, &height);
    }

    // Copy pixel data
    std::vector<BYTE> pixels;
    if (SUCCEEDED(hr))
    {
        UINT stride = width * 4; // 4 bytes per pixel (RGBA)
        UINT imageSize = stride * height;
        pixels.resize(imageSize);

        hr = converter->CopyPixels(
            nullptr,
            stride,
            imageSize,
            pixels.data()
        );
    }

    // Create DirectX texture
    bool result = false;
    if (SUCCEEDED(hr))
    {
        result = Create(device, width, height, TextureFormat::RGBA8, TextureUsage::ShaderResource, pixels.data());
    }

    // Cleanup
    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();

    return result;
}

bool Texture::Create(ID3D11Device* device,
                    int width,
                    int height,
                    TextureFormat format,
                    TextureUsage usage,
                    const void* initialData)
{
    if (!device || width <= 0 || height <= 0)
    {
        return false;
    }

    Shutdown(); // Clean up existing resources

    m_width = width;
    m_height = height;
    m_format = format;
    m_usage = usage;

    // Create texture description
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = GetDXGIFormat(format);
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;

    // Set usage based on flags
    if (static_cast<int>(usage) & static_cast<int>(TextureUsage::Dynamic))
    {
        textureDesc.Usage = D3D11_USAGE_DYNAMIC;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else if (static_cast<int>(usage) & static_cast<int>(TextureUsage::Staging))
    {
        textureDesc.Usage = D3D11_USAGE_STAGING;
        textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }
    else
    {
        textureDesc.Usage = D3D11_USAGE_DEFAULT;
        textureDesc.CPUAccessFlags = 0;
    }

    // Set bind flags
    textureDesc.BindFlags = 0;
    if (static_cast<int>(usage) & static_cast<int>(TextureUsage::RenderTarget))
    {
        textureDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    if (static_cast<int>(usage) & static_cast<int>(TextureUsage::DepthStencil))
    {
        textureDesc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
        textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Override format for depth
    }
    else
    {
        textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }

    // Create initial data structure if provided
    D3D11_SUBRESOURCE_DATA* pInitialData = nullptr;
    D3D11_SUBRESOURCE_DATA initialDataStruct = {};
    if (initialData)
    {
        initialDataStruct.pSysMem = initialData;
        initialDataStruct.SysMemPitch = width * 4; // Assuming 4 bytes per pixel
        initialDataStruct.SysMemSlicePitch = 0;
        pInitialData = &initialDataStruct;
    }

    // Create the texture
    HRESULT hr = device->CreateTexture2D(&textureDesc, pInitialData, &m_texture);
    if (FAILED(hr))
    {
        return false;
    }

    // Create views based on usage
    if (textureDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        if (!CreateShaderResourceView(device))
        {
            Shutdown();
            return false;
        }
    }

    if (textureDesc.BindFlags & D3D11_BIND_RENDER_TARGET)
    {
        if (!CreateRenderTargetView(device))
        {
            Shutdown();
            return false;
        }
    }

    if (textureDesc.BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        if (!CreateDepthStencilView(device))
        {
            Shutdown();
            return false;
        }
    }

    m_isInitialized = true;
    return true;
}

void Texture::Shutdown()
{
    if (m_depthStencilView)
    {
        m_depthStencilView->Release();
        m_depthStencilView = nullptr;
    }

    if (m_renderTargetView)
    {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }

    if (m_shaderResourceView)
    {
        m_shaderResourceView->Release();
        m_shaderResourceView = nullptr;
    }

    if (m_texture)
    {
        m_texture->Release();
        m_texture = nullptr;
    }

    m_width = 0;
    m_height = 0;
    m_format = TextureFormat::Unknown;
    m_filepath.clear();
    m_isInitialized = false;
}

void Texture::GenerateMipmaps(ID3D11DeviceContext* context)
{
    if (context && m_shaderResourceView)
    {
        context->GenerateMips(m_shaderResourceView);
    }
}

void Texture::SaveToFile(ID3D11DeviceContext* context, const std::string& filepath) const
{
    // Simplified implementation - would need proper image encoding
    // This is a placeholder for texture saving functionality
}

DXGI_FORMAT Texture::GetDXGIFormat(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8G8B8A8_UNORM:    return DXGI_FORMAT_R8G8B8A8_UNORM;
    case TextureFormat::R8G8B8A8_SRGB:     return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case TextureFormat::BC1_UNORM:         return DXGI_FORMAT_BC1_UNORM;
    case TextureFormat::BC3_UNORM:         return DXGI_FORMAT_BC3_UNORM;
    case TextureFormat::BC5_UNORM:         return DXGI_FORMAT_BC5_UNORM;
    case TextureFormat::R32G32B32A32_FLOAT:return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case TextureFormat::R16G16B16A16_FLOAT:return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default:                               return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
}

TextureFormat Texture::GetTextureFormat(DXGI_FORMAT dxgiFormat)
{
    switch (dxgiFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:       return TextureFormat::R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return TextureFormat::R8G8B8A8_SRGB;
    case DXGI_FORMAT_BC1_UNORM:            return TextureFormat::BC1_UNORM;
    case DXGI_FORMAT_BC3_UNORM:            return TextureFormat::BC3_UNORM;
    case DXGI_FORMAT_BC5_UNORM:            return TextureFormat::BC5_UNORM;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:   return TextureFormat::R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:   return TextureFormat::R16G16B16A16_FLOAT;
    default:                               return TextureFormat::Unknown;
    }
}

std::shared_ptr<Texture> Texture::CreateRenderTarget(ID3D11Device* device, int width, int height, TextureFormat format)
{
    auto texture = std::make_shared<Texture>();
    if (texture->Create(device, width, height, format, TextureUsage::RenderTarget))
    {
        return texture;
    }
    return nullptr;
}

std::shared_ptr<Texture> Texture::CreateDepthStencil(ID3D11Device* device, int width, int height)
{
    auto texture = std::make_shared<Texture>();
    if (texture->Create(device, width, height, TextureFormat::R8G8B8A8_UNORM, TextureUsage::DepthStencil))
    {
        return texture;
    }
    return nullptr;
}

bool Texture::CreateShaderResourceView(ID3D11Device* device)
{
    if (!device || !m_texture)
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = GetDXGIFormat(m_format);
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    HRESULT hr = device->CreateShaderResourceView(m_texture, &srvDesc, &m_shaderResourceView);
    return SUCCEEDED(hr);
}

bool Texture::CreateRenderTargetView(ID3D11Device* device)
{
    if (!device || !m_texture)
    {
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = GetDXGIFormat(m_format);
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    HRESULT hr = device->CreateRenderTargetView(m_texture, &rtvDesc, &m_renderTargetView);
    return SUCCEEDED(hr);
}

bool Texture::CreateDepthStencilView(ID3D11Device* device)
{
    if (!device || !m_texture)
    {
        return false;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    HRESULT hr = device->CreateDepthStencilView(m_texture, &dsvDesc, &m_depthStencilView);
    return SUCCEEDED(hr);
}

bool Texture::LoadDDS(ID3D11Device* device, const std::string& filepath)
{
    // Simplified DDS loading - in real implementation, use DirectXTK
    // This is a placeholder that creates a default texture
    return Create(device, 256, 256, TextureFormat::R8G8B8A8_UNORM);
}

bool Texture::LoadWIC(ID3D11Device* device, const std::string& filepath)
{
    // Simplified WIC loading - in real implementation, use DirectXTK or WIC directly
    // This is a placeholder that creates a default texture
    return Create(device, 256, 256, TextureFormat::R8G8B8A8_UNORM);
}

// TextureManager implementation
TextureManager& TextureManager::GetInstance()
{
    static TextureManager instance;
    return instance;
}

std::shared_ptr<Texture> TextureManager::LoadTexture(ID3D11Device* device, const std::string& filepath)
{
    // Check if texture is already cached
    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end())
    {
        return it->second;
    }

    // Load new texture
    auto texture = std::make_shared<Texture>();
    if (texture->LoadFromFile(device, filepath))
    {
        m_textureCache[filepath] = texture;
        return texture;
    }

    return nullptr;
}

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string& filepath)
{
    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end())
    {
        return it->second;
    }
    return nullptr;
}

void TextureManager::RegisterTexture(const std::string& name, std::shared_ptr<Texture> texture)
{
    if (texture)
    {
        m_textureCache[name] = texture;
    }
}

void TextureManager::ClearCache()
{
    m_textureCache.clear();
}

void TextureManager::RemoveTexture(const std::string& filepath)
{
    m_textureCache.erase(filepath);
}

size_t TextureManager::GetCacheSize() const
{
    return m_textureCache.size();
}

void TextureManager::PrintCacheInfo() const
{
    std::cout << "Texture Cache Info:" << std::endl;
    std::cout << "  Cached textures: " << m_textureCache.size() << std::endl;
    for (const auto& pair : m_textureCache)
    {
        std::cout << "  - " << pair.first << std::endl;
    }
}
