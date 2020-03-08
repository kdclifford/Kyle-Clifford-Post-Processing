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
//Texture2D SceneTexture2 : register(t1);
SamplerState PointSample : register(s0); // We don't usually want to filter (bilinear, trilinear etc.) the scene texture when
                                          // post-processing so this sampler will use "point sampling" - no filtering

// This shader also uses a "burn" texture, which is basically a height map that the burn level ascends
SamplerState TrilinearWrap : register(s1);


//--------------------------------------------------------------------------------------
// Shader code
//--------------------------------------------------------------------------------------


float2 crt_coords(float2 uv, float bend)
{
    uv -= 0.5;
    uv *= 2.;
    uv.x *= 1. + pow(abs(uv.y) / bend, 2.);
    uv.y *= 1. + pow(abs(uv.x) / bend, 2.);
    
    uv /= 2.5;
    return uv + .5;
}

float vignette(float2 uv, float size, float smoothness, float edgeRounding)
{
    uv -= .5;
    uv *= size;
    float amount = sqrt(pow(abs(uv.x), edgeRounding) + pow(abs(uv.y), edgeRounding));
    amount = 1. - amount;
    return smoothstep(0., smoothness, amount);
}

float scanline(float2 uv, float lines, float speed, float timer)
{
    return sin(uv.y * lines + timer * speed);
}

float random(float2 uv)
{
    return frac(sin(dot(uv, float2(15.5151, 42.2561))) * 12341.14122 * sin(gScanLineTimer * 0.3));
}

float noise(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    
    float a = random(i);
    float b = random(i + float2(1.0f, 0.0f));
    float c = random(i + float2(0.0f, 1.0f));
    float d = random(i + float2(1.0f, 0.0f));
    
    float2 u = smoothstep(0.0f, 1.0f, f);
    
    return lerp(a, b, u.x) + (c - a) * u.y * (1. - u.x) + (d - b) * u.x * u.y;
                     
}

float hash(float n)
{
    return frac(sin(n) * 43.5453);
}

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{    
    float2 crt_uv = crt_coords(input.sceneUV, 10.0f);
    
    
    float s1 = scanline(input.sceneUV, 200.0f, -10.0f, gScanLineTimer);
    float s2 = scanline(input.sceneUV, 20.0f, -3.0f, gScanLineTimer);
        
    float4 col;
    col.r = SceneTexture.Sample(PointSample, crt_uv + float2(0.0f, 0.01f)).r;
    col.g = SceneTexture.Sample(PointSample, crt_uv).r;
    col.b = SceneTexture.Sample(PointSample, crt_uv + float2(0.0f, -0.01f)).b;
    col.a = SceneTexture.Sample(PointSample, crt_uv).a;
    
    float test2 = lerp(0.05, 0.00, input.sceneUV.y);
    
    col = lerp(col, float4(s1 + s2, s1 + s2, s1 + s2, s1 + s2), 0.05);
    float test = noise(input.sceneUV * 75.);
    return lerp(col, float4(test, test, test, test), 0.05) * vignette(input.sceneUV, 1.9f, 0.2f, 6.0f);       
}