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

int uGhosts = 5; // number of ghost samples
float uGhostDispersal = 0.6; // dispersion factor

//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{    
    float3 color = float3(0, 0, 0);
    const float Exposure = 1.0; // Directly scale the effect (0 = no effect, 1 = full)
    float3 orignal = SceneTexture.Sample(PointSample, input.sceneUV);
    
    float pixelAverage = (orignal.r + orignal.g + orignal.b) / 3;
    
    if (pixelAverage < 0.6f)
    {
       orignal = float3(0, 0, 0);
        //discard;
    }
        return float4(orignal , 1.0f);

    
    
   //     int NUM_SAMPLES = 164;
    
    
    
        
   //     const float MyDecay = -0.0;
   //     const float CircleSize = 0.5;
   //     float Weight = 1.0 / float(NUM_SAMPLES);
   //     float Decay = 1.0 - MyDecay / float(NUM_SAMPLES);
    
   //   	// Store initial sample.  
   //     float3 originalColor = SceneTexture.Sample(PointSample, input.sceneUV);
  
       
  	//// Set up illumination decay factor.  
   //     float illuminationDecay = 1.0;
    
   //     float3 sampl = float3(0, 0, 0);
   //     float pixelBrightness = float3(0, 0, 0);
   // //https://devansh.space/screen-space-god-rays/
    
   //     float2 newuv = -input.sceneUV + float2(1, 1);
  	//// Evaluate summation from Equation 3 NUM_SAMPLES iterations.  
   //     for (int i = 0; i < NUM_SAMPLES; i++)
   //     {
	  //  // Step sample location along ray.  
   //         float2 uv = lerp(input.sceneUV, float2(0.5f, 0.5f), float(i) / float(NUM_SAMPLES - 1));
   //    // uv = -uv;
   // 	// Retrieve sample at new location.  
   //         float3 sampl = SceneTexture.Sample(PointSample, uv);
   //         pixelBrightness = (sampl.r + sampl.g + sampl.b) / 3;
   //     //if (pixelBrightness > 0.7f)
   //     //{
   //     //    break;
   //     //}
   // 	// Apply sample attenuation scale/decay factors.  
   //         sampl *= illuminationDecay * Weight;
   // 	// Accumulate combined color.  
   //         color += sampl;
   // 	// Update exponential decay factor.  
   //         illuminationDecay *= Decay;
   //     }
    
    
    
   // //return (1);
   // return float4(orignal + (color * Exposure), 1.0f);
    
    

   // return float4(outputColour, 0.4f);
}