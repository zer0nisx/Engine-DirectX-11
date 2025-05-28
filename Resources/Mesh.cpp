#include "Mesh.h"
#include "Material.h"
#include <iostream>
#include <algorithm>
#include <unordered_map>

// BoundingBox implementation
void BoundingBox::UpdateFromVertices(const std::vector<Vertex>& vertices)
{
    if (vertices.empty())
    {
        min = max = center = extents = XMFLOAT3(0.0f, 0.0f, 0.0f);
        return;
    }

    min = max = vertices[0].position;

    for (const auto& vertex : vertices)
    {
        min.x = std::min(min.x, vertex.position.x);
        min.y = std::min(min.y, vertex.position.y);
        min.z = std::min(min.z, vertex.position.z);

        max.x = std::max(max.x, vertex.position.x);
        max.y = std::max(max.y, vertex.position.y);
        max.z = std::max(max.z, vertex.position.z);
    }

    center.x = (min.x + max.x) * 0.5f;
    center.y = (min.y + max.y) * 0.5f;
    center.z = (min.z + max.z) * 0.5f;

    extents.x = (max.x - min.x) * 0.5f;
    extents.y = (max.y - min.y) * 0.5f;
    extents.z = (max.z - min.z) * 0.5f;
}

void BoundingBox::UpdateFromVertices(const std::vector<SkinnedVertex>& vertices)
{
    if (vertices.empty())
    {
        min = max = center = extents = XMFLOAT3(0.0f, 0.0f, 0.0f);
        return;
    }

    min = max = vertices[0].position;

    for (const auto& vertex : vertices)
    {
        min.x = std::min(min.x, vertex.position.x);
        min.y = std::min(min.y, vertex.position.y);
        min.z = std::min(min.z, vertex.position.z);

        max.x = std::max(max.x, vertex.position.x);
        max.y = std::max(max.y, vertex.position.y);
        max.z = std::max(max.z, vertex.position.z);
    }

    center.x = (min.x + max.x) * 0.5f;
    center.y = (min.y + max.y) * 0.5f;
    center.z = (min.z + max.z) * 0.5f;

    extents.x = (max.x - min.x) * 0.5f;
    extents.y = (max.y - min.y) * 0.5f;
    extents.z = (max.z - min.z) * 0.5f;
}

bool BoundingBox::ContainsPoint(const XMFLOAT3& point) const
{
    return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
}

bool BoundingBox::IntersectsBox(const BoundingBox& other) const
{
    return (min.x <= other.max.x && max.x >= other.min.x &&
            min.y <= other.max.y && max.y >= other.min.y &&
            min.z <= other.max.z && max.z >= other.min.z);
}

// Mesh implementation
Mesh::Mesh()
    : m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_materialIndex(-1)
    , m_isInitialized(false)
    , m_isSkinnedMesh(false)
    , m_primitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    , m_stride(sizeof(Vertex))
    , m_offset(0)
{
}

Mesh::~Mesh()
{
    Shutdown();
}

bool Mesh::Initialize(ID3D11Device* device)
{
    if (!device)
    {
        return false;
    }

    m_isInitialized = true;
    return true;
}

bool Mesh::InitializeFromVertices(ID3D11Device* device,
                                 const std::vector<Vertex>& vertices,
                                 const std::vector<unsigned int>& indices)
{
    if (!device || vertices.empty())
    {
        return false;
    }

    Shutdown(); // Clean up existing resources

    m_vertices = vertices;
    m_indices = indices;
    m_isSkinnedMesh = false;
    m_stride = sizeof(Vertex);

    if (!CreateBuffers(device))
    {
        return false;
    }

    UpdateBoundingBox();
    m_isInitialized = true;
    return true;
}

bool Mesh::InitializeFromSkinnedVertices(ID3D11Device* device,
                                        const std::vector<SkinnedVertex>& vertices,
                                        const std::vector<unsigned int>& indices)
{
    if (!device || vertices.empty())
    {
        return false;
    }

    Shutdown(); // Clean up existing resources

    m_skinnedVertices = vertices;
    m_indices = indices;
    m_isSkinnedMesh = true;
    m_stride = sizeof(SkinnedVertex);

    if (!CreateBuffers(device))
    {
        return false;
    }

    UpdateBoundingBox();
    m_isInitialized = true;
    return true;
}

void Mesh::Shutdown()
{
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

    m_vertices.clear();
    m_skinnedVertices.clear();
    m_indices.clear();
    m_material.reset();

    m_isInitialized = false;
    m_isSkinnedMesh = false;
}

void Mesh::Render(ID3D11DeviceContext* context)
{
    if (!context || !IsValid())
    {
        return;
    }

    // Apply material if available
    if (m_material)
    {
        // Note: Shader parameter would need to be passed or stored
        // m_material->Apply(context, shader);
    }

    // Set vertex buffer
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &m_stride, &m_offset);

    // Set index buffer
    if (m_indexBuffer)
    {
        context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    // Set primitive topology
    context->IASetPrimitiveTopology(m_primitiveTopology);

    // Draw
    if (m_indexBuffer && !m_indices.empty())
    {
        context->DrawIndexed(static_cast<UINT>(m_indices.size()), 0, 0);
    }
    else
    {
        context->Draw(GetVertexCount(), 0);
    }
}

void Mesh::RenderInstanced(ID3D11DeviceContext* context, int instanceCount)
{
    if (!context || !IsValid() || instanceCount <= 0)
    {
        return;
    }

    // Apply material if available
    if (m_material)
    {
        // Note: Shader parameter would need to be passed or stored
        // m_material->Apply(context, shader);
    }

    // Set vertex buffer
    context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &m_stride, &m_offset);

    // Set index buffer
    if (m_indexBuffer)
    {
        context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    // Set primitive topology
    context->IASetPrimitiveTopology(m_primitiveTopology);

    // Draw instanced
    if (m_indexBuffer && !m_indices.empty())
    {
        context->DrawIndexedInstanced(static_cast<UINT>(m_indices.size()), instanceCount, 0, 0, 0);
    }
    else
    {
        context->DrawInstanced(GetVertexCount(), instanceCount, 0, 0);
    }
}

int Mesh::GetVertexCount() const
{
    if (m_isSkinnedMesh)
    {
        return static_cast<int>(m_skinnedVertices.size());
    }
    return static_cast<int>(m_vertices.size());
}

void Mesh::CalculateNormals()
{
    if (m_isSkinnedMesh)
    {
        if (m_skinnedVertices.empty() || m_indices.empty())
            return;

        // Reset all normals to zero
        for (auto& vertex : m_skinnedVertices)
        {
            vertex.normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        // Calculate face normals and accumulate
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size())
                break;

            UINT i0 = m_indices[i];
            UINT i1 = m_indices[i + 1];
            UINT i2 = m_indices[i + 2];

            if (i0 >= m_skinnedVertices.size() || i1 >= m_skinnedVertices.size() || i2 >= m_skinnedVertices.size())
                continue;

            XMVECTOR v0 = XMLoadFloat3(&m_skinnedVertices[i0].position);
            XMVECTOR v1 = XMLoadFloat3(&m_skinnedVertices[i1].position);
            XMVECTOR v2 = XMLoadFloat3(&m_skinnedVertices[i2].position);

            XMVECTOR edge1 = XMVectorSubtract(v1, v0);
            XMVECTOR edge2 = XMVectorSubtract(v2, v0);
            XMVECTOR normal = XMVector3Cross(edge1, edge2);
            normal = XMVector3Normalize(normal);

            XMFLOAT3 normalFloat;
            XMStoreFloat3(&normalFloat, normal);

            m_skinnedVertices[i0].normal.x += normalFloat.x;
            m_skinnedVertices[i0].normal.y += normalFloat.y;
            m_skinnedVertices[i0].normal.z += normalFloat.z;

            m_skinnedVertices[i1].normal.x += normalFloat.x;
            m_skinnedVertices[i1].normal.y += normalFloat.y;
            m_skinnedVertices[i1].normal.z += normalFloat.z;

            m_skinnedVertices[i2].normal.x += normalFloat.x;
            m_skinnedVertices[i2].normal.y += normalFloat.y;
            m_skinnedVertices[i2].normal.z += normalFloat.z;
        }

        // Normalize all normals
        for (auto& vertex : m_skinnedVertices)
        {
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            normal = XMVector3Normalize(normal);
            XMStoreFloat3(&vertex.normal, normal);
        }
    }
    else
    {
        if (m_vertices.empty() || m_indices.empty())
            return;

        // Reset all normals to zero
        for (auto& vertex : m_vertices)
        {
            vertex.normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        // Calculate face normals and accumulate
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size())
                break;

            UINT i0 = m_indices[i];
            UINT i1 = m_indices[i + 1];
            UINT i2 = m_indices[i + 2];

            if (i0 >= m_vertices.size() || i1 >= m_vertices.size() || i2 >= m_vertices.size())
                continue;

            XMVECTOR v0 = XMLoadFloat3(&m_vertices[i0].position);
            XMVECTOR v1 = XMLoadFloat3(&m_vertices[i1].position);
            XMVECTOR v2 = XMLoadFloat3(&m_vertices[i2].position);

            XMVECTOR edge1 = XMVectorSubtract(v1, v0);
            XMVECTOR edge2 = XMVectorSubtract(v2, v0);
            XMVECTOR normal = XMVector3Cross(edge1, edge2);
            normal = XMVector3Normalize(normal);

            XMFLOAT3 normalFloat;
            XMStoreFloat3(&normalFloat, normal);

            m_vertices[i0].normal.x += normalFloat.x;
            m_vertices[i0].normal.y += normalFloat.y;
            m_vertices[i0].normal.z += normalFloat.z;

            m_vertices[i1].normal.x += normalFloat.x;
            m_vertices[i1].normal.y += normalFloat.y;
            m_vertices[i1].normal.z += normalFloat.z;

            m_vertices[i2].normal.x += normalFloat.x;
            m_vertices[i2].normal.y += normalFloat.y;
            m_vertices[i2].normal.z += normalFloat.z;
        }

        // Normalize all normals
        for (auto& vertex : m_vertices)
        {
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            normal = XMVector3Normalize(normal);
            XMStoreFloat3(&vertex.normal, normal);
        }
    }
}

void Mesh::CalculateTangentsAndBinormals()
{
    if (m_isSkinnedMesh)
    {
        if (m_skinnedVertices.empty() || m_indices.empty())
            return;

        // Reset tangents and binormals
        for (auto& vertex : m_skinnedVertices)
        {
            vertex.tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
            vertex.binormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        // Calculate tangents and binormals per triangle
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size())
                break;

            UINT i0 = m_indices[i];
            UINT i1 = m_indices[i + 1];
            UINT i2 = m_indices[i + 2];

            if (i0 >= m_skinnedVertices.size() || i1 >= m_skinnedVertices.size() || i2 >= m_skinnedVertices.size())
                continue;

            const auto& v0 = m_skinnedVertices[i0];
            const auto& v1 = m_skinnedVertices[i1];
            const auto& v2 = m_skinnedVertices[i2];

            XMFLOAT3 edge1(v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z);
            XMFLOAT3 edge2(v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z);

            XMFLOAT2 deltaUV1(v1.texCoord.x - v0.texCoord.x, v1.texCoord.y - v0.texCoord.y);
            XMFLOAT2 deltaUV2(v2.texCoord.x - v0.texCoord.x, v2.texCoord.y - v0.texCoord.y);

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            if (!isfinite(f))
                continue;

            XMFLOAT3 tangent(
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
            );

            XMFLOAT3 binormal(
                f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
                f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
                f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
            );

            // Accumulate tangents and binormals
            m_skinnedVertices[i0].tangent.x += tangent.x;
            m_skinnedVertices[i0].tangent.y += tangent.y;
            m_skinnedVertices[i0].tangent.z += tangent.z;
            m_skinnedVertices[i0].binormal.x += binormal.x;
            m_skinnedVertices[i0].binormal.y += binormal.y;
            m_skinnedVertices[i0].binormal.z += binormal.z;

            m_skinnedVertices[i1].tangent.x += tangent.x;
            m_skinnedVertices[i1].tangent.y += tangent.y;
            m_skinnedVertices[i1].tangent.z += tangent.z;
            m_skinnedVertices[i1].binormal.x += binormal.x;
            m_skinnedVertices[i1].binormal.y += binormal.y;
            m_skinnedVertices[i1].binormal.z += binormal.z;

            m_skinnedVertices[i2].tangent.x += tangent.x;
            m_skinnedVertices[i2].tangent.y += tangent.y;
            m_skinnedVertices[i2].tangent.z += tangent.z;
            m_skinnedVertices[i2].binormal.x += binormal.x;
            m_skinnedVertices[i2].binormal.y += binormal.y;
            m_skinnedVertices[i2].binormal.z += binormal.z;
        }

        // Normalize tangents and binormals
        for (auto& vertex : m_skinnedVertices)
        {
            XMVECTOR tangent = XMLoadFloat3(&vertex.tangent);
            XMVECTOR binormal = XMLoadFloat3(&vertex.binormal);
            tangent = XMVector3Normalize(tangent);
            binormal = XMVector3Normalize(binormal);
            XMStoreFloat3(&vertex.tangent, tangent);
            XMStoreFloat3(&vertex.binormal, binormal);
        }
    }
    else
    {
        // Similar implementation for regular vertices
        if (m_vertices.empty() || m_indices.empty())
            return;

        // Reset tangents and binormals
        for (auto& vertex : m_vertices)
        {
            vertex.tangent = XMFLOAT3(0.0f, 0.0f, 0.0f);
            vertex.binormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        // Calculate tangents and binormals per triangle
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size())
                break;

            UINT i0 = m_indices[i];
            UINT i1 = m_indices[i + 1];
            UINT i2 = m_indices[i + 2];

            if (i0 >= m_vertices.size() || i1 >= m_vertices.size() || i2 >= m_vertices.size())
                continue;

            const auto& v0 = m_vertices[i0];
            const auto& v1 = m_vertices[i1];
            const auto& v2 = m_vertices[i2];

            XMFLOAT3 edge1(v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z);
            XMFLOAT3 edge2(v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z);

            XMFLOAT2 deltaUV1(v1.texCoord.x - v0.texCoord.x, v1.texCoord.y - v0.texCoord.y);
            XMFLOAT2 deltaUV2(v2.texCoord.x - v0.texCoord.x, v2.texCoord.y - v0.texCoord.y);

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            if (!isfinite(f))
                continue;

            XMFLOAT3 tangent(
                f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
            );

            XMFLOAT3 binormal(
                f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
                f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
                f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
            );

            // Accumulate tangents and binormals
            m_vertices[i0].tangent.x += tangent.x;
            m_vertices[i0].tangent.y += tangent.y;
            m_vertices[i0].tangent.z += tangent.z;
            m_vertices[i0].binormal.x += binormal.x;
            m_vertices[i0].binormal.y += binormal.y;
            m_vertices[i0].binormal.z += binormal.z;

            m_vertices[i1].tangent.x += tangent.x;
            m_vertices[i1].tangent.y += tangent.y;
            m_vertices[i1].tangent.z += tangent.z;
            m_vertices[i1].binormal.x += binormal.x;
            m_vertices[i1].binormal.y += binormal.y;
            m_vertices[i1].binormal.z += binormal.z;

            m_vertices[i2].tangent.x += tangent.x;
            m_vertices[i2].tangent.y += tangent.y;
            m_vertices[i2].tangent.z += tangent.z;
            m_vertices[i2].binormal.x += binormal.x;
            m_vertices[i2].binormal.y += binormal.y;
            m_vertices[i2].binormal.z += binormal.z;
        }

        // Normalize tangents and binormals
        for (auto& vertex : m_vertices)
        {
            XMVECTOR tangent = XMLoadFloat3(&vertex.tangent);
            XMVECTOR binormal = XMLoadFloat3(&vertex.binormal);
            tangent = XMVector3Normalize(tangent);
            binormal = XMVector3Normalize(binormal);
            XMStoreFloat3(&vertex.tangent, tangent);
            XMStoreFloat3(&vertex.binormal, binormal);
        }
    }
}

void Mesh::OptimizeVertices()
{
    if (m_isSkinnedMesh)
    {
        // Create a map to track unique vertices
        std::unordered_map<size_t, size_t> vertexMap;
        std::vector<SkinnedVertex> optimizedVertices;
        std::vector<unsigned int> optimizedIndices;

        for (unsigned int index : m_indices)
        {
            if (index >= m_skinnedVertices.size())
                continue;

            const auto& vertex = m_skinnedVertices[index];

            // Create a simple hash for the vertex (position + normal + texcoord)
            size_t hash = 0;
            hash ^= std::hash<float>{}(vertex.position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.texCoord.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.texCoord.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            auto it = vertexMap.find(hash);
            if (it != vertexMap.end())
            {
                // Vertex already exists, reuse index
                optimizedIndices.push_back(static_cast<unsigned int>(it->second));
            }
            else
            {
                // New vertex, add to optimized list
                size_t newIndex = optimizedVertices.size();
                optimizedVertices.push_back(vertex);
                vertexMap[hash] = newIndex;
                optimizedIndices.push_back(static_cast<unsigned int>(newIndex));
            }
        }

        m_skinnedVertices = std::move(optimizedVertices);
        m_indices = std::move(optimizedIndices);
    }
    else
    {
        // Similar implementation for regular vertices
        std::unordered_map<size_t, size_t> vertexMap;
        std::vector<Vertex> optimizedVertices;
        std::vector<unsigned int> optimizedIndices;

        for (unsigned int index : m_indices)
        {
            if (index >= m_vertices.size())
                continue;

            const auto& vertex = m_vertices[index];

            // Create a simple hash for the vertex
            size_t hash = 0;
            hash ^= std::hash<float>{}(vertex.position.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.position.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.position.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.normal.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.texCoord.x) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            hash ^= std::hash<float>{}(vertex.texCoord.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

            auto it = vertexMap.find(hash);
            if (it != vertexMap.end())
            {
                optimizedIndices.push_back(static_cast<unsigned int>(it->second));
            }
            else
            {
                size_t newIndex = optimizedVertices.size();
                optimizedVertices.push_back(vertex);
                vertexMap[hash] = newIndex;
                optimizedIndices.push_back(static_cast<unsigned int>(newIndex));
            }
        }

        m_vertices = std::move(optimizedVertices);
        m_indices = std::move(optimizedIndices);
    }
}

void Mesh::FlipNormals()
{
    if (m_isSkinnedMesh)
    {
        for (auto& vertex : m_skinnedVertices)
        {
            vertex.normal.x = -vertex.normal.x;
            vertex.normal.y = -vertex.normal.y;
            vertex.normal.z = -vertex.normal.z;
        }
    }
    else
    {
        for (auto& vertex : m_vertices)
        {
            vertex.normal.x = -vertex.normal.x;
            vertex.normal.y = -vertex.normal.y;
            vertex.normal.z = -vertex.normal.z;
        }
    }

    // Also flip triangle winding order
    for (size_t i = 0; i < m_indices.size(); i += 3)
    {
        if (i + 2 < m_indices.size())
        {
            std::swap(m_indices[i + 1], m_indices[i + 2]);
        }
    }
}

void Mesh::ScaleMesh(float scale)
{
    if (m_isSkinnedMesh)
    {
        for (auto& vertex : m_skinnedVertices)
        {
            vertex.position.x *= scale;
            vertex.position.y *= scale;
            vertex.position.z *= scale;
        }
    }
    else
    {
        for (auto& vertex : m_vertices)
        {
            vertex.position.x *= scale;
            vertex.position.y *= scale;
            vertex.position.z *= scale;
        }
    }

    UpdateBoundingBox();
}

void Mesh::TransformMesh(const XMMATRIX& transform)
{
    XMMATRIX normalTransform = XMMatrixTranspose(XMMatrixInverse(nullptr, transform));

    if (m_isSkinnedMesh)
    {
        for (auto& vertex : m_skinnedVertices)
        {
            XMVECTOR pos = XMLoadFloat3(&vertex.position);
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            XMVECTOR tangent = XMLoadFloat3(&vertex.tangent);
            XMVECTOR binormal = XMLoadFloat3(&vertex.binormal);

            pos = XMVector3TransformCoord(pos, transform);
            normal = XMVector3TransformNormal(normal, normalTransform);
            tangent = XMVector3TransformNormal(tangent, normalTransform);
            binormal = XMVector3TransformNormal(binormal, normalTransform);

            normal = XMVector3Normalize(normal);
            tangent = XMVector3Normalize(tangent);
            binormal = XMVector3Normalize(binormal);

            XMStoreFloat3(&vertex.position, pos);
            XMStoreFloat3(&vertex.normal, normal);
            XMStoreFloat3(&vertex.tangent, tangent);
            XMStoreFloat3(&vertex.binormal, binormal);
        }
    }
    else
    {
        for (auto& vertex : m_vertices)
        {
            XMVECTOR pos = XMLoadFloat3(&vertex.position);
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            XMVECTOR tangent = XMLoadFloat3(&vertex.tangent);
            XMVECTOR binormal = XMLoadFloat3(&vertex.binormal);

            pos = XMVector3TransformCoord(pos, transform);
            normal = XMVector3TransformNormal(normal, normalTransform);
            tangent = XMVector3TransformNormal(tangent, normalTransform);
            binormal = XMVector3TransformNormal(binormal, normalTransform);

            normal = XMVector3Normalize(normal);
            tangent = XMVector3Normalize(tangent);
            binormal = XMVector3Normalize(binormal);

            XMStoreFloat3(&vertex.position, pos);
            XMStoreFloat3(&vertex.normal, normal);
            XMStoreFloat3(&vertex.tangent, tangent);
            XMStoreFloat3(&vertex.binormal, binormal);
        }
    }

    UpdateBoundingBox();
}

// Static utility functions for creating primitive meshes
std::shared_ptr<Mesh> Mesh::CreateCube(ID3D11Device* device, float size)
{
    float halfSize = size * 0.5f;

    std::vector<Vertex> vertices = {
        // Front face
        { XMFLOAT3(-halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },

        // Back face
        { XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

        // Left face
        { XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(-halfSize,  halfSize, -halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-halfSize, -halfSize, -halfSize), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // Right face
        { XMFLOAT3( halfSize, -halfSize, -halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( halfSize,  halfSize, -halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // Top face
        { XMFLOAT3(-halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( halfSize,  halfSize,  halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( halfSize,  halfSize, -halfSize), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },

        // Bottom face
        { XMFLOAT3(-halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( halfSize, -halfSize, -halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( halfSize, -halfSize,  halfSize), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
    };

    std::vector<unsigned int> indices = {
        // Front face
        0, 1, 2, 0, 2, 3,
        // Back face
        4, 5, 6, 4, 6, 7,
        // Left face
        8, 9, 10, 8, 10, 11,
        // Right face
        12, 13, 14, 12, 14, 15,
        // Top face
        16, 17, 18, 16, 18, 19,
        // Bottom face
        20, 21, 22, 20, 22, 23
    };

    auto mesh = std::make_shared<Mesh>();
    if (mesh->InitializeFromVertices(device, vertices, indices))
    {
        mesh->SetName("Cube");
        mesh->CalculateTangentsAndBinormals();
        return mesh;
    }
    return nullptr;
}

std::shared_ptr<Mesh> Mesh::CreateSphere(ID3D11Device* device, float radius, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Generate vertices
    for (int lat = 0; lat <= segments; ++lat)
    {
        float theta = lat * XM_PI / segments;
        float sinTheta = sinf(theta);
        float cosTheta = cosf(theta);

        for (int lon = 0; lon <= segments; ++lon)
        {
            float phi = lon * 2 * XM_PI / segments;
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);

            Vertex vertex;
            vertex.position.x = radius * sinTheta * cosPhi;
            vertex.position.y = radius * cosTheta;
            vertex.position.z = radius * sinTheta * sinPhi;

            vertex.normal.x = sinTheta * cosPhi;
            vertex.normal.y = cosTheta;
            vertex.normal.z = sinTheta * sinPhi;

            vertex.texCoord.x = (float)lon / segments;
            vertex.texCoord.y = (float)lat / segments;

            vertices.push_back(vertex);
        }
    }

    // Generate indices
    for (int lat = 0; lat < segments; ++lat)
    {
        for (int lon = 0; lon < segments; ++lon)
        {
            int current = lat * (segments + 1) + lon;
            int next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    auto mesh = std::make_shared<Mesh>();
    if (mesh->InitializeFromVertices(device, vertices, indices))
    {
        mesh->SetName("Sphere");
        mesh->CalculateTangentsAndBinormals();
        return mesh;
    }
    return nullptr;
}

std::shared_ptr<Mesh> Mesh::CreatePlane(ID3D11Device* device, float width, float height)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    std::vector<Vertex> vertices = {
        { XMFLOAT3(-halfWidth, 0.0f, -halfHeight), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-halfWidth, 0.0f,  halfHeight), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( halfWidth, 0.0f,  halfHeight), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3( halfWidth, 0.0f, -halfHeight), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) }
    };

    std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };

    auto mesh = std::make_shared<Mesh>();
    if (mesh->InitializeFromVertices(device, vertices, indices))
    {
        mesh->SetName("Plane");
        mesh->CalculateTangentsAndBinormals();
        return mesh;
    }
    return nullptr;
}

std::shared_ptr<Mesh> Mesh::CreateCylinder(ID3D11Device* device, float radius, float height, int segments)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float halfHeight = height * 0.5f;

    // Generate side vertices
    for (int i = 0; i <= segments; ++i)
    {
        float angle = (float)i / segments * 2.0f * XM_PI;
        float x = cosf(angle) * radius;
        float z = sinf(angle) * radius;

        // Top vertex
        Vertex topVertex;
        topVertex.position = XMFLOAT3(x, halfHeight, z);
        topVertex.normal = XMFLOAT3(cosf(angle), 0.0f, sinf(angle));
        topVertex.texCoord = XMFLOAT2((float)i / segments, 0.0f);
        vertices.push_back(topVertex);

        // Bottom vertex
        Vertex bottomVertex;
        bottomVertex.position = XMFLOAT3(x, -halfHeight, z);
        bottomVertex.normal = XMFLOAT3(cosf(angle), 0.0f, sinf(angle));
        bottomVertex.texCoord = XMFLOAT2((float)i / segments, 1.0f);
        vertices.push_back(bottomVertex);
    }

    // Generate side indices
    for (int i = 0; i < segments; ++i)
    {
        int topLeft = i * 2;
        int bottomLeft = i * 2 + 1;
        int topRight = (i + 1) * 2;
        int bottomRight = (i + 1) * 2 + 1;

        // Two triangles per segment
        indices.push_back(topLeft);
        indices.push_back(bottomLeft);
        indices.push_back(topRight);

        indices.push_back(topRight);
        indices.push_back(bottomLeft);
        indices.push_back(bottomRight);
    }

    auto mesh = std::make_shared<Mesh>();
    if (mesh->InitializeFromVertices(device, vertices, indices))
    {
        mesh->SetName("Cylinder");
        mesh->CalculateTangentsAndBinormals();
        return mesh;
    }
    return nullptr;
}

void Mesh::CreateBuffers(ID3D11Device* device)
{
    if (!device)
        return;

    // Create vertex buffer
    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vertexData = {};

    if (m_isSkinnedMesh && !m_skinnedVertices.empty())
    {
        vertexBufferDesc.ByteWidth = sizeof(SkinnedVertex) * static_cast<UINT>(m_skinnedVertices.size());
        vertexData.pSysMem = m_skinnedVertices.data();
    }
    else if (!m_vertices.empty())
    {
        vertexBufferDesc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(m_vertices.size());
        vertexData.pSysMem = m_vertices.data();
    }
    else
    {
        return; // No vertex data
    }

    HRESULT hr = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
    if (FAILED(hr))
    {
        std::cout << "Failed to create vertex buffer" << std::endl;
        return;
    }

    // Create index buffer if indices exist
    if (!m_indices.empty())
    {
        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned int) * static_cast<UINT>(m_indices.size());
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA indexData = {};
        indexData.pSysMem = m_indices.data();

        hr = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
        if (FAILED(hr))
        {
            std::cout << "Failed to create index buffer" << std::endl;
        }
    }
}

void Mesh::UpdateBoundingBox()
{
    if (m_isSkinnedMesh)
    {
        m_boundingBox.UpdateFromVertices(m_skinnedVertices);
    }
    else
    {
        m_boundingBox.UpdateFromVertices(m_vertices);
    }
}
