# Third Person Camera Implementation

## Overview
This implementation adds a complete third-person camera system to the DirectX 11 engine, allowing smooth character movement and camera orbit controls.

## Features Added

### 1. Camera Mode System
- **Dual Mode Support**: Switch between First Person and Third Person modes
- **Toggle Key**: Press 'C' to switch camera modes
- **Seamless Transition**: Camera smoothly adapts to mode changes

### 2. Third Person Camera Controls
- **Target Following**: Camera orbits around a target position (character)
- **Orbit Control**: Mouse movement orbits camera around the character
- **Zoom Function**: Mouse wheel zooms in/out (distance from target)
- **Distance Limits**: Configurable min/max zoom distances

### 3. Character Movement
- **WASD Movement**: Move the character (target) in world space
- **Q/E Vertical**: Move character up/down
- **Visual Representation**: Small cube represents the character position

### 4. Enhanced Input System
- **Mouse Wheel Support**: Added WM_MOUSEWHEEL handling for zoom
- **Mode-Aware Controls**: Controls adapt based on camera mode
- **Improved Sensitivity**: Different mouse sensitivity for each mode

## Technical Implementation

### Camera Class Changes
```cpp
enum class CameraMode { FirstPerson, ThirdPerson };

// New member variables:
- m_cameraMode: Current camera mode
- m_target: Target position to follow
- m_distance: Distance from target
- m_orbitYaw/m_orbitPitch: Orbit angles
- m_minDistance/m_maxDistance: Zoom limits
```

### Key Functions
- `SetCameraMode()`: Switch between camera modes
- `MoveTarget()`: Move the target position
- `OrbitAroundTarget()`: Rotate camera around target
- `ZoomToTarget()`: Adjust camera distance
- `UpdateThirdPersonPosition()`: Calculate camera position using spherical coordinates

### Rendering Enhancements
- **Target Visualization**: Small cube rendered at target position
- **Conditional Rendering**: Different render path for third person mode
- **Reused Geometry**: Target uses scaled cube geometry

## Controls

### Common Controls
- **WASD**: Movement (camera in FP mode, character in TP mode)
- **Q/E**: Vertical movement
- **ESC**: Exit application
- **C**: Toggle camera mode

### Third Person Specific
- **Left Mouse + Drag**: Orbit camera around character
- **Mouse Wheel**: Zoom in/out
- **Target follows**: Character movement with WASD

### First Person Specific
- **Left Mouse + Drag**: Look around (FPS style)
- **Direct camera control**: WASD moves camera directly

## Configuration Parameters

### Camera Settings
```cpp
m_distance = 10.0f;        // Default distance from target
m_minDistance = 2.0f;      // Minimum zoom distance
m_maxDistance = 50.0f;     // Maximum zoom distance
m_orbitPitch = 0.3f;       // Initial vertical angle
m_zoomSpeed = 2.0f;        // Zoom sensitivity
```

### Input Sensitivity
```cpp
// Third person: higher sensitivity for smooth orbiting
float sensitivity = 0.01f;
// First person: lower sensitivity for precise aiming
float sensitivity = 0.005f;
```

## Mathematical Implementation

### Spherical Coordinates
The camera position is calculated using spherical coordinates around the target:
```cpp
x = target.x + distance * sin(pitch) * cos(yaw)
y = target.y + distance * cos(pitch)
z = target.z + distance * sin(pitch) * sin(yaw)
```

### View Matrix
- **Third Person**: LookAt matrix pointing from camera to target
- **First Person**: Standard FPS view matrix

## Usage Examples

### Basic Third Person Setup
```cpp
camera->SetCameraMode(CameraMode::ThirdPerson);
camera->SetTarget(XMFLOAT3(0.0f, 0.0f, 0.0f));
camera->SetDistance(10.0f);
```

### Character Movement
```cpp
// Move character forward
if (keys['W']) {
    camera->MoveTarget(0.0f, 0.0f, speed * deltaTime);
}
```

### Camera Control
```cpp
// Orbit camera with mouse
camera->OrbitAroundTarget(mouseX * sensitivity, mouseY * sensitivity);

// Zoom with mouse wheel
camera->ZoomToTarget(wheelDelta);
```

## Benefits

1. **Flexibility**: Easy switching between camera modes
2. **Smooth Controls**: Interpolated camera movement
3. **Visual Feedback**: Character representation in third person
4. **Configurable**: Adjustable distances and sensitivities
5. **Performance**: Efficient spherical coordinate calculations

## Future Enhancements

- **Camera Collision**: Prevent camera from clipping through objects
- **Auto-Follow**: Smooth camera interpolation toward target
- **Multiple Targets**: Support for following different objects
- **Animation Support**: Integration with character animation system
- **Cinematic Modes**: Pre-defined camera angles and movements

## Compatibility

- **Platform**: Windows with DirectX 11 support
- **Compiler**: Visual Studio 2019+ or compatible C++17 compiler
- **Dependencies**: Windows SDK, DirectX 11, DirectX Math Library

This implementation provides a solid foundation for third-person games and can be easily extended with additional camera features and controls.
