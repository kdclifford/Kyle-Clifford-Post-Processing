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

    const int kernalSize = 128;
    float GKernel[kernalSize];
    
    int oddSize = gKernalSize;
    
    if(gKernalSize % 2 == 0)
    {
        oddSize--;

    }        
        
    int mean = oddSize / 2;
	// intialising standard deviation to 1.0 
    float sum = 0;
    float sigma = 100;
    float r, s = 2.0 * sigma * sigma;

	// sum is for normalization 


	// generating 5x5 kernel 
    for (int x = 0; x < oddSize; x++)
    {
        GKernel[x] = (float) exp(-0.5 * pow((x - mean) / sigma, 2.0)) / (sigma * sqrt(2 * 3.1415f));
		// Accumulate the kernel values
        sum += GKernel[x];
    }

	// normalising the Kernel 
    for (int i = 0; i < oddSize; ++i)
    {
        GKernel[i] /= sum;
    }



   // for (int i = 0; i < oddSize; ++i)
   //{
   //    GKernel[i] = gWeights[i];
   //}
   

    

    float xPixel = (1 / gViewportWidth);
    float yPixel = (1 / gViewportHeight);

    float3 ppColour = SceneTexture.Sample(PointSample, input.sceneUV) * GKernel[((oddSize - 1) / 2) + 1];
	float3 FragmentColor = float3(0.0f, 0.0f, 0.0f);

        int offset = 1;

    for (int i = ((oddSize - 1) / 2) + 1; i < oddSize; ++i)
    {
        
        FragmentColor += (SceneTexture.Sample(PointSample, input.sceneUV + float2(0.0f, yPixel * offset)) * GKernel[i] +
		 SceneTexture.Sample(PointSample, input.sceneUV - float2(0.0f, yPixel * offset)) * GKernel[i]); //;+
		 //SceneTexture.Sample(TrilinearWrap, input.sceneUV + float2(0.0f, yPixel * offset[i])) * weight[i] +
		 //SceneTexture.Sample(TrilinearWrap, input.sceneUV + float2(0.0f, -yPixel * offset[i])) * weight[i]);

        offset++;
		//FragmentColor /= 6;

	}


	float3 orginal = SceneTexture.Sample(PointSample, input.sceneUV);

	ppColour += FragmentColor;



	//return (1);
	return float4(ppColour, 1.0f);



	// return float4(outputColour, 0.4f);
}