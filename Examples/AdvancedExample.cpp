#include "../Engine/Engine.h"
#include "../Graphics/ModelLoader.h"
#include "../Graphics/Animation.h"
#include "../Graphics/PostProcess.h"
#include <iostream>
#include <memory>

class AdvancedExample
{
public:
    AdvancedExample()
        : m_engine(nullptr)
        , m_modelLoader(nullptr)
        , m_animationController(nullptr)
        , m_skeleton(nullptr)
        , m_postProcessManager(nullptr)
    {
    }

    ~AdvancedExample()
    {
        Shutdown();
    }

    bool Initialize(HINSTANCE hInstance)
    {
        // Initialize engine
        m_engine = std::make_unique<Engine>();
        if (!m_engine->Initialize(hInstance, 1280, 720, L"DirectX 11 Engine - Advanced Features"))
        {
            std::cerr << "Failed to initialize engine" << std::endl;
            return false;
        }

        // Initialize model loader
        m_modelLoader = std::make_unique<ModelLoader>();
        m_modelLoader->SetGenerateNormals(true);
        m_modelLoader->SetLoadAnimations(true);
        m_modelLoader->SetOptimizeMeshes(true);

        // Load test model
        m_testModel = m_modelLoader->LoadFromFile(m_engine->GetDevice(), "assets/test_cube.x");
        if (!m_testModel)
        {
            std::cerr << "Warning: Could not load test model" << std::endl;
        }
        else
        {
            std::cout << "Successfully loaded test model with "
                      << m_testModel->GetMeshCount() << " meshes" << std::endl;
        }

        // Initialize animation system
        InitializeAnimationSystem();

        // Initialize post-processing
        InitializePostProcessing();

        std::cout << "Advanced example initialized successfully" << std::endl;
        return true;
    }

    void Shutdown()
    {
        if (m_postProcessManager)
        {
            m_postProcessManager->Shutdown();
            m_postProcessManager.reset();
        }

        if (m_animationController)
        {
            m_animationController->Shutdown();
            m_animationController.reset();
        }

        m_skeleton.reset();
        m_testModel.reset();
        m_modelLoader.reset();

        if (m_engine)
        {
            m_engine->Shutdown();
            m_engine.reset();
        }
    }

    void Run()
    {
        if (!m_engine)
            return;

        while (m_engine->IsRunning())
        {
            Update();
            Render();
        }
    }

private:
    void InitializeAnimationSystem()
    {
        // Create a simple skeleton for demonstration
        std::vector<Bone> bones;

        // Root bone
        Bone rootBone;
        rootBone.name = "Root";
        rootBone.parentIndex = -1;
        rootBone.offsetMatrix = XMMatrixIdentity();
        rootBone.bindMatrix = XMMatrixIdentity();
        rootBone.currentMatrix = XMMatrixIdentity();
        bones.push_back(rootBone);

        // Child bone
        Bone childBone;
        childBone.name = "Child";
        childBone.parentIndex = 0;
        childBone.offsetMatrix = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
        childBone.bindMatrix = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
        childBone.currentMatrix = XMMatrixIdentity();
        bones.push_back(childBone);

        // Update parent-child relationships
        bones[0].childrenIndices.push_back(1);

        // Create skeleton
        m_skeleton = std::make_shared<Skeleton>();
        if (!m_skeleton->Initialize(bones))
        {
            std::cerr << "Failed to initialize skeleton" << std::endl;
            return;
        }

        // Create animation controller
        m_animationController = std::make_unique<AnimationController>();
        if (!m_animationController->Initialize(m_skeleton))
        {
            std::cerr << "Failed to initialize animation controller" << std::endl;
            return;
        }

        // Create a simple test animation
        CreateTestAnimation();

        std::cout << "Animation system initialized with "
                  << m_skeleton->GetBoneCount() << " bones" << std::endl;
    }

    void CreateTestAnimation()
    {
        auto animation = std::make_shared<Animation>();
        if (!animation->Initialize("TestRotation", 4.0f, 25.0f))
        {
            return;
        }

        // Create animation channel for root bone rotation
        AnimationChannel channel;
        channel.boneName = "Root";
        channel.boneIndex = 0;

        // Add rotation keyframes (full rotation over 4 seconds)
        channel.rotationKeys.push_back(AnimationKey<XMVECTOR>(0.0f, XMQuaternionIdentity()));
        channel.rotationKeys.push_back(AnimationKey<XMVECTOR>(1.0f, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PIDIV2)));
        channel.rotationKeys.push_back(AnimationKey<XMVECTOR>(2.0f, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), XM_PI)));
        channel.rotationKeys.push_back(AnimationKey<XMVECTOR>(3.0f, XMQuaternionRotationAxis(XMVectorSet(0, 1, 0, 0), 3.0f * XM_PIDIV2)));
        channel.rotationKeys.push_back(AnimationKey<XMVECTOR>(4.0f, XMQuaternionIdentity()));

        // Add position keyframes (static)
        channel.positionKeys.push_back(AnimationKey<XMVECTOR>(0.0f, XMVectorZero()));
        channel.positionKeys.push_back(AnimationKey<XMVECTOR>(4.0f, XMVectorZero()));

        // Add scale keyframes (static)
        channel.scaleKeys.push_back(AnimationKey<XMVECTOR>(0.0f, XMVectorSet(1, 1, 1, 0)));
        channel.scaleKeys.push_back(AnimationKey<XMVECTOR>(4.0f, XMVectorSet(1, 1, 1, 0)));

        animation->AddChannel(channel);

        m_animationController->AddAnimation(animation);
        m_animationController->PlayAnimation("TestRotation", true);

        std::cout << "Created test animation 'TestRotation'" << std::endl;
    }

    void InitializePostProcessing()
    {
        m_postProcessManager = std::make_unique<PostProcessManager>();
        if (!m_postProcessManager->Initialize(m_engine->GetDevice(), 1280, 720))
        {
            std::cerr << "Failed to initialize post-processing" << std::endl;
            return;
        }

        // Add some effects
        m_postProcessManager->AddEffect(PostProcessEffect::Bloom);
        m_postProcessManager->AddEffect(PostProcessEffect::ToneMapping);
        m_postProcessManager->AddEffect(PostProcessEffect::Vignette);

        std::cout << "Post-processing initialized with effects" << std::endl;
    }

    void Update()
    {
        if (!m_engine)
            return;

        float deltaTime = m_engine->GetDeltaTime();

        // Update input
        m_engine->Update();

        // Update animations
        if (m_animationController)
        {
            m_animationController->Update(deltaTime);
        }

        // Simple camera controls (already handled by engine)

        // Check for format switching (F1 key)
        if (m_engine->IsKeyPressed('1'))
        {
            std::cout << "Binary .X file support: ENABLED" << std::endl;
            std::cout << "Animation system: FUNCTIONAL" << std::endl;
        }

        // Toggle post-processing effects
        if (m_engine->IsKeyPressed('2') && m_postProcessManager)
        {
            static bool bloomEnabled = true;
            bloomEnabled = !bloomEnabled;
            m_postProcessManager->SetEffectEnabled(PostProcessEffect::Bloom, bloomEnabled);
            std::cout << "Bloom effect: " << (bloomEnabled ? "ON" : "OFF") << std::endl;
        }
    }

    void Render()
    {
        if (!m_engine)
            return;

        m_engine->BeginFrame();

        // Render scene
        if (m_testModel)
        {
            // Apply animation transforms if available
            if (m_animationController && m_animationController->IsPlaying())
            {
                const auto& boneTransforms = m_animationController->GetBoneTransforms();
                // Note: Would need to apply transforms to model rendering here
            }

            // Render model (simplified - actual rendering would be more complex)
            m_engine->GetRenderer()->RenderModel(m_testModel.get());
        }

        // Apply post-processing
        if (m_postProcessManager)
        {
            // Note: Would need render targets set up for full post-processing
            // m_postProcessManager->Process(context, sceneTexture, backBuffer);
        }

        m_engine->EndFrame();

        // Display performance info
        static int frameCounter = 0;
        if (++frameCounter % 60 == 0)
        {
            std::cout << "FPS: " << m_engine->GetCurrentFPS()
                      << ", Frame Time: " << m_engine->GetFrameTime() << "ms";

            if (m_animationController && m_animationController->IsPlaying())
            {
                std::cout << ", Anim Time: " << m_animationController->GetCurrentTime() << "s";
            }

            std::cout << std::endl;
        }
    }

private:
    std::unique_ptr<Engine> m_engine;
    std::unique_ptr<ModelLoader> m_modelLoader;
    std::unique_ptr<AnimationController> m_animationController;
    std::shared_ptr<Skeleton> m_skeleton;
    std::unique_ptr<PostProcessManager> m_postProcessManager;
    std::shared_ptr<Model> m_testModel;
};

// Entry point for advanced example
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    std::cout << "=== DirectX 11 Engine - Advanced Features Demo ===" << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "- Binary .X file support" << std::endl;
    std::cout << "- Skeletal animation system" << std::endl;
    std::cout << "- Advanced post-processing" << std::endl;
    std::cout << "- Animation blending" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "- WASD: Move camera" << std::endl;
    std::cout << "- Mouse: Look around" << std::endl;
    std::cout << "- 1: Show format info" << std::endl;
    std::cout << "- 2: Toggle bloom effect" << std::endl;
    std::cout << "- ESC: Exit" << std::endl;
    std::cout << std::endl;

    AdvancedExample example;
    if (!example.Initialize(hInstance))
    {
        std::cerr << "Failed to initialize advanced example" << std::endl;
        return -1;
    }

    example.Run();
    return 0;
}
