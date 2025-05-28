#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

using namespace DirectX;

// Forward declarations
class Shader;
class Texture;

// Post-process effect types
enum class PostProcessEffect
{
    None = 0,
    Grayscale,
    Sepia,
    Invert,
    Blur,
    GaussianBlur,
    Bloom,
    ToneMapping,
    FXAA,
    Vignette,
    ColorCorrection,
    DepthOfField,
    MotionBlur,
    Count
};

// Post-process parameters structure
struct PostProcessParams
{
    // General parameters
    float intensity;
    float threshold;
    float radius;
    float sigma;

    // Color parameters
    XMFLOAT3 colorTint;
    float contrast;
    float brightness;
    float saturation;
    float gamma;

    // Bloom parameters
    float bloomThreshold;
    float bloomIntensity;
    int bloomBlurPasses;

    // Tone mapping parameters
    float exposure;
    float whitePoint;

    // FXAA parameters
    float fxaaSpanMax;
    float fxaaReduceMin;
    float fxaaReduceMul;

    // Vignette parameters
    float vignetteRadius;
    float vignetteSoftness;
    XMFLOAT3 vignetteColor;

    PostProcessParams()
        : intensity(1.0f)
        , threshold(0.5f)
        , radius(1.0f)
        , sigma(1.0f)
        , colorTint(1.0f, 1.0f, 1.0f)
        , contrast(1.0f)
        , brightness(0.0f)
        , saturation(1.0f)
        , gamma(2.2f)
        , bloomThreshold(1.0f)
        , bloomIntensity(1.0f)
        , bloomBlurPasses(3)
        , exposure(1.0f)
        , whitePoint(1.0f)
        , fxaaSpanMax(8.0f)
        , fxaaReduceMin(1.0f/128.0f)
        , fxaaReduceMul(1.0f/8.0f)
        , vignetteRadius(0.8f)
        , vignetteSoftness(0.2f)
        , vignetteColor(0.0f, 0.0f, 0.0f)
    {
    }
};

// Individual post-process effect
class PostProcessEffect_Base
{
public:
    PostProcessEffect_Base(PostProcessEffect type);
    virtual ~PostProcessEffect_Base();

    // Effect application
    virtual bool Initialize(ID3D11Device* device) = 0;
    virtual void Apply(ID3D11DeviceContext* context,
                      ID3D11ShaderResourceView* inputTexture,
                      ID3D11RenderTargetView* outputTarget,
                      const PostProcessParams& params) = 0;
    virtual void Shutdown() = 0;

    // Properties
    PostProcessEffect GetType() const { return m_type; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }
    bool IsInitialized() const { return m_initialized; }

protected:
    PostProcessEffect m_type;
    bool m_enabled;
    bool m_initialized;
};

// Grayscale effect
class GrayscaleEffect : public PostProcessEffect_Base
{
public:
    GrayscaleEffect();
    bool Initialize(ID3D11Device* device) override;
    void Apply(ID3D11DeviceContext* context,
              ID3D11ShaderResourceView* inputTexture,
              ID3D11RenderTargetView* outputTarget,
              const PostProcessParams& params) override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_shader;
    ID3D11Buffer* m_constantBuffer;
};

// Blur effect
class BlurEffect : public PostProcessEffect_Base
{
public:
    BlurEffect();
    bool Initialize(ID3D11Device* device) override;
    void Apply(ID3D11DeviceContext* context,
              ID3D11ShaderResourceView* inputTexture,
              ID3D11RenderTargetView* outputTarget,
              const PostProcessParams& params) override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_horizontalBlurShader;
    std::shared_ptr<Shader> m_verticalBlurShader;
    ID3D11Buffer* m_constantBuffer;
    std::shared_ptr<Texture> m_tempTexture;
};

// Bloom effect
class BloomEffect : public PostProcessEffect_Base
{
public:
    BloomEffect();
    bool Initialize(ID3D11Device* device) override;
    void Apply(ID3D11DeviceContext* context,
              ID3D11ShaderResourceView* inputTexture,
              ID3D11RenderTargetView* outputTarget,
              const PostProcessParams& params) override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_brightPassShader;
    std::shared_ptr<Shader> m_combineShader;
    std::shared_ptr<BlurEffect> m_blurEffect;
    std::shared_ptr<Texture> m_brightPassTexture;
    std::shared_ptr<Texture> m_blurTexture;
    ID3D11Buffer* m_constantBuffer;
};

// Tone mapping effect
class ToneMappingEffect : public PostProcessEffect_Base
{
public:
    ToneMappingEffect();
    bool Initialize(ID3D11Device* device) override;
    void Apply(ID3D11DeviceContext* context,
              ID3D11ShaderResourceView* inputTexture,
              ID3D11RenderTargetView* outputTarget,
              const PostProcessParams& params) override;
    void Shutdown() override;

private:
    std::shared_ptr<Shader> m_shader;
    ID3D11Buffer* m_constantBuffer;
};

// Main post-processing manager
class PostProcessManager
{
public:
    PostProcessManager();
    ~PostProcessManager();

    // Initialization
    bool Initialize(ID3D11Device* device, int width, int height);
    void Shutdown();

    // Effect management
    void AddEffect(PostProcessEffect effectType);
    void RemoveEffect(PostProcessEffect effectType);
    void ClearEffects();
    void SetEffectEnabled(PostProcessEffect effectType, bool enabled);
    bool IsEffectEnabled(PostProcessEffect effectType) const;

    // Parameter management
    void SetEffectParameters(const PostProcessParams& params) { m_parameters = params; }
    const PostProcessParams& GetEffectParameters() const { return m_parameters; }
    PostProcessParams& GetEffectParameters() { return m_parameters; }

    // Main processing function
    void Process(ID3D11DeviceContext* context,
                ID3D11ShaderResourceView* inputTexture,
                ID3D11RenderTargetView* finalOutput);

    // Render target management
    void ResizeRenderTargets(ID3D11Device* device, int width, int height);

    // Utility
    bool IsInitialized() const { return m_initialized; }
    int GetActiveEffectCount() const;

    // Debug
    void SetDebugMode(bool debug) { m_debugMode = debug; }
    bool IsDebugMode() const { return m_debugMode; }

private:
    void CreateRenderTargets(ID3D11Device* device, int width, int height);
    void CreateFullscreenQuad(ID3D11Device* device);
    void RenderFullscreenQuad(ID3D11DeviceContext* context);
    std::shared_ptr<PostProcessEffect_Base> CreateEffect(PostProcessEffect effectType);

private:
    // Effects chain
    std::vector<std::shared_ptr<PostProcessEffect_Base>> m_effects;
    std::unordered_map<PostProcessEffect, std::shared_ptr<PostProcessEffect_Base>> m_effectMap;

    // Parameters
    PostProcessParams m_parameters;

    // Render targets for ping-pong rendering
    std::shared_ptr<Texture> m_renderTarget1;
    std::shared_ptr<Texture> m_renderTarget2;

    // Fullscreen quad for rendering
    ID3D11Buffer* m_fullscreenQuadVB;
    ID3D11Buffer* m_fullscreenQuadIB;
    std::shared_ptr<Shader> m_fullscreenQuadShader;

    // State
    bool m_initialized;
    bool m_debugMode;
    int m_width;
    int m_height;

    // Sampler states
    ID3D11SamplerState* m_linearSampler;
    ID3D11SamplerState* m_pointSampler;
};

// Utility functions and shader code
namespace PostProcessShaders
{
    // Shader source code strings
    extern const char* FULLSCREEN_QUAD_VS;
    extern const char* GRAYSCALE_PS;
    extern const char* BLUR_HORIZONTAL_PS;
    extern const char* BLUR_VERTICAL_PS;
    extern const char* BLOOM_BRIGHT_PASS_PS;
    extern const char* BLOOM_COMBINE_PS;
    extern const char* TONE_MAPPING_PS;
    extern const char* SEPIA_PS;
    extern const char* INVERT_PS;
    extern const char* VIGNETTE_PS;
    extern const char* COLOR_CORRECTION_PS;

    // Utility functions
    const char* GetEffectName(PostProcessEffect effect);
    PostProcessEffect GetEffectFromName(const std::string& name);
}
