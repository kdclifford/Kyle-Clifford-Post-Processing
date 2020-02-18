//--------------------------------------------------------------------------------------
// Soft Particle Pixel Shader
//--------------------------------------------------------------------------------------
// Saples a diffuse texture map and adjusts depth buffer value to give fake depth to the particle

#include "Common.hlsli" // Shaders can also use include files - note the extension


//--------------------------------------------------------------------------------------
// Textures (texture maps)
//--------------------------------------------------------------------------------------

// Here we allow the shader access to a texture that has been loaded from the C++ side and stored in GPU memory.
// Note that textures are often called maps (because texture mapping describes wrapping a texture round a mesh).
// Get used to people using the word "texture" and "map" interchangably.
Texture2D SceneTexture : register(t0); // A diffuse map is the main texture for a model.
                                        // The t0 indicates this texture is in slot 0 and the C++ code must load the texture into the this slot
SamplerState TexSampler : register(s0); // A sampler is a filter for a texture like bilinear, trilinear or anisotropic

// Access depth buffer as a texture map - see C++ side to see how this was set up
SamplerState PointSample : register(s1); // This sampler switches off filtering (e.g. bilinear, trilinear) when accessing depth buffer


float4 main(PostProcessingInput input) : SV_Target
{
    float PixelX = (1 / gViewportWidth) * 3;
    float PixelY = (1 / gViewportHeight) * 3;
    
    
    float3 ppCol = float3(0.0f, 0.0f, 0.0f);
	
    float e11 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, -PixelY));
    float e12 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, -PixelY));
    float e13 = SceneTexture.Sample(PointSample, input.sceneUV + float2(+PixelX, -PixelY));
                                               
    float e21 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, 0));
    float e22 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, 0));
    float e23 = SceneTexture.Sample(PointSample, input.sceneUV + float2(+PixelX, 0));
                                               
    float e31 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, +PixelY));
    float e32 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, +PixelY));
    float e33 = SceneTexture.Sample(PointSample, input.sceneUV + float2(+PixelX, +PixelY));

    float t1 = e13 + e33 + (2 * e23) - e11 - (2 * e21) - e31;
    float t2 = e31 + (2 * e32) + e33 - e11 - (2 * e12) - e13;
	
    if (((t1 * t1) + (t2 * t2)) > 0.05)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}