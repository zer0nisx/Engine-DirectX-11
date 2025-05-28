#include "Material.h"
#include "Texture.h"
#include "../Graphics/Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

Material::Material()
    : m_name("DefaultMaterial")
    , m_constantBuffer(nullptr)
    , m_device(nullptr)
    , m_isDirty(true)
    , m_isInitialized(false)
{
    // Initialize all texture pointers to nullptr
    for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
    {
        m_textures[i] = nullptr;
    }

    SetDefaultValues();
}

Material::Material(const std::string& name)
    : Material()
{
    m_name = name;
}

Material::~Material()
{
    Shutdown();
}

bool Material::Initialize(ID3D11Device* device)
{
    if (m_isInitialized)
    {
        return true;
    }

    if (!device)
    {
        return false;
    }

    m_device = device;

    // Create constant buffer for material properties
    if (!CreateConstantBuffer(device))
    {
        return false;
    }

    m_isInitialized = true;
    return true;
}

void Material::Shutdown()
{
    if (m_constantBuffer)
    {
        m_constantBuffer->Release();
        m_constantBuffer = nullptr;
    }

    // Clear texture references
    for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
    {
        m_textures[i].reset();
    }

    m_device = nullptr;
    m_isInitialized = false;
}

void Material::SetTexture(TextureType type, std::shared_ptr<Texture> texture)
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(TextureType::Count))
    {
        m_textures[index] = texture;
    }
}

std::shared_ptr<Texture> Material::GetTexture(TextureType type) const
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(TextureType::Count))
    {
        return m_textures[index];
    }
    return nullptr;
}

bool Material::HasTexture(TextureType type) const
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(TextureType::Count))
    {
        return m_textures[index] != nullptr;
    }
    return false;
}

void Material::RemoveTexture(TextureType type)
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(TextureType::Count))
    {
        m_textures[index].reset();
    }
}

void Material::Apply(ID3D11DeviceContext* context, Shader* shader)
{
    if (!context || !shader || !m_isInitialized)
    {
        return;
    }

    // Update constant buffer if properties have changed
    if (m_isDirty)
    {
        UpdateConstantBuffer(context);
        m_isDirty = false;
    }

    // Bind material constant buffer to shader
    context->VSSetConstantBuffers(1, 1, &m_constantBuffer);  // Slot 1 for vertex shader
    context->PSSetConstantBuffers(1, 1, &m_constantBuffer);  // Slot 1 for pixel shader

    // Bind textures to appropriate shader slots
    UpdateTextureBindings(context);
}

void Material::UpdateConstantBuffer(ID3D11DeviceContext* context)
{
    if (!context || !m_constantBuffer)
    {
        return;
    }

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(m_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    if (SUCCEEDED(hr))
    {
        MaterialProperties* bufferData = (MaterialProperties*)mappedResource.pData;
        *bufferData = m_properties;
        context->Unmap(m_constantBuffer, 0);
    }
}

void Material::SetDefaultValues()
{
    // Set default material properties
    m_properties.diffuseColor = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);   // Light gray
    m_properties.specularColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);  // White specular
    m_properties.emissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);  // No emission
    m_properties.shininess = 32.0f;                                   // Moderate shininess
    m_properties.transparency = 1.0f;                                 // Fully opaque
    m_properties.reflectivity = 0.1f;                                // Low reflectivity
    m_properties.padding = 0.0f;

    m_isDirty = true;
}

void Material::SaveToFile(const std::string& filepath) const
{
    std::ofstream file(filepath);
    if (!file.is_open())
    {
        return;
    }

    // Save material properties in a simple text format
    file << "Material: " << m_name << std::endl;
    file << "DiffuseColor: " << m_properties.diffuseColor.x << " "
         << m_properties.diffuseColor.y << " " << m_properties.diffuseColor.z << " "
         << m_properties.diffuseColor.w << std::endl;
    file << "SpecularColor: " << m_properties.specularColor.x << " "
         << m_properties.specularColor.y << " " << m_properties.specularColor.z << " "
         << m_properties.specularColor.w << std::endl;
    file << "EmissiveColor: " << m_properties.emissiveColor.x << " "
         << m_properties.emissiveColor.y << " " << m_properties.emissiveColor.z << " "
         << m_properties.emissiveColor.w << std::endl;
    file << "Shininess: " << m_properties.shininess << std::endl;
    file << "Transparency: " << m_properties.transparency << std::endl;
    file << "Reflectivity: " << m_properties.reflectivity << std::endl;

    // Save texture paths if they exist
    for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
    {
        if (m_textures[i])
        {
            file << "Texture_" << MaterialUtils::TextureTypeToString(static_cast<TextureType>(i))
                 << ": " << m_textures[i]->GetFilePath() << std::endl;
        }
    }

    file.close();
}

bool Material::LoadFromFile(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "Material:")
        {
            iss >> m_name;
        }
        else if (key == "DiffuseColor:")
        {
            iss >> m_properties.diffuseColor.x >> m_properties.diffuseColor.y
                >> m_properties.diffuseColor.z >> m_properties.diffuseColor.w;
        }
        else if (key == "SpecularColor:")
        {
            iss >> m_properties.specularColor.x >> m_properties.specularColor.y
                >> m_properties.specularColor.z >> m_properties.specularColor.w;
        }
        else if (key == "EmissiveColor:")
        {
            iss >> m_properties.emissiveColor.x >> m_properties.emissiveColor.y
                >> m_properties.emissiveColor.z >> m_properties.emissiveColor.w;
        }
        else if (key == "Shininess:")
        {
            iss >> m_properties.shininess;
        }
        else if (key == "Transparency:")
        {
            iss >> m_properties.transparency;
        }
        else if (key == "Reflectivity:")
        {
            iss >> m_properties.reflectivity;
        }
        // Texture loading would go here - simplified for now
    }

    file.close();
    m_isDirty = true;
    return true;
}

void Material::CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = sizeof(MaterialProperties);
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    HRESULT hr = device->CreateBuffer(&bufferDesc, nullptr, &m_constantBuffer);
    if (FAILED(hr))
    {
        m_constantBuffer = nullptr;
    }
}

void Material::UpdateTextureBindings(ID3D11DeviceContext* context)
{
    // Bind textures to pixel shader slots
    for (int i = 0; i < static_cast<int>(TextureType::Count); ++i)
    {
        if (m_textures[i])
        {
            ID3D11ShaderResourceView* srv = m_textures[i]->GetShaderResourceView();
            if (srv)
            {
                context->PSSetShaderResources(i, 1, &srv);
            }
        }
        else
        {
            // Set null texture for unused slots
            ID3D11ShaderResourceView* nullSRV = nullptr;
            context->PSSetShaderResources(i, 1, &nullSRV);
        }
    }
}

// Material utility functions implementation
namespace MaterialUtils
{
    std::shared_ptr<Material> CreateDefaultMaterial(ID3D11Device* device)
    {
        auto material = std::make_shared<Material>("DefaultMaterial");
        if (material->Initialize(device))
        {
            return material;
        }
        return nullptr;
    }

    std::shared_ptr<Material> CreateMetallicMaterial(ID3D11Device* device, const XMFLOAT4& baseColor)
    {
        auto material = std::make_shared<Material>("MetallicMaterial");
        if (material->Initialize(device))
        {
            material->SetDiffuseColor(baseColor);
            material->SetSpecularColor(XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f));
            material->SetShininess(128.0f);
            material->SetReflectivity(0.8f);
            return material;
        }
        return nullptr;
    }

    std::shared_ptr<Material> CreateGlassMaterial(ID3D11Device* device, float transparency)
    {
        auto material = std::make_shared<Material>("GlassMaterial");
        if (material->Initialize(device))
        {
            material->SetDiffuseColor(XMFLOAT4(0.9f, 0.9f, 1.0f, transparency));
            material->SetSpecularColor(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
            material->SetShininess(256.0f);
            material->SetTransparency(transparency);
            material->SetReflectivity(0.9f);
            return material;
        }
        return nullptr;
    }

    std::shared_ptr<Material> CreateEmissiveMaterial(ID3D11Device* device, const XMFLOAT4& emissiveColor)
    {
        auto material = std::make_shared<Material>("EmissiveMaterial");
        if (material->Initialize(device))
        {
            material->SetDiffuseColor(XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f));
            material->SetEmissiveColor(emissiveColor);
            material->SetShininess(1.0f);
            return material;
        }
        return nullptr;
    }

    const char* TextureTypeToString(TextureType type)
    {
        switch (type)
        {
        case TextureType::Diffuse:      return "Diffuse";
        case TextureType::Specular:     return "Specular";
        case TextureType::Normal:       return "Normal";
        case TextureType::Emissive:     return "Emissive";
        case TextureType::Opacity:      return "Opacity";
        case TextureType::Environment:  return "Environment";
        default:                        return "Unknown";
        }
    }

    TextureType StringToTextureType(const std::string& typeStr)
    {
        if (typeStr == "Diffuse")      return TextureType::Diffuse;
        if (typeStr == "Specular")     return TextureType::Specular;
        if (typeStr == "Normal")       return TextureType::Normal;
        if (typeStr == "Emissive")     return TextureType::Emissive;
        if (typeStr == "Opacity")      return TextureType::Opacity;
        if (typeStr == "Environment")  return TextureType::Environment;
        return TextureType::Diffuse;  // Default fallback
    }
}
