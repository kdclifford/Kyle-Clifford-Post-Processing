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
Texture2D LightSceneTexture : register(t1);
//Texture2D SceneTexture2 : register(t1);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

// This shader also uses a "burn" texture, which is basically a height map that the burn level ascends
SamplerState TrilinearWrap : register(s1);


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{   
    
    float3 orignal = LightSceneTexture.Sample(PointSample, input.sceneUV);
    
    float pixelAverage = (orignal.r + orignal.g + orignal.b) / 3;
    
    //if (pixelAverage < 1.0f)
    //{
    //    orignal = (orignal + SceneTexture.Sample(PointSample, input.sceneUV));

    //}
    
    
    orignal = (orignal + (SceneTexture.Sample(PointSample, input.sceneUV) * 2.5));
	//orignal = (orignal + SceneTexture.Sample(PointSample, input.sceneUV));
    
    
    //return (1);
    return float4(orignal, 1.0f);
    
    

   // return float4(outputColour, 0.4f);
}