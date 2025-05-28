#include "Animation.h"
#include <algorithm>
#include <iostream>

// Animation implementation
Animation::Animation()
    : m_duration(0.0f)
    , m_ticksPerSecond(25.0f)
{
}

Animation::~Animation()
{
    Shutdown();
}

bool Animation::Initialize(const std::string& name, float duration, float ticksPerSecond)
{
    m_name = name;
    m_duration = duration;
    m_ticksPerSecond = ticksPerSecond;
    return true;
}

void Animation::Shutdown()
{
    m_channels.clear();
}

void Animation::AddChannel(const AnimationChannel& channel)
{
    m_channels.push_back(channel);
}

void Animation::EvaluateAnimation(float timeInSeconds, const std::vector<Bone>& skeleton,
                                 std::vector<XMMATRIX>& boneTransforms) const
{
    float animationTime = timeInSeconds * m_ticksPerSecond;

    // Wrap animation time if needed
    if (m_duration > 0.0f)
    {
        animationTime = fmod(animationTime, m_duration);
    }

    for (const auto& channel : m_channels)
    {
        if (channel.boneIndex < 0 || channel.boneIndex >= skeleton.size())
            continue;

        // Interpolate transformations
        XMVECTOR position = InterpolatePosition(channel, animationTime);
        XMVECTOR rotation = InterpolateRotation(channel, animationTime);
        XMVECTOR scale = InterpolateScale(channel, animationTime);

        // Create transformation matrix
        XMMATRIX scaleMatrix = XMMatrixScalingFromVector(scale);
        XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);
        XMMATRIX translationMatrix = XMMatrixTranslationFromVector(position);

        // Combine transformations (Scale * Rotation * Translation)
        XMMATRIX localTransform = scaleMatrix * rotationMatrix * translationMatrix;

        // Store in bone transforms
        if (channel.boneIndex < boneTransforms.size())
        {
            boneTransforms[channel.boneIndex] = localTransform;
        }
    }
}

XMVECTOR Animation::InterpolatePosition(const AnimationChannel& channel, float animationTime) const
{
    if (channel.positionKeys.empty())
    {
        return XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
    }

    if (channel.positionKeys.size() == 1)
    {
        return channel.positionKeys[0].value;
    }

    int keyIndex = FindPositionKeyIndex(channel, animationTime);
    int nextKeyIndex = keyIndex + 1;

    if (nextKeyIndex >= channel.positionKeys.size())
    {
        return channel.positionKeys[keyIndex].value;
    }

    float deltaTime = channel.positionKeys[nextKeyIndex].time - channel.positionKeys[keyIndex].time;
    float factor = (animationTime - channel.positionKeys[keyIndex].time) / deltaTime;
    factor = std::max(0.0f, std::min(1.0f, factor));

    XMVECTOR start = channel.positionKeys[keyIndex].value;
    XMVECTOR end = channel.positionKeys[nextKeyIndex].value;

    return XMVectorLerp(start, end, factor);
}

XMVECTOR Animation::InterpolateRotation(const AnimationChannel& channel, float animationTime) const
{
    if (channel.rotationKeys.empty())
    {
        return XMQuaternionIdentity();
    }

    if (channel.rotationKeys.size() == 1)
    {
        return channel.rotationKeys[0].value;
    }

    int keyIndex = FindRotationKeyIndex(channel, animationTime);
    int nextKeyIndex = keyIndex + 1;

    if (nextKeyIndex >= channel.rotationKeys.size())
    {
        return channel.rotationKeys[keyIndex].value;
    }

    float deltaTime = channel.rotationKeys[nextKeyIndex].time - channel.rotationKeys[keyIndex].time;
    float factor = (animationTime - channel.rotationKeys[keyIndex].time) / deltaTime;
    factor = std::max(0.0f, std::min(1.0f, factor));

    XMVECTOR start = channel.rotationKeys[keyIndex].value;
    XMVECTOR end = channel.rotationKeys[nextKeyIndex].value;

    return XMQuaternionSlerp(start, end, factor);
}

XMVECTOR Animation::InterpolateScale(const AnimationChannel& channel, float animationTime) const
{
    if (channel.scaleKeys.empty())
    {
        return XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
    }

    if (channel.scaleKeys.size() == 1)
    {
        return channel.scaleKeys[0].value;
    }

    int keyIndex = FindScaleKeyIndex(channel, animationTime);
    int nextKeyIndex = keyIndex + 1;

    if (nextKeyIndex >= channel.scaleKeys.size())
    {
        return channel.scaleKeys[keyIndex].value;
    }

    float deltaTime = channel.scaleKeys[nextKeyIndex].time - channel.scaleKeys[keyIndex].time;
    float factor = (animationTime - channel.scaleKeys[keyIndex].time) / deltaTime;
    factor = std::max(0.0f, std::min(1.0f, factor));

    XMVECTOR start = channel.scaleKeys[keyIndex].value;
    XMVECTOR end = channel.scaleKeys[nextKeyIndex].value;

    return XMVectorLerp(start, end, factor);
}

int Animation::FindPositionKeyIndex(const AnimationChannel& channel, float animationTime) const
{
    for (int i = 0; i < channel.positionKeys.size() - 1; ++i)
    {
        if (animationTime < channel.positionKeys[i + 1].time)
        {
            return i;
        }
    }
    return static_cast<int>(channel.positionKeys.size()) - 1;
}

int Animation::FindRotationKeyIndex(const AnimationChannel& channel, float animationTime) const
{
    for (int i = 0; i < channel.rotationKeys.size() - 1; ++i)
    {
        if (animationTime < channel.rotationKeys[i + 1].time)
        {
            return i;
        }
    }
    return static_cast<int>(channel.rotationKeys.size()) - 1;
}

int Animation::FindScaleKeyIndex(const AnimationChannel& channel, float animationTime) const
{
    for (int i = 0; i < channel.scaleKeys.size() - 1; ++i)
    {
        if (animationTime < channel.scaleKeys[i + 1].time)
        {
            return i;
        }
    }
    return static_cast<int>(channel.scaleKeys.size()) - 1;
}

// Skeleton implementation
Skeleton::Skeleton()
{
    m_rootTransform = XMMatrixIdentity();
}

Skeleton::~Skeleton()
{
    Shutdown();
}

bool Skeleton::Initialize(const std::vector<Bone>& bones)
{
    m_bones = bones;

    // Build name to index mapping
    m_boneNameToIndex.clear();
    for (int i = 0; i < m_bones.size(); ++i)
    {
        m_boneNameToIndex[m_bones[i].name] = i;
    }

    return true;
}

void Skeleton::Shutdown()
{
    m_bones.clear();
    m_boneNameToIndex.clear();
}

int Skeleton::AddBone(const Bone& bone)
{
    int index = static_cast<int>(m_bones.size());
    m_bones.push_back(bone);
    m_boneNameToIndex[bone.name] = index;
    return index;
}

int Skeleton::FindBoneIndex(const std::string& name) const
{
    auto it = m_boneNameToIndex.find(name);
    return (it != m_boneNameToIndex.end()) ? it->second : -1;
}

void Skeleton::CalculateBoneTransforms(std::vector<XMMATRIX>& boneTransforms) const
{
    boneTransforms.resize(m_bones.size());

    // Find root bones and calculate recursively
    for (int i = 0; i < m_bones.size(); ++i)
    {
        if (m_bones[i].parentIndex == -1)
        {
            CalculateBoneTransformRecursive(i, m_rootTransform, boneTransforms);
        }
    }
}

void Skeleton::SetBonePose(int boneIndex, const XMMATRIX& transform)
{
    if (boneIndex >= 0 && boneIndex < m_bones.size())
    {
        m_bones[boneIndex].currentMatrix = transform;
    }
}

void Skeleton::CalculateBoneTransformRecursive(int boneIndex, const XMMATRIX& parentTransform,
                                              std::vector<XMMATRIX>& boneTransforms) const
{
    if (boneIndex < 0 || boneIndex >= m_bones.size())
        return;

    const Bone& bone = m_bones[boneIndex];

    // Calculate global transform: Parent * Local * Offset
    XMMATRIX globalTransform = bone.currentMatrix * parentTransform;
    boneTransforms[boneIndex] = bone.offsetMatrix * globalTransform;

    // Process children
    for (int childIndex : bone.childrenIndices)
    {
        CalculateBoneTransformRecursive(childIndex, globalTransform, boneTransforms);
    }
}

// AnimationController implementation
AnimationController::AnimationController()
    : m_currentAnimationIndex(-1)
    , m_currentTime(0.0f)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_isLooping(false)
    , m_enableBlending(false)
    , m_blendTime(0.5f)
    , m_currentBlendTime(0.0f)
    , m_previousAnimationIndex(-1)
{
}

AnimationController::~AnimationController()
{
    Shutdown();
}

bool AnimationController::Initialize(std::shared_ptr<Skeleton> skeleton)
{
    if (!skeleton)
        return false;

    m_skeleton = skeleton;
    m_boneTransforms.resize(skeleton->GetBoneCount());
    m_previousBoneTransforms.resize(skeleton->GetBoneCount());

    // Initialize with identity matrices
    for (auto& transform : m_boneTransforms)
    {
        transform = XMMatrixIdentity();
    }

    return true;
}

void AnimationController::Shutdown()
{
    m_animations.clear();
    m_animationNameToIndex.clear();
    m_skeleton.reset();
}

void AnimationController::AddAnimation(std::shared_ptr<Animation> animation)
{
    if (!animation)
        return;

    int index = static_cast<int>(m_animations.size());
    m_animations.push_back(animation);
    m_animationNameToIndex[animation->GetName()] = index;
}

bool AnimationController::PlayAnimation(const std::string& animationName, bool loop)
{
    auto it = m_animationNameToIndex.find(animationName);
    if (it == m_animationNameToIndex.end())
        return false;

    int newAnimationIndex = it->second;

    // Setup blending if enabled and switching animations
    if (m_enableBlending && m_isPlaying && m_currentAnimationIndex != newAnimationIndex)
    {
        m_previousAnimationIndex = m_currentAnimationIndex;
        m_previousBoneTransforms = m_boneTransforms;
        m_currentBlendTime = 0.0f;
    }

    m_currentAnimationIndex = newAnimationIndex;
    m_currentTime = 0.0f;
    m_isPlaying = true;
    m_isPaused = false;
    m_isLooping = loop;

    return true;
}

void AnimationController::StopAnimation()
{
    m_isPlaying = false;
    m_isPaused = false;
    m_currentTime = 0.0f;
}

void AnimationController::PauseAnimation()
{
    m_isPaused = true;
}

void AnimationController::ResumeAnimation()
{
    m_isPaused = false;
}

void AnimationController::Update(float deltaTime)
{
    if (!m_isPlaying || m_isPaused || m_currentAnimationIndex < 0)
        return;

    if (m_currentAnimationIndex >= m_animations.size())
        return;

    auto currentAnimation = m_animations[m_currentAnimationIndex];
    if (!currentAnimation)
        return;

    // Update animation time
    m_currentTime += deltaTime;

    // Check for animation end
    float duration = currentAnimation->GetDuration() / currentAnimation->GetTicksPerSecond();
    if (m_currentTime >= duration)
    {
        if (m_isLooping)
        {
            m_currentTime = fmod(m_currentTime, duration);
        }
        else
        {
            m_currentTime = duration;
            m_isPlaying = false;
        }
    }

    // Update bone transforms
    UpdateBoneTransforms();

    // Update blending
    if (m_enableBlending && m_currentBlendTime < m_blendTime)
    {
        m_currentBlendTime += deltaTime;
        if (m_currentBlendTime >= m_blendTime)
        {
            m_currentBlendTime = m_blendTime;
            m_previousAnimationIndex = -1; // End blending
        }
    }
}

void AnimationController::UpdateBoneTransforms()
{
    if (!m_skeleton || m_currentAnimationIndex < 0)
        return;

    auto currentAnimation = m_animations[m_currentAnimationIndex];
    if (!currentAnimation)
        return;

    // Get bone transforms from skeleton
    std::vector<Bone> bones(m_skeleton->GetBoneCount());
    for (int i = 0; i < m_skeleton->GetBoneCount(); ++i)
    {
        bones[i] = m_skeleton->GetBone(i);
    }

    // Evaluate current animation
    currentAnimation->EvaluateAnimation(m_currentTime, bones, m_boneTransforms);

    // Apply blending if active
    if (m_enableBlending && m_previousAnimationIndex >= 0 && m_currentBlendTime < m_blendTime)
    {
        float blendFactor = m_currentBlendTime / m_blendTime;
        BlendAnimations(blendFactor);
    }

    // Calculate final bone transforms
    m_skeleton->CalculateBoneTransforms(m_boneTransforms);
}

void AnimationController::BlendAnimations(float blendFactor)
{
    // Blend between previous and current bone transforms
    for (int i = 0; i < m_boneTransforms.size(); ++i)
    {
        // Decompose matrices for proper blending
        XMVECTOR scale1, rotation1, translation1;
        XMVECTOR scale2, rotation2, translation2;

        XMMatrixDecompose(&scale1, &rotation1, &translation1, m_previousBoneTransforms[i]);
        XMMatrixDecompose(&scale2, &rotation2, &translation2, m_boneTransforms[i]);

        // Interpolate components
        XMVECTOR blendedScale = XMVectorLerp(scale1, scale2, blendFactor);
        XMVECTOR blendedRotation = XMQuaternionSlerp(rotation1, rotation2, blendFactor);
        XMVECTOR blendedTranslation = XMVectorLerp(translation1, translation2, blendFactor);

        // Recompose matrix
        m_boneTransforms[i] = XMMatrixScalingFromVector(blendedScale) *
                             XMMatrixRotationQuaternion(blendedRotation) *
                             XMMatrixTranslationFromVector(blendedTranslation);
    }
}
