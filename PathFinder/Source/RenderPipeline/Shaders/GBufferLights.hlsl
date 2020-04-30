#ifndef _GBufferLights__
#define _GBufferLights__

#include "MandatoryEntryPointInclude.hlsl"
#include "GBuffer.hlsl"
#include "Vertices.hlsl"
#include "Geometry.hlsl"
#include "Matrix.hlsl"
#include "Light.hlsl"

struct RootConstants
{
    uint LightTableIndex;
};

ConstantBuffer<RootConstants> RootConstantBuffer : register(b0);
StructuredBuffer<Light> LightTable : register(t0);

//------------------------  Vertex  ------------------------------//

struct VertexOut
{
    float4 Position : SV_POSITION;
    float3 Luminance : LUMINANCE;
    float2 LocalSpacePosition : DIST_FROM_CENTER;
    float2 LightSize : LIGHT_SIZE;
};

void GetLightVertexWS(Light light, uint vertexId, out float3 worldSpaceCoord, out float2 localSpaceCoord)
{
    float halfWidth = light.Width * 0.5;
    float halfHeight = light.Height * 0.5;

    float3 position = light.Position.xyz;
    float3 orientation = light.Orientation.xyz;

    // Get billboard points at the origin
    float dx = (vertexId == 0 || vertexId == 1) ? halfWidth : -halfWidth;
    float dy = (vertexId == 0 || vertexId == 3) ? -halfHeight : halfHeight;

    localSpaceCoord = float2(dx, dy);
    float3 lightPoint = float3(dx, dy, 0.0f);

    float3x3 diskRotation = RotationMatrix3x3(orientation);

    // Rotate around origin
    lightPoint = mul(diskRotation, lightPoint);

    // Move points to light's location
    worldSpaceCoord = lightPoint + light.Position.xyz;
}

VertexOut VSMain(uint vertexId : SV_VertexID)
{
    uint lightVertexIndex = vertexId;
    lightVertexIndex = lightVertexIndex == 3 ? 2 : lightVertexIndex;
    lightVertexIndex = lightVertexIndex == 4 ? 3 : lightVertexIndex;
    lightVertexIndex = lightVertexIndex == 5 ? 0 : lightVertexIndex;

    Light light = LightTable[RootConstantBuffer.LightTableIndex];

    // Sphere light is a disk always oriented towards the camera
    if (light.LightType == LightTypeSphere)
    {
        light.Orientation = float4(normalize(FrameDataCB.Camera.Position.xyz - light.Position.xyz), 0);
    }

    float2 localSpacePosition;
    float3 worldSpacePosition;
    GetLightVertexWS(light, lightVertexIndex, worldSpacePosition, localSpacePosition);

    float4 lightCornerVertex = float4(worldSpacePosition, 1.0);

    VertexOut vout;
    vout.Position = mul(FrameDataCB.Camera.ViewProjection, lightCornerVertex);
    vout.Luminance = light.Luminance * light.Color.rgb;
    vout.LocalSpacePosition = light.LightType != LightTypeRectangle ? localSpacePosition : 0.xx;
    vout.LightSize = float2(light.Width, light.Height);

    return vout;
}

//------------------------  Pixel  ------------------------------//

GBufferPixelOut PSMain(VertexOut pin)
{
    bool pixelOutsideLightRadius = !IsPointInsideEllipse(pin.LocalSpacePosition, 0.xx, pin.LightSize);
    
    if (pixelOutsideLightRadius) 
    {
        discard;
    }

    GBufferEmissive gBuffer;
    gBuffer.Luminance = pin.Luminance;

    GBufferEncoded encodedGBuffer = EncodeGBuffer(gBuffer);

    GBufferPixelOut pixelOut;
    pixelOut.MaterialData = encodedGBuffer.MaterialData;
    return pixelOut;
}

#endif