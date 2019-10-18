struct PlaygroundPassData
{
    
};

#define PassDataType PlaygroundPassData

#include "BaseRootSignature.hlsl"

struct VertexIn
{
    float3 Position : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;  
};

VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
    vout.Color = float4(0.6, 0.8, 1.0, 1.0);

    float4 position = float4(vin.Position, 1.0);
    vout.PosH = mul(FrameDataCB.CameraViewProjection, position);

    return vout;
}

float4 PSMain(VertexOut pin) : SV_Target
{
    return pin.Color;
}