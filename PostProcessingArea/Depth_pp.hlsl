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
 
                                      
 //   //PostProcessingInput output;
                                      
 //   // Extract diffuse material colour for this pixel from a texture. Using alpha channel here so use float4
 //   float4 diffuseMapColour = SceneTexture.Sample(TexSampler, input.sceneUV);

 //   float depthAdjust = diffuseMapColour.a;
    
 //   float pixelDepth = (input.projectedPosition.z * input.projectedPosition.w - depthAdjust) / (input.projectedPosition.w - depthAdjust);
    
    
    
 //   //output.depth = pixelDepth;
    
    
 //   float2 viewportUV = input.projectedPosition.xy;
    
 //   viewportUV.x /= gViewportWidth;
 //   viewportUV.y /= gViewportHeight;
    
 //   float4 viewportDepth = DepthMap.Sample(PointSample, input.sceneUV);
    
 //   //viewportDepth *= input.projectedPosition.w;
 //   pixelDepth *= input.projectedPosition.w;
    
 //   float depthDiff = viewportDepth - pixelDepth;
    
    
 ////   if (depthDiff > 0.99f)
 ////   {
 ////       discard;
 ////   }
    
 //// //  float depthFade = saturate(depthDiff / 0.025f);
    
	////// Combine texture alpha with particle alpha
 //   float4 Ncolour = viewportDepth;
 ////  // input.alpha * depthFade;
 //   Ncolour.a = 1.0f; // If you increase the number of particles then you might want to reduce the 1.0f here to make them more transparent

 //   return viewportDepth;
    
    //int nsamples = 100;
    //float BlurStart = 0.5f;
    //float BlurWidth = 0.5f;
    //float GlowGamma = 0.5;
    //float Intensity = 0.5f;
    //float CX = gViewportWidth / 2;
    //float CY = gViewportHeight / 2;
    
    //float4 blurred = 0;
    //float2 ctrPt = float2(CX, CY);
    //// this loop will be unrolled by compiler and the constants precalculated:
    //for (int i = 0; i < nsamples; i++)
    //{
    //    float scale = BlurStart + BlurWidth * (i / (float) (nsamples - 1));
    //    blurred += SceneTexture.Sample(PointSample, input.uv * scale + ctrPt);
    //}
    //blurred /= nsamples;
    //blurred.rgb = pow(blurred.rgb, GlowGamma);
    //blurred.rgb *= Intensity;
    //blurred.rgb = saturate(blurred.rgb);
    //float4 origTex = SceneTexture.Sample(PointSample, input.uv + ctrPt);
    //float3 newC = origTex.rgb + (1.0 - origTex.a) * blurred.rgb;
    //float newA = max(origTex.a, blurred.a);
    //return float4(newC.rgb, newA);
    
    //float Threshhold = 30.7f;
    //float size = 1.0 / 100;
    //float2 Pbase = input.sceneUV - fmod(input.sceneUV, size.xx);
    //float2 PCenter = Pbase + (size / 2.0).xx;
    //float2 st = (input.sceneUV - Pbase) / size;
    //float4 c1 = (float4) 0;
    //float4 c2 = (float4) 0;
    //float4 invOff = float4((1 - float3(0.0f, 0.5f, 0.3f)), 1);
    //if (st.x > st.y)
    //{
    //    c1 = invOff;
    //}
    //float threshholdB = 1.0 - Threshhold;
    //if (st.x > threshholdB)
    //{
    //    c2 = c1;
    //}
    //if (st.y > threshholdB)
    //{
    //    c2 = c1;
    //}
    //float4 cBottom = c2;
    //c1 = (float4) 0;
    //c2 = (float4) 0;
    //if (st.x > st.y)
    //{
    //    c1 = invOff;
    //}
    //if (st.x < Threshhold)
    //{
    //    c2 = c1;
    //}
    //if (st.y < Threshhold)
    //{
    //    c2 = c1;
    //}
    //float4 cTop = c2;
    //float4 tileColor = SceneTexture.Sample(PointSample, PCenter);
    //float4 result = tileColor + cTop - cBottom;
    //return result;
    int NUM_SAMPLES = 164;
    
    const float Exposure = 1.0; // Directly scale the effect (0 = no effect, 1 = full)
    const float MyDecay = -0.0;
    const float CircleSize = 0.5;
    float Weight = 1.0 / float(NUM_SAMPLES);
    float Decay = 1.0 - MyDecay / float(NUM_SAMPLES);
    
      	// Store initial sample.  
    float3 originalColor =  SceneTexture.Sample(PointSample, input.sceneUV );
  
    float3 color = float3(0, 0, 0);
  	// Set up illumination decay factor.  
    float illuminationDecay = 1.0;
    
    float3 sampl = float3(0, 0, 0);
    float pixelBrightness = float3(0, 0, 0);
    https://devansh.space/screen-space-god-rays/
  	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.  
        for (int i = 0; i < NUM_SAMPLES; i++)
        {
	    // Step sample location along ray.  
            float2 uv = lerp(input.sceneUV, float2(0.5f, 0.5f), float(i) / float(NUM_SAMPLES - 1));
   	   
    	// Retrieve sample at new location.  
            float3 sampl = SceneTexture.Sample(PointSample, uv);
            pixelBrightness = (sampl.r + sampl.g + sampl.b) / 3;
        //if (pixelBrightness > 0.7f)
        //{
        //    break;
        //}
    	// Apply sample attenuation scale/decay factors.  
            sampl *= illuminationDecay * Weight;
    	// Accumulate combined color.  
            color += sampl;
    	// Update exponential decay factor.  
            illuminationDecay *= Decay;
        }
    
    //if (pixelBrightness > 0.7f)
    //{
  	// Output final color with a further scale control factor.  
        return float4(color * Exposure, 1);
    //}
    //else
    //{
    //    color = SceneTexture.Sample(PointSample, input.sceneUV);
    //    return float4(color, 1);
    //}
    
    
}