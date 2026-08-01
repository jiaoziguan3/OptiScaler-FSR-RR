#include "FSRDPreprocessCommon.hlsli"

#define MainRS \
    "RootFlags(0), " \
    "CBV(b0), " \
    "DescriptorTable(SRV(t0, numDescriptors = 3), visibility = SHADER_VISIBILITY_ALL), " \
    "DescriptorTable(UAV(u0, numDescriptors = 1), visibility = SHADER_VISIBILITY_ALL)"

#define THREAD_GROUP_SIZE_X     8
#define THREAD_GROUP_SIZE_Y     8
#define NUM_THREADS             (THREAD_GROUP_SIZE_X * THREAD_GROUP_SIZE_Y)

static const uint2 s_ThreadGroupSize = uint2(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y);

Texture2D<half4> InCurrent : register(t0); // Current composition color (RGBA16F)
Texture2D<half4> InHistory : register(t1); // Previous frame's stabilized output
Texture2D<half4> InMotion  : register(t2); // RG: pixel-space motion (Prev - Cur, in pixels)

RWTexture2D<half4> OutBlended : register(u0); // Stabilized color

cbuffer CB_Temporal : register(b0)
{
    float4 DstTexSize;     // XY = tex size, ZW = 1 / tex size

    float Alpha;           // User blend factor for static pixels [0..1]. Lower = more smoothing.
    float MotionRampEnd;   // Motion magnitude (UV) at which alpha reaches 1.0
    uint  Flags;           // bit 0: reset (force copy current, bypass blend)
    float _Padding;
}

bool IsSet(uint mask) { return (Flags & mask) == mask; }

[RootSignature(MainRS)]
[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(uint3 groupID : SV_GroupID, uint3 gtID : SV_GroupThreadID)
{
    const int2 px = groupID.xy * s_ThreadGroupSize + gtID.xy;

    if (px.x >= DstTexSize.x || px.y >= DstTexSize.y)
    {
        OutBlended[px] = half4(0, 0, 0, 0);
        return;
    }

    const half4 current = InCurrent[px];

    // On reset (first frame or camera cut), bypass temporal blend entirely.
    [branch]
    if (IsSet(0x1u))
    {
        OutBlended[px] = current;
        return;
    }

    // Sample history at the SAME pixel position (no reprojection).
    // Reprojection was tried before and caused trailing due to MV inaccuracies.
    // Without reprojection, static areas blend correctly (history matches current
    // surface). Moving areas would trail - but we kill that via motion-adaptive
    // alpha below, which forces alpha=1 (current frame only) when motion is present.
    const half4 history = InHistory[px];

    // ---- Motion-adaptive alpha ----
    // InMotion.rg stores pixel-space motion vectors (PreviousUV - CurrentUV in
    // pixel units). Convert to UV space and measure magnitude.
    const half4 motionData = InMotion[px];
    const float2 motionUV   = float2(motionData.r, motionData.g) * DstTexSize.zw;
    const float motionMag   = length(motionUV);

    // Ramp alpha from user value (static) to 1.0 (moving). Beyond MotionRampEnd,
    // pixel is unambiguously moving - use current frame only, no trailing.
    const float motionFactor   = saturate(motionMag / max(MotionRampEnd, 1e-6));
    const float effectiveAlpha = lerp(Alpha, 1.0, motionFactor);

    const half4 blended = half4(lerp(history.rgb, current.rgb, (half)effectiveAlpha), current.a);
    OutBlended[px] = GetSafeFP16(blended);
}
