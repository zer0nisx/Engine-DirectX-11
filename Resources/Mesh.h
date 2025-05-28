#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>

using namespace DirectX;

// Forward declarations
class Material;

// Vertex structure for standard mesh rendering
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
    XMFLOAT3 tangent;    // For normal mapping
    XMFLOAT3 binormal;   // For normal mapping

    Vertex()
        : position(0.0f, 0.0f, 0.0f)
        , normal(0.0f, 1.0f, 0.0f)
        , texCoord(0.0f, 0.0f)
        , tangent(1.0f, 0.0f, 0.0f)
        , binormal(0.0f, 0.0f, 1.0f)
    {
    }

    Vertex(const XMFLOAT3& pos, const XMFLOAT3& norm, const XMFLOAT2& tex)
        : position(pos)
        , normal(norm)
        , texCoord(tex)
        , tangent(1.0f, 0.0f, 0.0f)
        , binormal(0.0f, 0.0f, 1.0f)
    {
    }
};

// Vertex structure for skinned mesh rendering (with bone weights)
struct SkinnedVertex : public Vertex
{
    static const int MAX_BONE_INFLUENCES = 4;

    int boneIndices[MAX_BONE_INFLUENCES];
    float boneWeights[MAX_BONE_INFLUENCES];

    SkinnedVertex() : Vertex()
    {
        for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            boneIndices[i] = 0;
            boneWeights[i] = 0.0f;
        }
    }

    void AddBoneInfluence(int boneIndex, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            if (boneWeights[i] == 0.0f)
            {
                boneIndices[i] = boneIndex;
                boneWeights[i] = weight;
                break;
            }
        }
    }

    void NormalizeBoneWeights()
    {
        float totalWeight = 0.0f;
        for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
        {
            totalWeight += boneWeights[i];
        }

        if (totalWeight > 0.0f)
        {
            for (int i = 0; i < MAX_BONE_INFLUENCES; ++i)
            {
                boneWeights[i] /= totalWeight;
            }
        }
    }
};

// Bounding box structure
struct BoundingBox
{
    XMFLOAT3 min;
    XMFLOAT3 max;
    XMFLOAT3 center;
    XMFLOAT3 extents;

    BoundingBox()
        : min(FLT_MAX, FLT_MAX, FLT_MAX)
        , max(-FLT_MAX, -FLT_MAX, -FLT_MAX)
        , center(0.0f, 0.0f, 0.0f)
        , extents(0.0f, 0.0f, 0.0f)
    {
    }

    void UpdateFromVertices(const std::vector<Vertex>& vertices);
    void UpdateFromVertices(const std::vector<SkinnedVertex>& vertices);
    bool ContainsPoint(const XMFLOAT3& point) const;
    bool IntersectsBox(const BoundingBox& other) const;
};

// Mesh class for storing and rendering 3D geometry
class Mesh
{
public:
    Mesh();
    ~Mesh();

    // Initialization
    bool Initialize(ID3D11Device* device);
    bool InitializeFromVertices(ID3D11Device* device,
                               const std::vector<Vertex>& vertices,
                               const std::vector<unsigned int>& indices);
    bool InitializeFromSkinnedVertices(ID3D11Device* device,
                                      const std::vector<SkinnedVertex>& vertices,
                                      const std::vector<unsigned int>& indices);

    // Cleanup
    void Shutdown();

    // Rendering
    void Render(ID3D11DeviceContext* context);
    void RenderInstanced(ID3D11DeviceContext* context, int instanceCount);

    // Data access
    const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    const std::vector<SkinnedVertex>& GetSkinnedVertices() const { return m_skinnedVertices; }
    const std::vector<unsigned int>& GetIndices() const { return m_indices; }
    const BoundingBox& GetBoundingBox() const { return m_boundingBox; }

    // Material
    void SetMaterial(std::shared_ptr<Material> material) { m_material = material; }
    std::shared_ptr<Material> GetMaterial() const { return m_material; }
    void SetMaterialIndex(int index) { m_materialIndex = index; }
    int GetMaterialIndex() const { return m_materialIndex; }

    // Properties
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }

    int GetVertexCount() const;
    int GetIndexCount() const { return static_cast<int>(m_indices.size()); }
    int GetTriangleCount() const { return GetIndexCount() / 3; }

    bool IsSkinnedMesh() const { return m_isSkinnedMesh; }
    bool IsValid() const { return m_isInitialized && m_vertexBuffer && m_indexBuffer; }

    // Utility functions
    void CalculateNormals();
    void CalculateTangentsAndBinormals();
    void OptimizeVertices(); // Remove duplicate vertices
    void FlipNormals();
    void ScaleMesh(float scale);
    void TransformMesh(const XMMATRIX& transform);

    // Static utility functions
    static std::shared_ptr<Mesh> CreateCube(ID3D11Device* device, float size = 1.0f);
    static std::shared_ptr<Mesh> CreateSphere(ID3D11Device* device, float radius = 1.0f, int segments = 16);
    static std::shared_ptr<Mesh> CreatePlane(ID3D11Device* device, float width = 1.0f, float height = 1.0f);
    static std::shared_ptr<Mesh> CreateCylinder(ID3D11Device* device, float radius = 1.0f, float height = 1.0f, int segments = 16);

private:
    void CreateBuffers(ID3D11Device* device);
    void UpdateBoundingBox();

private:
    std::string m_name;

    // Vertex data
    std::vector<Vertex> m_vertices;
    std::vector<SkinnedVertex> m_skinnedVertices;
    std::vector<unsigned int> m_indices;

    // DirectX buffers
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;

    // Bounding volume
    BoundingBox m_boundingBox;

    // Material reference
    std::shared_ptr<Material> m_material;
    int m_materialIndex; // Index into model's material array

    // State
    bool m_isInitialized;
    bool m_isSkinnedMesh;

    // Rendering properties
    D3D11_PRIMITIVE_TOPOLOGY m_primitiveTopology;
    UINT m_stride;
    UINT m_offset;
};
