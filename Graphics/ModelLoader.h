#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Forward declarations
class Model;
class Mesh;
class Material;
class Texture;

// DirectX .x file format structures
namespace XFileFormat
{
    // Header structure for .x files
    struct XFileHeader
    {
        char magic[4];        // "xof "
        char majorVersion[2]; // "03"
        char minorVersion[2]; // "02" or "03"
        char format[4];       // "txt " or "bin " or "tzip" or "bzip"
        char floatSize[4];    // "0032" or "0064"
    };

    // Template definitions used in .x files
    enum class TemplateType
    {
        Header,
        Vector,
        Coords2d,
        Matrix4x4,
        ColorRGBA,
        ColorRGB,
        IndexedColor,
        Boolean,
        Boolean2d,
        MaterialWrap,
        TextureFilename,
        Material,
        MeshFace,
        MeshFaceWraps,
        MeshTextureCoords,
        MeshNormals,
        MeshVertexColors,
        MeshMaterialList,
        Mesh,
        FrameTransformMatrix,
        Frame,
        FloatKeys,
        TimedFloatKeys,
        AnimationKey,
        AnimationOptions,
        Animation,
        AnimationSet,
        SkinMeshHeader,
        SkinWeights,
        XSkinMeshHeader,
        VertexDuplicationIndices,
        FaceAdjacency,
        DeclData,
        VertexElement,
        VertexElementType,
        VertexElementMethod,
        VertexElementUsage,
        DeclType
    };
}

// .X file parsing context
struct XFileContext
{
    std::string content;
    size_t position;
    bool isBinary;
    bool isCompressed;

    XFileContext() : position(0), isBinary(false), isCompressed(false) {}
};

// Parsed material data from .x file
struct XMaterialData
{
    std::string name;
    DirectX::XMFLOAT4 diffuseColor;
    float specularPower;
    DirectX::XMFLOAT3 specularColor;
    DirectX::XMFLOAT3 emissiveColor;
    std::string textureFilename;

    XMaterialData()
        : diffuseColor(0.8f, 0.8f, 0.8f, 1.0f)
        , specularPower(1.0f)
        , specularColor(1.0f, 1.0f, 1.0f)
        , emissiveColor(0.0f, 0.0f, 0.0f)
    {
    }
};

// Parsed mesh data from .x file
struct XMeshData
{
    std::string name;
    std::vector<DirectX::XMFLOAT3> vertices;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT2> textureCoords;
    std::vector<DirectX::XMFLOAT4> vertexColors;
    std::vector<unsigned int> indices;
    std::vector<int> materialIndices; // Per-face material assignment

    // Skinning data
    std::vector<std::vector<int>> boneIndices;    // Per-vertex bone indices
    std::vector<std::vector<float>> boneWeights;  // Per-vertex bone weights
};

// Parsed bone/frame data from .x file
struct XFrameData
{
    std::string name;
    DirectX::XMMATRIX transformMatrix;
    std::vector<std::unique_ptr<XFrameData>> children;
    std::unique_ptr<XMeshData> mesh;

    XFrameData() : transformMatrix(DirectX::XMMatrixIdentity()) {}
};

// Animation data structures
struct XAnimationKey
{
    enum Type
    {
        Rotation = 0,
        Scale = 1,
        Position = 2,
        Matrix = 4
    };

    Type keyType;
    std::vector<float> times;
    std::vector<std::vector<float>> values;
};

struct XAnimationData
{
    std::string name;
    std::string targetFrame;
    std::vector<XAnimationKey> keys;
};

// ModelLoader class for loading DirectX .x files
class ModelLoader
{
public:
    ModelLoader();
    ~ModelLoader();

    // Main loading function
    std::shared_ptr<Model> LoadFromFile(ID3D11Device* device, const std::string& filepath);

    // Configuration
    void SetFlipTextureCoordinates(bool flip) { m_flipTextureCoords = flip; }
    void SetGenerateNormals(bool generate) { m_generateNormals = generate; }
    void SetOptimizeMeshes(bool optimize) { m_optimizeMeshes = optimize; }
    void SetLoadAnimations(bool loadAnims) { m_loadAnimations = loadAnims; }

    // Error handling
    bool HasErrors() const { return !m_errorMessages.empty(); }
    const std::vector<std::string>& GetErrorMessages() const { return m_errorMessages; }
    void ClearErrors() { m_errorMessages.clear(); }

    // Statistics
    struct LoadingStats
    {
        int meshCount;
        int materialCount;
        int animationCount;
        int boneCount;
        float loadingTime;

        LoadingStats() : meshCount(0), materialCount(0), animationCount(0), boneCount(0), loadingTime(0.0f) {}
    };

    const LoadingStats& GetLastLoadingStats() const { return m_lastStats; }

private:
    // File parsing
    bool ParseHeader(XFileContext& context);
    bool ParseContent(XFileContext& context, std::shared_ptr<Model> model, ID3D11Device* device);

    // Template parsing
    std::unique_ptr<XFrameData> ParseFrame(XFileContext& context);
    std::unique_ptr<XMeshData> ParseMesh(XFileContext& context);
    XMaterialData ParseMaterial(XFileContext& context);
    std::vector<XAnimationData> ParseAnimationSet(XFileContext& context);

    // Mesh processing
    void ProcessMeshData(const XMeshData& meshData,
                        const std::vector<XMaterialData>& materials,
                        std::shared_ptr<Model> model,
                        ID3D11Device* device);

    void ProcessFrameHierarchy(const XFrameData& frameData,
                              const DirectX::XMMATRIX& parentTransform,
                              std::shared_ptr<Model> model,
                              ID3D11Device* device);

    // Material processing
    std::shared_ptr<Material> CreateMaterialFromData(const XMaterialData& materialData,
                                                     ID3D11Device* device,
                                                     const std::string& modelDirectory);

    // Animation processing
    void ProcessAnimations(const std::vector<XAnimationData>& animData, std::shared_ptr<Model> model);

    // Utility functions
    bool IsTextFormat(const std::string& content) const;
    std::string ReadString(XFileContext& context);
    int ReadInt(XFileContext& context);
    float ReadFloat(XFileContext& context);
    DirectX::XMFLOAT3 ReadVector3(XFileContext& context);
    DirectX::XMFLOAT4 ReadVector4(XFileContext& context);
    DirectX::XMMATRIX ReadMatrix(XFileContext& context);
    void SkipWhitespace(XFileContext& context);
    void SkipToNext(XFileContext& context, char delimiter);

    // Validation and error handling
    bool ValidateXFile(const std::string& content) const;
    void AddError(const std::string& message);
    void AddWarning(const std::string& message);

    // Post-processing
    void GenerateNormalsForMesh(XMeshData& meshData);
    void OptimizeMeshData(XMeshData& meshData);
    void FlipTextureCoordinates(XMeshData& meshData);

private:
    // Configuration flags
    bool m_flipTextureCoords;
    bool m_generateNormals;
    bool m_optimizeMeshes;
    bool m_loadAnimations;

    // Error tracking
    std::vector<std::string> m_errorMessages;
    std::vector<std::string> m_warningMessages;

    // Statistics
    LoadingStats m_lastStats;

    // Parsing state
    std::string m_currentDirectory;
    std::unordered_map<std::string, int> m_frameNameToIndex;
};

// Utility functions for .x file processing
namespace ModelLoaderUtils
{
    // File operations
    std::string LoadFileContent(const std::string& filepath);
    std::string GetFileDirectory(const std::string& filepath);
    std::string GetFileExtension(const std::string& filepath);
    bool FileExists(const std::string& filepath);

    // Math utilities
    DirectX::XMMATRIX ConvertMatrix(const float matrix[16]);
    void DecomposeMatrix(const DirectX::XMMATRIX& matrix,
                        DirectX::XMFLOAT3& position,
                        DirectX::XMFLOAT4& rotation,
                        DirectX::XMFLOAT3& scale);

    // String utilities
    std::string Trim(const std::string& str);
    std::vector<std::string> Split(const std::string& str, char delimiter);
    bool StartsWith(const std::string& str, const std::string& prefix);
    bool EndsWith(const std::string& str, const std::string& suffix);

    // Mesh utilities
    void CalculateNormals(std::vector<DirectX::XMFLOAT3>& vertices,
                         const std::vector<unsigned int>& indices,
                         std::vector<DirectX::XMFLOAT3>& normals);

    void CalculateTangents(const std::vector<DirectX::XMFLOAT3>& vertices,
                          const std::vector<DirectX::XMFLOAT3>& normals,
                          const std::vector<DirectX::XMFLOAT2>& texCoords,
                          const std::vector<unsigned int>& indices,
                          std::vector<DirectX::XMFLOAT3>& tangents,
                          std::vector<DirectX::XMFLOAT3>& binormals);
}
