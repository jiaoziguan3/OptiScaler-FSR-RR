#pragma once
#include "FSR31Feature_Dx12.h"
#include "OptiTexts.h"
#include "shaders/fsrd_preprocess/FSRDPreprocessor_Dx12.h"
#include <DirectXMath.h>

/**
 * @brief Unfied denoiser-upscaler utilising AMD FSR Ray Regeneration and Super Resolution with
 * DLSS-RR inputs. Extends FSR 3.1+ upscaler implementation.
 */
class FSRDFeatureDx12 : public FSR31FeatureDx12
{
  public:
    using FSRDConvDesc = FSRDPreprocessor_Dx12::ConversionDesc;

    FSRDFeatureDx12(uint32_t InHandleId, NVSDK_NGX_Parameter* InParameters);

    ~FSRDFeatureDx12();

    feature_version Version() override { return FSR31FeatureDx12::Version(); }

    std::string Name() const override { return OptiTexts::FSR_RR_Name; }

    std::string SubFeatureVersionString() const override;

    Upscaler GetUpscalerType() const override { return Upscaler::FSR_RR; }

    bool Evaluate(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) override;

  private:

    union DenoiserConfiguration
    {
        static constexpr uint32_t kCount = FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD;

        // Ordered by FfxApiConfigureDenoiserKey
        struct
        {
            float m_CrossBilateralNormalStrength;
            float m_StabilityBias;
            float m_MaxRadiance;
            float m_RadianceClipStdK;
            float m_GaussianKernelRelaxation;
            float m_DisocclusionThreshold;
        };

        float AsArray[kCount];

        static int GetKeyIndex(FfxApiConfigureDenoiserKey key) 
        {
            return std::clamp((int) key - 1, 0, (int)DenoiserConfiguration::kCount - 1);
        }

        static FfxApiConfigureDenoiserKey GetIndexKey(int index)
        {
            index = std::clamp(index + 1, 1, (int) DenoiserConfiguration::kCount);
            return static_cast<FfxApiConfigureDenoiserKey>(index);
        }

        float& GetMember(int index) { return AsArray[index]; }

        float& GetMember(FfxApiConfigureDenoiserKey key) { return AsArray[GetKeyIndex(key)]; }
    };

    ffxContext _pDenoiserCtx;
    ffxCreateContextDescDenoiser _denoiserCtxDesc;
    DenoiserConfiguration _denoiserSettings;
    bool _isMode2;

    static bool s_isHWDepth;
    static bool s_isRoughnessPacked;

    FSRDConvDesc _convDesc;
    DirectX::XMFLOAT3 _lastCamPos; // Last world space camera position
    DirectX::XMFLOAT2 _lastJitter; // Last frame's sub-pixel jitter offsets
    float _deltaTime;              // Frame delta time in milliseconds

    // Matrices
    DirectX::XMMATRIX _invViewMatrix;          // Camera rotation and translation
    DirectX::XMMATRIX _viewMatrix;             // World to camera space
    DirectX::XMMATRIX _prevViewMatrix;         // Last world to camera space
    DirectX::XMMATRIX _projMatrix;             // Perspective projection matrix (may contain jitter)
    DirectX::XMMATRIX _unjitteredProjMatrix;   // Same projection with jitter removed for FSR-RR inputs
    bool _isRightHanded;                       // True if the camera matrix is right handed

    std::unique_ptr<FSRDPreprocessor_Dx12> FSRDConvShader;

    bool InitFSR3(const NVSDK_NGX_Parameter* InParameters) override;

    void ConfigureUpscalerResponse() override;

    bool CreateDenoiserContext();

    bool QueryDenoiserVersions();

    void DestroyDenoiserContext();

    bool UpdateSize();

    /**
     * @brief Generates FFX denoiser dispatch configuration from DLSS-RR inputs and NGX configurations.
     * Sets up motion vectors, depth, camera matrices, jitter, etc. Does NOT link signal descriptors.
     * @note v1.2.0: Call GetSignal afterwards to link signal descriptors.
     */
    bool PrepareDenoiserDispatch(ID3D12GraphicsCommandList* InCommandList, const NVSDK_NGX_Parameter& ngxParams,
                                 ffxDispatchDescDenoiser& dispatchDesc);

    /**
     * @brief Retrieves DLSS-RR inputs to populate the inputs for the interop layer in order to generate
     FSR-RR compatible buffers.
     */
    bool PrepareDenoiseConvInput(const NVSDK_NGX_Parameter& inParams);

    /**
     * @brief Converts previously retrieved DLSS-RR resources into FSR-RR inputs.
     */
    bool ConvertDenoiserBuffers(ID3D12GraphicsCommandList* InCommandList);

    /**
     * @brief Dispatches FSR-RR denoiser converted inputs. Runs before upscaler.
     */
    bool DispatchDenoiser(ID3D12GraphicsCommandList* InCommandList, const ffxDispatchDescDenoiser& dispatchDesc);

    void SetDefaultConfiguration();

    ffxReturnCode_t SetDefaultConfiguration(FfxApiConfigureDenoiserKey key);

    ffxReturnCode_t ApplyConfiguration(FfxApiConfigureDenoiserKey key);
};
