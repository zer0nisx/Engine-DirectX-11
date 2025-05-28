#include "Model.h"
#include "Mesh.h"
#include "Material.h"
#include "../Graphics/ModelLoader.h"
#include <iostream>
#include <algorithm>

// Animation implementation
XMMATRIX Animation::GetBoneTransform(int boneIndex, float timeInSeconds) const
{
    if (boneIndex < 0 || boneIndex >= static_cast<int>(channels.size()))
    {
        return XMMatrixIdentity();
    }

    const auto& channel = channels[boneIndex];

    // Get position
    XMVECTOR position = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    if (!channel.positionKeys.empty())
    {
        if (channel.positionKeys.size() == 1)
        {
            position = XMLoadFloat3(&channel.positionKeys[0].position);
        }
        else
        {
            // Find surrounding keyframes
            int keyIndex = 0;
            for (size_t i = 0; i < channel.positionKeys.size() - 1; ++i)
            {
                if (timeInSeconds >= channel.positionKeys[i].time &&
                    timeInSeconds <= channel.positionKeys[i + 1].time)
                {
                    keyIndex = static_cast<int>(i);
                    break;
                }
            }

            if (keyIndex < static_cast<int>(channel.positionKeys.size()) - 1)
            {
                const auto& key1 = channel.positionKeys[keyIndex];
                const auto& key2 = channel.positionKeys[keyIndex + 1];

                float deltaTime = key2.time - key1.time;
                if (deltaTime > 0.0f)
                {
                    float factor = (timeInSeconds - key1.time) / deltaTime;
                    factor = std::max(0.0f, std::min(1.0f, factor));

                    XMVECTOR pos1 = XMLoadFloat3(&key1.position);
                    XMVECTOR pos2 = XMLoadFloat3(&key2.position);
                    position = XMVectorLerp(pos1, pos2, factor);
                }
                else
                {
                    position = XMLoadFloat3(&key1.position);
                }
            }
            else
            {
                position = XMLoadFloat3(&channel.positionKeys.back().position);
            }
        }
    }

    // Get rotation
    XMVECTOR rotation = XMQuaternionIdentity();
    if (!channel.rotationKeys.empty())
    {
        if (channel.rotationKeys.size() == 1)
        {
            rotation = XMLoadFloat4(&channel.rotationKeys[0].rotation);
        }
        else
        {
            // Find surrounding keyframes
            int keyIndex = 0;
            for (size_t i = 0; i < channel.rotationKeys.size() - 1; ++i)
            {
                if (timeInSeconds >= channel.rotationKeys[i].time &&
                    timeInSeconds <= channel.rotationKeys[i + 1].time)
                {
                    keyIndex = static_cast<int>(i);
                    break;
                }
            }

            if (keyIndex < static_cast<int>(channel.rotationKeys.size()) - 1)
            {
                const auto& key1 = channel.rotationKeys[keyIndex];
                const auto& key2 = channel.rotationKeys[keyIndex + 1];

                float deltaTime = key2.time - key1.time;
                if (deltaTime > 0.0f)
                {
                    float factor = (timeInSeconds - key1.time) / deltaTime;
                    factor = std::max(0.0f, std::min(1.0f, factor));

                    XMVECTOR rot1 = XMLoadFloat4(&key1.rotation);
                    XMVECTOR rot2 = XMLoadFloat4(&key2.rotation);
                    rotation = XMQuaternionSlerp(rot1, rot2, factor);
                }
                else
                {
                    rotation = XMLoadFloat4(&key1.rotation);
                }
            }
            else
            {
                rotation = XMLoadFloat4(&channel.rotationKeys.back().rotation);
            }
        }
    }

    // Get scale
    XMVECTOR scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    if (!channel.scaleKeys.empty())
    {
        if (channel.scaleKeys.size() == 1)
        {
            scale = XMLoadFloat3(&channel.scaleKeys[0].scale);
        }
        else
        {
            // Find surrounding keyframes
            int keyIndex = 0;
            for (size_t i = 0; i < channel.scaleKeys.size() - 1; ++i)
            {
                if (timeInSeconds >= channel.scaleKeys[i].time &&
                    timeInSeconds <= channel.scaleKeys[i + 1].time)
                {
                    keyIndex = static_cast<int>(i);
                    break;
                }
            }

            if (keyIndex < static_cast<int>(channel.scaleKeys.size()) - 1)
            {
                const auto& key1 = channel.scaleKeys[keyIndex];
                const auto& key2 = channel.scaleKeys[keyIndex + 1];

                float deltaTime = key2.time - key1.time;
                if (deltaTime > 0.0f)
                {
                    float factor = (timeInSeconds - key1.time) / deltaTime;
                    factor = std::max(0.0f, std::min(1.0f, factor));

                    XMVECTOR scale1 = XMLoadFloat3(&key1.scale);
                    XMVECTOR scale2 = XMLoadFloat3(&key2.scale);
                    scale = XMVectorLerp(scale1, scale2, factor);
                }
                else
                {
                    scale = XMLoadFloat3(&key1.scale);
                }
            }
            else
            {
                scale = XMLoadFloat3(&channel.scaleKeys.back().scale);
            }
        }
    }

    // Combine transformations: Scale * Rotation * Translation
    XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scale);
    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(position);

    return scaleMatrix * rotationMatrix * translationMatrix;
}

XMMATRIX Animation::GetBoneTransform(const std::string& boneName, float timeInSeconds) const
{
    for (int i = 0; i < static_cast<int>(channels.size()); ++i)
    {
        if (channels[i].boneName == boneName)
        {
            return GetBoneTransform(i, timeInSeconds);
        }
    }
    return XMMatrixIdentity();
}

// SkinInfo implementation
int SkinInfo::FindBoneIndex(const std::string& boneName) const
{
    auto it = boneNameToIndex.find(boneName);
    if (it != boneNameToIndex.end())
    {
        return it->second;
    }
    return -1;
}

void SkinInfo::AddBone(const Bone& bone)
{
    int index = static_cast<int>(bones.size());
    bones.push_back(bone);
    boneNameToIndex[bone.name] = index;
}

XMMATRIX SkinInfo::GetBoneMatrix(int boneIndex, const std::vector<XMMATRIX>& currentPose) const
{
    if (boneIndex < 0 || boneIndex >= static_cast<int>(bones.size()) ||
        boneIndex >= static_cast<int>(currentPose.size()))
    {
        return XMMatrixIdentity();
    }

    return bones[boneIndex].offsetMatrix * currentPose[boneIndex];
}

// Model implementation
Model::Model()
    : m_currentAnimationIndex(-1)
    , m_currentAnimationTime(0.0f)
    , m_isAnimationPaused(false)
    , m_loopAnimation(true)
    , m_worldTransform(XMMatrixIdentity())
    , m_isLoaded(false)
    , m_boundingBoxDirty(true)
{
}

Model::~Model()
{
    Shutdown();
}

bool Model::LoadFromFile(ID3D11Device* device, const std::string& filepath)
{
    if (!device || filepath.empty())
    {
        return false;
    }

    Shutdown(); // Clean up existing data

    m_filepath = filepath;

    // Use ModelLoader to load the model
    // Note: This would use the actual ModelLoader implementation
    // For now, this is a placeholder that would be completed when ModelLoader.cpp is implemented

    std::cout << "Model::LoadFromFile - Loading: " << filepath << std::endl;
    std::cout << "Note: ModelLoader.cpp implementation needed for actual .x file parsing" << std::endl;

    // Create a simple test model for now
    CreateTestModel(device);

    return m_isLoaded;
}

void Model::Shutdown()
{
    m_meshes.clear();
    m_materials.clear();
    m_animations.clear();
    m_skinInfo = SkinInfo();
    m_boneMatrices.clear();
    m_finalBoneMatrices.clear();

    m_currentAnimationIndex = -1;
    m_currentAnimationTime = 0.0f;
    m_isAnimationPaused = false;
    m_worldTransform = XMMatrixIdentity();
    m_isLoaded = false;
    m_boundingBoxDirty = true;
    m_name.clear();
    m_filepath.clear();
}

void Model::Render(ID3D11DeviceContext* context)
{
    if (!context || !IsValid())
    {
        return;
    }

    for (auto& mesh : m_meshes)
    {
        if (mesh)
        {
            mesh->Render(context);
        }
    }
}

void Model::RenderWithMaterials(ID3D11DeviceContext* context, Shader* shader)
{
    if (!context || !shader || !IsValid())
    {
        return;
    }

    for (auto& mesh : m_meshes)
    {
        if (mesh)
        {
            // Apply material if mesh has one
            auto material = mesh->GetMaterial();
            if (material)
            {
                material->Apply(context, shader);
            }
            else if (mesh->GetMaterialIndex() >= 0 &&
                     mesh->GetMaterialIndex() < static_cast<int>(m_materials.size()))
            {
                // Use material from model's material list
                m_materials[mesh->GetMaterialIndex()]->Apply(context, shader);
            }

            mesh->Render(context);
        }
    }
}

void Model::UpdateAnimation(float deltaTime)
{
    if (!IsAnimated() || m_isAnimationPaused || m_currentAnimationIndex < 0 ||
        m_currentAnimationIndex >= static_cast<int>(m_animations.size()))
    {
        return;
    }

    const auto& animation = m_animations[m_currentAnimationIndex];

    m_currentAnimationTime += deltaTime;

    // Handle looping
    if (m_loopAnimation && m_currentAnimationTime > animation.duration)
    {
        m_currentAnimationTime = fmod(m_currentAnimationTime, animation.duration);
    }
    else if (m_currentAnimationTime > animation.duration)
    {
        m_currentAnimationTime = animation.duration;
    }

    UpdateBoneMatrices();
}

void Model::SetAnimation(const std::string& animationName)
{
    for (int i = 0; i < static_cast<int>(m_animations.size()); ++i)
    {
        if (m_animations[i].name == animationName)
        {
            SetAnimation(i);
            return;
        }
    }

    std::cout << "Animation not found: " << animationName << std::endl;
}

void Model::SetAnimation(int animationIndex)
{
    if (animationIndex >= 0 && animationIndex < static_cast<int>(m_animations.size()))
    {
        m_currentAnimationIndex = animationIndex;
        m_currentAnimationTime = 0.0f;

        // Initialize bone matrices if this is the first animation
        if (m_boneMatrices.empty() && m_skinInfo.IsValid())
        {
            m_boneMatrices.resize(m_skinInfo.bones.size(), XMMatrixIdentity());
            m_finalBoneMatrices.resize(m_skinInfo.bones.size(), XMMatrixIdentity());
        }
    }
}

void Model::SetAnimationTime(float time)
{
    if (m_currentAnimationIndex >= 0 && m_currentAnimationIndex < static_cast<int>(m_animations.size()))
    {
        const auto& animation = m_animations[m_currentAnimationIndex];
        m_currentAnimationTime = std::max(0.0f, std::min(time, animation.duration));
        UpdateBoneMatrices();
    }
}

void Model::PauseAnimation(bool pause)
{
    m_isAnimationPaused = pause;
}

void Model::ResetAnimation()
{
    m_currentAnimationTime = 0.0f;
    UpdateBoneMatrices();
}

std::shared_ptr<Mesh> Model::GetMesh(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_meshes.size()))
    {
        return m_meshes[index];
    }
    return nullptr;
}

std::shared_ptr<Material> Model::GetMaterial(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_materials.size()))
    {
        return m_materials[index];
    }
    return nullptr;
}

const Animation* Model::GetCurrentAnimation() const
{
    if (m_currentAnimationIndex >= 0 && m_currentAnimationIndex < static_cast<int>(m_animations.size()))
    {
        return &m_animations[m_currentAnimationIndex];
    }
    return nullptr;
}

void Model::SetPosition(const XMFLOAT3& position)
{
    XMVECTOR pos = XMLoadFloat3(&position);
    XMMATRIX translation = XMMatrixTranslationFromVector(pos);

    // Extract rotation and scale from current transform
    XMVECTOR scale, rotation, currentPos;
    XMMatrixDecompose(&scale, &rotation, &currentPos, m_worldTransform);

    XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scale);
    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

    m_worldTransform = scaleMatrix * rotationMatrix * translation;
    m_boundingBoxDirty = true;
}

void Model::SetRotation(const XMFLOAT3& rotation)
{
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

    // Extract position and scale from current transform
    XMVECTOR scale, currentRotation, position;
    XMMatrixDecompose(&scale, &currentRotation, &position, m_worldTransform);

    XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scale);
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(position);

    m_worldTransform = scaleMatrix * rotationMatrix * translationMatrix;
    m_boundingBoxDirty = true;
}

void Model::SetScale(const XMFLOAT3& scale)
{
    XMVECTOR scaleVec = XMLoadFloat3(&scale);
    XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scaleVec);

    // Extract position and rotation from current transform
    XMVECTOR currentScale, rotation, position;
    XMMatrixDecompose(&currentScale, &rotation, &position, m_worldTransform);

    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(position);

    m_worldTransform = scaleMatrix * rotationMatrix * translationMatrix;
    m_boundingBoxDirty = true;
}

void Model::CalculateBoundingBox()
{
    if (m_meshes.empty())
    {
        m_boundingBox = BoundingBox();
        m_boundingBoxDirty = false;
        return;
    }

    bool first = true;
    XMFLOAT3 minPoint(FLT_MAX, FLT_MAX, FLT_MAX);
    XMFLOAT3 maxPoint(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& mesh : m_meshes)
    {
        if (!mesh)
            continue;

        const auto& meshBB = mesh->GetBoundingBox();

        if (first)
        {
            minPoint = meshBB.min;
            maxPoint = meshBB.max;
            first = false;
        }
        else
        {
            minPoint.x = std::min(minPoint.x, meshBB.min.x);
            minPoint.y = std::min(minPoint.y, meshBB.min.y);
            minPoint.z = std::min(minPoint.z, meshBB.min.z);

            maxPoint.x = std::max(maxPoint.x, meshBB.max.x);
            maxPoint.y = std::max(maxPoint.y, meshBB.max.y);
            maxPoint.z = std::max(maxPoint.z, meshBB.max.z);
        }
    }

    m_boundingBox.min = minPoint;
    m_boundingBox.max = maxPoint;
    m_boundingBox.center.x = (minPoint.x + maxPoint.x) * 0.5f;
    m_boundingBox.center.y = (minPoint.y + maxPoint.y) * 0.5f;
    m_boundingBox.center.z = (minPoint.z + maxPoint.z) * 0.5f;
    m_boundingBox.extents.x = (maxPoint.x - minPoint.x) * 0.5f;
    m_boundingBox.extents.y = (maxPoint.y - minPoint.y) * 0.5f;
    m_boundingBox.extents.z = (maxPoint.z - minPoint.z) * 0.5f;

    m_boundingBoxDirty = false;
}

const BoundingBox& Model::GetBoundingBox() const
{
    if (m_boundingBoxDirty)
    {
        const_cast<Model*>(this)->CalculateBoundingBox();
    }
    return m_boundingBox;
}

void Model::AddMesh(std::shared_ptr<Mesh> mesh)
{
    if (mesh)
    {
        m_meshes.push_back(mesh);
        m_boundingBoxDirty = true;
    }
}

void Model::AddMaterial(std::shared_ptr<Material> material)
{
    if (material)
    {
        m_materials.push_back(material);
    }
}

void Model::AddAnimation(const Animation& animation)
{
    m_animations.push_back(animation);
}

void Model::AssignMaterialToMesh(int meshIndex, int materialIndex)
{
    if (meshIndex >= 0 && meshIndex < static_cast<int>(m_meshes.size()) &&
        materialIndex >= 0 && materialIndex < static_cast<int>(m_materials.size()))
    {
        m_meshes[meshIndex]->SetMaterialIndex(materialIndex);
        m_meshes[meshIndex]->SetMaterial(m_materials[materialIndex]);
    }
}

void Model::AssignMaterialToMesh(int meshIndex, std::shared_ptr<Material> material)
{
    if (meshIndex >= 0 && meshIndex < static_cast<int>(m_meshes.size()) && material)
    {
        m_meshes[meshIndex]->SetMaterial(material);
    }
}

void Model::UpdateBoneMatrices()
{
    if (!IsAnimated() || m_currentAnimationIndex < 0 ||
        m_currentAnimationIndex >= static_cast<int>(m_animations.size()))
    {
        return;
    }

    const auto& animation = m_animations[m_currentAnimationIndex];

    // Update bone matrices based on current animation time
    for (int i = 0; i < static_cast<int>(m_skinInfo.bones.size()); ++i)
    {
        if (i < static_cast<int>(m_boneMatrices.size()))
        {
            // Get animated transform for this bone
            XMMATRIX animatedTransform = animation.GetBoneTransform(i, m_currentAnimationTime);

            // Apply parent bone transforms
            const auto& bone = m_skinInfo.bones[i];
            if (bone.parentIndex >= 0 && bone.parentIndex < static_cast<int>(m_boneMatrices.size()))
            {
                m_boneMatrices[i] = animatedTransform * m_boneMatrices[bone.parentIndex];
            }
            else
            {
                m_boneMatrices[i] = animatedTransform;
            }

            // Calculate final bone matrix for skinning
            if (i < static_cast<int>(m_finalBoneMatrices.size()))
            {
                m_finalBoneMatrices[i] = m_skinInfo.GetBoneMatrix(i, m_boneMatrices);
            }
        }
    }
}

void Model::UpdateSkinnedMeshes(ID3D11DeviceContext* context)
{
    // This would update vertex buffers for skinned meshes
    // Implementation would depend on having the bone matrices uploaded to GPU
    // and shaders that perform vertex skinning
}

XMMATRIX Model::InterpolatePosition(const std::vector<PositionKey>& keys, float time) const
{
    if (keys.empty())
        return XMMatrixIdentity();

    if (keys.size() == 1)
        return XMMatrixTranslationFromVector(XMLoadFloat3(&keys[0].position));

    // Find surrounding keyframes
    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time >= keys[i].time && time <= keys[i + 1].time)
        {
            float deltaTime = keys[i + 1].time - keys[i].time;
            if (deltaTime > 0.0f)
            {
                float factor = (time - keys[i].time) / deltaTime;
                XMVECTOR pos1 = XMLoadFloat3(&keys[i].position);
                XMVECTOR pos2 = XMLoadFloat3(&keys[i + 1].position);
                XMVECTOR result = XMVectorLerp(pos1, pos2, factor);
                return XMMatrixTranslationFromVector(result);
            }
            else
            {
                return XMMatrixTranslationFromVector(XMLoadFloat3(&keys[i].position));
            }
        }
    }

    // Use last keyframe if time is beyond all keys
    return XMMatrixTranslationFromVector(XMLoadFloat3(&keys.back().position));
}

XMMATRIX Model::InterpolateRotation(const std::vector<RotationKey>& keys, float time) const
{
    if (keys.empty())
        return XMMatrixIdentity();

    if (keys.size() == 1)
        return XMMatrixRotationQuaternion(XMLoadFloat4(&keys[0].rotation));

    // Find surrounding keyframes
    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time >= keys[i].time && time <= keys[i + 1].time)
        {
            float deltaTime = keys[i + 1].time - keys[i].time;
            if (deltaTime > 0.0f)
            {
                float factor = (time - keys[i].time) / deltaTime;
                XMVECTOR rot1 = XMLoadFloat4(&keys[i].rotation);
                XMVECTOR rot2 = XMLoadFloat4(&keys[i + 1].rotation);
                XMVECTOR result = XMQuaternionSlerp(rot1, rot2, factor);
                return XMMatrixRotationQuaternion(result);
            }
            else
            {
                return XMMatrixRotationQuaternion(XMLoadFloat4(&keys[i].rotation));
            }
        }
    }

    // Use last keyframe if time is beyond all keys
    return XMMatrixRotationQuaternion(XMLoadFloat4(&keys.back().rotation));
}

XMMATRIX Model::InterpolateScale(const std::vector<ScaleKey>& keys, float time) const
{
    if (keys.empty())
        return XMMatrixIdentity();

    if (keys.size() == 1)
        return XMMatrixScalingFromVector(XMLoadFloat3(&keys[0].scale));

    // Find surrounding keyframes
    for (size_t i = 0; i < keys.size() - 1; ++i)
    {
        if (time >= keys[i].time && time <= keys[i + 1].time)
        {
            float deltaTime = keys[i + 1].time - keys[i].time;
            if (deltaTime > 0.0f)
            {
                float factor = (time - keys[i].time) / deltaTime;
                XMVECTOR scale1 = XMLoadFloat3(&keys[i].scale);
                XMVECTOR scale2 = XMLoadFloat3(&keys[i + 1].scale);
                XMVECTOR result = XMVectorLerp(scale1, scale2, factor);
                return XMMatrixScalingFromVector(result);
            }
            else
            {
                return XMMatrixScalingFromVector(XMLoadFloat3(&keys[i].scale));
            }
        }
    }

    // Use last keyframe if time is beyond all keys
    return XMMatrixScalingFromVector(XMLoadFloat3(&keys.back().scale));
}

// Helper function to create a test model (placeholder until ModelLoader is implemented)
void Model::CreateTestModel(ID3D11Device* device)
{
    // Create a simple test model with a cube mesh
    auto cubeMesh = Mesh::CreateCube(device, 2.0f);
    if (cubeMesh)
    {
        AddMesh(cubeMesh);
        m_name = "TestModel";
        m_isLoaded = true;

        std::cout << "Created test model with cube mesh" << std::endl;
    }
}
