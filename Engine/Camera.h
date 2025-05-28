#pragma once

#include <DirectXMath.h>

using namespace DirectX;

enum class CameraMode
{
    FirstPerson,
    ThirdPerson
};

class Camera
{
public:
    Camera();
    ~Camera();

    void Initialize(float screenWidth, float screenHeight);
    void Update(float deltaTime);

    // Camera mode
    void SetCameraMode(CameraMode mode) { m_cameraMode = mode; }
    CameraMode GetCameraMode() const { return m_cameraMode; }

    // First person movement functions
    void MoveForward(float deltaTime);
    void MoveBackward(float deltaTime);
    void MoveLeft(float deltaTime);
    void MoveRight(float deltaTime);
    void MoveUp(float deltaTime);
    void MoveDown(float deltaTime);

    // Third person functions
    void SetTarget(const XMFLOAT3& target) { m_target = target; }
    void MoveTarget(float x, float y, float z);
    void OrbitAroundTarget(float yaw, float pitch);
    void ZoomToTarget(float zoomDelta);
    void SetDistance(float distance) { m_distance = distance; }

    // Rotation functions (first person)
    void Rotate(float yaw, float pitch);

    // Matrix getters
    XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
    XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }

    // Position and rotation getters
    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetRotation() const { return m_rotation; }
    XMFLOAT3 GetTarget() const { return m_target; }

    // Set functions
    void SetPosition(float x, float y, float z);
    void SetRotation(float pitch, float yaw, float roll);

private:
    void UpdateViewMatrix();
    void UpdateThirdPersonPosition();

    // Camera mode
    CameraMode m_cameraMode;

    // Common properties
    XMFLOAT3 m_position;
    XMFLOAT3 m_rotation;
    XMFLOAT3 m_forward;
    XMFLOAT3 m_up;
    XMFLOAT3 m_right;

    // Third person properties
    XMFLOAT3 m_target;           // Target to follow
    float m_distance;            // Distance from target
    float m_orbitYaw;            // Horizontal orbit angle
    float m_orbitPitch;          // Vertical orbit angle
    float m_minDistance;         // Minimum zoom distance
    float m_maxDistance;         // Maximum zoom distance
    float m_followSpeed;         // Speed of camera following target

    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;

    float m_moveSpeed;
    float m_rotationSpeed;
    float m_zoomSpeed;
    float m_fieldOfView;
    float m_nearPlane;
    float m_farPlane;
};
