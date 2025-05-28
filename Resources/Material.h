#pragma once

#include <DirectXMath.h>
#include <d3d11.h>
#include <string>
#include <memory>
#include <vector>

using namespace DirectX;

// Forward declarations
class Texture;
class Shader;

// Material properties structure for constant buffer
struct MaterialProperties
{
    XMFLOAT4 diffuseColor;      // Base color (RGBA)
    XMFLOAT4 specularColor;     // Specular reflection color
    XMFLOAT4 emissiveColor;     // Self-illumination color
    float shininess;            // Specular power/shininess
    float transparency;         // Alpha transparency (0.0 = transparent, 1.0 = opaque)
    float reflectivity;         // Reflection strength
    float padding;              // Padding for 16-byte alignment
};

// Material texture types
enum class TextureType
{
    Diffuse = 0,
    Specular,
    Normal,
    Emissive,
    Opacity,
    Environment,
    Count  // Total number of texture types
};

class Material
{
public:
    Material();
    Material(const std::string& name);
    ~Material();

    // Initialization and cleanup
    bool Initialize(ID3D11Device* device);
    void Shutdown();

    // Material properties
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    void SetDiffuseColor(const XMFLOAT4& color) { m_properties.diffuseColor = color; m_isDirty = true; }
    void SetSpecularColor(const XMFLOAT4& color) { m_properties.specularColor = color; m_isDirty = true; }
    void SetEmissiveColor(const XMFLOAT4& color) { m_properties.emissiveColor = color; m_isDirty = true; }
    void SetShininess(float shininess) { m_properties.shininess = shininess; m_isDirty = true; }
    void SetTransparency(float transparency) { m_properties.transparency = transparency; m_isDirty = true; }
    void SetReflectivity(float reflectivity) { m_properties.reflectivity = reflectivity; m_isDirty = true; }

    const MaterialProperties& GetProperties() const { return m_properties; }

    // Texture management
    void SetTexture(TextureType type, std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> GetTexture(TextureType type) const;
    bool HasTexture(TextureType type) const;
    void RemoveTexture(TextureType type);

    // Rendering
    void Apply(ID3D11DeviceContext* context, Shader* shader);
    void UpdateConstantBuffer(ID3D11DeviceContext* context);

    // Utility functions
    bool IsTransparent() const { return m_properties.transparency < 1.0f; }
    void SetDefaultValues();

    // Serialization (for saving/loading)
    void SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);

private:
    void CreateConstantBuffer(ID3D11Device* device);
    void UpdateTextureBindings(ID3D11DeviceContext* context);

private:
    std::string m_name;
    MaterialProperties m_properties;

    // Texture storage
    std::shared_ptr<Texture> m_textures[static_cast<int>(TextureType::Count)];

    // DirectX resources
    ID3D11Buffer* m_constantBuffer;
    ID3D11Device* m_device;

    // State tracking
    bool m_isDirty;  // True if properties have changed and need to be updated
    bool m_isInitialized;
};

// Material utility functions
namespace MaterialUtils
{
    // Create common material types
    std::shared_ptr<Material> CreateDefaultMaterial(ID3D11Device* device);
    std::shared_ptr<Material> CreateMetallicMaterial(ID3D11Device* device, const XMFLOAT4& baseColor);
    std::shared_ptr<Material> CreateGlassMaterial(ID3D11Device* device, float transparency = 0.7f);
    std::shared_ptr<Material> CreateEmissiveMaterial(ID3D11Device* device, const XMFLOAT4& emissiveColor);

    // Texture type utilities
    const char* TextureTypeToString(TextureType type);
    TextureType StringToTextureType(const std::string& typeStr);
}
