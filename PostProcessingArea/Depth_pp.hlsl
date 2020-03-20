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
Texture2D DepthMap : register(t1);
SamplerState PointSample : register(s1); // This sampler switches off filtering (e.g. bilinear, trilinear) when accessing depth buffer




//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Pixel shader entry point - each shader has a "main" function
float4 main(PostProcessingInput input) : SV_Target
{                                   
    float3 finalColour = SceneTexture.Sample(PointSample, input.sceneUV);
  float4 depth = DepthMap.Sample(PointSample, input.sceneUV);
    
	float4 backGround = (0.0f, 0.0f, 0.0f, 0.0f);

	//depth.r == backGround.r && depth.g == backGround.g && depth.b == backGround.b

	if (depth.a <= 0.0f && depth.r == backGround.r && depth.g == backGround.g && depth.b == backGround.b || depth.a < 0.1f)
	{
		//finalColour = float4(0, 0, 0,1);
		finalColour += depth;
		//discard;
	}
	else
	{
		finalColour = depth;

	}

    
        return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
  //  return 1;
}