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
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------


static float HueToRGB(float v1, float v2, float vH)
{
    if (vH < 0)
        vH += 1;

    if (vH > 1)
        vH -= 1;

    if ((6 * vH) < 1)
        return (v1 + (v2 - v1) * 6 * vH);

    if ((2 * vH) < 1)
        return v2;

    if ((3 * vH) < 2)
        return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

    return v1;
};


static float Min(float a, float b)
{
    return a <= b ? a : b;
};

static float Max(float a, float b)
{
    return a >= b ? a : b;
};


// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	// Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)

    
    
    float3 RGB;
    
    RGB.r = lerp(gTintColour.r, gTintColour2.r, input.sceneUV.y);
    RGB.g = lerp(gTintColour.g, gTintColour2.g, input.sceneUV.y);
    RGB.b = lerp(gTintColour.b, gTintColour2.b, input.sceneUV.y);
    
    float3 colour = (SceneTexture.Sample(PointSample, input.sceneUV).rgb) * RGB;
    
    // Convert an RGB colour to a HSL colour

 // Fill in the correct code here for question 4, the functions Min and Max above will help
    
    // rgb to hsl*****************************
    float r = colour.r / 255.0f;
    float g = colour.g / 255.0f;
    float b = colour.b / 255.0f;

    float min = Min(Min(r, g), b);
    float max = Max(Max(r, g), b);
    float delta = max - min;

    int H;
    float S;
    float L = (max + min) / 2;

    if (delta == 0)
    {
        H = 0;
        S = 0.0f;
    }
    else
    {
        S = (L <= 0.5) ? (delta / (max + min)) : (delta / (2 - max - min));

        float hue;

        if (r == max)
        {
            hue = ((g - b) / 6) / delta;
        }
        else if (g == max)
        {
            hue = (1.0f / 3) + ((b - r) / 6) / delta;
        }
        else
        {
            hue = (2.0f / 3) + ((r - g) / 6) / delta;
        }

        if (hue < 0)
            hue += 1;
        if (hue > 1)
            hue -= 1;

        H = (int) (hue * 360);
    }


    
    //*******************************

//// Convert a HSL colour to an RGB colour

    


    float newR = 0;

    float newG = 0;

    float newB = 0;

    if (S == 0)
    {
        newR = newG = newB = (L * 255);
    }
    else
    {
        float hue;
        float v1, v2;
        if (H * gHueTimer > 0)
        {
            hue = (float) (H * gHueTimer) / 360;
        }
        else
        {
            hue = (float) H / 360;
        }
        
        v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
        v1 = 2 * L - v2;

        newR = 255 * HueToRGB(v1, v2, hue + (1.0f / 3));
        newG = 255 * HueToRGB(v1, v2, hue);
        newB = 255 * HueToRGB(v1, v2, hue - (1.0f / 3));
    }

    RGB = float3(newR, newB, newG);
    

    
    
    
    
    


	// Sample a pixel from the scene texture and multiply it with the tint colour (comes from a constant buffer defined in Common.hlsli)

    
  
    
   
    
    	// Calculate alpha to display the effect in a softened circle, could use a texture rather than calculations for the same task.
	// Uses the second set of area texture coordinates, which range from (0,0) to (1,1) over the area being processed
    float softEdge = 0.10f; // Softness of the edge of the circle - range 0.001 (hard edge) to 0.25 (very soft)
    float2 centreVector = input.areaUV - float2(0.5f, 0.5f);
    float centreLengthSq = dot(centreVector, centreVector);
    float alpha = 1.0f - saturate((centreLengthSq - 0.25f + softEdge) / softEdge);
	// Got the RGB from the scene texture, set alpha to 1 for final output
    return float4(RGB, 1.0f);
}