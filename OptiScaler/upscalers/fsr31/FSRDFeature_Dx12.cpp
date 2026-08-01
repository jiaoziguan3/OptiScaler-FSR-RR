#include "pch.h"
#include <nvsdk_ngx_defs_dlssd.h>
#include <DirectXMath.h>
#include "NVNGX_Parameter.h"
#include "hooks/Streamline_Hooks.h"
#include "FSRDFeature_Dx12.h"
#include "shaders/fsrd_preprocess/FSRDPreprocessor_Dx12.h"
#include "MathUtils.h"

using namespace DirectX;
using namespace OptiMath;

using FSRDConvDesc = FSRDPreprocessor_Dx12::ConversionDesc;
using FSRDCompDesc = FSRDPreprocessor_Dx12::CompositionDesc;

/**
 * @brief Retrieves a matrix from the given parameter table. Matrices used by DLSS are in column-major
 * order, but DirectXMath operations assume row-major. Appropriate for passing to DirectX shaders, but not for
 * CPU-side operations without transposing.
 */
static bool TryGetNGXMatrixTranspose(const NVSDK_NGX_Parameter& ngxParams, const char* key, DirectX::XMMATRIX& outValue)
{
    float* pMat = nullptr;

    if (ngxParams.Get(key, (void**) &pMat) == NVSDK_NGX_Result_Success && pMat != nullptr)
    {
        memcpy_s(&outValue, sizeof(DirectX::XMMATRIX), pMat, sizeof(float) * 16);
        return true;
    }
    else
        return false;
}

/**
 * @brief Retrieves a matrix from the given parameter table and transposes it for CPU-side
 * operations with DirectXMath.
 */
static bool TryGetNGXMatrix(const NVSDK_NGX_Parameter& ngxParams, const char* key, DirectX::XMMATRIX& outValue)
{
    if (TryGetNGXMatrixTranspose(ngxParams, key, outValue))
    {
        outValue = XMMatrixTranspose(outValue);
        return true;
    }
    else
        return false;
}

template <typename T>
static bool TryGetLoggedResource(const NVSDK_NGX_Parameter& ngxParams, const char* key, T*& outValue)
{
    const bool success = TryGetNGXVoidPointer(ngxParams, key, outValue);

    if (success)
        LOG_DEBUG("{} exists..", key);
    else
        LOG_ERROR("{} is missing!!", key);

    return success;
}

/**
 * @brief Calculates vertical FOV according to: FOVv = 2 * arctan( 1 / M22 )
 * @param proj View to Clip / Perspective projection matrix
 * @return Vertical field of view in radians
 */
static float GetVertFovFromProjectionMatrixRad(const XMMATRIX& proj)
{
    return float(2.0 * (std::atan(1.0 / (double) proj.r[1].m128_f32[1])));
}

/**
 * @brief Calculates horizontal FOV according to: FOVh = 2 * arctan( 1 / M11 )
 * @param proj View to Clip / Perspective projection matrix
 * @return Horizontal field of view in radians
 */
static float GetHorzFovFromProjectionMatrixRad(const XMMATRIX& proj)
{
    return float(2.0 * (std::atan(1.0 / (double) proj.r[0].m128_f32[0])));
}

/**
 * @brief Calculates aspect ratio (width / height) as AR = M22 / M11
 * @param proj View to Clip / Perspective projection matrix
 * @return Aspect ratio as an fp32 decimal e.g. 1.778
 */
static float GetAspectRatioFromProjectionMatrix(const XMMATRIX& proj)
{
    return proj.r[1].m128_f32[1] / proj.r[0].m128_f32[0];
}

/**
 * @brief Converts XMMATRIX to FfxApiMatrix4x4 format (row-major)
 * DirectXMath XMMATRIX and FFX API both use row-major storage with row-vector convention.
 * No transpose needed.
 */
static FfxApiMatrix4x4 ConvertToFFXMatrix(const XMMATRIX& mat)
{
    FfxApiMatrix4x4 result;
    for (int i = 0; i < 4; i++)
    {
        result.rows[i].x = mat.r[i].m128_f32[0];
        result.rows[i].y = mat.r[i].m128_f32[1];
        result.rows[i].z = mat.r[i].m128_f32[2];
        result.rows[i].w = mat.r[i].m128_f32[3];
    }
    return result;
}

/**
 * @brief Converts view matrix (world-to-view) to FFX format
 * FFX-RR requires left-handed view matrix.
 * For right-handed input, negate the Z row to flip view-space Z direction.
 */
static FfxApiMatrix4x4 GetViewMatrixFFX(const XMMATRIX& viewMatrix, bool isRightHanded)
{
    XMMATRIX result = viewMatrix;
    if (isRightHanded)
    {
        // Negate the Z row to flip from right-handed (-Z forward) to left-handed (+Z forward)
        result.r[2] = XMVectorNegate(result.r[2]);
    }
    return ConvertToFFXMatrix(result);
}

/**
 * @brief Converts projection matrix to FFX format
 * The projection matrix is already converted to FSR-RR's left-handed convention before reaching here.
 */
static FfxApiMatrix4x4 GetProjectionMatrixFFX(const XMMATRIX& projMatrix)
{
    return ConvertToFFXMatrix(projMatrix);
}

static XMFLOAT3 GetFloat3(const XMVECTOR& vec4)
{
    XMFLOAT3 vec3 = {};
    XMStoreFloat3(&vec3, vec4);
    return vec3;
}

static XMVECTOR GetColumn(const XMMATRIX& mat, int col)
{
    return { mat.r[0].m128_f32[col], mat.r[1].m128_f32[col], mat.r[2].m128_f32[col], 0 };
}

static void SetColumn(const XMVECTOR& vec, int col, XMMATRIX& mat)
{ 
    mat.r[0].m128_f32[col] = vec.m128_f32[0]; 
    mat.r[1].m128_f32[col] = vec.m128_f32[1]; 
    mat.r[2].m128_f32[col] = vec.m128_f32[2]; 
    mat.r[3].m128_f32[col] = vec.m128_f32[3]; 
}

static XMFLOAT3 GetFloat3Column(const XMMATRIX& mat, int col)
{
    return { mat.r[0].m128_f32[col], mat.r[1].m128_f32[col], mat.r[2].m128_f32[col] };
}

static FfxApiFloatCoords3D GetFloat3ColumnFFX(const XMMATRIX& mat, int col)
{
    return { mat.r[0].m128_f32[col], mat.r[1].m128_f32[col], mat.r[2].m128_f32[col] };
}

static FfxApiFloatCoords3D GetFloat3FFX(const XMVECTOR& vec4)
{
    FfxApiFloatCoords3D vec3 = {};
    XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&vec3), vec4);
    return vec3;
}

static const FfxApiFloatCoords3D& GetFloat3FFX(const XMFLOAT3& vec3)
{
    return *reinterpret_cast<const FfxApiFloatCoords3D*>(&vec3);
}

static ID3D12Resource* GetD3D12ResFromFFX(const FfxApiResource& resource)
{
    return static_cast<ID3D12Resource*>(resource.resource);
}

struct ViewPlanes
{
    float nearPlane;
    float farPlane;
    bool isInfinite;
    bool isRightHanded;
};

static ViewPlanes GetViewPlanes(const DirectX::XMMATRIX& projection, bool isInverted)
{
    ViewPlanes planes;
    // View to clip
    float A = projection.r[2].m128_f32[2];
    float B = projection.r[2].m128_f32[3];
    float W = projection.r[3].m128_f32[2];

    float infiniteCheckVal = isInverted ? A : (A - W);
    planes.isInfinite = std::abs(infiniteCheckVal) < 1e-6f;
    planes.isRightHanded = B < 0.0f;

    if (isInverted)
    {
        // Inverted: Near is at D=1, Far is at D=0
        // 1 = A/W + B/(n*W) -> n = B / (W - A)
        planes.nearPlane = std::abs(B / (W - A));

        // 0 = A/W + B/(f*W) -> f = -B / A
        planes.farPlane = std::abs(-B / A);
    }
    else
    {
        // Standard: Near is at D=0, Far is at D=1
        // 0 = A/W + B/(n*W) -> n = -B / A
        planes.nearPlane = std::abs(-B / A);

        // 1 = A/W + B/(f*W) -> f = B / (W - A)
        planes.farPlane = std::abs(B / (W - A));
    }

    return planes;
}

using FSRDConvFlags = FSRDPreprocessor_Dx12::ConvFlags;
using FSRDCompFlags = FSRDPreprocessor_Dx12::CompFlags;

enum class DebugModes : uint64_t
{
    None = 0,
    DenoiserBypass = 1,
    UpscalerBypass = 2,
    RawColor = 3,
    DlssBias = 4,
    DlssColorBeforeParticles = 5,
    DlssColorBeforeTransparency = 6,
    DlssTransparencyLayer = 7,
    FfxDebug = 8,

    ConversionDebug = FSRDConvFlags::Debug,
    ConversionDebugMask = FSRDConvFlags::DebugModeMask,

    OutRadiance = FSRDConvFlags::DebugOutRadiance,

    InSpecHitDist = FSRDConvFlags::DebugInSpecHitDist,
    InMotion = FSRDConvFlags::DebugInMotion,
    InNormals = FSRDConvFlags::DebugInNormals,
    InRoughness = FSRDConvFlags::DebugInRoughness,
    InDiffAlbedo = FSRDConvFlags::DebugInDiffAlbedo,
    InSpecAlbedo = FSRDConvFlags::DebugInSpecAlbedo,

    OutFusedAlbedo = FSRDConvFlags::DebugOutFusedAlbedo,
    OutLinearDepth = FSRDConvFlags::DebugOutLinearDepth,
    OutMotion = FSRDConvFlags::DebugOutMotion,
    OutNormals = FSRDConvFlags::DebugOutNormals,
    OutSpecAlbedo = FSRDConvFlags::DebugOutSpecAlbedo,
    OutDiffAlbedo = FSRDConvFlags::DebugOutDiffAlbedo,

    OutDepthDelta = FSRDConvFlags::DebugOutDepthDelta,
    NormDepth = FSRDConvFlags::DebugNormDepth,
    AlbedoError = FSRDConvFlags::DebugAlbedoError,

    FloorVariance = FSRDConvFlags::DebugFloorVariance,
    FloorColor = FSRDConvFlags::DebugFloorColor,
    LodProxy = FSRDConvFlags::DebugLodProxy,

    CompositionDebugOffset = 16u,
    CompositionDebug = (uint64_t) FSRDCompFlags::Debug << CompositionDebugOffset,
    CompositionDebugMask = (uint64_t)FSRDCompFlags::DebugModeMask,

    Correlation = (uint64_t)FSRDCompFlags::DebugCorrelation << CompositionDebugOffset,
    SkipSignal = (uint64_t) FSRDCompFlags::DebugSkipSignal << CompositionDebugOffset,
    DenoiserOutput = (uint64_t) FSRDCompFlags::DebugDenoiserOutput << CompositionDebugOffset,
    Signal1 = (uint64_t) FSRDCompFlags::DebugSignal1 << CompositionDebugOffset,
    Signal2 = (uint64_t) FSRDCompFlags::DebugSignal2 << CompositionDebugOffset,
};

static FSRDConvFlags GetConvDebugFlags(DebugModes mode) 
{ 
    uint32_t flags = uint32_t(mode);
    flags &= uint32_t(DebugModes::ConversionDebugMask);
    return FSRDConvFlags(flags);
}

static FSRDCompFlags GetCompDebugFlags(DebugModes mode) 
{ 
    uint64_t flags = uint64_t(mode);
    flags >>= uint64_t(DebugModes::CompositionDebugOffset);
    flags &= uint64_t(DebugModes::CompositionDebugMask);
    return FSRDCompFlags(flags);
}

using ModeNamePair = std::pair<const char*, uint64_t>;
constexpr auto kDebugModes = std::to_array<ModeNamePair>(
{
    { "None", (uint64_t) DebugModes::None },
    { "DebugOverview", (uint64_t) DebugModes::FfxDebug },

    { "DenoiserBypass", (uint64_t) DebugModes::DenoiserBypass },
    { "UpscalerBypass", (uint64_t) DebugModes::UpscalerBypass },
    { "DenoiserOutput", (uint64_t) DebugModes::DenoiserOutput },
    { "SkipSignal", (uint64_t) DebugModes::SkipSignal },

    { "RawColor", (uint64_t) DebugModes::RawColor },
    { "DlssBias", (uint64_t) DebugModes::DlssBias },
    { "DlssColorBeforeParticles", (uint64_t) DebugModes::DlssColorBeforeParticles },
    { "DlssColorBeforeTransparency", (uint64_t) DebugModes::DlssColorBeforeTransparency },
    { "DlssTransparencyLayer", (uint64_t) DebugModes::DlssTransparencyLayer },

    { "InMotionVectors", (uint64_t) DebugModes::InMotion },
    { "InNormals", (uint64_t) DebugModes::InNormals },
    { "InRoughness", (uint64_t) DebugModes::InRoughness },
    { "InSpecHitDist", (uint64_t) DebugModes::InSpecHitDist },
    { "InDiffAlbedo", (uint64_t) DebugModes::InDiffAlbedo },
    { "InSpecAlbedo", (uint64_t) DebugModes::InSpecAlbedo },

    { "OutRadiance", (uint64_t) DebugModes::OutRadiance },
    { "OutFusedAlbedo", (uint64_t) DebugModes::OutFusedAlbedo },
    { "OutLinearDepth", (uint64_t) DebugModes::OutLinearDepth },
    { "OutMotionVectors", (uint64_t) DebugModes::OutMotion },
    { "OutNormals", (uint64_t) DebugModes::OutNormals },
    { "OutSpecAlbedo", (uint64_t) DebugModes::OutSpecAlbedo },
    { "OutDiffAlbedo", (uint64_t) DebugModes::OutDiffAlbedo },
    { "OutDepthDelta", (uint64_t) DebugModes::OutDepthDelta },
    { "NormDepth", (uint64_t) DebugModes::NormDepth },

    { "AlbedoError", (uint64_t) DebugModes::AlbedoError },
    { "Correlation", (uint64_t) DebugModes::Correlation },

    { "FloorVariance", (uint64_t) DebugModes::FloorVariance },
    { "FloorColor", (uint64_t) DebugModes::FloorColor },
    { "LodProxy", (uint64_t) DebugModes::LodProxy },

    { "Signal1", (uint64_t) DebugModes::Signal1 },
    { "Signal2", (uint64_t) DebugModes::Signal2 },
});


constexpr auto kDenoiserModes = std::to_array<std::pair<const char*, int>>(
{ 
    { "Mode 2", 0 }, 
    { "Mode 1", 1 }, 
});

bool FSRDFeatureDx12::s_isHWDepth = false;
bool FSRDFeatureDx12::s_isRoughnessPacked = false;

FSRDFeatureDx12::FSRDFeatureDx12(uint32_t InHandleId, NVSDK_NGX_Parameter* InParameters) : 
    FSR31FeatureDx12(InHandleId, InParameters),
    IFeature(InHandleId, SetParameters(InParameters)),  
    _pDenoiserCtx(nullptr), 
    _denoiserCtxDesc({}),
    _denoiserSettings({}), 
    _convDesc({}),
    _isMode2(false),
    _isRightHanded(false),
    _lastJitter({ 0.0f, 0.0f })
{
    _moduleLoaded = FfxApiProxy::IsDenoiserReady();

    if (_moduleLoaded)
        LOG_INFO("amd_fidelityfx_denoiser_dx12.dll methods loaded!");
    else
        LOG_ERROR("can't load amd_fidelityfx_denoiser_dx12.dll methods!");
}

FSRDFeatureDx12::~FSRDFeatureDx12() 
{
    if (State::Instance().isShuttingDown)
        return;

    DestroyDenoiserContext();
}

bool FSRDFeatureDx12::InitFSR3(const NVSDK_NGX_Parameter* InParameters)
{
    LOG_FUNC();

    // Init upscaler first - borrow some init boilerplate and some cfg
    if (FSR31FeatureDx12::InitFSR3(InParameters))
    {
        SetInit(false);

        LOG_DEBUG("FSR Ray Regeneration Initializing");
        _name = OptiTexts::FSR_RR_Name;

        if (int value; InParameters->Get(NVSDK_NGX_Parameter_Use_HW_Depth, &value) == NVSDK_NGX_Result_Success)
            s_isHWDepth = value == NVSDK_NGX_DLSS_Depth_Type_HW;

        if (int value; InParameters->Get(NVSDK_NGX_Parameter_DLSS_Roughness_Mode, &value) == NVSDK_NGX_Result_Success)
            s_isRoughnessPacked = value == NVSDK_NGX_DLSS_Roughness_Mode_Packed;

        LOG_INFO("DLSSD Flags HWDepth: {} - IsRoughnessPacked: {}", s_isHWDepth, s_isRoughnessPacked);

        if (!CreateDenoiserContext())
            return false;

        LOG_INFO("FSR Ray Regeneration Initialized");

        SetInit(true);
        return true;
    }
 
    return false;
}

void FSRDFeatureDx12::ConfigureUpscalerResponse()
{
    const auto& cfg = *Config::Instance();
    const float shadingScale = std::max(cfg.FsrShadingScale.value_or_default(), 4.0f);
    const float accumulation = std::max(cfg.FsrAccAddPerFrame.value_or_default(), 0.75f);
    ApplyUpscalerResponse(shadingScale, accumulation);
}

bool FSRDFeatureDx12::CreateDenoiserContext() 
{
    ScopedSkipSpoofing skipSpoofing {};
    auto& state = State::Instance();
    const auto& cfg = *Config::Instance();

    if (!QueryDenoiserVersions())
        return false;

    state.ffxDenoiserUpscalerVersion = Version();
    parse_version(state.ffxDenoiserVersionNames[cfg.FfxDenoiserIndex.value_or_default()]);

    // parse_version mutates the shared FSR version store. Restore the upscaler
    // version immediately so that Version() keeps reporting the FSR version
    // (used by the UI and by FSR31Feature checks), while the denoiser version
    // remains available through state.ffxDenoiserVersionNames.
    {
        const feature_version savedUpscalerVersion = state.ffxDenoiserUpscalerVersion;
        const std::string verStr = std::to_string(savedUpscalerVersion.major) + "." +
                                   std::to_string(savedUpscalerVersion.minor) + "." +
                                   std::to_string(savedUpscalerVersion.patch);
        parse_version(verStr.c_str());
    }

    // Get current mode and populate mode map
    _isMode2 = cfg.FfxDenoiserMode.value_or_default() == 0;
    state.ffxDenoiserModes.resize(kDenoiserModes.size());
    state.ffxDenoiserModeNames.reserve(kDenoiserModes.size());
    state.ffxDenoiserModes.clear();
    state.ffxDenoiserModeNames.clear();

    for (const auto& mode : kDenoiserModes)
    {
        state.ffxDenoiserModes.push_back(mode.second);
        state.ffxDenoiserModeNames.emplace(mode.second, mode.first);
    }

    ffxOverrideVersion vidOverride = 
    {
        .header = { .type = FFX_API_DESC_TYPE_OVERRIDE_VERSION },
        .versionId = state.ffxDenoiserVersionIds[cfg.FfxDenoiserIndex.value_or_default()]
    };
    // Create context
    // Backend desc
    ffxCreateBackendDX12Desc backendDesc = 
    { 
        .header = 
        { 
            .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12,
            .pNext = &vidOverride.header // Chain override into backend desc
        },
        .device = Device
    };    
    // Chain: ContextDesc -> BackendDesc -> OverrideVersion
    // Composited radiance with fused albedo without a dominant light source
    _denoiserCtxDesc = 
    {
        .header = 
        { 
            .type = FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER,
            // Chain backend desc into context desc
            .pNext = &backendDesc.header
        },
        .version = FFX_DENOISER_VERSION,
        .maxRenderSize = { RenderWidth(), RenderHeight() },
        // v1.2.0: DLSS-RR inputs are ray-traced indirect lighting.
        // Mode2: separate indirect specular + indirect diffuse
        // Mode1: composited radiance mapped to indirect diffuse
        .signalFlags = _isMode2 ? uint32_t(FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR | FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE) : uint32_t(FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE),
        .checkerboardSignalFlags = 0,
        .flags = 0
    };

#ifdef _DEBUG
    LOG_INFO("Debug checking enabled for denoiser!");
    _denoiserCtxDesc.flags |= FFX_DENOISER_ENABLE_DEBUGGING;
#endif

    // Create the denoiser context
    {   
        ScopedSkipHeapCapture skipHeapCapture {};
        auto ret = FfxApiProxy::D3D12_CreateContext(&_pDenoiserCtx, &_denoiserCtxDesc.header, NULL);

        if (ret != FFX_API_RETURN_OK)
        {
            LOG_ERROR("_denoiserCtx error: {0}", FfxApiProxy::ReturnCodeToString(ret));
            return false;
        }
    }

    // Query default settings
    SetDefaultConfiguration();

    // Create DLSS-RR to FSR-RR input converter
    FSRDConvShader = std::make_unique<FSRDPreprocessor_Dx12>("FSRD Converter", Device, _isMode2);

    if (!FSRDConvShader->IsInit())
        return false;

    if (!FSRDConvShader->SetMaxRenderSize(_denoiserCtxDesc.maxRenderSize.width, _denoiserCtxDesc.maxRenderSize.height))
        return false;

    return true;
}

bool FSRDFeatureDx12::QueryDenoiserVersions() 
{
    ScopedSkipSpoofing skipSpoofing {};
    auto& state = State::Instance();

    // Get version count
    uint64_t versionCount = 0;
    ffxQueryDescGetVersions queryVersionsDesc = 
    { 
        .header = { .type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS },
        .createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER,
        .device = Device,
        .outputCount = &versionCount
    };
    ffxReturnCode_t queryResult = FfxApiProxy::D3D12_Query(nullptr, &queryVersionsDesc.header);
    if (queryResult != FFX_API_RETURN_OK)
    {
        LOG_ERROR("D3D12_Query for denoiser versions failed with code: {}", queryResult);
        return false;
    }

    state.ffxDenoiserVersionIds.resize(versionCount);
    state.ffxDenoiserVersionNames.resize(versionCount);

    state.ffxDenoiserDebugModes.clear();
    state.ffxDenoiserDebugModeNames.clear();

    for (const auto& mode : kDebugModes)
    {
        state.ffxDenoiserDebugModes.push_back(mode.second);
        state.ffxDenoiserDebugModeNames.emplace(mode.second, mode.first);
    }

    if (versionCount == 0)
    {
        LOG_ERROR("No FSR-RR denoisers were found.");
        return false;
    }
    else
        LOG_DEBUG("Found {} versions of FSR-RR", versionCount);

    LOG_DEBUG("Initialising FSR denoiser context");

    // Get version IDs
    queryVersionsDesc.versionIds = state.ffxDenoiserVersionIds.data();
    queryVersionsDesc.versionNames = state.ffxDenoiserVersionNames.data();
    FfxApiProxy::D3D12_Query(nullptr, &queryVersionsDesc.header);

    return true;
}

void FSRDFeatureDx12::DestroyDenoiserContext() 
{
    if (_pDenoiserCtx != nullptr)
    {
        FfxApiProxy::D3D12_DestroyContext(&_pDenoiserCtx, nullptr);
        _pDenoiserCtx = nullptr;
    }
}

bool FSRDFeatureDx12::UpdateSize() 
{
    // FSR-RR doesn't currently have proper DRS support. The example implementation 
    // reinits on resolution change as well.
    const bool needsReInit = 
        _denoiserCtxDesc.maxRenderSize.width != RenderWidth() ||
        _denoiserCtxDesc.maxRenderSize.height != RenderHeight();

    if (needsReInit)
    {
        LOG_INFO(
            "Reinitializing FSR-RR for resolution change. "
            "Previous: {} x {}, New: {} x {}",
            _denoiserCtxDesc.maxRenderSize.width, _denoiserCtxDesc.maxRenderSize.height,
            RenderWidth(), RenderHeight());

        DestroyDenoiserContext();
        return CreateDenoiserContext();
    }

    return true;
}

bool FSRDFeatureDx12::Evaluate(ID3D12GraphicsCommandList* InCommandList, NVSDK_NGX_Parameter* InParameters) 
{
    LOG_FUNC();

    if (!IsInited())
        return false;

    auto& state = State::Instance();
    auto& cfg = *Config::Instance();
    const auto& inParams = *InParameters;

    unsigned int renderWidth = RenderWidth();
    unsigned int renderHeight = RenderHeight();
    GetRenderResolution(InParameters, &renderWidth, &renderHeight);

    if (!UpdateSize())
        return false;

    const auto dbgMode = static_cast<DebugModes>(cfg.FfxDenoiserDebugMode.value_or_default());
    const bool isDebugVis = (uint32_t)dbgMode & (uint32_t) DebugModes::ConversionDebug;
    const bool isDebugComp = ((uint64_t)dbgMode & (uint64_t)DebugModes::CompositionDebug);
    const bool isFfxDebug = dbgMode == DebugModes::FfxDebug;
    const bool hasAnyDebug = (dbgMode != DebugModes::None);

    // Denoise is bypassed if we are debugging something OTHER than the final outputs
    const bool isDenoiseBypassed = !isFfxDebug && !isDebugComp && 
        hasAnyDebug && dbgMode != DebugModes::DenoiserOutput && dbgMode != DebugModes::UpscalerBypass;

    // Upscale is bypassed if we are in a debug mode that isn't the DenoiserBypass (final raw)
    const bool isUpscaleBypassed = hasAnyDebug && dbgMode != DebugModes::DenoiserBypass;

    // Validate helper features
    if (!RCAS->IsInit())
        cfg.RcasEnabled.set_volatile_value(false);
    if (!OutputScaler->IsInit())
        cfg.OutputScalingEnabled.set_volatile_value(false);

    _isInReset = false;

    if (uint32_t value = 0; inParams.Get(NVSDK_NGX_Parameter_Reset, &value) == NVSDK_NGX_Result_Success)
        _isInReset = value > 0;

    // Denoiser start
    ffxDispatchDescDenoiserIndirectDiffuse mode1Signal = {};
    ffxDispatchDescDenoiserIndirectSpecular mode2SpecularSignal = {};
    ffxDispatchDescDenoiserIndirectDiffuse mode2DiffuseSignal = {};
    ffxDispatchDescDenoiser denoiserDesc = {};
    bool isDenoiserReady = false;

    // Pull configuration and input buffers for DLSS-RR from the param table, convert and 
    // repack input buffers into intermediate FSR-RR input buffers, and configure dispatch descriptors.
    if (!PrepareDenoiserDispatch(InCommandList, *InParameters, denoiserDesc))
        return false;

    // Link signal descriptors (v1.2.0: separate desc per signal type)
    if (_isMode2)
        FSRDConvShader->GetSignal(mode2SpecularSignal, mode2DiffuseSignal, denoiserDesc);
    else
        FSRDConvShader->GetSignal(mode1Signal, denoiserDesc);

    // Dispatch denoiser
    if (!isDenoiseBypassed)
    {
        ffxDispatchDescDenoiserDebugView dispatchDebugView = {};

        if (isFfxDebug)
        {
            // v1.2.0 pNext chain: dispatch -> specular -> diffuse -> debugView (Mode2)
            //                   or dispatch -> diffuse -> debugView (Mode1)
            ffxDispatchDescHeader* tail = denoiserDesc.header.pNext;
            while (tail->pNext != nullptr)
                tail = tail->pNext;
            tail->pNext = &dispatchDebugView.header;

            ID3D12Resource* dstTex;
            TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Output, dstTex);

            dispatchDebugView = 
            { 
                .header = { .type = FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW }, 
                .output = ffxApiGetResourceDX12(dstTex, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS),
                .outputSize = { TargetWidth(), TargetHeight() },
                .mode = FFX_API_DENOISER_DEBUG_VIEW_MODE_OVERVIEW,
                .viewportIndex = 0
            };
        }

        isDenoiserReady = DispatchDenoiser(InCommandList, denoiserDesc);

        if (!isDenoiserReady)
            return false;

        // Compose denoised signals
        FSRDCompDesc compDesc = 
        { 
            .DstTexSize = _convDesc.RenderSize,
            .CorrelationBias = cfg.FfxDenoiserCorrelationBias.value_or_default(),
            .Flags = (uint32_t)GetCompDebugFlags(dbgMode) |
                     (_isMode2 ? (uint32_t)FSRDCompFlags::Mode2Signal : 0u)
        };

        TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_Color, compDesc.InRawColor);
        TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles, compDesc.InColorBeforeParticles);

        if (!isFfxDebug && !FSRDConvShader->DispatchComposition(InCommandList, compDesc))
            return false;

        // Optional post-composition EMA temporal stabilization layer with
        // motion-adaptive alpha. Suppresses residual per-frame flicker that
        // survives the FSR-RR denoiser on static areas; moving areas bypass
        // the blend (alpha=1) so no trailing on camera/object motion.
        if (cfg.FfxDenoiserTemporalStable.value_or_default())
        {
            const float alpha = cfg.FfxDenoiserTemporalAlpha.value_or_default();
            FSRDConvShader->SetTemporalStableEnabled(true);
            if (!FSRDConvShader->DispatchTemporalStable(InCommandList, alpha, false))
                return false;
        }
        else
        {
            FSRDConvShader->SetTemporalStableEnabled(false);
        }

        isDenoiserReady = true;
    }

    // Upscaler start
    if (!isUpscaleBypassed)
    {
        ffxDispatchDescUpscale upscalerDesc = {};

        if (!PrepareUpscalerInput(InCommandList, inParams, upscalerDesc))
            return false;

        // If PrepareUpscalerInput requested a backend change (e.g. missing ExposureTexture
        // forcing AutoExposure), skip dispatch this frame and let the feature recreate next frame.
        if (state.changeBackend[Handle()->Id])
        {
            LOG_DEBUG("FSRDFeatureDx12::Evaluate skipping upscaler dispatch because backend change is pending");
            return true;
        }

        // Override upscaler config
        if (isDenoiserReady)
        {
            upscalerDesc.color = ffxApiGetResourceDX12(FSRDConvShader->GetCompositionOutput());
            // v1.2.0: cameraFovAngleVertical and deltaTime removed from denoiserDesc
            // Get them from projection matrix and member variable
            upscalerDesc.cameraFovAngleVertical = GetVertFovFromProjectionMatrixRad(_projMatrix);
            upscalerDesc.frameTimeDelta = _deltaTime;
        }

        // Sets optional, configurable resource barriers
        FSR31FeatureDx12::SetConfigurableBarriers(InCommandList);

        bool isUpscalerReady = DispatchUpscaler(InCommandList, upscalerDesc);

        // Post-Process
        if (isUpscalerReady)
            PostProcess(InCommandList, inParams, upscalerDesc);

        // Cleanup
        FSR31FeatureDx12::ResetConfigurableBarriers(InCommandList);
    }
    else if (!isFfxDebug) // Debug visualization
    {
        ID3D12Resource* srcTex = nullptr;

        if (dbgMode == DebugModes::DlssColorBeforeParticles)
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeParticles, srcTex);
        else if (dbgMode == DebugModes::DlssColorBeforeTransparency)
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSSD_ColorBeforeTransparency, srcTex);
        else if (dbgMode == DebugModes::DlssTransparencyLayer)
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_TransparencyLayer, srcTex);
        else if (dbgMode == DebugModes::DlssBias)
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, srcTex);
        else if (dbgMode == DebugModes::RawColor || dbgMode == DebugModes::DenoiserBypass)
            TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_Color, srcTex);
        else if (isDebugVis)
        {
            if (_isMode2)
                srcTex = GetD3D12ResFromFFX(mode2SpecularSignal.signal.input);
            else
                srcTex = GetD3D12ResFromFFX(mode1Signal.signal.input);
        }
        else
            srcTex = FSRDConvShader->GetCompositionOutput();

        ID3D12Resource* dstTex;

        if (!srcTex || !TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Output, dstTex))
        {
            _frameCount++;
            return true;
        }

        FSRDConvShader->Blit(InCommandList, srcTex, dstTex);
    }

    _frameCount++;
    return isDenoiserReady || isDenoiseBypassed;
}

bool FSRDFeatureDx12::PrepareDenoiserDispatch(ID3D12GraphicsCommandList* InCommandList, const NVSDK_NGX_Parameter& inParams,
    ffxDispatchDescDenoiser& dispatchDesc)
{
    const auto& cfg = *Config::Instance(); 
    const auto& slData = State::Instance().slLastConstants;

    // Read jitter and motion vector scale early - needed for conversion shader before dispatch
    float MVScaleX = 1.0f, MVScaleY = 1.0f;
    inParams.Get(NVSDK_NGX_Parameter_MV_Scale_X, &MVScaleX);
    inParams.Get(NVSDK_NGX_Parameter_MV_Scale_Y, &MVScaleY);

    float jitterX = 0.0f, jitterY = 0.0f;
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_X, &jitterX);
    inParams.Get(NVSDK_NGX_Parameter_Jitter_Offset_Y, &jitterY);

    // Compute JitterCorrection for the conversion shader BEFORE ConvertDenoiserBuffers runs.
    // Previously this was done after the conversion, causing the shader to use stale
    // (previous frame) jitter correction and producing sub-pixel reprojection flickering.
    if (JitteredMV())
    {
        _convDesc.Flags |= (uint32_t)FSRDConvFlags::JitteredMV;

        const float safeMVScaleX = (MVScaleX != 0.0f) ? MVScaleX : 1.0f;
        const float safeMVScaleY = (MVScaleY != 0.0f) ? MVScaleY : 1.0f;

        _convDesc.JitterCorrection =
        {
            (jitterX - _lastJitter.x) / safeMVScaleX,
            (jitterY - _lastJitter.y) / safeMVScaleY
        };
    }
    else
    {
        _convDesc.Flags &= ~(uint32_t)FSRDConvFlags::JitteredMV;
    }
    _lastJitter = { jitterX, jitterY };

    // Gather DLSS-RR input buffers for conversion and repacking for FSR-RR
    if (!PrepareDenoiseConvInput(inParams))
        return false;   

    if (!ConvertDenoiserBuffers(InCommandList))
        return false;

    // Camera matrix - translation and rotation, from viewMatrix^-1
    const XMFLOAT3 camPos = GetFloat3Column(_invViewMatrix, 3);

    // Pack dispatch configuration
    dispatchDesc = 
    {
        .commandList = InCommandList,
        .motionVectorScale = { 1.0f, 1.0f, 1.0f },
        // Camera movement since last frame (PreviousPosition - CurrentPosition)
        .cameraPositionDelta = { (_lastCamPos.x - camPos.x), (_lastCamPos.y - camPos.y), (_lastCamPos.z - camPos.z) },
        .view = GetViewMatrixFFX(_viewMatrix, _isRightHanded),
        .projection = GetProjectionMatrixFFX(_unjitteredProjMatrix),
        .linearDepthBounds = { _convDesc.NearPlane, _convDesc.FarPlane },
        .renderSize = { RenderWidth(), RenderHeight() }, 
        .frameIndex = (uint32_t)_frameCount,
        .flags = FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO
    };
    
    if (_isInReset)
        dispatchDesc.flags |= FFX_DENOISER_DISPATCH_RESET;

    // Update camera position for next frame
    _lastCamPos = camPos;

    // deltaTime is no longer part of ffxDispatchDescDenoiser in v1.2.0
    // Store it in member variable for upscaler use
    _deltaTime = 0.0f;
    if (!TryGetToggleableNGXParam(inParams, OptiKeys::FSR_FrameTimeDelta, cfg.FsrUseFsrInputValues, _deltaTime))
    {
        if (inParams.Get(NVSDK_NGX_Parameter_FrameTimeDeltaInMsec, &_deltaTime) !=
                NVSDK_NGX_Result_Success || _deltaTime < 1.0f)
        {
            _deltaTime = (float)GetDeltaTime();
        }
    }

    // Motion Vector Scaling
    // Motion vectors are in pixel space; scale converts to UV space (pixel / renderSize = UV).
    dispatchDesc.motionVectorScale = { MVScaleX / dispatchDesc.renderSize.width,
                                       MVScaleY / dispatchDesc.renderSize.height,
                                       1.0f };

    // AMD FSR-RR SDK documentation states jitterOffsets are "expressed in screen pixels".
    dispatchDesc.jitterOffsets.x = jitterX;
    dispatchDesc.jitterOffsets.y = jitterY;

    LOG_DEBUG("Jitter px [{:.6f}, {:.6f}] MVScale=({:.4f},{:.4f}) JitterCorr=({:.6f},{:.6f}) JitteredMV={}",
              dispatchDesc.jitterOffsets.x, dispatchDesc.jitterOffsets.y,
              MVScaleX, MVScaleY,
              _convDesc.JitterCorrection.x, _convDesc.JitterCorrection.y,
              JitteredMV());

    return true;
}

bool FSRDFeatureDx12::PrepareDenoiseConvInput(const NVSDK_NGX_Parameter& inParams)
{
    const auto& slData = State::Instance().slLastConstants;

    // Gather DLSS-RR input buffers for conversion and repacking for FSR-RR
    bool isReady = true;

    // Standard TSR buffers
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Color, _convDesc.Resources.InColor))
        isReady = false;
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_MotionVectors, _convDesc.Resources.InMotionVectors))
        isReady = false;
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_Depth, _convDesc.Resources.InDepth) && LowResMV())
        isReady = false;

    // DLSSD-specific buffers
    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_GBuffer_Normals, _convDesc.Resources.InNormals))
        isReady = false;

    // If roughness is not packed into normals, then this texture is mandatory.
    // This value should be available in one of these two buffers in any DLSS-RR implementation.
    if (!s_isRoughnessPacked && !TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_GBuffer_Roughness, _convDesc.Resources.InRoughness))
    {
        LOG_WARN("Expected unpacked roughness buffer from DLSS-RR. Defaulting to packed roughness...");
        s_isRoughnessPacked = true;
    }

    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_DiffuseAlbedo, _convDesc.Resources.InDiffAlbedo))
        isReady = false;

    if (!TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_SpecularAlbedo, _convDesc.Resources.InSpecAlbedo))
        isReady = false;

    TryGetNGXVoidPointer(inParams, NVSDK_NGX_Parameter_DLSS_Input_Bias_Current_Color_Mask, _convDesc.Resources.InBiasMask);

    // Optional. Specular hit distance can be used with mode-2 denoising to track movement inside reflections, 
    // in addition to primary motion tracking for the surface and camera.
    TryGetLoggedResource(inParams, NVSDK_NGX_Parameter_DLSSD_SpecularHitDistance, _convDesc.Resources.InSpecHitDist);
    
    // Get DLSSD matrices and derive related values
    // World to view/camera space (V)
    _prevViewMatrix = _viewMatrix;
    _viewMatrix = {};

    if (!TryGetNGXMatrix(inParams, NVSDK_NGX_Parameter_DLSS_WORLD_TO_VIEW_MATRIX, _viewMatrix))
    {
        if (StreamlineHooks::isSetConstantsHooked())
        {
            SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraRight), 0, _invViewMatrix);
            SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraUp), 1, _invViewMatrix);
            SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraFwd), 2, _invViewMatrix);
            SetColumn(XMLoadFloat3((XMFLOAT3*) &slData.cameraPos), 3, _invViewMatrix);
            _invViewMatrix.r[3].m128_f32[3] = 1.0f;

            _viewMatrix = XMMatrixInverse(nullptr, _invViewMatrix);
        }
        else
        {
            LOG_ERROR("View matrix missing! Denoiser not ready.");
            isReady = false;
        }
    }
    else
    {
        // Camera rotation and position
        _invViewMatrix = XMMatrixInverse(nullptr, _viewMatrix);
    }

    // Perspective projection matrix (P)
    _projMatrix = {};

    if (!TryGetNGXMatrix(inParams, NVSDK_NGX_Parameter_DLSS_VIEW_TO_CLIP_MATRIX, _projMatrix))
    {
        if (StreamlineHooks::isSetConstantsHooked())
        {
            if (slData.cameraFOV != sl::INVALID_FLOAT && slData.cameraNear != slData.cameraFar)
            {
                // These measurements are supposed to be in radians, but some titles supply degrees.
                // Valid FOV in radians never exceeds PI. Realistic FOV in degrees is basically never in the single
                // digits.
                const float fov = (slData.cameraFOV < 4.0f) ? slData.cameraFOV : GetRadiansFromDeg(slData.cameraFOV);
                const float nearPlane = slData.cameraNear;
                const float farPlane = slData.cameraFar;
                _isRightHanded = slData.cameraViewToClip[2].w < 0.0f;

                // Actual SL view to clip matrix isn't reliable. This is harder to fuck up.
                if (_isRightHanded)
                    _projMatrix = XMMatrixPerspectiveFovRH(fov, slData.cameraAspectRatio, nearPlane, farPlane);
                else
                    _projMatrix = XMMatrixPerspectiveFovLH(fov, slData.cameraAspectRatio, nearPlane, farPlane);

                _projMatrix = XMMatrixTranspose(_projMatrix);
            }
        }
        else
        {
            LOG_ERROR("Projection matrix missing! Denoiser not ready.");
            isReady = false;
        }
    }
    else
    {
        // DLSSD provided a projection matrix directly. Infer handedness so that
        // cameraForward is flipped correctly for right-handed view spaces.
        _isRightHanded = GetViewPlanes(_projMatrix, DepthInverted()).isRightHanded;
    }

    // FSR-RR expects an unjittered, left-handed projection matrix.
    // DLSS may bake sub-pixel jitter into the view-to-clip matrix, so strip it out.
    _unjitteredProjMatrix = _projMatrix;
    _unjitteredProjMatrix.r[0].m128_f32[2] = 0.0f;
    _unjitteredProjMatrix.r[1].m128_f32[2] = 0.0f;

    if (_isRightHanded && DepthInverted())
    {
        // Cyberpunk supplies a right-handed, reverse-depth projection matrix.
        // FSR-RR expects a left-handed, standard-depth projection matrix.
        // Convert: P_RH_rev = [..; 0 0 A -1; 0 0 B 0]
        //          P_LH_std = [..; 0 0 A+1 1; 0 0 -B 0]
        _unjitteredProjMatrix.r[2].m128_f32[2] += 1.0f;
        _unjitteredProjMatrix.r[2].m128_f32[3] = -_unjitteredProjMatrix.r[2].m128_f32[3];
        _unjitteredProjMatrix.r[3].m128_f32[2] = -_unjitteredProjMatrix.r[3].m128_f32[2];
    }
    else if (_isRightHanded)
    {
        // Right-handed standard depth: only flip handedness to left-handed.
        // The view-space Z direction needs to be negated for FSR-RR's LH convention.
        _unjitteredProjMatrix.r[2].m128_f32[3] = -_unjitteredProjMatrix.r[2].m128_f32[3];
        _unjitteredProjMatrix.r[3].m128_f32[2] = -_unjitteredProjMatrix.r[3].m128_f32[2];
    }

    return isReady;
}

bool FSRDFeatureDx12::ConvertDenoiserBuffers(ID3D12GraphicsCommandList* InCommandList)
{
    const uint32_t dbgMode = (uint32_t)Config::Instance()->FfxDenoiserDebugMode.value_or_default(); 
    const auto& cfg = *Config::Instance(); 
    const auto& slData = State::Instance().slLastConstants;

    // Prepare input converter
    _convDesc.RenderSize = 
    { 
        (float) RenderWidth(), (float) RenderHeight(), 
        1.0f / (float) RenderWidth(), 1.0f / (float) RenderHeight()
    };
    // Preserve the jittered-MV flag set in PrepareDenoiserDispatch; add albedo/debug flags.
    _convDesc.Flags = (uint32_t) FSRDConvFlags::NonGammaAlbedo |
                      (dbgMode & (uint32_t) FSRDConvFlags::DebugModeMask) |
                      (_convDesc.Flags & (uint32_t) FSRDConvFlags::JitteredMV);
    _convDesc.FloorIsolation = cfg.FfxDenoiserFloorIsolation.value_or_default();

    if (s_isRoughnessPacked)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::IsRoughnessPacked;

    // Store in column major order for GPU
    XMStoreFloat4x4(&_convDesc.InvViewMatrix, XMMatrixTranspose(_invViewMatrix));

    // Inverse perspective projection. The projection matrix has already been flipped to
    // left-handed for FSR-RR, so its inverse will reconstruct view-space positions correctly.
    XMMATRIX invProjMatrix = XMMatrixInverse(nullptr, _unjitteredProjMatrix);

    // Previous world to view for linear depth delta. Match the left-handed convention.
    XMMATRIX prevViewMatrix = _prevViewMatrix;
    if (_isRightHanded)
        prevViewMatrix.r[2] = XMVectorNegate(prevViewMatrix.r[2]);

    XMStoreFloat4x4(&_convDesc.InvProjMatrix, XMMatrixTranspose(invProjMatrix));
    XMStoreFloat4x4(&_convDesc.PrevViewMatrix, XMMatrixTranspose(prevViewMatrix));

    // Near and far planes (jitter does not affect these).
    // The unjittered matrix is always left-handed standard-depth at this point.
    const ViewPlanes planes = GetViewPlanes(_unjitteredProjMatrix, false);
    _convDesc.NearPlane = std::min(planes.nearPlane, planes.farPlane);
    _convDesc.FarPlane = std::max(planes.nearPlane, planes.farPlane);

    if (!s_isHWDepth)
        _convDesc.Flags |= (uint32_t) FSRDConvFlags::IsDepthLinear;

#pragma region debug-point conv-input
    LOG_DEBUG("FSRD ConvInput: size=({0},{1}) FloorIso={2} Flags={3} Near={4} Far={5} HWDepth={6} RoughPacked={7}",
              _convDesc.RenderSize.x, _convDesc.RenderSize.y,
              _convDesc.FloorIsolation, _convDesc.Flags,
              _convDesc.NearPlane, _convDesc.FarPlane,
              s_isHWDepth, s_isRoughnessPacked);
#pragma endregion

    LOG_DEBUG("Distpaching FSRD Input Converter");

    // Dispatch resource converter. Outputs are automatically transitioned for reading.
    if (!FSRDConvShader->DispatchConversion(InCommandList, _convDesc))
        return false;

    return true;
}

static bool TryUpdateOption(const CustomOptional<float>& cfgValue, float& currentValue)
{
    if (cfgValue.value_or_default() != currentValue)
    {
        currentValue = cfgValue.value_or_default();
        return true;
    }
    else
        return false;
}

bool FSRDFeatureDx12::DispatchDenoiser(ID3D12GraphicsCommandList* InCommandList,
                                       const ffxDispatchDescDenoiser& dispatchDesc)
{
    auto& state = State::Instance();
    const auto& cfg = *Config::Instance();
    bool cfgChanged = false;

    if (TryUpdateOption(cfg.FfxDenoiserDisocThreshold, _denoiserSettings.m_DisocclusionThreshold))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD);
    if (TryUpdateOption(cfg.FfxDenoiserCrossBlNormStr, _denoiserSettings.m_CrossBilateralNormalStrength))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH);
    if (TryUpdateOption(cfg.FfxDenoiserStabilityBias, _denoiserSettings.m_StabilityBias))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS);
    if (TryUpdateOption(cfg.FfxDenoiserMaxRadiance, _denoiserSettings.m_MaxRadiance))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE);
    if (TryUpdateOption(cfg.FfxDenoiserRadianceClip, _denoiserSettings.m_RadianceClipStdK))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K);
    if (TryUpdateOption(cfg.FfxDenoiserGaussKernRelax, _denoiserSettings.m_GaussianKernelRelaxation))
        ApplyConfiguration(FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION);

#pragma region debug-point denoiser-settings
    LOG_DEBUG("FSRD DenoiserSettings: Disoc={0} CrossNorm={1} StabBias={2} MaxRad={3} RadClip={4} GaussRelax={5} CorrBias={6}",
              _denoiserSettings.m_DisocclusionThreshold,
              _denoiserSettings.m_CrossBilateralNormalStrength,
              _denoiserSettings.m_StabilityBias,
              _denoiserSettings.m_MaxRadiance,
              _denoiserSettings.m_RadianceClipStdK,
              _denoiserSettings.m_GaussianKernelRelaxation,
              cfg.FfxDenoiserCorrelationBias.value_or_default());
#pragma endregion

    LOG_DEBUG("Dispatching FSR-RR...");
#pragma region debug-point denoiser-dispatch-pre
    {
        const auto* pNext = dispatchDesc.header.pNext;
        int chainIdx = 0;
        while (pNext != nullptr)
        {
            // Layout: ffxDispatchDescDenoiserIndirectSpecular/Diffuse = { header; FfxApiDenoiserSignal signal; }
            // FfxApiDenoiserSignal = { FfxApiResource input; FfxApiResource output; uint32_t checkerboardOrigin; }
            const auto* signal = reinterpret_cast<const FfxApiDenoiserSignal*>(pNext + 1);
            LOG_DEBUG("[DispatchPre] pNext[{0}] type=0x{1:X} input.res=0x{2:X} input.state=0x{3:X} output.res=0x{4:X} output.state=0x{5:X}",
                      chainIdx, (uint64_t)pNext->type,
                      reinterpret_cast<uintptr_t>(signal->input.resource), signal->input.state,
                      reinterpret_cast<uintptr_t>(signal->output.resource), signal->output.state);
            pNext = pNext->pNext;
            ++chainIdx;
        }
        LOG_DEBUG("[DispatchPre] dispatchDesc.header.type=0x{0:X} renderSize=({1},{2}) frameIndex={3} flags=0x{4:X} ctx=0x{5:X}",
                  (uint64_t)dispatchDesc.header.type,
                  dispatchDesc.renderSize.width, dispatchDesc.renderSize.height,
                  dispatchDesc.frameIndex, dispatchDesc.flags,
                  reinterpret_cast<uintptr_t>(_pDenoiserCtx));
    }
#pragma endregion
    const ffxReturnCode_t result = FfxApiProxy::D3D12_Dispatch(&_pDenoiserCtx, &dispatchDesc.header);
    LOG_DEBUG("[DispatchPost] FSR-RR dispatch result={0} (0=OK)", (int)result);

    if (result != FFX_API_RETURN_OK)
    {
        LOG_ERROR("Dispatch error: {0}", FfxApiProxy::ReturnCodeToString(result));

        if (result == FFX_API_RETURN_ERROR_RUNTIME_ERROR)
        {
            LOG_WARN("Trying to recover by recreating the feature");
            state.changeBackend[Handle()->Id] = true;
        }

        return false;
    }

    return true;
}

void FSRDFeatureDx12::SetDefaultConfiguration()
{
    for (int i = 0; i < DenoiserConfiguration::kCount; i++)
        SetDefaultConfiguration(DenoiserConfiguration::GetIndexKey(i));
}

ffxReturnCode_t FSRDFeatureDx12::SetDefaultConfiguration(FfxApiConfigureDenoiserKey key)
{
    ffxQueryDescDenoiserGetDefaultKeyValue queryDesc = 
    {
        .header = { .type = FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE }, 
        .key = (uint64_t)key, 
        .count = 1u,
        .data = &_denoiserSettings.GetMember(key)
    };

    const ffxReturnCode_t code = FfxApiProxy::D3D12_Query(&_pDenoiserCtx, &queryDesc.header);
    return code;
}

ffxReturnCode_t FSRDFeatureDx12::ApplyConfiguration(FfxApiConfigureDenoiserKey key)
{
    ffxConfigureDescDenoiserKeyValue configureDesc =
    {
        .header = { .type = FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE },
        .key = (uint64_t)key,
        .count = 1u,
        .data = &_denoiserSettings.GetMember(key)
    };

    const ffxReturnCode_t code = FfxApiProxy::D3D12_Configure(&_pDenoiserCtx, &configureDesc.header);
    return code;
}

std::string FSRDFeatureDx12::SubFeatureVersionString() const
{
    const auto& cfg = *Config::Instance();
    const auto& state = State::Instance();
    const auto index = cfg.FfxDenoiserIndex.value_or_default();
    if (index < state.ffxDenoiserVersionNames.size())
        return state.ffxDenoiserVersionNames[index];
    return "";
}
