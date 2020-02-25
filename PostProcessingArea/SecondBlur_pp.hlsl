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

// Post-processing shader that tints the scene texture to a given colour
float4 main(PostProcessingInput input) : SV_Target
{
	float blur = gBlurLevel;
	if (blur > 100.0f)
	{
		blur = 100.0f;
	}
	else if (blur < 0.0f)
	{
		blur = 0.0f;
	}



	const float weight[] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};
	const float offset[] = { 0.0, 1.0 * blur, 2.0 * blur, 3.0 * blur, 4.0 * blur };



	float xPixel = (1 / gViewportWidth);
	float yPixel = (1 / gViewportHeight);

	float3 ppColour = SceneTexture.Sample(PointSample, input.sceneUV) * weight[0];
	float3 FragmentColor = float3(0.0f, 0.0f, 0.0f);


	for (int i = 1; i < 5; ++i)
	{

		FragmentColor += (SceneTexture.Sample(PointSample, input.sceneUV + float2(0.0f, yPixel * offset[i])) * weight[i] +
		 SceneTexture.Sample(PointSample, input.sceneUV - float2(0.0f, yPixel * offset[i])) * weight[i]); //;+
		 //SceneTexture.Sample(TrilinearWrap, input.sceneUV + float2(0.0f, yPixel * offset[i])) * weight[i] +
		 //SceneTexture.Sample(TrilinearWrap, input.sceneUV + float2(0.0f, -yPixel * offset[i])) * weight[i]);

		//FragmentColor /= 6;

	}


	float3 orginal = SceneTexture.Sample(PointSample, input.sceneUV);

	ppColour += FragmentColor;



	//return (1);
	return float4(ppColour, 0.5f);



	// return float4(outputColour, 0.4f);
}