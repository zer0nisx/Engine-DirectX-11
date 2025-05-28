#include "Camera.h"
#include <algorithm>

Camera::Camera()
    : m_cameraMode(CameraMode::ThirdPerson)
    , m_position(0.0f, 5.0f, -10.0f)
    , m_rotation(0.0f, 0.0f, 0.0f)
    , m_forward(0.0f, 0.0f, 1.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_right(1.0f, 0.0f, 0.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_distance(10.0f)
    , m_orbitYaw(0.0f)
    , m_orbitPitch(0.3f)
    , m_minDistance(2.0f)
    , m_maxDistance(50.0f)
    , m_followSpeed(5.0f)
    , m_moveSpeed(10.0f)
    , m_rotationSpeed(1.0f)
    , m_zoomSpeed(2.0f)
    , m_fieldOfView(XM_PIDIV4)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
{
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();
}

Camera::~Camera()
{
}

void Camera::Initialize(float screenWidth, float screenHeight)
{
    // Create projection matrix
    float aspectRatio = screenWidth / screenHeight;
    m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fieldOfView, aspectRatio, m_nearPlane, m_farPlane);

    // Update view matrix
    UpdateViewMatrix();
}

void Camera::Update(float deltaTime)
{
    if (m_cameraMode == CameraMode::ThirdPerson)
    {
        UpdateThirdPersonPosition();
    }

    // Update view matrix every frame
    UpdateViewMatrix();
}

void Camera::MoveForward(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR forward = XMLoadFloat3(&m_forward);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorAdd(position, XMVectorScale(forward, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target forward
        MoveTarget(0.0f, 0.0f, m_moveSpeed * deltaTime);
    }
}

void Camera::MoveBackward(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR forward = XMLoadFloat3(&m_forward);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorSubtract(position, XMVectorScale(forward, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target backward
        MoveTarget(0.0f, 0.0f, -m_moveSpeed * deltaTime);
    }
}

void Camera::MoveLeft(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR right = XMLoadFloat3(&m_right);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorSubtract(position, XMVectorScale(right, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target left
        MoveTarget(-m_moveSpeed * deltaTime, 0.0f, 0.0f);
    }
}

void Camera::MoveRight(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR right = XMLoadFloat3(&m_right);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorAdd(position, XMVectorScale(right, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target right
        MoveTarget(m_moveSpeed * deltaTime, 0.0f, 0.0f);
    }
}

void Camera::MoveUp(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR up = XMLoadFloat3(&m_up);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorAdd(position, XMVectorScale(up, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target up
        MoveTarget(0.0f, m_moveSpeed * deltaTime, 0.0f);
    }
}

void Camera::MoveDown(float deltaTime)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        XMVECTOR up = XMLoadFloat3(&m_up);
        XMVECTOR position = XMLoadFloat3(&m_position);
        position = XMVectorSubtract(position, XMVectorScale(up, m_moveSpeed * deltaTime));
        XMStoreFloat3(&m_position, position);
    }
    else
    {
        // In third person, move the target down
        MoveTarget(0.0f, -m_moveSpeed * deltaTime, 0.0f);
    }
}

void Camera::MoveTarget(float x, float y, float z)
{
    m_target.x += x;
    m_target.y += y;
    m_target.z += z;
}

void Camera::OrbitAroundTarget(float yaw, float pitch)
{
    m_orbitYaw += yaw * m_rotationSpeed;
    m_orbitPitch += pitch * m_rotationSpeed;

    // Clamp pitch to prevent flipping
    m_orbitPitch = std::max(0.1f, std::min(XM_PI - 0.1f, m_orbitPitch));

    // Wrap yaw around 2π
    if (m_orbitYaw > XM_2PI)
        m_orbitYaw -= XM_2PI;
    else if (m_orbitYaw < -XM_2PI)
        m_orbitYaw += XM_2PI;
}

void Camera::ZoomToTarget(float zoomDelta)
{
    m_distance -= zoomDelta * m_zoomSpeed;
    m_distance = std::max(m_minDistance, std::min(m_maxDistance, m_distance));
}

void Camera::Rotate(float yaw, float pitch)
{
    if (m_cameraMode == CameraMode::FirstPerson)
    {
        m_rotation.y += yaw;
        m_rotation.x += pitch;

        // Clamp pitch to prevent over-rotation
        m_rotation.x = std::max(-XM_PIDIV2 + 0.1f, std::min(XM_PIDIV2 - 0.1f, m_rotation.x));

        // Wrap yaw around 2π
        if (m_rotation.y > XM_2PI)
            m_rotation.y -= XM_2PI;
        else if (m_rotation.y < -XM_2PI)
            m_rotation.y += XM_2PI;
    }
    else
    {
        // In third person mode, orbit around target
        OrbitAroundTarget(yaw, pitch);
    }
}

void Camera::SetPosition(float x, float y, float z)
{
    m_position.x = x;
    m_position.y = y;
    m_position.z = z;
}

void Camera::SetRotation(float pitch, float yaw, float roll)
{
    m_rotation.x = pitch;
    m_rotation.y = yaw;
    m_rotation.z = roll;
}

void Camera::UpdateThirdPersonPosition()
{
    // Calculate position based on spherical coordinates around target
    float x = m_target.x + m_distance * sin(m_orbitPitch) * cos(m_orbitYaw);
    float y = m_target.y + m_distance * cos(m_orbitPitch);
    float z = m_target.z + m_distance * sin(m_orbitPitch) * sin(m_orbitYaw);

    m_position.x = x;
    m_position.y = y;
    m_position.z = z;
}

void Camera::UpdateViewMatrix()
{
    if (m_cameraMode == CameraMode::ThirdPerson)
    {
        // Third person: always look at target
        XMVECTOR position = XMLoadFloat3(&m_position);
        XMVECTOR target = XMLoadFloat3(&m_target);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        m_viewMatrix = XMMatrixLookAtLH(position, target, up);

        // Update camera vectors for consistency
        XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(target, position));
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
        XMVECTOR realUp = XMVector3Cross(forward, right);

        XMStoreFloat3(&m_forward, forward);
        XMStoreFloat3(&m_right, right);
        XMStoreFloat3(&m_up, realUp);
    }
    else
    {
        // First person: existing implementation
        // Create rotation matrix from pitch and yaw
        XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);

        // Calculate camera vectors
        XMVECTOR forwardBase = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMVECTOR upBase = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR rightBase = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

        // Transform base vectors by rotation matrix
        XMVECTOR forward = XMVector3TransformCoord(forwardBase, rotationMatrix);
        XMVECTOR up = XMVector3TransformCoord(upBase, rotationMatrix);
        XMVECTOR right = XMVector3TransformCoord(rightBase, rotationMatrix);

        // Store transformed vectors
        XMStoreFloat3(&m_forward, forward);
        XMStoreFloat3(&m_up, up);
        XMStoreFloat3(&m_right, right);

        // Calculate look at position
        XMVECTOR position = XMLoadFloat3(&m_position);
        XMVECTOR lookAt = XMVectorAdd(position, forward);

        // Create view matrix
        m_viewMatrix = XMMatrixLookAtLH(position, lookAt, up);
    }
}
