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
float4 main(LightingPixelShaderInput input) : SV_Target
{
 
                                      
    //PostProcessingInput output;
                                      
    // Extract diffuse material colour for this pixel from a texture. Using alpha channel here so use float4
    float4 diffuseMapColour = SceneTexture.Sample(TexSampler, input.uv);

    float depthAdjust = diffuseMapColour.a;
    
    float pixelDepth = (input.projectedPosition.z * input.projectedPosition.w - depthAdjust) / (input.projectedPosition.w - depthAdjust);
    
    
    
    //output.depth = pixelDepth;
    
    
    float2 viewportUV = input.projectedPosition.xy;
    
    viewportUV.x /= gViewportWidth;
    viewportUV.y /= gViewportHeight;
    
    float viewportDepth = DepthMap.Sample(PointSample, viewportUV);
    
    viewportDepth *= input.projectedPosition.w;
    pixelDepth *= input.projectedPosition.w;
    
    float depthDiff = viewportDepth - pixelDepth;
    
    
    if (depthDiff < 0.0f)
    {
        discard;
    }
    
  //  float depthFade = saturate(depthDiff / 0.025f);
    
	// Combine texture alpha with particle alpha
    float4 Ncolour = diffuseMapColour;
   // input.alpha * depthFade;
    Ncolour.a = 1.0f; // If you increase the number of particles then you might want to reduce the 1.0f here to make them more transparent

    return Ncolour;
    
}