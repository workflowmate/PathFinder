#ifndef _SeparableBlur__
#define _SeparableBlur__

#include "GroupsharedMemoryHelpers.hlsl"

static const int BlurMaximumRadius = 64;
static const int WeightCount = 64;
static const int BlurGroupSize = 256;
static const int BlurGroupSharedBufferSize = BlurGroupSize + 2 * BlurMaximumRadius;

struct PassCBData
{
    // Packing into 4-component vectors 
    // to satisfy constant buffer alignment rules
    float4 Weights[WeightCount / 4];
    float2 ImageSize;
    uint IsHorizontal;
    uint BlurRadius;
    uint InputTexIdx;
    uint OutputTexIdx;
};

#define PassDataType PassCBData

#include "MandatoryEntryPointInclude.hlsl"

groupshared float3 gCache[BlurGroupSharedBufferSize]; // Around 5KB

[numthreads(BlurGroupSize, 1, 1)]
void CSMain(int3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
    RWTexture2D<float4> source = RW_Float4_Textures2D[PassDataCB.InputTexIdx];
    RWTexture2D<float4> destination = RW_Float4_Textures2D[PassDataCB.OutputTexIdx];

    int2 texelIndex = PassDataCB.IsHorizontal ? dispatchThreadID.xy : dispatchThreadID.yx;

    GSLineLoadStoreCoords loadStoreCoords = GetGSLineLoadStoreCoords(
        texelIndex, groupThreadID.x, PassDataCB.ImageSize, BlurGroupSize, PassDataCB.BlurRadius, PassDataCB.IsHorizontal
    );

    int groupThreadIndex = groupThreadID.x;
    int radius = int(PassDataCB.BlurRadius);

    gCache[loadStoreCoords.StoreCoord0] = source[loadStoreCoords.LoadCoord0].rgb;

    if (loadStoreCoords.IsLoadStore1Required)
        gCache[loadStoreCoords.StoreCoord1] = source[loadStoreCoords.LoadCoord1].rgb;

    GroupMemoryBarrierWithGroupSync();

    float3 color = float3(0.0, 0.0, 0.0);

    for (int i = -radius; i <= radius; i++)
    {
        uint index = uint(abs(i));
        uint vectorIndex = index / 4;
        uint elementIndex = index % 4;
        float4 weightVector = PassDataCB.Weights[vectorIndex];
        float weight = weightVector[elementIndex];
        color += gCache[i + radius + groupThreadID.x] * weight;
    }

    destination[texelIndex] = float4(color, 1.0);
}

#endif