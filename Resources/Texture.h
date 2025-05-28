#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <memory>

using namespace DirectX;

// Texture format enumeration
enum class TextureFormat
{
    Unknown = 0,
    R8G8B8A8_UNORM,
    R8G8B8A8_SRGB,
    BC1_UNORM,      // DXT1
    BC3_UNORM,      // DXT5
    BC5_UNORM,      // Normal maps
    R32G32B32A32_FLOAT,
    R16G16B16A16_FLOAT
};

// Texture usage flags
enum class TextureUsage
{
    Default = 0,
    RenderTarget = 1,
    DepthStencil = 2,
    Dynamic = 4,
    Staging = 8
};

class Texture
{
public:
    Texture();
    ~Texture();

    // Creation from file
    bool LoadFromFile(ID3D11Device* device, const std::string& filepath);

    // Creation from memory
    bool CreateFromMemory(ID3D11Device* device, const void* data, size_t dataSize);

    // Manual texture creation
    bool Create(ID3D11Device* device,
                int width,
                int height,
                TextureFormat format = TextureFormat::R8G8B8A8_UNORM,
                TextureUsage usage = TextureUsage::Default,
                const void* initialData = nullptr);

    // Cleanup
    void Shutdown();

    // Getters
    ID3D11Texture2D* GetTexture() const { return m_texture; }
    ID3D11ShaderResourceView* GetShaderResourceView() const { return m_shaderResourceView; }
    ID3D11RenderTargetView* GetRenderTargetView() const { return m_renderTargetView; }
    ID3D11DepthStencilView* GetDepthStencilView() const { return m_depthStencilView; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    TextureFormat GetFormat() const { return m_format; }
    const std::string& GetFilePath() const { return m_filepath; }

    bool IsValid() const { return m_texture != nullptr; }
    bool HasRenderTarget() const { return m_renderTargetView != nullptr; }
    bool HasDepthStencil() const { return m_depthStencilView != nullptr; }

    // Utility functions
    void GenerateMipmaps(ID3D11DeviceContext* context);
    void SaveToFile(ID3D11DeviceContext* context, const std::string& filepath) const;

    // Static utility functions
    static DXGI_FORMAT GetDXGIFormat(TextureFormat format);
    static TextureFormat GetTextureFormat(DXGI_FORMAT dxgiFormat);
    static std::shared_ptr<Texture> CreateRenderTarget(ID3D11Device* device, int width, int height, TextureFormat format = TextureFormat::R8G8B8A8_UNORM);
    static std::shared_ptr<Texture> CreateDepthStencil(ID3D11Device* device, int width, int height);

private:
    bool CreateShaderResourceView(ID3D11Device* device);
    bool CreateRenderTargetView(ID3D11Device* device);
    bool CreateDepthStencilView(ID3D11Device* device);

    // Helper functions for file loading
    bool LoadDDS(ID3D11Device* device, const std::string& filepath);
    bool LoadWIC(ID3D11Device* device, const std::string& filepath);

private:
    ID3D11Texture2D* m_texture;
    ID3D11ShaderResourceView* m_shaderResourceView;
    ID3D11RenderTargetView* m_renderTargetView;
    ID3D11DepthStencilView* m_depthStencilView;

    int m_width;
    int m_height;
    TextureFormat m_format;
    TextureUsage m_usage;
    std::string m_filepath;

    bool m_isInitialized;
};

// Texture Manager for caching and resource management
class TextureManager
{
public:
    static TextureManager& GetInstance();

    // Texture loading with caching
    std::shared_ptr<Texture> LoadTexture(ID3D11Device* device, const std::string& filepath);
    std::shared_ptr<Texture> GetTexture(const std::string& filepath);

    // Manual texture registration
    void RegisterTexture(const std::string& name, std::shared_ptr<Texture> texture);

    // Cache management
    void ClearCache();
    void RemoveTexture(const std::string& filepath);

    // Statistics
    size_t GetCacheSize() const;
    void PrintCacheInfo() const;

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textureCache;
};
