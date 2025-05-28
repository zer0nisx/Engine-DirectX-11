#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

using namespace DirectX;

// Forward declarations
class Model;
class Mesh;

// Bone structure for skeletal animation
struct Bone
{
    std::string name;
    int parentIndex;
    XMMATRIX offsetMatrix;     // Transforms from mesh space to bone space
    XMMATRIX bindMatrix;       // Bind pose transformation
    XMMATRIX currentMatrix;    // Current transformation

    std::vector<int> childrenIndices;

    Bone() : parentIndex(-1)
    {
        offsetMatrix = XMMatrixIdentity();
        bindMatrix = XMMatrixIdentity();
        currentMatrix = XMMatrixIdentity();
    }
};

// Animation keyframe for different transformation types
template<typename T>
struct AnimationKey
{
    float time;
    T value;

    AnimationKey() : time(0.0f) {}
    AnimationKey(float t, const T& v) : time(t), value(v) {}
};

// Animation channel for a single bone
struct AnimationChannel
{
    std::string boneName;
    int boneIndex;

    std::vector<AnimationKey<XMVECTOR>> positionKeys;
    std::vector<AnimationKey<XMVECTOR>> rotationKeys;
    std::vector<AnimationKey<XMVECTOR>> scaleKeys;

    AnimationChannel() : boneIndex(-1) {}
};

// Complete animation
class Animation
{
public:
    Animation();
    ~Animation();

    bool Initialize(const std::string& name, float duration, float ticksPerSecond);
    void Shutdown();

    // Getters
    const std::string& GetName() const { return m_name; }
    float GetDuration() const { return m_duration; }
    float GetTicksPerSecond() const { return m_ticksPerSecond; }

    // Channel management
    void AddChannel(const AnimationChannel& channel);
    const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }

    // Animation evaluation
    void EvaluateAnimation(float timeInSeconds, const std::vector<Bone>& skeleton,
                          std::vector<XMMATRIX>& boneTransforms) const;

private:
    std::string m_name;
    float m_duration;
    float m_ticksPerSecond;
    std::vector<AnimationChannel> m_channels;

    // Helper functions for interpolation
    XMVECTOR InterpolatePosition(const AnimationChannel& channel, float animationTime) const;
    XMVECTOR InterpolateRotation(const AnimationChannel& channel, float animationTime) const;
    XMVECTOR InterpolateScale(const AnimationChannel& channel, float animationTime) const;

    int FindPositionKeyIndex(const AnimationChannel& channel, float animationTime) const;
    int FindRotationKeyIndex(const AnimationChannel& channel, float animationTime) const;
    int FindScaleKeyIndex(const AnimationChannel& channel, float animationTime) const;
};

// Skeleton for managing bones
class Skeleton
{
public:
    Skeleton();
    ~Skeleton();

    bool Initialize(const std::vector<Bone>& bones);
    void Shutdown();

    // Bone management
    int AddBone(const Bone& bone);
    const Bone& GetBone(int index) const { return m_bones[index]; }
    Bone& GetBone(int index) { return m_bones[index]; }

    int GetBoneCount() const { return static_cast<int>(m_bones.size()); }
    int FindBoneIndex(const std::string& name) const;

    // Transformation calculation
    void CalculateBoneTransforms(std::vector<XMMATRIX>& boneTransforms) const;
    void SetBonePose(int boneIndex, const XMMATRIX& transform);

    // Root transform
    void SetRootTransform(const XMMATRIX& transform) { m_rootTransform = transform; }
    const XMMATRIX& GetRootTransform() const { return m_rootTransform; }

private:
    std::vector<Bone> m_bones;
    std::unordered_map<std::string, int> m_boneNameToIndex;
    XMMATRIX m_rootTransform;

    void CalculateBoneTransformRecursive(int boneIndex, const XMMATRIX& parentTransform,
                                        std::vector<XMMATRIX>& boneTransforms) const;
};

// Animation controller for managing multiple animations
class AnimationController
{
public:
    AnimationController();
    ~AnimationController();

    bool Initialize(std::shared_ptr<Skeleton> skeleton);
    void Shutdown();

    // Animation management
    void AddAnimation(std::shared_ptr<Animation> animation);
    bool PlayAnimation(const std::string& animationName, bool loop = true);
    void StopAnimation();
    void PauseAnimation();
    void ResumeAnimation();

    // Update
    void Update(float deltaTime);

    // State
    bool IsPlaying() const { return m_isPlaying; }
    bool IsPaused() const { return m_isPaused; }
    float GetCurrentTime() const { return m_currentTime; }

    // Animation blending
    void SetBlendMode(bool enable) { m_enableBlending = enable; }
    void SetBlendTime(float blendTime) { m_blendTime = blendTime; }

    // Get current bone transforms for rendering
    const std::vector<XMMATRIX>& GetBoneTransforms() const { return m_boneTransforms; }

private:
    std::shared_ptr<Skeleton> m_skeleton;
    std::vector<std::shared_ptr<Animation>> m_animations;
    std::unordered_map<std::string, int> m_animationNameToIndex;

    // Current animation state
    int m_currentAnimationIndex;
    float m_currentTime;
    bool m_isPlaying;
    bool m_isPaused;
    bool m_isLooping;

    // Animation blending
    bool m_enableBlending;
    float m_blendTime;
    float m_currentBlendTime;
    int m_previousAnimationIndex;

    // Bone transforms
    std::vector<XMMATRIX> m_boneTransforms;
    std::vector<XMMATRIX> m_previousBoneTransforms;

    void UpdateBoneTransforms();
    void BlendAnimations(float blendFactor);
};

// Vertex structure with bone weights
struct SkinnedVertex
{
    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
    XMFLOAT3 tangent;
    XMFLOAT3 binormal;

    // Skinning data (max 4 bones per vertex)
    uint32_t boneIndices[4];
    float boneWeights[4];

    SkinnedVertex()
    {
        position = XMFLOAT3(0.0f, 0.0f, 0.0f);
        normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
        texCoord = XMFLOAT2(0.0f, 0.0f);
        tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
        binormal = XMFLOAT3(0.0f, 0.0f, 1.0f);

        for (int i = 0; i < 4; ++i)
        {
            boneIndices[i] = 0;
            boneWeights[i] = 0.0f;
        }
    }
};
