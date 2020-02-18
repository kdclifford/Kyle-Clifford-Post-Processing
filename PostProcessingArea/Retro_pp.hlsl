//--------------------------------------------------------------------------------------
// Colour Tint Post-Processing Pixel Shader
//--------------------------------------------------------------------------------------
// Just samples a pixel from the scene texture and multiplies it by a fixed colour to tint the scene

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// The scene has been rendered to a texture, these variables allow access to that texture
Texture2D SceneTexture : register(t0);
//Texture2D SceneTexture2 : register(t1);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
    float X = 15.0f * (1 / gViewportWidth);
    float Y = 15.0f * (1 / gViewportHeight);
    
    float2 coord = float2(X * floor(input.sceneUV.x / X), Y * floor(input.sceneUV.y / Y));    

    
    float3 ppColour = SceneTexture.Sample(PointSample, coord);
    
    return float4(ppColour, 1.0f);
    

    
}