#include "ModelLoader.h"
#include "Animation.h"
#include "../Resources/Model.h"
#include "../Resources/Mesh.h"
#include "../Resources/Material.h"
#include "../Resources/Texture.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

ModelLoader::ModelLoader()
    : m_generateNormals(true)
    , m_optimizeMeshes(true)
    , m_loadAnimations(false)
    , m_generateTangents(false)
    , m_flipWindingOrder(false)
    , m_scaleFactor(1.0f)
{
}

ModelLoader::~ModelLoader()
{
}

std::shared_ptr<Model> ModelLoader::LoadFromFile(ID3D11Device* device, const std::string& filepath)
{
    if (!device || filepath.empty())
    {
        std::cerr << "ModelLoader: Invalid parameters" << std::endl;
        return nullptr;
    }

    // Read file content
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "ModelLoader: Failed to open file: " << filepath << std::endl;
        return nullptr;
    }

    // Read entire file
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), fileSize);
    file.close();

    std::string content(buffer.begin(), buffer.end());

    // Parse .X file
    XFileContext context;
    context.content = content;
    context.position = 0;
    context.isBinary = false;
    context.isCompressed = false;

    return ParseXFile(device, context, filepath);
}

std::shared_ptr<Model> ModelLoader::LoadFromMemory(ID3D11Device* device, const void* data, size_t size)
{
    if (!device || !data || size == 0)
    {
        return nullptr;
    }

    std::string content(static_cast<const char*>(data), size);

    XFileContext context;
    context.content = content;
    context.position = 0;
    context.isBinary = false;
    context.isCompressed = false;

    return ParseXFile(device, context, "memory");
}

void ModelLoader::SetGenerateNormals(bool generate)
{
    m_generateNormals = generate;
}

void ModelLoader::SetOptimizeMeshes(bool optimize)
{
    m_optimizeMeshes = optimize;
}

void ModelLoader::SetLoadAnimations(bool load)
{
    m_loadAnimations = load;
}

void ModelLoader::SetGenerateTangents(bool generate)
{
    m_generateTangents = generate;
}

void ModelLoader::SetFlipWindingOrder(bool flip)
{
    m_flipWindingOrder = flip;
}

void ModelLoader::SetScaleFactor(float scale)
{
    m_scaleFactor = scale;
}

std::shared_ptr<Model> ModelLoader::ParseXFile(ID3D11Device* device, XFileContext& context, const std::string& basePath)
{
    // Parse header
    if (!ParseXFileHeader(context))
    {
        std::cerr << "ModelLoader: Invalid .X file header" << std::endl;
        return nullptr;
    }

    auto model = std::make_shared<Model>();
    if (!model->Initialize(device))
    {
        std::cerr << "ModelLoader: Failed to initialize model" << std::endl;
        return nullptr;
    }

    // Skip templates section (we use hardcoded knowledge of templates)
    SkipTemplates(context);

    // Parse main data
    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        if (context.position >= context.content.length())
            break;

        std::string token = ReadToken(context);
        if (token.empty())
            break;

        if (token == "Mesh")
        {
            auto mesh = ParseMesh(device, context, basePath);
            if (mesh)
            {
                model->AddMesh(mesh);
            }
        }
        else if (token == "Frame")
        {
            ParseFrame(device, context, model, basePath);
        }
        else if (token == "Material")
        {
            auto material = ParseMaterial(device, context, basePath);
            if (material)
            {
                model->AddMaterial(material);
            }
        }
        else if (token == "AnimationSet" && m_loadAnimations)
        {
            ParseAnimationSet(context, model);
        }
        else
        {
            // Skip unknown objects
            SkipObject(context);
        }
    }

    // Post-processing
    if (m_generateNormals)
    {
        GenerateNormals(model);
    }

    if (m_generateTangents)
    {
        GenerateTangents(model);
    }

    if (m_optimizeMeshes)
    {
        OptimizeMeshes(model);
    }

    std::cout << "ModelLoader: Loaded model with " << model->GetMeshCount()
              << " meshes and " << model->GetMaterialCount() << " materials" << std::endl;

    return model;
}

bool ModelLoader::ParseXFileHeader(XFileContext& context)
{
    if (context.content.length() < 16)
        return false;

    // Check magic number
    if (context.content.substr(0, 4) != "xof ")
        return false;

    // Parse version
    std::string majorVersion = context.content.substr(4, 2);
    std::string minorVersion = context.content.substr(6, 2);

    // Parse format
    std::string format = context.content.substr(8, 4);
    context.isBinary = (format == "bin " || format == "bzip");
    context.isCompressed = (format == "tzip" || format == "bzip");

    if (context.isCompressed)
    {
        std::cerr << "ModelLoader: Compressed .X files not supported yet" << std::endl;
        return false;
    }

    std::cout << "ModelLoader: File format - " << (context.isBinary ? "Binary" : "Text") << std::endl;

    // Parse float size
    std::string floatSize = context.content.substr(12, 4);

    context.position = 16;
    return true;
}

void ModelLoader::SkipTemplates(XFileContext& context)
{
    // Templates are usually at the beginning, skip them
    // This is a simplified approach - in a full implementation,
    // you would parse templates to understand the data structure

    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        std::string token = ReadToken(context);

        if (token == "template")
        {
            SkipObject(context);
        }
        else
        {
            // Put the token back by moving position back
            context.position -= token.length();
            break;
        }
    }
}

std::shared_ptr<Mesh> ModelLoader::ParseMesh(ID3D11Device* device, XFileContext& context, const std::string& basePath)
{
    std::string meshName;

    if (context.isBinary)
    {
        meshName = ReadStringBinary(context);
        // Binary format doesn't use braces, data follows directly
    }
    else
    {
        // Expect: Mesh meshName {
        meshName = ReadToken(context);
        SkipWhitespace(context);

        if (ReadChar(context) != '{')
        {
            std::cerr << "ModelLoader: Expected '{' after Mesh name" << std::endl;
            return nullptr;
        }
    }

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->Initialize(device))
    {
        return nullptr;
    }

    mesh->SetName(meshName);

    // Parse vertex count
    int vertexCount;
    if (context.isBinary)
    {
        vertexCount = static_cast<int>(ReadUInt32Binary(context));
    }
    else
    {
        SkipWhitespace(context);
        vertexCount = ReadInt(context);
    }

    if (vertexCount <= 0)
    {
        std::cerr << "ModelLoader: Invalid vertex count: " << vertexCount << std::endl;
        return nullptr;
    }

    // Parse vertices
    std::vector<XMFLOAT3> positions;
    positions.reserve(vertexCount);

    for (int i = 0; i < vertexCount; ++i)
    {
        float x, y, z;

        if (context.isBinary)
        {
            x = ReadFloatBinary(context) * m_scaleFactor;
            y = ReadFloatBinary(context) * m_scaleFactor;
            z = ReadFloatBinary(context) * m_scaleFactor;
        }
        else
        {
            SkipWhitespace(context);
            x = ReadFloat(context) * m_scaleFactor;
            SkipChar(context, ';');
            y = ReadFloat(context) * m_scaleFactor;
            SkipChar(context, ';');
            z = ReadFloat(context) * m_scaleFactor;

            if (i < vertexCount - 1)
            {
                SkipChar(context, ',');
            }
            SkipChar(context, ';');
        }

        positions.push_back(XMFLOAT3(x, y, z));
    }

    // Parse face count
    int faceCount;
    if (context.isBinary)
    {
        faceCount = static_cast<int>(ReadUInt32Binary(context));
    }
    else
    {
        SkipWhitespace(context);
        faceCount = ReadInt(context);
    }

    // Parse faces (indices)
    std::vector<uint32_t> indices;

    for (int i = 0; i < faceCount; ++i)
    {
        SkipWhitespace(context);
        int verticesPerFace = ReadInt(context);
        SkipChar(context, ';');

        if (verticesPerFace == 3) // Triangle
        {
            for (int j = 0; j < 3; ++j)
            {
                uint32_t index = ReadInt(context);
                indices.push_back(index);

                if (j < 2)
                    SkipChar(context, ',');
            }
        }
        else if (verticesPerFace == 4) // Quad - split into two triangles
        {
            uint32_t idx[4];
            for (int j = 0; j < 4; ++j)
            {
                idx[j] = ReadInt(context);
                if (j < 3)
                    SkipChar(context, ',');
            }

            // First triangle
            indices.push_back(idx[0]);
            indices.push_back(idx[1]);
            indices.push_back(idx[2]);

            // Second triangle
            indices.push_back(idx[0]);
            indices.push_back(idx[2]);
            indices.push_back(idx[3]);
        }

        if (i < faceCount - 1)
        {
            SkipChar(context, ',');
        }
        SkipChar(context, ';');
    }

    // Create vertices with positions
    std::vector<Vertex> vertices;
    vertices.reserve(positions.size());

    for (const auto& pos : positions)
    {
        Vertex vertex;
        vertex.position = pos;
        vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f); // Default normal
        vertex.texCoord = XMFLOAT2(0.0f, 0.0f); // Default UV
        vertex.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f); // Default tangent
        vertex.binormal = XMFLOAT3(0.0f, 0.0f, 1.0f); // Default binormal
        vertices.push_back(vertex);
    }

    // Apply winding order flip if requested
    if (m_flipWindingOrder)
    {
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            std::swap(indices[i + 1], indices[i + 2]);
        }
    }

    // Set mesh data
    mesh->SetVertices(vertices);
    mesh->SetIndices(indices);

    // Parse optional data (normals, texture coordinates, materials)
    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        if (context.position >= context.content.length())
            break;

        char nextChar = PeekChar(context);
        if (nextChar == '}')
        {
            ReadChar(context); // consume '}'
            break;
        }

        std::string token = ReadToken(context);
        if (token == "MeshNormals")
        {
            ParseMeshNormals(context, mesh);
        }
        else if (token == "MeshTextureCoords")
        {
            ParseMeshTextureCoords(context, mesh);
        }
        else if (token == "MeshMaterialList")
        {
            ParseMeshMaterialList(device, context, mesh, basePath);
        }
        else
        {
            SkipObject(context);
        }
    }

    return mesh;
}

void ModelLoader::ParseMeshNormals(XFileContext& context, std::shared_ptr<Mesh> mesh)
{
    SkipWhitespace(context);
    SkipChar(context, '{');

    // Read normal count
    int normalCount = ReadInt(context);
    std::vector<XMFLOAT3> normals;
    normals.reserve(normalCount);

    // Read normals
    for (int i = 0; i < normalCount; ++i)
    {
        SkipWhitespace(context);
        float x = ReadFloat(context);
        SkipChar(context, ';');
        float y = ReadFloat(context);
        SkipChar(context, ';');
        float z = ReadFloat(context);

        normals.push_back(XMFLOAT3(x, y, z));

        if (i < normalCount - 1)
            SkipChar(context, ',');
        SkipChar(context, ';');
    }

    // Read face normal indices (usually we just use per-vertex normals)
    int faceCount = ReadInt(context);
    for (int i = 0; i < faceCount; ++i)
    {
        SkipWhitespace(context);
        int verticesPerFace = ReadInt(context);
        SkipChar(context, ';');

        for (int j = 0; j < verticesPerFace; ++j)
        {
            ReadInt(context); // Skip face normal indices
            if (j < verticesPerFace - 1)
                SkipChar(context, ',');
        }

        if (i < faceCount - 1)
            SkipChar(context, ',');
        SkipChar(context, ';');
    }

    // Apply normals to mesh vertices
    auto vertices = mesh->GetVertices();
    if (normals.size() == vertices.size())
    {
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vertices[i].normal = normals[i];
        }
        mesh->SetVertices(vertices);
    }

    SkipChar(context, '}');
}

void ModelLoader::ParseMeshTextureCoords(XFileContext& context, std::shared_ptr<Mesh> mesh)
{
    SkipWhitespace(context);
    SkipChar(context, '{');

    // Read texture coordinate count
    int texCoordCount = ReadInt(context);

    // Read texture coordinates
    std::vector<XMFLOAT2> texCoords;
    texCoords.reserve(texCoordCount);

    for (int i = 0; i < texCoordCount; ++i)
    {
        SkipWhitespace(context);
        float u = ReadFloat(context);
        SkipChar(context, ';');
        float v = ReadFloat(context);

        texCoords.push_back(XMFLOAT2(u, v));

        if (i < texCoordCount - 1)
            SkipChar(context, ',');
        SkipChar(context, ';');
    }

    // Apply texture coordinates to mesh vertices
    auto vertices = mesh->GetVertices();
    if (texCoords.size() == vertices.size())
    {
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vertices[i].texCoord = texCoords[i];
        }
        mesh->SetVertices(vertices);
    }

    SkipChar(context, '}');
}

// Utility functions for parsing
void ModelLoader::SkipWhitespace(XFileContext& context)
{
    while (context.position < context.content.length() &&
           std::isspace(context.content[context.position]))
    {
        context.position++;
    }
}

std::string ModelLoader::ReadToken(XFileContext& context)
{
    SkipWhitespace(context);

    std::string token;
    while (context.position < context.content.length())
    {
        char c = context.content[context.position];
        if (std::isalnum(c) || c == '_')
        {
            token += c;
            context.position++;
        }
        else
        {
            break;
        }
    }

    return token;
}

char ModelLoader::ReadChar(XFileContext& context)
{
    if (context.position < context.content.length())
    {
        return context.content[context.position++];
    }
    return 0;
}

char ModelLoader::PeekChar(XFileContext& context)
{
    if (context.position < context.content.length())
    {
        return context.content[context.position];
    }
    return 0;
}

void ModelLoader::SkipChar(XFileContext& context, char expected)
{
    SkipWhitespace(context);
    if (context.position < context.content.length() &&
        context.content[context.position] == expected)
    {
        context.position++;
    }
}

int ModelLoader::ReadInt(XFileContext& context)
{
    SkipWhitespace(context);

    std::string numberStr;
    while (context.position < context.content.length())
    {
        char c = context.content[context.position];
        if (std::isdigit(c) || c == '-')
        {
            numberStr += c;
            context.position++;
        }
        else
        {
            break;
        }
    }

    return numberStr.empty() ? 0 : std::stoi(numberStr);
}

float ModelLoader::ReadFloat(XFileContext& context)
{
    SkipWhitespace(context);

    std::string numberStr;
    while (context.position < context.content.length())
    {
        char c = context.content[context.position];
        if (std::isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+')
        {
            numberStr += c;
            context.position++;
        }
        else
        {
            break;
        }
    }

    return numberStr.empty() ? 0.0f : std::stof(numberStr);
}

void ModelLoader::SkipObject(XFileContext& context)
{
    SkipWhitespace(context);

    // Skip until we find opening brace
    while (context.position < context.content.length() &&
           context.content[context.position] != '{')
    {
        context.position++;
    }

    if (context.position >= context.content.length())
        return;

    context.position++; // Skip '{'

    int braceLevel = 1;
    while (context.position < context.content.length() && braceLevel > 0)
    {
        char c = context.content[context.position++];
        if (c == '{')
            braceLevel++;
        else if (c == '}')
            braceLevel--;
    }
}

std::shared_ptr<Material> ModelLoader::ParseMaterial(ID3D11Device* device, XFileContext& context, const std::string& basePath)
{
    std::string materialName = ReadToken(context);
    SkipWhitespace(context);
    SkipChar(context, '{');

    auto material = std::make_shared<Material>(materialName);
    if (!material->Initialize(device))
    {
        SkipObject(context);
        return nullptr;
    }

    // Default material properties
    XMFLOAT4 diffuseColor(0.8f, 0.8f, 0.8f, 1.0f);
    XMFLOAT4 specularColor(1.0f, 1.0f, 1.0f, 1.0f);
    XMFLOAT4 emissiveColor(0.0f, 0.0f, 0.0f, 1.0f);
    float shininess = 32.0f;

    // Parse material properties
    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        if (PeekChar(context) == '}')
        {
            ReadChar(context); // consume '}'
            break;
        }

        // Read diffuse color (first 4 floats)
        diffuseColor.x = ReadFloat(context); SkipChar(context, ';');
        diffuseColor.y = ReadFloat(context); SkipChar(context, ';');
        diffuseColor.z = ReadFloat(context); SkipChar(context, ';');
        diffuseColor.w = ReadFloat(context); SkipChar(context, ';');

        // Read power (shininess)
        shininess = ReadFloat(context); SkipChar(context, ';');

        // Read specular color
        specularColor.x = ReadFloat(context); SkipChar(context, ';');
        specularColor.y = ReadFloat(context); SkipChar(context, ';');
        specularColor.z = ReadFloat(context); SkipChar(context, ';');

        // Read emissive color
        emissiveColor.x = ReadFloat(context); SkipChar(context, ';');
        emissiveColor.y = ReadFloat(context); SkipChar(context, ';');
        emissiveColor.z = ReadFloat(context); SkipChar(context, ';');

        // Skip any additional data or TextureFilename objects
        while (context.position < context.content.length())
        {
            SkipWhitespace(context);
            char nextChar = PeekChar(context);
            if (nextChar == '}')
                break;

            std::string token = ReadToken(context);
            if (token == "TextureFilename")
            {
                // Parse texture filename
                SkipWhitespace(context);
                SkipChar(context, '{');

                // Read string (texture path)
                SkipWhitespace(context);
                if (PeekChar(context) == '"')
                {
                    ReadChar(context); // skip opening quote
                    std::string texturePath;
                    while (context.position < context.content.length() &&
                           context.content[context.position] != '"')
                    {
                        texturePath += context.content[context.position++];
                    }
                    ReadChar(context); // skip closing quote

                    // Try to load texture
                    std::string fullPath = basePath + "/" + texturePath;
                    // Note: Actual texture loading would require TextureManager integration
                }

                SkipChar(context, '}');
            }
            else
            {
                SkipObject(context);
            }
        }
        break; // Exit after parsing material data
    }

    // Apply parsed properties to material
    material->SetDiffuseColor(diffuseColor);
    material->SetSpecularColor(specularColor);
    material->SetEmissiveColor(emissiveColor);
    material->SetShininess(shininess);

    return material;
}

void ModelLoader::ParseFrame(ID3D11Device* device, XFileContext& context, std::shared_ptr<Model> model, const std::string& basePath)
{
    std::string frameName = ReadToken(context);
    SkipWhitespace(context);
    SkipChar(context, '{');

    // Parse frame contents
    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        if (PeekChar(context) == '}')
        {
            ReadChar(context); // consume '}'
            break;
        }

        std::string token = ReadToken(context);

        if (token == "FrameTransformMatrix")
        {
            // Parse transformation matrix
            SkipWhitespace(context);
            SkipChar(context, '{');

            // Read 16 matrix elements (4x4 matrix)
            for (int i = 0; i < 16; ++i)
            {
                float value = ReadFloat(context);
                if (i < 15)
                    SkipChar(context, ',');
                SkipChar(context, ';');
            }

            SkipChar(context, '}');
        }
        else if (token == "Mesh")
        {
            // Parse mesh within this frame
            auto mesh = ParseMesh(device, context, basePath);
            if (mesh)
            {
                mesh->SetName(frameName + "_Mesh");
                model->AddMesh(mesh);
            }
        }
        else if (token == "Frame")
        {
            // Recursive frame parsing (child frames)
            ParseFrame(device, context, model, basePath);
        }
        else
        {
            SkipObject(context);
        }
    }
}

void ModelLoader::ParseMeshMaterialList(ID3D11Device* device, XFileContext& context, std::shared_ptr<Mesh> mesh, const std::string& basePath)
{
    SkipWhitespace(context);
    SkipChar(context, '{');

    // Read material count
    int materialCount = ReadInt(context);

    // Read face count
    int faceCount = ReadInt(context);

    // Read material indices per face
    std::vector<int> faceMatIndices;
    faceMatIndices.reserve(faceCount);

    for (int i = 0; i < faceCount; ++i)
    {
        SkipWhitespace(context);
        int materialIndex = ReadInt(context);
        faceMatIndices.push_back(materialIndex);

        if (i < faceCount - 1)
            SkipChar(context, ',');
        SkipChar(context, ';');
    }

    // Parse material definitions or references
    for (int i = 0; i < materialCount; ++i)
    {
        SkipWhitespace(context);
        std::string token = ReadToken(context);

        if (token == "Material")
        {
            auto material = ParseMaterial(device, context, basePath);
            if (material && i == 0) // Use first material for now
            {
                mesh->SetMaterial(material);
            }
        }
        else if (token == "{")
        {
            // Reference to existing material by name
            std::string materialName = ReadToken(context);
            SkipChar(context, '}');
            // Would need material database lookup here
        }
        else
        {
            SkipObject(context);
        }
    }

    SkipChar(context, '}');
}

void ModelLoader::ParseAnimationSet(XFileContext& context, std::shared_ptr<Model> model)
{
    std::string animationName = ReadToken(context);
    SkipWhitespace(context);
    SkipChar(context, '{');

    // Create animation
    auto animation = std::make_shared<Animation>();
    animation->Initialize(animationName, 0.0f, 25.0f); // Default values, will be updated

    std::vector<AnimationChannel> channels;

    // Parse animation content
    while (context.position < context.content.length())
    {
        SkipWhitespace(context);
        if (PeekChar(context) == '}')
        {
            ReadChar(context); // consume '}'
            break;
        }

        std::string token = ReadToken(context);

        if (token == "Animation")
        {
            // Parse individual animation for a bone
            std::string boneName = ReadToken(context);
            SkipWhitespace(context);
            SkipChar(context, '{');

            AnimationChannel channel;
            channel.boneName = boneName;

            // Parse animation keys
            while (context.position < context.content.length())
            {
                SkipWhitespace(context);
                if (PeekChar(context) == '}')
                {
                    ReadChar(context); // consume '}'
                    break;
                }

                std::string keyToken = ReadToken(context);

                if (keyToken == "AnimationKey")
                {
                    SkipWhitespace(context);
                    SkipChar(context, '{');

                    // Read key type (0=rotation, 1=scale, 2=position)
                    int keyType = ReadInt(context);

                    // Read number of keys
                    int numKeys = ReadInt(context);

                    for (int i = 0; i < numKeys; ++i)
                    {
                        // Read time
                        float time = ReadFloat(context);
                        SkipChar(context, ';');

                        // Read number of values
                        int numValues = ReadInt(context);
                        SkipChar(context, ';');

                        if (keyType == 2) // Position
                        {
                            if (numValues >= 3)
                            {
                                float x = ReadFloat(context); SkipChar(context, ',');
                                float y = ReadFloat(context); SkipChar(context, ',');
                                float z = ReadFloat(context);

                                AnimationKey<XMVECTOR> key(time, XMVectorSet(x, y, z, 0.0f));
                                channel.positionKeys.push_back(key);
                            }
                        }
                        else if (keyType == 0) // Rotation (quaternion)
                        {
                            if (numValues >= 4)
                            {
                                float w = ReadFloat(context); SkipChar(context, ',');
                                float x = ReadFloat(context); SkipChar(context, ',');
                                float y = ReadFloat(context); SkipChar(context, ',');
                                float z = ReadFloat(context);

                                AnimationKey<XMVECTOR> key(time, XMVectorSet(x, y, z, w));
                                channel.rotationKeys.push_back(key);
                            }
                        }
                        else if (keyType == 1) // Scale
                        {
                            if (numValues >= 3)
                            {
                                float x = ReadFloat(context); SkipChar(context, ',');
                                float y = ReadFloat(context); SkipChar(context, ',');
                                float z = ReadFloat(context);

                                AnimationKey<XMVECTOR> key(time, XMVectorSet(x, y, z, 0.0f));
                                channel.scaleKeys.push_back(key);
                            }
                        }

                        // Skip remaining values and separators
                        for (int j = 3; j < numValues; ++j)
                        {
                            ReadFloat(context);
                            if (j < numValues - 1) SkipChar(context, ',');
                        }

                        if (i < numKeys - 1) SkipChar(context, ',');
                        SkipChar(context, ';');
                    }

                    SkipChar(context, '}');
                }
                else
                {
                    SkipObject(context);
                }
            }

            if (!channel.positionKeys.empty() || !channel.rotationKeys.empty() || !channel.scaleKeys.empty())
            {
                channels.push_back(channel);
            }
        }
        else
        {
            SkipObject(context);
        }
    }

    // Add channels to animation
    for (const auto& channel : channels)
    {
        animation->AddChannel(channel);
    }

    // Calculate animation duration
    float maxTime = 0.0f;
    for (const auto& channel : channels)
    {
        for (const auto& key : channel.positionKeys)
        {
            maxTime = std::max(maxTime, key.time);
        }
        for (const auto& key : channel.rotationKeys)
        {
            maxTime = std::max(maxTime, key.time);
        }
        for (const auto& key : channel.scaleKeys)
        {
            maxTime = std::max(maxTime, key.time);
        }
    }

    if (maxTime > 0.0f)
    {
        animation->Initialize(animationName, maxTime, 25.0f);
        // Note: Would need to add animation to model here
        // model->AddAnimation(animation);

        std::cout << "ModelLoader: Loaded animation '" << animationName
                  << "' with duration " << maxTime << " and "
                  << channels.size() << " channels" << std::endl;
    }
}

// Post-processing functions
void ModelLoader::GenerateNormals(std::shared_ptr<Model> model)
{
    // Generate smooth normals for meshes that don't have them
    // This is a simplified implementation
    for (size_t i = 0; i < model->GetMeshCount(); ++i)
    {
        auto mesh = model->GetMesh(i);
        auto vertices = mesh->GetVertices();
        auto indices = mesh->GetIndices();

        // Reset normals
        for (auto& vertex : vertices)
        {
            vertex.normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }

        // Calculate face normals and accumulate
        for (size_t j = 0; j < indices.size(); j += 3)
        {
            uint32_t i0 = indices[j];
            uint32_t i1 = indices[j + 1];
            uint32_t i2 = indices[j + 2];

            XMVECTOR v0 = XMLoadFloat3(&vertices[i0].position);
            XMVECTOR v1 = XMLoadFloat3(&vertices[i1].position);
            XMVECTOR v2 = XMLoadFloat3(&vertices[i2].position);

            XMVECTOR edge1 = XMVectorSubtract(v1, v0);
            XMVECTOR edge2 = XMVectorSubtract(v2, v0);
            XMVECTOR normal = XMVector3Cross(edge1, edge2);
            normal = XMVector3Normalize(normal);

            XMFLOAT3 normalFloat;
            XMStoreFloat3(&normalFloat, normal);

            // Add to each vertex
            vertices[i0].normal.x += normalFloat.x;
            vertices[i0].normal.y += normalFloat.y;
            vertices[i0].normal.z += normalFloat.z;

            vertices[i1].normal.x += normalFloat.x;
            vertices[i1].normal.y += normalFloat.y;
            vertices[i1].normal.z += normalFloat.z;

            vertices[i2].normal.x += normalFloat.x;
            vertices[i2].normal.y += normalFloat.y;
            vertices[i2].normal.z += normalFloat.z;
        }

        // Normalize accumulated normals
        for (auto& vertex : vertices)
        {
            XMVECTOR normal = XMLoadFloat3(&vertex.normal);
            normal = XMVector3Normalize(normal);
            XMStoreFloat3(&vertex.normal, normal);
        }

        mesh->SetVertices(vertices);
    }
}

void ModelLoader::GenerateTangents(std::shared_ptr<Model> model)
{
    // Generate tangents for normal mapping - simplified implementation
    // In a full implementation, this would calculate proper tangent space
}

void ModelLoader::OptimizeMeshes(std::shared_ptr<Model> model)
{
    // Optimize meshes by removing duplicate vertices, etc.
    // This is a placeholder for optimization algorithms
}

// Binary file reading implementations
template<typename T>
T ModelLoader::ReadBinary(XFileContext& context)
{
    if (context.position + sizeof(T) > context.content.length())
    {
        throw std::runtime_error("ModelLoader: Unexpected end of binary data");
    }

    T value;
    std::memcpy(&value, context.content.data() + context.position, sizeof(T));
    context.position += sizeof(T);
    return value;
}

uint16_t ModelLoader::ReadUInt16Binary(XFileContext& context)
{
    return ReadBinary<uint16_t>(context);
}

uint32_t ModelLoader::ReadUInt32Binary(XFileContext& context)
{
    return ReadBinary<uint32_t>(context);
}

float ModelLoader::ReadFloatBinary(XFileContext& context)
{
    return ReadBinary<float>(context);
}

std::string ModelLoader::ReadStringBinary(XFileContext& context)
{
    uint32_t length = ReadUInt32Binary(context);
    if (length == 0) return "";

    if (context.position + length > context.content.length())
    {
        throw std::runtime_error("ModelLoader: String length exceeds available data");
    }

    std::string result(context.content.data() + context.position, length);
    context.position += length;

    // Skip null terminator if present
    if (context.position < context.content.length() &&
        context.content[context.position] == '\0')
    {
        context.position++;
    }

    return result;
}

ModelLoader::BinaryToken ModelLoader::ReadBinaryToken(XFileContext& context)
{
    BinaryToken token;
    token.type = ReadUInt16Binary(context);
    token.size = ReadUInt16Binary(context);
    return token;
}
