cmake_minimum_required(VERSION 3.20)
project(DX11Engine)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Find DirectX
if(WIN32)
    # DirectX libraries are typically found in Windows SDK
    find_library(D3D11_LIBRARY d3d11)
    find_library(D3DCOMPILER_LIBRARY d3dcompiler)
    find_library(DXGI_LIBRARY dxgi)

    if(NOT D3D11_LIBRARY)
        message(FATAL_ERROR "DirectX 11 library not found")
    endif()

    if(NOT D3DCOMPILER_LIBRARY)
        message(FATAL_ERROR "D3D compiler library not found")
    endif()

    if(NOT DXGI_LIBRARY)
        message(FATAL_ERROR "DXGI library not found")
    endif()
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Source files
set(ENGINE_SOURCES
    Engine/Engine.cpp
    Engine/Camera.cpp
    Engine/Renderer.cpp
    Engine/GameLoop.cpp
)

set(ENGINE_HEADERS
    Engine/Engine.h
    Engine/Camera.h
    Engine/Renderer.h
    Engine/GameLoop.h
)

# Graphics subsystem
set(GRAPHICS_SOURCES
    Graphics/Shader.cpp
    Graphics/ModelLoader.cpp
    Graphics/PostProcess.cpp
    Graphics/Animation.cpp
)

set(GRAPHICS_HEADERS
    Graphics/Shader.h
    Graphics/ModelLoader.h
    Graphics/PostProcess.h
    Graphics/Animation.h
)

# Resources subsystem
set(RESOURCES_SOURCES
    Resources/Material.cpp
    Resources/Texture.cpp
    Resources/Mesh.cpp
    Resources/Model.cpp
)

set(RESOURCES_HEADERS
    Resources/Material.h
    Resources/Texture.h
    Resources/Mesh.h
    Resources/Model.h
)

# Create executable
add_executable(${PROJECT_NAME}
    main.cpp
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${GRAPHICS_SOURCES}
    ${GRAPHICS_HEADERS}
    ${RESOURCES_SOURCES}
    ${RESOURCES_HEADERS}
)

# Link libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${D3D11_LIBRARY}
        ${D3DCOMPILER_LIBRARY}
        ${DXGI_LIBRARY}
    )
endif()

# Set startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# Copy DLLs to output directory for debug builds
if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add custom command to copy necessary DLLs if needed
endif()

# Set working directory for Visual Studio
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Group source files in Visual Studio
source_group("Engine" FILES ${ENGINE_SOURCES} ${ENGINE_HEADERS})
source_group("Graphics" FILES ${GRAPHICS_SOURCES} ${GRAPHICS_HEADERS})
source_group("Resources" FILES ${RESOURCES_SOURCES} ${RESOURCES_HEADERS})
source_group("Main" FILES main.cpp)
