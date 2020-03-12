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
// Slight adjustment to calculated depth of pixels so they don't shadow themselves
    const float DepthAdjust = 0.0005f;

 

	// Direction from pixel to light
  //  float3 light1Direction = normalize(gLight1Position - input.worldPosition);

	//// Check if pixel is within light cone
 //   if (dot(gLight1Facing, -light1Direction) > gLight1CosHalfAngle) //**** TODO: This condition needs to be written as the first exercise to get spotlights working
 //          //           As well as the variables above, you also will need values from the constant buffers in "common.hlsli"
 //   {
	//    // Using the world position of the current pixel and the matrices of the light (as a camera), find the 2D position of the
	//    // pixel *as seen from the light*. Will use this to find which part of the shadow map to look at.
	//    // These are the same as the view / projection matrix multiplies in a vertex shader (can improve performance by putting these lines in vertex shader)
 //       float4 light1ViewPosition = mul(gLight1ViewMatrix, float4(input.worldPosition, 1.0f));
 //       float4 light1Projection = mul(gLight1ProjectionMatrix, light1ViewPosition);

	//	// Convert 2D pixel position as viewed from light into texture coordinates for shadow map - an advanced topic related to the projection step
	//	// Detail: 2D position x & y get perspective divide, then converted from range -1->1 to UV range 0->1. Also flip V axis
 //       float2 shadowMapUV = 0.5f * light1Projection.xy / light1Projection.w + float2(0.5f, 0.5f);
 //       shadowMapUV.y = 1.0f - shadowMapUV.y; // Check if pixel is within light cone

	//	// Get depth of this pixel if it were visible from the light (another advanced projection step)
 //       float depthFromLight = light1Projection.z / light1Projection.w; // - DepthAdjust; //*** Adjustment so polygons don't shadow themselves
		
	//	// Compare pixel depth from light with depth held in shadow map of the light. If shadow map depth is less than something is nearer
	//	// to the light than this pixel - so the pixel gets no effect from this light
 //       if (depthFromLight < ShadowMapLight1.Sample(PointClamp, shadowMapUV).r)
 //       {
 //           float3 light1Dist = length(gLight1Position - input.worldPosition);
 //           diffuseLight1 = gLight1Colour * max(dot(input.worldNormal, light1Direction), 0) / light1Dist; // Equations from lighting lecture
 //           float3 halfway = normalize(light1Direction + cameraDirection);
 //           specularLight1 = diffuseLight1 * pow(max(dot(input.worldNormal, halfway), 0), gSpecularPower); // Multiplying by diffuseLight instead of light colour - my own personal preference
 //       }
 //   }
    float depthFromLight = -input.projectedPosition.z / input.projectedPosition.w /*- DepthAdjust*/;

    float3 finalColour = SceneTexture.Sample(PointSample, input.sceneUV);
    
    if (depthFromLight < DepthMap.Sample(PointSample, input.sceneUV).r)
    {
        finalColour = 0;
    }
    

        return float4(finalColour, 1.0f); // Always use 1.0f for output alpha - no alpha blending in this lab
  //  return 1;
}