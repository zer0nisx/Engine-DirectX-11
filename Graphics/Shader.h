#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <unordered_map>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// Shader types
enum class ShaderType
{
    Vertex,
    Pixel,
    Geometry,
    Hull,
    Domain,
    Compute
};

// Input layout element description
struct InputLayoutElement
{
    std::string semanticName;
    UINT semanticIndex;
    DXGI_FORMAT format;
    UINT inputSlot;
    UINT alignedByteOffset;
    D3D11_INPUT_CLASSIFICATION inputSlotClass;
    UINT instanceDataStepRate;
};

class Shader
{
public:
    Shader();
    ~Shader();

    // Compilation from source code
    bool CompileFromString(ID3D11Device* device,
                          const std::string& shaderCode,
                          const std::string& entryPoint,
                          ShaderType type,
                          const std::vector<InputLayoutElement>& layoutElements = {});

    // Compilation from file
    bool CompileFromFile(ID3D11Device* device,
                        const std::string& filepath,
                        const std::string& entryPoint,
                        ShaderType type,
                        const std::vector<InputLayoutElement>& layoutElements = {});

    // Bind shader to pipeline
    void Bind(ID3D11DeviceContext* context);
    void Unbind(ID3D11DeviceContext* context);

    // Cleanup
    void Shutdown();

    // Getters
    ShaderType GetType() const { return m_type; }
    ID3D11VertexShader* GetVertexShader() const { return m_vertexShader; }
    ID3D11PixelShader* GetPixelShader() const { return m_pixelShader; }
    ID3D11InputLayout* GetInputLayout() const { return m_inputLayout; }
    ID3DBlob* GetShaderBlob() const { return m_shaderBlob; }

    // Utility
    bool IsValid() const;
    const std::string& GetEntryPoint() const { return m_entryPoint; }
    const std::string& GetFilePath() const { return m_filepath; }

    // Reflection helpers (for debugging)
    void PrintShaderInfo() const;

private:
    bool CreateInputLayout(ID3D11Device* device, const std::vector<InputLayoutElement>& layoutElements);
    std::string GetShaderProfile(ShaderType type) const;
    void ReleaseShaderResources();

private:
    ShaderType m_type;
    std::string m_entryPoint;
    std::string m_filepath;

    // Shader objects
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11GeometryShader* m_geometryShader;
    ID3D11HullShader* m_hullShader;
    ID3D11DomainShader* m_domainShader;
    ID3D11ComputeShader* m_computeShader;

    // Input layout (only for vertex shaders)
    ID3D11InputLayout* m_inputLayout;

    // Compiled shader bytecode
    ID3DBlob* m_shaderBlob;

    bool m_isCompiled;
};

// Shader program that combines vertex and pixel shaders
class ShaderProgram
{
public:
    ShaderProgram();
    ~ShaderProgram();

    // Set shaders
    void SetVertexShader(std::shared_ptr<Shader> vertexShader);
    void SetPixelShader(std::shared_ptr<Shader> pixelShader);

    // Bind entire program
    void Bind(ID3D11DeviceContext* context);
    void Unbind(ID3D11DeviceContext* context);

    // Getters
    std::shared_ptr<Shader> GetVertexShader() const { return m_vertexShader; }
    std::shared_ptr<Shader> GetPixelShader() const { return m_pixelShader; }

    // Validation
    bool IsValid() const;

    // Cleanup
    void Shutdown();

private:
    std::shared_ptr<Shader> m_vertexShader;
    std::shared_ptr<Shader> m_pixelShader;
};

// Utility functions for common shader operations
namespace ShaderUtils
{
    // Create basic vertex input layout for position + normal + texcoord
    std::vector<InputLayoutElement> CreateBasicInputLayout();

    // Create input layout for position + color
    std::vector<InputLayoutElement> CreatePositionColorLayout();

    // Load shader from embedded string
    std::shared_ptr<Shader> CreateVertexShaderFromString(ID3D11Device* device,
                                                        const std::string& shaderCode,
                                                        const std::vector<InputLayoutElement>& layout);

    std::shared_ptr<Shader> CreatePixelShaderFromString(ID3D11Device* device,
                                                       const std::string& shaderCode);

    // Default shaders
    extern const char* DEFAULT_VERTEX_SHADER;
    extern const char* DEFAULT_PIXEL_SHADER;
    extern const char* MATERIAL_VERTEX_SHADER;
    extern const char* MATERIAL_PIXEL_SHADER;
}
