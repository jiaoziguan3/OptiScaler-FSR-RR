// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2026 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

/// A structure encapsulating a 3-dimensional set of floating point coordinates.
struct FfxApiFloatCoords3D
{
    float x; ///< The x coordinate of a 3-dimensional point.
    float y; ///< The y coordinate of a 3-dimensional point.
    float z; ///< The z coordinate of a 3-dimensional point.
};

/// A 4-component floating point vector
struct FfxApiFloat4
{
    float x;
    float y;
    float z;
    float w;
};

/// A 4x4 matrix in row-major layout with row-vector convention.
struct FfxApiMatrix4x4
{
    FfxApiFloat4 rows[4];  ///< Four rows of the matrix. Access as rows[row].x, rows[row].y etc.
};

/// An inclusive floating-point interval [min, max].
struct FfxApiFloatBounds
{
    float min; ///< Lower bound of the interval.
    float max; ///< Upper bound of the interval.
};

//------------------------------------------------------------------------------
// External Includes
//------------------------------------------------------------------------------

// uint32_t
// uint64_t
#include <stdint.h>

//------------------------------------------------------------------------------
// FFX Denoiser Defines
//------------------------------------------------------------------------------

#define FFX_DENOISER_VERSION_MAJOR 1
#define FFX_DENOISER_VERSION_MINOR 2
#define FFX_DENOISER_VERSION_PATCH 0

#ifndef FFX_API_EFFECT_ID_DENOISER
#define FFX_API_EFFECT_ID_DENOISER 0x00050000u
#endif
#ifndef FFX_API_EFFECT_ID_RADIANCECACHE
#define FFX_API_EFFECT_ID_RADIANCECACHE 0x00060000u
#endif

#ifndef FFX_API_MAKE_EFFECT_SUB_ID
#define FFX_API_MAKE_EFFECT_SUB_ID(effectId, subversion)                                                               \
    ((effectId & FFX_API_EFFECT_MASK) | (subversion & ~FFX_API_EFFECT_MASK))
#endif

#ifndef FFX_API_MAKE_BACKEND_SUB_ID
#define FFX_API_MAKE_BACKEND_SUB_ID(backendId, subversion)                                                             \
    ((backendId & FFX_API_BACKEND_MASK) | (subversion & ~FFX_API_BACKEND_MASK))
#endif

// Combiner for BACKEND-specific EFFECT sub-Ids
#ifndef FFX_API_MAKE_BACKEND_EFFECT_SUB_ID
#define FFX_API_MAKE_BACKEND_EFFECT_SUB_ID(backendId, effectId, subversion)                                            \
    ((subversion & ~FFX_API_EFFECT_MASK) | (effectId & FFX_API_EFFECT_MASK) | (backendId & FFX_API_BACKEND_MASK) |     \
     (subversion & ~(FFX_API_BACKEND_MASK | FFX_API_EFFECT_MASK)))
#endif

#define FFX_DENOISER_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define FFX_DENOISER_VERSION                                                                                           \
    FFX_DENOISER_MAKE_VERSION(FFX_DENOISER_VERSION_MAJOR, FFX_DENOISER_VERSION_MINOR, FFX_DENOISER_VERSION_PATCH)

#define FFX_API_DISPATCH_DESC_TYPE_RADIANCECACHE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_RADIANCECACHE, 0x04)

//------------------------------------------------------------------------------
// FFX Denoiser Declarations
//------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Create Context
//------------------------------------------------------------------------------

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x01)
typedef struct ffxCreateContextDescDenoiser
{
    ffxCreateContextDescHeader header;
    uint32_t                   version;                     ///< The version of the API the application was built against. This must be set to <c>FFX_DENOISER_VERSION</c>.
    struct FfxApiDimensions2D  maxRenderSize;               ///< The maximum size that rendering will be performed at.
    uint32_t                   signalFlags;                 ///< Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags.
    uint32_t                   checkerboardSignalFlags;     ///< Signal flags corresponding to a combination of values from @c FfxApiDenoiserSignalFlags for signals that use checkerboard reconstruction. Must be a subset of @c signalFlags.
    uint32_t                   flags;                       ///< Zero or a combination of values from <c>FfxApiCreateContextDenoiserFlags</c>.
} ffxCreateContextDescDenoiser;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Configure
//------------------------------------------------------------------------------

#define FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x21)
typedef struct ffxConfigureDescDenoiserKeyValue
{
    ffxConfigureDescHeader header;                          ///< Header descriptor, use type FFX_API_CONFIGURE_DESC_TYPE_DENOISER_KEYVALUE.
    uint64_t               key;                             ///< Configuration key, member of the FfxApiConfigureDenoiserKey enumeration.
    uint64_t               count;                           ///< The number of elements to configure.
    const void*            data;                            ///< Pointer to an array containing the elements to configure.
} ffxConfigureDescDenoiserKeyValue;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Dispatch
//------------------------------------------------------------------------------

typedef struct FfxApiDenoiserSignal
{
    struct FfxApiResource input;                            ///< Input signal to be denoised.
    struct FfxApiResource output;                           ///< Resulting denoised signal.

    uint32_t checkerboardOrigin;                            ///< The X-coordinate of the first traced pixel at Y=0 in a checkerboard pattern. 0: pixel (0, 0) was traced. 1: pixel (1, 0) was traced. Must be zero-initialized if this signal is not set in checkerboardSignalFlags.
} FfxApiDenoiserSignal;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x41)
typedef struct ffxDispatchDescDenoiser
{
    ffxDispatchDescHeader      header;
    void*                      commandList;                 ///< Command list to record upscaling rendering commands into.
                               
    struct FfxApiResource      linearDepth;                 ///< R:   Signed linear depth values for the current frame (<c>CurrentLinearDepth</c>).
    struct FfxApiResource      motionVectors;               ///< RG:  2D motion vectors (<c>PreviousUV - CurrentUV</c>), B: Signed linear depth delta (<c>PreviousLinearDepth - CurrentLinearDepth</c>).
    struct FfxApiResource      normals;                     ///< RG:  Octahedrally encoded normals, B: Linear Roughness, A: Material Type - See docs for more info, 
    struct FfxApiResource      specularAlbedo;              ///< RGB: Specular albedo - sqrt encoding assumed unless <c>FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO</c> is provided. <c>sqrt(SpecularAlbedo)</c>
    struct FfxApiResource      diffuseAlbedo;               ///< RGB: Diffuse albedo (e.g., <c>BaseColor * (1 - Metalness)</c>) - sqrt encoding assumed unless <c>FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO</c> is provided. <c>sqrt(DiffuseAlbedo)</c>
                                                            
    struct FfxApiFloatCoords3D motionVectorScale;           ///< RG:  The scale factor for transforming the 2D motion vectors into UV space. For 2D motion vectors computed as <c>PreviousUV - CurrentUV</c>, use <c>{ .x = +1.0f, .y = +1.0f }</c>. For 2D motion vectors computed as <c>PreviousNDC - CurrentNDC</c>, use <c>{ .x = +0.5f, .y = -0.5f }</c>. B: The scale factor for transforming the signed linear depth delta. For linear depth deltas computed as <c>PreviousLinearDepth - CurrentLinearDepth</c>, use <c>{ .z = +1.0f }</c>.
    struct FfxApiFloatCoords2D jitterOffsets;               ///< The subpixel jitter offset applied to the camera projection. (Expressed in screen pixels)
                                                            
    struct FfxApiFloatCoords3D cameraPositionDelta;         ///< The position delta of the camera since last frame (PreviousPosition - CurrentPosition).
    FfxApiMatrix4x4            view;                        ///< The world-to-view transformation matrix for the current frame. Row-major storage, row vectors.
    FfxApiMatrix4x4            projection;                  ///< The (unjittered) view-to-projection transformation matrix for the current frame. Row-major storage, row vectors.
    FfxApiFloatBounds          linearDepthBounds;           ///< The absolute linear depth bounds considered for denoising. Passthrough is enabled for signal texels where the corresponding absolute linear depth values are outside [min, max].
                                                            
    struct FfxApiDimensions2D  renderSize;                  ///< The resolution that was used for rendering the input resources.
    uint32_t                   frameIndex;                  ///< The index of the current frame.
                                                            
    uint32_t                   flags;                       ///< Zero or a combination of values from <c>FfxApiDispatchDenoiserFlags</c>.
} ffxDispatchDescDenoiser;

#define FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS 12        ///< The maximum number of viewports that can be visualized.

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DEBUG_VIEW FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x42)
typedef struct ffxDispatchDescDenoiserDebugView
{
    ffxDispatchDescHeader                header;
    struct FfxApiResource                output;            ///< Target output resource for debug visualization.
    struct FfxApiDimensions2D            outputSize;        ///< The resolution of the output resource.
    uint32_t                             mode;              ///< An entry of <c>FfxApiDenoiserDebugViewMode</c> that selects the mode used for visualization.
    uint32_t                             viewportIndex;     ///< The index of the viewport to visualize when <c>FFX_API_DENOISER_DEBUG_VIEW_MODE_FULLSCREEN_VIEWPORT</c> mode is active. Clamped between 0 and <c>FFX_API_DENOISER_DEBUG_VIEW_MAX_VIEWPORTS - 1</c>.
} ffxDispatchDescDenoiserDebugView;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_AMBIENT_OCCLUSION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x43)
typedef struct ffxDispatchDescDenoiserAmbientOcclusion
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserAmbientOcclusion;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_DIFFUSE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x44)
typedef struct ffxDispatchDescDenoiserDirectDiffuse
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserDirectDiffuse;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DIRECT_SPECULAR FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x45)
typedef struct ffxDispatchDescDenoiserDirectSpecular
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserDirectSpecular;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_DOMINANT_LIGHT FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x46)
typedef struct ffxDispatchDescDenoiserDominantLight
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
    struct FfxApiFloatCoords3D  direction;
    struct FfxApiFloatCoords3D  emission;
    float                       angularRadius;
} ffxDispatchDescDenoiserDominantLight;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_DIFFUSE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x47)
typedef struct ffxDispatchDescDenoiserIndirectDiffuse
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserIndirectDiffuse;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_INDIRECT_SPECULAR FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x48)
typedef struct ffxDispatchDescDenoiserIndirectSpecular
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserIndirectSpecular;

#define FFX_API_DISPATCH_DESC_TYPE_DENOISER_SPECULAR_OCCLUSION FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x49)
typedef struct ffxDispatchDescDenoiserSpecularOcclusion
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal signal;
} ffxDispatchDescDenoiserSpecularOcclusion;

// Legacy v1.1.0 structures (kept for backwards compatibility where needed)
#define FFX_API_DISPATCH_DESC_INPUT_DOMINANT_LIGHT_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x07)
typedef struct ffxDispatchDescDenoiserInputDominantLight
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal dominantLightVisibility;
    struct FfxApiFloatCoords3D  dominantLightDirection;
    struct FfxApiFloatCoords3D  dominantLightEmission;
} ffxDispatchDescDenoiserInputDominantLight;

#define FFX_API_DISPATCH_DESC_INPUT_4_SIGNALS_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x03)
typedef struct ffxDispatchDescDenoiserInput4Signals
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal indirectSpecularRadiance;
    struct FfxApiDenoiserSignal indirectDiffuseRadiance;
    struct FfxApiDenoiserSignal directSpecularRadiance;
    struct FfxApiDenoiserSignal directDiffuseRadiance;
} ffxDispatchDescDenoiserInput4Signals;

#define FFX_API_DISPATCH_DESC_INPUT_2_SIGNALS_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x04)
typedef struct ffxDispatchDescDenoiserInput2Signals
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal specularRadiance;
    struct FfxApiDenoiserSignal diffuseRadiance;
} ffxDispatchDescDenoiserInput2Signals;

#define FFX_API_DISPATCH_DESC_INPUT_1_SIGNAL_TYPE_DENOISER FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x05)
typedef struct ffxDispatchDescDenoiserInput1Signal
{
    ffxDispatchDescHeader       header;
    struct FfxApiDenoiserSignal radiance;
    struct FfxApiResource       fusedAlbedo;
} ffxDispatchDescDenoiserInput1Signal;

//------------------------------------------------------------------------------
// FFX Denoiser Descriptions: Query
//------------------------------------------------------------------------------

#define FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x81)
typedef struct ffxQueryDescDenoiserGetDefaultKeyValue
{
    ffxQueryDescHeader     header;                          ///< Header descriptor, use type FFX_API_QUERY_DESC_TYPE_DENOISER_GET_DEFAULT_KEYVALUE.
    uint64_t               key;                             ///< Configuration key, member of the FfxApiConfigureDenoiserKey enumeration.
    uint64_t               count;                           ///< The number of elements to query.
    void*                  data;                            ///< Pointer to an array for storing the querried elements.
} ffxQueryDescDenoiserGetDefaultKeyValue;

#define FFX_API_QUERY_DESC_TYPE_DENOISER_GPU_MEMORY_USAGE FFX_API_MAKE_EFFECT_SUB_ID(FFX_API_EFFECT_ID_DENOISER, 0x82)
typedef struct ffxQueryDescDenoiserGetGPUMemoryUsage        ///< If a valid FfxContext is passed into ffx::Query with this structure, current context usage will be reported. If no context is passed, the memory usage will be estimated based on the provided parameters.
{
    ffxQueryDescHeader        header;
    void*                     device;                       ///< For DX12: pointer to ID3D12Device.
    struct FfxApiDimensions2D maxRenderSize;                ///< Maximum size that rendering will be performed at.
    uint32_t                  signalFlags;                  ///< Signal flags corresponding to a combination of values from <c>FfxApiDenoiserSignalFlags</c>.
    uint32_t                  checkerboardSignalFlags;      ///< Signal flags corresponding to a combination of values from <c>FfxApiDenoiserSignalFlags</c> for signals that use checkerboard reconstruction. Must be a subset of @c signalFlags.
    uint32_t                  flags;                        ///< Zero or a combination of values from <c>FfxApiCreateContextDenoiserFlags</c>.

    struct FfxApiEffectMemoryUsage* gpuMemoryUsage;         ///< A pointer to a <c>FfxApiEffectMemoryUsage</c> structure that will hold the GPU memory usage.
} ffxQueryDescDenoiserGetGPUMemoryUsage;

//------------------------------------------------------------------------------
// FFX Denoiser Enums
//------------------------------------------------------------------------------

typedef enum FfxApiCreateContextDenoiserFlags
{
    FFX_DENOISER_ENABLE_DEBUGGING      = (1 << 0),          ///< A bit indicating that debug features may be enabled, memory consumption may increase.
    FFX_DENOISER_ENABLE_VALIDATION     = (1 << 1),          ///< A bit indicating that exhaustive validation should be enabled. Performance may decrease.
} FfxApiCreateContextDenoiserFlags;

typedef enum FfxApiConfigureDenoiserKey
{
    FFX_API_CONFIGURE_DENOISER_KEY_CROSS_BILATERAL_NORMAL_STRENGTH = 1, ///< Override the strength of the cross bilateral normal term. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_STABILITY_BIAS                  = 2, ///< Override the bias of the temporal accumulation to be more stable but less responsive. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_MAX_RADIANCE                    = 3, ///< Override the maximum radiance value. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_RADIANCE_CLIP_STD_K             = 4, ///< Override the standard deviation K value used for radiance clipping. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_GAUSSIAN_KERNEL_RELAXATION      = 5, ///< Override the Gaussian kernel relaxation factor. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_DISOCCLUSION_THRESHOLD          = 6, ///< Override the disocclusion threshold used for depth comparisons during temporal reprojection. A single float scalar.
    FFX_API_CONFIGURE_DENOISER_KEY_DEBUG_VIEW_LINEAR_DEPTH_BOUNDS  = 7, ///< Override the absolute linear depth bounds used for normalizing the absolute linear depth debug view. A single FfxApiFloatBounds.
} FfxApiConfigureDenoiserKey;

typedef enum FfxApiDenoiserDebugViewMode
{
    FFX_API_DENOISER_DEBUG_VIEW_MODE_OVERVIEW            = 0,
    FFX_API_DENOISER_DEBUG_VIEW_MODE_FULLSCREEN_VIEWPORT = 1,
} FfxApiDenoiserDebugViewMode;

typedef enum FfxApiDenoiserSignalFlags
{
    FFX_DENOISER_SIGNAL_NONE                      = 0,
    FFX_DENOISER_SIGNAL_AMBIENT_OCCLUSION         = (1 << 0),
    FFX_DENOISER_SIGNAL_DIRECT_DIFFUSE            = (1 << 1),
    FFX_DENOISER_SIGNAL_DIRECT_SPECULAR           = (1 << 2),
    FFX_DENOISER_SIGNAL_DOMINANT_LIGHT_VISIBILITY = (1 << 3),
    FFX_DENOISER_SIGNAL_INDIRECT_DIFFUSE          = (1 << 4),
    FFX_DENOISER_SIGNAL_INDIRECT_SPECULAR         = (1 << 5),
    FFX_DENOISER_SIGNAL_SPECULAR_OCCLUSION        = (1 << 6),
} FfxApiDenoiserSignalFlags;

typedef enum FfxApiDenoiserMode
{
    FFX_DENOISER_MODE_4_SIGNALS = 0,                        ///< The denoiser expects 4 split radiance signals as input to perform denoising on. (Direct Specular, Direct Diffuse, Indirect Specular, Indirect Diffuse)
    FFX_DENOISER_MODE_2_SIGNALS = 1,                        ///< The denoiser expects 2 fused radiance signals as input to perform denoising on. (Specular, Diffuse)
    FFX_DENOISER_MODE_1_SIGNAL  = 2,                        ///< The denoiser expects 1 fused radiance signal as input to perform denoising on. (Composited)
} FfxApiDenoiserMode;

typedef enum FfxApiDispatchDenoiserFlags
{
    FFX_DENOISER_DISPATCH_RESET            = (1 << 0),      ///< A bit indicating that the we need to reset history accumulation.
    FFX_DENOISER_DISPATCH_NON_GAMMA_ALBEDO = (1 << 1),      ///< A bit indicating that the input albedo textures are not gamma encoded.
} FfxApiDispatchDenoiserFlags;

#ifdef __cplusplus
}
#endif // __cplusplus
