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
    float PixelX = (1 / gViewportWidth);
    float PixelY = (1 / gViewportHeight);
    
    float3 ppColour = SceneTexture.Sample(PointSample, input.sceneUV);
    
    float3 ppCol = float3(0.0f, 0.0f, 0.0f);
	
    float e11 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, -PixelY));
    float e12 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, -PixelY));
    float e13 = SceneTexture.Sample(PointSample, input.sceneUV + float2(PixelX, -PixelY));
                                               
    float e21 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, 0));
    float e22 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, 0));
    float e23 = SceneTexture.Sample(PointSample, input.sceneUV + float2(PixelX, 0));
                                               
    float e31 = SceneTexture.Sample(PointSample, input.sceneUV + float2(-PixelX, PixelY));
    float e32 = SceneTexture.Sample(PointSample, input.sceneUV + float2(0, PixelY));
    float e33 = SceneTexture.Sample(PointSample, input.sceneUV + float2(PixelX, PixelY));

    float t1 = e13 + e33 + (2 * e23) - e11 - (2 * e21) - e31;
    float t2 = e31 + (2 * e32) + e33 - e11 - (2 * e12) - e13;
	
    if (((t1 * t1) + (t2 * t2)) > 0.05)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        return float4(ppColour, 1.0f);
    }
    
    //float exposure = 0.0034f;
    //float decay = 1.0f;
    //float density = 1000.84f;
    //float weight = 5.65f;
    
    //float3 gl_FragColor;
    //float2 deltaTextCoord = float2(input.sceneUV - gLight1Position.xy);
    //float2 textCoo = input.areaUV;
    //deltaTextCoord *= 1.0 / float(100) * density;
    //float illuminationDecay = 1.0;
	
	
    //for (int i = 0; i < 100; i++)
    //{
    //    textCoo -= deltaTextCoord;
    //    float4 sample = SceneTexture.Sample(TexSampler, input.areaUV);
			
    //    sample *= illuminationDecay * weight;
			
    //    gl_FragColor += sample;
			
    //    illuminationDecay *= decay;
    //}
	
	
    //gl_FragColor *= exposure;
    
    //return float4(gl_FragColor, 1.0f);
    
    //float4 P = (gLight1Position, 1.0f);
    //float4 Q = P * gViewProjectionMatrix;

    //if (Q.w < 0)
    //{
    //    return false;
    //}
    //else
    //{
    //    Q.x /= Q.w;
    //    Q.y /= Q.w;
    //}

    //PixelPt - > x = (Q.x + 1) * (ViewportWidth / 2);
    //PixelPt - > y = (1 - Q.y) * (ViewportHeight / 2);
    
    
    
    //float4 gl_FragColor;
    //  //compute ray from pixel to light center
    //float2 ray = input.areaUV - gLightPixelPosition;
    ////output color
    //float3 color = float3(0, 0, 0);

    //int samples = 100;
    //const float blurWidth = -0.85;
    
    ////sample the texture NUM_SAMPLES times
    //for (int i = 0; i < samples; i++)
    //{
    //    //sample the texture on the pixel-to-center ray getting closer to the center every iteration
    //    float scale = 1.0 + blurWidth * (float(i) / float(samples - 1));
    //    //summing all the samples togheter
    //    color += (SceneTexture.Sample(PointSample, (ray * scale) + gLightPixelPosition.xy).xyz) / float(samples);
    //}
    ////return final color
    //gl_FragColor = float4(color, 1.0);
    
    
    //return gl_FragColor;
    
    
   
}
    
    
    
    
