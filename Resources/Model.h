#pragma once

#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

using namespace DirectX;

// Forward declarations
class Mesh;
class Material;
class Texture;

// Bone structure for skeletal animation
struct Bone
{
    std::string name;
    int parentIndex;
    XMMATRIX offsetMatrix;      // Bone-to-mesh space transform
    XMMATRIX bindPoseMatrix;    // Local transform in bind pose

    std::vector<int> childIndices;

    Bone()
        : parentIndex(-1)
        , offsetMatrix(XMMatrixIdentity())
        , bindPoseMatrix(XMMatrixIdentity())
    {
    }
};

// Animation keyframe structures
struct PositionKey
{
    float time;
    XMFLOAT3 position;

    PositionKey() : time(0.0f), position(0.0f, 0.0f, 0.0f) {}
    PositionKey(float t, const XMFLOAT3& pos) : time(t), position(pos) {}
};

struct RotationKey
{
    float time;
    XMFLOAT4 rotation; // Quaternion

    RotationKey() : time(0.0f), rotation(0.0f, 0.0f, 0.0f, 1.0f) {}
    RotationKey(float t, const XMFLOAT4& rot) : time(t), rotation(rot) {}
};

struct ScaleKey
{
    float time;
    XMFLOAT3 scale;

    ScaleKey() : time(0.0f), scale(1.0f, 1.0f, 1.0f) {}
    ScaleKey(float t, const XMFLOAT3& s) : time(t), scale(s) {}
};

// Animation channel for a single bone
struct AnimationChannel
{
    std::string boneName;
    int boneIndex;

    std::vector<PositionKey> positionKeys;
    std::vector<RotationKey> rotationKeys;
    std::vector<ScaleKey> scaleKeys;

    AnimationChannel() : boneIndex(-1) {}
};

// Animation sequence
struct Animation
{
    std::string name;
    float duration;
    float ticksPerSecond;

    std::vector<AnimationChannel> channels;

    Animation() : duration(0.0f), ticksPerSecond(24.0f) {}

    // Get interpolated transform for a bone at a specific time
    XMMATRIX GetBoneTransform(int boneIndex, float timeInSeconds) const;
    XMMATRIX GetBoneTransform(const std::string& boneName, float timeInSeconds) const;
};

// Skin info for mesh deformation
struct SkinInfo
{
    std::vector<Bone> bones;
    std::unordered_map<std::string, int> boneNameToIndex;

    bool IsValid() const { return !bones.empty(); }
    int FindBoneIndex(const std::string& boneName) const;
    void AddBone(const Bone& bone);
    XMMATRIX GetBoneMatrix(int boneIndex, const std::vector<XMMATRIX>& currentPose) const;
};

// Model class that contains meshes, materials, and animation data
class Model
{
public:
    Model();
    ~Model();

    // Loading
    bool LoadFromFile(ID3D11Device* device, const std::string& filepath);

    // Cleanup
    void Shutdown();

    // Rendering
    void Render(ID3D11DeviceContext* context);
    void RenderWithMaterials(ID3D11DeviceContext* context, class Shader* shader);

    // Animation
    void UpdateAnimation(float deltaTime);
    void SetAnimation(const std::string& animationName);
    void SetAnimation(int animationIndex);
    void SetAnimationTime(float time);
    void PauseAnimation(bool pause);
    void ResetAnimation();

    // Data access
    const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_meshes; }
    const std::vector<std::shared_ptr<Material>>& GetMaterials() const { return m_materials; }
    const std::vector<Animation>& GetAnimations() const { return m_animations; }
    const SkinInfo& GetSkinInfo() const { return m_skinInfo; }

    std::shared_ptr<Mesh> GetMesh(int index) const;
    std::shared_ptr<Material> GetMaterial(int index) const;
    const Animation* GetCurrentAnimation() const;

    // Properties
    void SetName(const std::string& name) { m_name = name; }
    const std::string& GetName() const { return m_name; }
    const std::string& GetFilePath() const { return m_filepath; }

    int GetMeshCount() const { return static_cast<int>(m_meshes.size()); }
    int GetMaterialCount() const { return static_cast<int>(m_materials.size()); }
    int GetAnimationCount() const { return static_cast<int>(m_animations.size()); }

    bool IsAnimated() const { return !m_animations.empty() && m_skinInfo.IsValid(); }
    bool IsValid() const { return !m_meshes.empty(); }

    // Transform
    void SetTransform(const XMMATRIX& transform) { m_worldTransform = transform; }
    const XMMATRIX& GetTransform() const { return m_worldTransform; }
    void SetPosition(const XMFLOAT3& position);
    void SetRotation(const XMFLOAT3& rotation);
    void SetScale(const XMFLOAT3& scale);

    // Bounding volume
    void CalculateBoundingBox();
    const BoundingBox& GetBoundingBox() const;

    // Resource management
    void AddMesh(std::shared_ptr<Mesh> mesh);
    void AddMaterial(std::shared_ptr<Material> material);
    void AddAnimation(const Animation& animation);

    // Material assignment
    void AssignMaterialToMesh(int meshIndex, int materialIndex);
    void AssignMaterialToMesh(int meshIndex, std::shared_ptr<Material> material);

private:
    void UpdateBoneMatrices();
    void UpdateSkinnedMeshes(ID3D11DeviceContext* context);
    XMMATRIX InterpolatePosition(const std::vector<PositionKey>& keys, float time) const;
    XMMATRIX InterpolateRotation(const std::vector<RotationKey>& keys, float time) const;
    XMMATRIX InterpolateScale(const std::vector<ScaleKey>& keys, float time) const;

private:
    std::string m_name;
    std::string m_filepath;

    // Geometry and materials
    std::vector<std::shared_ptr<Mesh>> m_meshes;
    std::vector<std::shared_ptr<Material>> m_materials;

    // Animation data
    std::vector<Animation> m_animations;
    SkinInfo m_skinInfo;

    // Animation state
    int m_currentAnimationIndex;
    float m_currentAnimationTime;
    bool m_isAnimationPaused;
    bool m_loopAnimation;

    // Transform
    XMMATRIX m_worldTransform;

    // Bone matrices for skinning
    std::vector<XMMATRIX> m_boneMatrices;
    std::vector<XMMATRIX> m_finalBoneMatrices;

    // State
    bool m_isLoaded;
    mutable BoundingBox m_boundingBox;
    mutable bool m_boundingBoxDirty;
};
