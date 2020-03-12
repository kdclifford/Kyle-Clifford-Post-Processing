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


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

static const int NUM_STEPS = 64;
static const float NUM_DELTA = 1.0 / 64.0f;

// Pixel shader entry point - each shader has a "main" function
float4 main(PostProcessingInput input) : SV_Target
{                                     
    int NUM_SAMPLES = 64;
    
    float2 dirToLight = (gLight2ScreenSpacePos - input.sceneUV);
    float lengthToLight = length(dirToLight);
    dirToLight = normalize(dirToLight);
    
    float deltaLen = min(1, lengthToLight * NUM_DELTA);
   
    float2 rayDelta = dirToLight * deltaLen;
   
    float stepDecay = 20 * deltaLen;
    
    
    float decay = 1;
    float rayIntensity = 0.0f;
    
    const float2 centreUV = gArea2DTopLeft + gArea2DSize * 0.5f;
    
    const float Exposure = 1.0; // Directly scale the effect (0 = no effect, 1 = full)
    const float MyDecay = -0.0;
    const float CircleSize = 0.5;
    float Weight = 1.0 / float(NUM_SAMPLES);
    float Decay = 1.0 - MyDecay / float(NUM_SAMPLES);
    
      	// Store initial sample.  
    float3 originalColor = SceneTexture.Sample(PointSample, input.sceneUV);
  
    float3 colour = float3(0, 0, 0);
  	// Set up illumination decay factor.  
    float illuminationDecay = 1.0;
    
    float3 sampl = float3(0, 0, 0);
    float pixelBrightness = float3(0, 0, 0);
    //https://devansh.space/screen-space-god-rays/
    
    float Density = 1;
    float2 rayOffset = float2(0.0, 0.0);
    
    float2 newuv = float2(0.0, 1.0);
  	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.  
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        float2 sampPos = input.sceneUV + rayOffset;
        
	    // Step sample location along ray.  
        float2 uv = lerp(input.sceneUV, centreUV, float(i) / float(NUM_SAMPLES - 1));
       // uv = -uv;
    	// Retrieve sample at new location.  
        float3 sampl = SceneTexture.Sample(PointSample, uv);
        float3 fCurIntensity = SceneTexture.SampleLevel(PointSample, sampPos, 0.0);
        pixelBrightness = (sampl.r + sampl.g + sampl.b) / 3;
        //if (pixelBrightness > 0.7f)
        //{
        //    break;
        //}
    	// Apply sample attenuation scale/decay factors.  
        
        rayIntensity += fCurIntensity * decay;

        // Advance to the next position
        rayOffset += rayDelta;

        // Update the decay
        decay = saturate(decay - stepDecay);
        
        
        sampl *= illuminationDecay * Weight;
    	// Accumulate combined color.  
        colour += sampl;
    	// Update exponential decay factor.  
        illuminationDecay *= Decay;
    }
  
    
    return float4(colour * Exposure, 1);
    
    //************************** 
    
}