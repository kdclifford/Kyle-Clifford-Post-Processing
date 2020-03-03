//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#include "Scene.h"
#include "Mesh.h"
#include "Model.h"
#include "Camera.h"
#include "State.h"
#include "Shader.h"
#include "Input.h"
#include "Common.h"

#include "CVector2.h" 
#include "CVector3.h" 
#include "CMatrix4x4.h"
#include "MathHelpers.h"     // Helper functions for maths
#include "GraphicsHelpers.h" // Helper functions to unclutter the code here
#include "ColourRGBA.h" 

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include <array>
#include <sstream>
#include <memory>
#include <algorithm>


static ImVec4 color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
static ImVec4 color2 = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
//--------------------------------------------------------------------------------------
// Scene Data
//--------------------------------------------------------------------------------------

//********************
// Available post-processes
enum class PostProcess
{
	None,
	Copy,
	Tint,
	GreyNoise,
	Burn,
	Distort,
	Spiral,
	HeatHaze,
	Blur,
	Bloom,
	Depth,
	Retro,
	CellShading,
	Invert,
	UnderWater,
	SecondBlur,
	Combine,



	AmountOfPosts,
};

enum class PostProcessMode
{
	Fullscreen,
	Area,
	Polygon,
};

const int KernelMaxSize = 64;
float GKernel[KernelMaxSize];

auto gCurrentPostProcess = PostProcess::None;
auto gTvPostProcess = PostProcess(rand() % int(PostProcess::AmountOfPosts) + int(PostProcess::None));
std::vector<PostProcess> currentList;
std::vector<PostProcess> TVList;
//std::vector<PostProcess> polyList{ PostProcess::Tint, PostProcess::Spiral, PostProcess::Retro, PostProcess::Tint, PostProcess::UnderWater, };
auto gCurrentPostProcessMode = PostProcessMode::Fullscreen;

//********************
void ImagePostProcessing(PostProcess postProcess, int Target, int ResourseOutput, int pass);
void FullScreenPostProcess(PostProcess postProcess, int pass);

// Constants controlling speed of movement/rotation (measured in units per second because we're using frame time)
const float ROTATION_SPEED = 1.5f;  // Radians per second for rotation
const float MOVEMENT_SPEED = 50.0f; // Units per second for movement (what a unit of length is depends on 3D model - i.e. an artist decision usually)

// Lock FPS to monitor refresh rate, which will typically set it to 60fps. Press 'p' to toggle to full fps
bool lockFPS = true;

std::vector<Model*> allModels;
float t = 100;
int oldMouseWheelPos = 0;


//const int kernalSize = 64;

// Meshes, models and cameras, same meaning as TL-Engine. Meshes prepared in InitGeometry function, Models & camera in InitScene
Mesh* gStarsMesh;
Mesh* gGroundMesh;
Mesh* gCubeMesh;
Mesh* gCrateMesh;
Mesh* gLightMesh;
Mesh* gPortalMesh;
Mesh* gWallMesh;
Mesh* gWall2Mesh;

Model* gStars;
Model* gGround;
Model* gCube;
Model* gTv;
Model* gCrate;
Model* gPortal;
Model* gWall;
Model* gWall2;

Camera* gCamera;
Camera* gPortalCamera;

float zShift = 3.0f;

// Store lights in an array in this exercise
const int NUM_LIGHTS = 2;
struct Light
{
	Model* model;
	CVector3 colour;
	float    strength;
};
Light gLights[NUM_LIGHTS];

class Poly
{
public:
	std::array<CVector3, 4> points; // C++ strangely needs an extra pair of {} here... only for std:array...
	CMatrix4x4 polyMatrix;
	std::vector<PostProcess> process;
	float distance;
	//CVector3 position;
};


std::vector<Poly*> postProcessTing;

// Additional light information
CVector3 gAmbientColour = { 0.3f, 0.3f, 0.4f }; // Background level of light (slightly bluish to match the far background, which is dark blue)
float    gSpecularPower = 256; // Specular power controls shininess - same for all models in this app

ColourRGBA gBackgroundColor = { 0.3f, 0.3f, 0.4f, 1.0f };

// Variables controlling light1's orbiting of the cube
const float gLightOrbitRadius = 20.0f;
const float gPortalOrbitRadius = 160.0f;
const float gLightOrbitSpeed = 0.7f;
const float gPortalOrbitSpeed = 0.02f;

float gSpotlightConeAngle = 90.0f; // Spot light cone angle (degrees), like the FOV (field-of-view) of the spot light

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
// Variables sent over to the GPU each frame
// The structures are now in Common.h
// IMPORTANT: Any new data you add in C++ code (CPU-side) is not automatically available to the GPU
//            Anything the shaders need (per-frame or per-model) needs to be sent via a constant buffer

PerFrameConstants gPerFrameConstants;      // The constants (settings) that need to be sent to the GPU each frame (see common.h for structure)
ID3D11Buffer* gPerFrameConstantBuffer; // The GPU buffer that will recieve the constants above

PerModelConstants gPerModelConstants;      // As above, but constants (settings) that change per-model (e.g. world matrix)
ID3D11Buffer* gPerModelConstantBuffer; // --"--

//**************************
PostProcessingConstants gPostProcessingConstants;       // As above, but constants (settings) for each post-process
ID3D11Buffer* gPostProcessingConstantBuffer; // --"--
//**************************


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// DirectX objects controlling textures used in this lab
ID3D11Resource* gStarsDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gStarsDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gGroundDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gGroundDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gWoodDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gWoodDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gCrateDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCrateDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gCubeDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gCubeDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gTvDiffuseSpecularMap = nullptr;
ID3D11ShaderResourceView* gTvDiffuseSpecularMapSRV = nullptr;
ID3D11Resource* gBrick = nullptr;
ID3D11ShaderResourceView* gBrickSRV = nullptr;


ID3D11Resource* gLightDiffuseMap = nullptr;
ID3D11ShaderResourceView* gLightDiffuseMapSRV = nullptr;



//****************************
// Post processing textures

const int amountOfTextures = 6;

// This texture will have the scene renderered on it. Then the texture is then used for post-processing
ID3D11Texture2D* gSceneTexture[amountOfTextures]; // This object represents the memory used by the texture on the GPU
ID3D11RenderTargetView* gSceneRenderTarget[amountOfTextures]; // This object is used when we want to render to the texture above
ID3D11ShaderResourceView* gSceneTextureSRV[amountOfTextures]; // This object is used to give shaders access to the texture above (SRV = shader resource view)

ID3D11Texture2D* gPortalDepthStencil = nullptr; // This object represents the memory used by the texture on the GPU
ID3D11DepthStencilView* gPortalDepthStencilView = nullptr; // This object is used when we want to use the texture above as the depth buffer


// Additional textures used for specific post-processes
ID3D11Resource* gNoiseMap = nullptr;
ID3D11ShaderResourceView* gNoiseMapSRV = nullptr;
ID3D11Resource* gBurnMap = nullptr;
ID3D11ShaderResourceView* gBurnMapSRV = nullptr;
ID3D11Resource* gDistortMap = nullptr;
ID3D11ShaderResourceView* gDistortMapSRV = nullptr;
ID3D11Resource* gMovie = nullptr;
ID3D11ShaderResourceView* gMovieSRV = nullptr;

//****************************
Model* MoveNearestEntity = 0;
Model* ModelSelected = 0;
CVector2 MousePixel;


//portal stuff






//--------------------------------------------------------------------------------------
// Light Helper Functions
//--------------------------------------------------------------------------------------

// Get "camera-like" view matrix for a spotlight
CMatrix4x4 CalculateLightViewMatrix(int lightIndex)
{
	return InverseAffine(gLights[lightIndex].model->WorldMatrix());
}

// Get "camera-like" projection matrix for a spotlight
CMatrix4x4 CalculateLightProjectionMatrix(int lightIndex)
{
	return MakeProjectionMatrix(1.0f, ToRadians(gSpotlightConeAngle)); // Helper function in Utility\GraphicsHelpers.cpp
}


void CalculateWeights()
{
	int oddSize = gPostProcessingConstants.kernalSize;

	if (oddSize % 2 == 0)
	{
		oddSize--;
	}

	int halfSize = ((oddSize - 1) / 2) + 1;

	int mean = oddSize / 2;
	// intialising standard deviation to 1.0 
	float sum = 0;
	float sigma = 100;
	float r, s = 2.0 * sigma * sigma;

	// sum is for normalization 


	// generating 5x5 kernel 
	for (int x = 0; x < oddSize; x++)
	{
		GKernel[x] = (float)exp(-0.5 * pow((x - mean) / sigma, 2.0)) / (sigma * sqrt(2 * 3.1415f));
		// Accumulate the kernel values
		sum += GKernel[x];
	}

	// normalising the Kernel 
	for (int i = 0; i < oddSize; ++i)
	{
		GKernel[i] /= sum;
	}


	// send Kernel to postprocessing constants 
	for (int i = 0; i < oddSize; ++i)
	{
		gPostProcessingConstants.weights[i].x = GKernel[i];
	}
}


//--------------------------------------------------------------------------------------
// Initialise scene geometry, constant buffers and states
//--------------------------------------------------------------------------------------

// Prepare the geometry required for the scene
// Returns true on success
bool InitGeometry()
{
	for (int i = 0; i < amountOfTextures; i++)
	{
		gSceneTexture[i] = nullptr;
		gSceneRenderTarget[i] = nullptr;
		gSceneTextureSRV[i] = nullptr;
	}
	////--------------- Load meshes ---------------////

	// Load mesh geometry data, just like TL-Engine this doesn't create anything in the scene. Create a Model for that.
	try
	{
		gStarsMesh = new Mesh("Stars.x");
		gGroundMesh = new Mesh("Hills.x");
		gCubeMesh = new Mesh("Cube.x");
		gCrateMesh = new Mesh("CargoContainer.x");
		gLightMesh = new Mesh("Light.x");
		gPortalMesh = new Mesh("Portal.x");
		gWallMesh = new Mesh("Wall1.x");
		gWall2Mesh = new Mesh("Wall2.x");
	}
	catch (std::runtime_error e)  // Constructors cannot return error messages so use exceptions to catch mesh errors (fairly standard approach this)
	{
		gLastError = e.what(); // This picks up the error message put in the exception (see Mesh.cpp)
		return false;
	}


	////--------------- Load / prepare textures & GPU states ---------------////

	// Load textures and create DirectX objects for them
	// The LoadTexture function requires you to pass a ID3D11Resource* (e.g. &gCubeDiffuseMap), which manages the GPU memory for the
	// texture and also a ID3D11ShaderResourceView* (e.g. &gCubeDiffuseMapSRV), which allows us to use the texture in shaders
	// The function will fill in these pointers with usable data. The variables used here are globals found near the top of the file.
	if (!LoadTexture("Stars.jpg", &gStarsDiffuseSpecularMap, &gStarsDiffuseSpecularMapSRV) ||
		!LoadTexture("GrassDiffuseSpecular.dds", &gGroundDiffuseSpecularMap, &gGroundDiffuseSpecularMapSRV) ||
		!LoadTexture("WoodDiffuseSpecular.dds", &gWoodDiffuseSpecularMap, &gWoodDiffuseSpecularMapSRV) ||
		!LoadTexture("StoneDiffuseSpecular.dds", &gCubeDiffuseSpecularMap, &gCubeDiffuseSpecularMapSRV) ||
		!LoadTexture("Tv.dds", &gTvDiffuseSpecularMap, &gTvDiffuseSpecularMapSRV) ||
		!LoadTexture("CargoA.dds", &gCrateDiffuseSpecularMap, &gCrateDiffuseSpecularMapSRV) ||
		!LoadTexture("Flare.jpg", &gLightDiffuseMap, &gLightDiffuseMapSRV) ||
		!LoadTexture("Noise.png", &gNoiseMap, &gNoiseMapSRV) ||
		!LoadTexture("Burn.png", &gBurnMap, &gBurnMapSRV) ||
		!LoadTexture("HalftoneDots16x16.dds", &gMovie, &gMovieSRV) ||
		!LoadTexture("brick1.jpg", &gBrick, &gBrickSRV) ||
		!LoadTexture("Distort.png", &gDistortMap, &gDistortMapSRV))
	{
		gLastError = "Error loading textures";
		return false;
	}


	// Create all filtering modes, blending modes etc. used by the app (see State.cpp/.h)
	if (!CreateStates())
	{
		gLastError = "Error creating states";
		return false;
	}


	////--------------- Prepare shaders and constant buffers to communicate with them ---------------////

	// Load the shaders required for the geometry we will use (see Shader.cpp / .h)
	if (!LoadShaders())
	{
		gLastError = "Error loading shaders";
		return false;
	}

	// Create GPU-side constant buffers to receive the gPerFrameConstants and gPerModelConstants structures above
	// These allow us to pass data from CPU to shaders such as lighting information or matrices
	// See the comments above where these variable are declared and also the UpdateScene function
	gPerFrameConstantBuffer = CreateConstantBuffer(sizeof(gPerFrameConstants));
	gPerModelConstantBuffer = CreateConstantBuffer(sizeof(gPerModelConstants));
	gPostProcessingConstantBuffer = CreateConstantBuffer(sizeof(gPostProcessingConstants));
	if (gPerFrameConstantBuffer == nullptr || gPerModelConstantBuffer == nullptr || gPostProcessingConstantBuffer == nullptr)
	{
		gLastError = "Error creating constant buffers";
		return false;
	}



	//********************************************
	//**** Create Scene Texture

	// We will render the scene to this texture instead of the back-buffer (screen), then we post-process the texture onto the screen
	// This is exactly the same code we used in the graphics module when we were rendering the scene onto a cube using a texture

	// Using a helper function to load textures from files above. Here we create the scene texture manually
	// as we are creating a special kind of texture (one that we can render to). Many settings to prepare:
	D3D11_TEXTURE2D_DESC sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;  // Full-screen post-processing - use full screen size for texture
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1; // No mip-maps when rendering to textures (or we would have to render every level)
	sceneTextureDesc.ArraySize = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA texture (8-bits each)
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // IMPORTANT: Indicate we will use texture as render target, and pass it to shaders
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;

	// We also need to send this texture (resource) to the shaders. To do that we must create a shader-resource "view"
	D3D11_SHADER_RESOURCE_VIEW_DESC srDesc = {};
	srDesc.Format = sceneTextureDesc.Format;
	srDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = 1;


	for (int i = 0; i < amountOfTextures; i++)
	{

		if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gSceneTexture[i])))
		{
			gLastError = "Error creating scene texture";
			return false;
		}

		// We created the scene texture above, now we get a "view" of it as a render target, i.e. get a special pointer to the texture that
		// we use when rendering to it (see RenderScene function below)
		if (FAILED(gD3DDevice->CreateRenderTargetView(gSceneTexture[i], NULL, &gSceneRenderTarget[i])))
		{
			gLastError = "Error creating scene render target view";
			return false;
		}


		if (FAILED(gD3DDevice->CreateShaderResourceView(gSceneTexture[i], &srDesc, &gSceneTextureSRV[i])))
		{
			gLastError = "Error creating scene shader resource view";
			return false;
		}
	}

	//**** Create Portal Depth Buffer ****//

// We also need a depth buffer to go with our portal
//**** This depth buffer can be shared with any other portals of the same size
	sceneTextureDesc = {};
	sceneTextureDesc.Width = gViewportWidth;
	sceneTextureDesc.Height = gViewportHeight;
	sceneTextureDesc.MipLevels = 1;
	sceneTextureDesc.ArraySize = 1;
	sceneTextureDesc.Format = DXGI_FORMAT_D32_FLOAT; // Depth buffers contain a single float per pixel
	sceneTextureDesc.SampleDesc.Count = 1;
	sceneTextureDesc.SampleDesc.Quality = 0;
	sceneTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	sceneTextureDesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	sceneTextureDesc.CPUAccessFlags = 0;
	sceneTextureDesc.MiscFlags = 0;
	if (FAILED(gD3DDevice->CreateTexture2D(&sceneTextureDesc, NULL, &gPortalDepthStencil)))
	{
		gLastError = "Error creating portal depth stencil texture";
		return false;
	}

	// Create the depth stencil view, i.e. indicate that the texture just created is to be used as a depth buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC portalDescDSV = {};
	portalDescDSV.Format = sceneTextureDesc.Format;
	portalDescDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	portalDescDSV.Texture2D.MipSlice = 0;
	portalDescDSV.Flags = 0;
	if (FAILED(gD3DDevice->CreateDepthStencilView(gPortalDepthStencil, &portalDescDSV, &gPortalDepthStencilView)))
	{
		gLastError = "Error creating portal depth stencil view";
		return false;
	}


	//*****************************//

	return true;
}

void createPolys()
{
	// An array of four points in world space - a tapered square centred at the origin

	Poly* firstPoly = new Poly();
	Poly* secondPoly = new Poly();
	Poly* thirdPoly = new Poly();
	Poly* forthPoly = new Poly();
	Poly* fifthPoly = new Poly();

	firstPoly->points = { { {-15,15,0}, {-15,-15,0}, {15,15,0}, {15,-15,0} } };

	// A rotating matrix placing the model above in the scene
	firstPoly->polyMatrix = MatrixTranslation({ 20, 15, 0 });
	//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;
	//CMatrix4x4 polyMatrix;
	//polyMatrix.GetPosition

	firstPoly->polyMatrix.SetPosition(CVector3(gWall->Position().x, gWall->Position().y + 25, gWall->Position().z));
	firstPoly->process.push_back(PostProcess::Burn);

	postProcessTing.push_back(firstPoly);

	// Pass an array of 4 points and a matrix. Only supports 4 points.

	//**********************2

	//// An array of four points in world space - a tapered square centred at the origin
	secondPoly->points = { { {-15,15,0}, {-15,-15,0}, {15,15,0}, {15,-15,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

	// A rotating matrix placing the model above in the scene
	secondPoly->polyMatrix = MatrixTranslation({ 20, 15, 0 });
	//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;
	//CMatrix4x4 polyMatrix;
	//polyMatrix.GetPosition

	secondPoly->polyMatrix.SetPosition(CVector3(gWall2->Position().x + 15, gWall2->Position().y + 25, gWall2->Position().z));
	secondPoly->process.push_back(PostProcess::UnderWater);

	postProcessTing.push_back(secondPoly);
	//***********************3
	thirdPoly->points = { { {-15,15,0}, {-15,-15,0}, {15,15,0}, {15,-15,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

	// A rotating matrix placing the model above in the scene
	thirdPoly->polyMatrix = MatrixTranslation({ 20, 15, 0 });
	//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;
	//CMatrix4x4 polyMatrix;
	//polyMatrix.GetPosition

	thirdPoly->polyMatrix.SetPosition(CVector3(gWall2->Position().x - 15, gWall2->Position().y + 25, gWall2->Position().z));
	thirdPoly->process.push_back(PostProcess::Retro);
	postProcessTing.push_back(thirdPoly);
	//***********************4

	forthPoly->points = { { {-15,15,0}, {-15,-15,0}, {15,15,0}, {15,-15,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

	// A rotating matrix placing the model above in the scene
	forthPoly->polyMatrix = MatrixTranslation({ 20, 15, 0 });
	//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;
	//CMatrix4x4 polyMatrix;
	//polyMatrix.GetPosition

	forthPoly->polyMatrix.SetPosition(CVector3(gWall2->Position().x - 40, gWall2->Position().y + 25, gWall2->Position().z));

	forthPoly->process.push_back(PostProcess::Spiral);
	postProcessTing.push_back(forthPoly);
	//***********************5

	fifthPoly->points = { { {-15,15,0}, {-15,-15,0}, {15,15,0}, {15,-15,0} } }; // C++ strangely needs an extra pair of {} here... only for std:array...

	// A rotating matrix placing the model above in the scene
	fifthPoly->polyMatrix = MatrixTranslation({ 20, 15, 0 });
	//polyMatrix = MatrixRotationY(ToRadians(1)) * polyMatrix;
	//CMatrix4x4 polyMatrix;
	//polyMatrix.GetPosition

	fifthPoly->polyMatrix.SetPosition(CVector3(gWall2->Position().x + 45, gWall2->Position().y + 25, gWall2->Position().z));
	fifthPoly->process.push_back(PostProcess::CellShading);
	postProcessTing.push_back(fifthPoly);
}


// Prepare the scene
// Returns true on success
bool InitScene()
{
	////--------------- Set up scene ---------------////

	gStars = new Model(gStarsMesh);
	gGround = new Model(gGroundMesh);
	gCube = new Model(gCubeMesh);
	gTv = new Model(gCubeMesh);
	gCrate = new Model(gCrateMesh);
	gPortal = new Model(gPortalMesh);
	gWall = new Model(gWallMesh);
	gWall2 = new Model(gWall2Mesh);

	ModelSelected = gCube;

	// Initial positions
	gCube->SetPosition({ 42, 5, -10 });
	gCube->SetRotation({ 0.0f, ToRadians(-110.0f), 0.0f });
	gCube->SetScale(1.5f);
	gTv->SetPosition({ 60, 8, -10 });
	gTv->SetRotation({ 0.0f, ToRadians(180), 0.0f });
	//gTv->SetScale(1.5f);
	gTv->SetScale(CVector3(2.0f, 1.5f, 1.5f));

	CMatrix4x4 portalrotation = gTv->WorldMatrix() * gPortal->WorldMatrix();

	gPortal->SetPosition({ gTv->Position() });
	gPortal->SetRotation(CVector3(gTv->Rotation().x, gTv->Rotation().y, gTv->Rotation().z));
	//gPortal->SetPosition({ gPortal->Position().x, gPortal->Position().y,  gPortal->Position().z -7.6f });*/

	//gPortal->SetWorldMatrix(gTv->WorldMatrix());
	gPortal->SetPosition(gTv->Position() + (gTv->WorldMatrix().GetZAxis() * 5.1));


	gWall->SetPosition({ 0,20,170 });
	gWall->SetScale(100.0f);

	gWall2->SetPosition({ 0,50,0 });
	gWall2->SetScale(100.0f);

	gCrate->SetPosition({ -10, 0, 90 });
	gCrate->SetRotation({ 0.0f, ToRadians(40.0f), 0.0f });
	gCrate->SetScale(6.0f);
	gStars->SetScale(8000.0f);

	gGround->SetPosition({ 0, -20, 0 });

	// Light set-up - using an array this time
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gLights[i].model = new Model(gLightMesh);
	}

	gLights[0].colour = { 0.8f, 0.8f, 1.0f };
	gLights[0].strength = 10;
	gLights[0].model->SetPosition({ 30, 10, 0 });
	gLights[0].model->SetScale(pow(gLights[0].strength, 1.0f)); // Convert light strength into a nice value for the scale of the light - equation is ad-hoc.

	gLights[1].colour = { 1.0f, 0.8f, 0.2f };
	gLights[1].strength = 40;
	gLights[1].model->SetPosition({ -70, 30, 100 });
	gLights[1].model->SetScale(pow(gLights[1].strength, 1.0f));


	////--------------- Set up camera ---------------////

	//allModels.push_back(gPortal);
	allModels.push_back(gCube);
	allModels.push_back(gTv);
	allModels.push_back(gCrate);
	allModels.push_back(gWall);
	allModels.push_back(gWall2);
	allModels.push_back(gLights[0].model);
	allModels.push_back(gLights[1].model);

	gCamera = new Camera();
	gCamera->SetPosition({ 25, 18, -45 });
	gCamera->SetRotation({ ToRadians(10.0f), ToRadians(7.0f), 0.0f });

	//**** Portal camera is the view shown in the portal object's texture ****//
	gPortalCamera = new Camera();
	gPortalCamera->SetPosition({ 45, 70, 250 });
	gPortalCamera->SetRotation({ ToRadians(20.0f), ToRadians(270.0f), 0 });


	createPolys();

	gPostProcessingConstants.kernalSize = 3;

	return true;
}


// Release the geometry and scene resources created above
void ReleaseResources()
{
	ReleaseStates();

	for (int i = 0; i < amountOfTextures; i++)
	{
		if (gSceneTextureSRV[i])              gSceneTextureSRV[i]->Release();
		if (gSceneRenderTarget[i])            gSceneRenderTarget[i]->Release();
		if (gSceneTexture[i])                 gSceneTexture[i]->Release();
	}

	if (gPortalDepthStencilView)  gPortalDepthStencilView->Release();
	if (gPortalDepthStencil)      gPortalDepthStencil->Release();


	if (gDistortMapSRV)                gDistortMapSRV->Release();
	if (gDistortMap)                   gDistortMap->Release();
	if (gBurnMapSRV)                   gBurnMapSRV->Release();
	if (gBurnMap)                      gBurnMap->Release();
	if (gNoiseMapSRV)                  gNoiseMapSRV->Release();
	if (gNoiseMap)                     gNoiseMap->Release();
	if (gMovieSRV)                     gMovieSRV->Release();
	if (gMovie)                        gMovie->Release();


	if (gBrickSRV)                     gBrickSRV->Release();
	if (gBrick)                        gBrick->Release();
	if (gLightDiffuseMapSRV)           gLightDiffuseMapSRV->Release();
	if (gLightDiffuseMap)              gLightDiffuseMap->Release();
	if (gCrateDiffuseSpecularMapSRV)   gCrateDiffuseSpecularMapSRV->Release();
	if (gCrateDiffuseSpecularMap)      gCrateDiffuseSpecularMap->Release();
	if (gCubeDiffuseSpecularMapSRV)    gCubeDiffuseSpecularMapSRV->Release();
	if (gCubeDiffuseSpecularMap)       gCubeDiffuseSpecularMap->Release();
	if (gTvDiffuseSpecularMapSRV)      gTvDiffuseSpecularMapSRV->Release();
	if (gTvDiffuseSpecularMap)         gTvDiffuseSpecularMap->Release();
	if (gGroundDiffuseSpecularMapSRV)  gGroundDiffuseSpecularMapSRV->Release();
	if (gGroundDiffuseSpecularMap)     gGroundDiffuseSpecularMap->Release();
	if (gStarsDiffuseSpecularMapSRV)   gStarsDiffuseSpecularMapSRV->Release();
	if (gStarsDiffuseSpecularMap)      gStarsDiffuseSpecularMap->Release();

	if (gPostProcessingConstantBuffer)  gPostProcessingConstantBuffer->Release();
	if (gPerModelConstantBuffer)        gPerModelConstantBuffer->Release();
	if (gPerFrameConstantBuffer)        gPerFrameConstantBuffer->Release();

	ReleaseShaders();

	// See note in InitGeometry about why we're not using unique_ptr and having to manually delete
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		delete gLights[i].model;  gLights[i].model = nullptr;
	}
	delete gCamera;  gCamera = nullptr;
	delete gPortalCamera;  gPortalCamera = nullptr;
	delete gCrate;   gCrate = nullptr;
	delete gCube;    gCube = nullptr;
	delete gTv;      gTv = nullptr;
	delete gGround;  gGround = nullptr;
	delete gStars;   gStars = nullptr;
	delete gPortal;  gPortal = nullptr;
	delete gWall;    gWall = nullptr;
	delete gWall2;   gWall2 = nullptr;

	delete gPortalMesh;  gPortalMesh = nullptr;
	delete gLightMesh;   gLightMesh = nullptr;
	delete gCrateMesh;   gCrateMesh = nullptr;
	delete gCubeMesh;    gCubeMesh = nullptr;
	delete gGroundMesh;  gGroundMesh = nullptr;
	delete gStarsMesh;   gStarsMesh = nullptr;
	delete gWallMesh;    gWallMesh = nullptr;
	delete gWall2Mesh;    gWall2Mesh = nullptr;

}

// Render the scene from the given light's point of view. Only renders depth buffer
//void RenderDepthBufferFromLight()
//{
//	// Get camera-like matrices from the spotlight, seet in the constant buffer and send over to GPU
//	gPerFrameConstants.viewMatrix = CalculateLightViewMatrix(0);
//	gPerFrameConstants.projectionMatrix = CalculateLightProjectionMatrix(0);
//	gPerFrameConstants.viewProjectionMatrix = gPerFrameConstants.viewMatrix * gPerFrameConstants.projectionMatrix;
//	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);
//
//	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS) and pixel shader (PS)
//	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
//	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
//
//
//	//// Only render models that cast shadows ////
//
//	// Use special depth-only rendering shaders
//	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
//	gD3DContext->PSSetShader(gDepthOnlyPixelShader, nullptr, 0);
//
//	// States - no blending, normal depth buffer and culling
//	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
//	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
//	gD3DContext->RSSetState(gCullBackState);
//
//	// Render models - no state changes required between each object in this situation (no textures used in this step)
//	gGround->Render();
//	gTv->Render();
//	gCrate->Render();
//	gTv->Render();
//	gWall->Render();
//	gWall2->Render();
//	gStars->Render();
//}



//--------------------------------------------------------------------------------------
// Scene Rendering
//--------------------------------------------------------------------------------------

// Render everything in the scene from the given camera
void RenderSceneFromCamera(Camera* camera)
{
	// Set camera matrices in the constant buffer and send over to GPU
	gPerFrameConstants.cameraMatrix = camera->WorldMatrix();
	gPerFrameConstants.viewMatrix = camera->ViewMatrix();
	gPerFrameConstants.projectionMatrix = camera->ProjectionMatrix();
	gPerFrameConstants.viewProjectionMatrix = camera->ViewProjectionMatrix();
	UpdateConstantBuffer(gPerFrameConstantBuffer, gPerFrameConstants);

	// Indicate that the constant buffer we just updated is for use in the vertex shader (VS), geometry shader (GS) and pixel shader (PS)
	gD3DContext->VSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer); // First parameter must match constant buffer number in the shader 
	gD3DContext->GSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);
	gD3DContext->PSSetConstantBuffers(0, 1, &gPerFrameConstantBuffer);

	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);


	////--------------- Render ordinary models ---------------///

	// Select which shaders to use next
	gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPixelLightingPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);

	// Render lit models, only change textures for each onee
	gD3DContext->PSSetSamplers(0, 1, &gAnisotropic4xSampler);

	gD3DContext->PSSetShaderResources(0, 1, &gGroundDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gGround->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCrateDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCrate->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gCubeDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gCube->Render();

	gD3DContext->PSSetShaderResources(0, 1, &gWoodDiffuseSpecularMapSRV); // First parameter must match texture slot number in the shader
	gTv->Render();


	gD3DContext->PSSetShaderResources(0, 1, &gBrickSRV); // First parameter must match texture slot number in the shader
	gWall->Render();


	gD3DContext->PSSetShaderResources(0, 1, &gBrickSRV); // First parameter must match texture slot number in the shader
	gWall2->Render();

	//************************************


	// Select which shaders to use next
	gD3DContext->VSSetShader(gPixelLightingVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gPortalPixelShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)

	// States - no blending, normal depth buffer and back-face culling (standard set-up for opaque models)
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gUseDepthBufferState, 0);
	gD3DContext->RSSetState(gCullBackState);


	gD3DContext->PSSetShaderResources(0, 1, &gTvDiffuseSpecularMapSRV);
	gD3DContext->PSSetShaderResources(1, 1, &gSceneTextureSRV[3]); // First parameter must match texture slot number in the shader
	gPortal->Render();

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(1, 1, &nullSRV);

	////--------------- Render sky ---------------////

	// Select which shaders to use next
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Using a pixel shader that tints the texture - don't need a tint on the sky so set it to white
	gPerModelConstants.objectColour = { 1, 1, 1 };

	// Stars point inwards
	gD3DContext->RSSetState(gCullNoneState);

	// Render sky
	gD3DContext->PSSetShaderResources(0, 1, &gStarsDiffuseSpecularMapSRV);
	gStars->Render();

	////--------------- Render lights ---------------////

	// Select which shaders to use next (actually same as before, so we could skip this)
	gD3DContext->VSSetShader(gBasicTransformVertexShader, nullptr, 0);
	gD3DContext->PSSetShader(gTintedTexturePixelShader, nullptr, 0);

	// Select the texture and sampler to use in the pixel shader
	gD3DContext->PSSetShaderResources(0, 1, &gLightDiffuseMapSRV); // First parameter must match texture slot number in the shaer

	// States - additive blending, read-only depth buffer and no culling (standard set-up for blending)
	gD3DContext->OMSetBlendState(gAdditiveBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);

	// Render all the lights in the array
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		gPerModelConstants.objectColour = gLights[i].colour; // Set any per-model constants apart from the world matrix just before calling render (light colour here)
		gLights[i].model->Render();
	}
}



//**************************

// Select the appropriate shader plus any additional textures required for a given post-process
// Helper function shared by full-screen, area and polygon post-processing functions below
void SelectPostProcessShaderAndTextures(PostProcess postProcess, int index)
{
	if (postProcess == PostProcess::Copy)
	{
		gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Tint)
	{
		gD3DContext->PSSetShader(gTintPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::GreyNoise)
	{
		gD3DContext->PSSetShader(gGreyNoisePostProcess, nullptr, 0);

		// Give pixel shader access to the noise texture
		gD3DContext->PSSetShaderResources(1, 1, &gNoiseMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Burn)
	{
		gD3DContext->PSSetShader(gBurnPostProcess, nullptr, 0);

		// Give pixel shader access to the burn texture (basically a height map that the burn level ascends)
		gD3DContext->PSSetShaderResources(1, 1, &gBurnMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Distort)
	{
		gD3DContext->PSSetShader(gDistortPostProcess, nullptr, 0);

		// Give pixel shader access to the distortion texture (containts 2D vectors (in R & G) to shift the texture UVs to give a cut-glass impression)
		gD3DContext->PSSetShaderResources(1, 1, &gDistortMapSRV);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Spiral)
	{
		gD3DContext->PSSetShader(gSpiralPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::HeatHaze)
	{
		gD3DContext->PSSetShader(gHeatHazePostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::UnderWater)
	{
		gD3DContext->PSSetShader(gUnderWaterPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Blur)
	{

		CalculateWeights();
		gD3DContext->PSSetShader(gBlurPostProcess, nullptr, 0);
		//gPostProcessingConstants.kernalSize = kernalSize2;
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::SecondBlur)
	{
		gD3DContext->PSSetShader(gSecondBlurPostProcess, nullptr, 0);
		gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);
	}

	else if (postProcess == PostProcess::Bloom)
	{
		int pass = index;
		//gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);


		if (pass % 2 == 0)
		{
			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);
			gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[0]);
			//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
			SelectPostProcessShaderAndTextures(PostProcess::Copy, 0);
		}
		else
		{
			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);
			gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);
			//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
			SelectPostProcessShaderAndTextures(PostProcess::Copy, 0);
		}
			gD3DContext->Draw(4, 0);

			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);
			gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[5]);
			gD3DContext->PSSetShader(gBloomPostProcess, nullptr, 0);

			gD3DContext->Draw(4, 0);

			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);

			gD3DContext->Draw(4, 0);

			//gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);
			//gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[4]);
			//SelectPostProcessShaderAndTextures(PostProcess::Blur, 0);
			//gD3DContext->Draw(4, 0);
			////gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);
			////gD3DContext->PSSetShaderResources(1, 1, &gSceneTextureSRV[4]);
			////gD3DContext->PSSetShader(gCombinePostProcess, nullptr, 0);
			//gD3DContext->Draw(4, 0);

		//pass++;
		if (pass % 2 == 0)
		{
			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[1], gDepthStencil);
			gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[0]);
			//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
		}
		else
		{
			gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[0], gDepthStencil);
			gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);
			//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
		}

	}

	else if (postProcess == PostProcess::Retro)
	{
		gD3DContext->PSSetShader(gRetroPostProcess, nullptr, 0);
	}

	//else if (postProcess == PostProcess::Depth)
	//{
	//	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
	//	gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
	//	gD3DContext->PSSetSamplers(1, 1, &gPointSampler);
	//	gD3DContext->PSSetShader(gDepthPostProcess, nullptr, 0);
	//	/*gD3DContext->PSSetShaderResources(1, 1, &gMovieSRV);
	//	gD3DContext->PSSetSamplers(1, 1, &gTrilinearSampler);*/
	//}

	else if (postProcess == PostProcess::CellShading)
	{
		gD3DContext->PSSetShader(gCellPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Invert)
	{
		gD3DContext->PSSetShader(gInvertPostProcess, nullptr, 0);
	}

	else if (postProcess == PostProcess::Combine)
	{
		gD3DContext->PSSetShader(gCombinePostProcess, nullptr, 0);
		//gD3DContext->PSSetShaderResources(1, 1, &gSceneTextureSRV[4]);
	}

}

// Perform a full-screen post process from "scene texture" to back buffer
void ImagePostProcessing(PostProcess postProcess, int Target, int ResourseOutput, int pass)
{
	//gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[Target], gPortalDepthStencilView);
	//gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[ResourseOutput]);
	//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)


	if (pass % 2 == 0)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[Target], gDepthStencil);
		//gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[ResourseOutput]);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[ResourseOutput], gDepthStencil);
		////gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[Target]);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	}


	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);




	// Select shader and textures needed for the required post-processes (helper function above)
	SelectPostProcessShaderAndTextures(postProcess, 0);

	/*	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
		gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);*/


		//gD3DContext->Draw(4, 0);



	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);

	if (pass % 2 == 0)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[ResourseOutput], gDepthStencil);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[Target]);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[Target], gDepthStencil);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[ResourseOutput]);

	}

	SelectPostProcessShaderAndTextures(PostProcess::Copy, 0);

	gD3DContext->Draw(4, 0);

	//gD3DContext->PSSetShader(gCopyPostProcess, nullptr, 0);

}


// Perform a full-screen post process from "scene texture" to back buffer
void FullScreenPostProcess(PostProcess postProcess, int pass)
{
	//// Select the back buffer to use for rendering. Not going to clear the back-buffer because we're going to overwrite it all
	//gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);


	//// Give the pixel shader (post-processing shader) access to the scene texture 
	//gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[0]);
	//gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)




	// Using special vertex shader that creates its own data for a 2D screen quad
	gD3DContext->VSSetShader(g2DQuadVertexShader, nullptr, 0);
	gD3DContext->GSSetShader(nullptr, nullptr, 0);  // Switch off geometry shader when not using it (pass nullptr for first parameter)


	// States - no blending, don't write to depth buffer and ignore back-face culling
	gD3DContext->OMSetBlendState(gNoBlendingState, nullptr, 0xffffff);
	gD3DContext->OMSetDepthStencilState(gDepthReadOnlyState, 0);
	gD3DContext->RSSetState(gCullNoneState);


	// No need to set vertex/index buffer (see 2D quad vertex shader), just indicate that the quad will be created as a triangle strip
	gD3DContext->IASetInputLayout(NULL); // No vertex data
	gD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	if (pass % 2 == 0)
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[1], gDepthStencil);
		//gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[0]);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[0], gDepthStencil);
		////gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
		gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);
		gD3DContext->PSSetSamplers(0, 1, &gPointSampler); // Use point sampling (no bilinear, trilinear, mip-mapping etc. for most post-processes)

	}



	// Select shader and textures needed for the required post-processes (helper function above)
	SelectPostProcessShaderAndTextures(postProcess, pass);

	/*	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
		gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);*/


		//gD3DContext->Draw(4, 0);



	// Set 2D area for full-screen post-processing (coordinates in 0->1 range)
	gPostProcessingConstants.area2DTopLeft = { 0, 0 }; // Top-left of entire screen
	gPostProcessingConstants.area2DSize = { 1, 1 }; // Full size of screen
	gPostProcessingConstants.area2DDepth = 0;        // Depth buffer value for full screen is as close as possible


	// Pass over the above post-processing settings (also the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);


	// Draw a quad
	gD3DContext->Draw(4, 0);


	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);


	gD3DContext->Draw(4, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
}


// Perform an area post process from "scene texture" to back buffer at a given point in the world, with a given size (world units)
void AreaPostProcess(PostProcess postProcess, CVector3 worldPoint, CVector2 areaSize)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, 0);


	gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[0], gDepthStencil);

	gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);

	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess, 0);

	// Enable alpha blending - area effects need to fade out at the edges or the hard edge of the area is visible
	// A couple of the shaders have been updated to put the effect into a soft circle
	// Alpha blending isn't enabled for fullscreen and polygon effects so it doesn't affect those (except heat-haze, which works a bit differently)
	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);


	// Use picking methods to find the 2D position of the 3D point at the centre of the area effect
	auto worldPointTo2D = gCamera->PixelFromWorldPt(worldPoint, gViewportWidth, gViewportHeight);
	CVector2 area2DCentre = { worldPointTo2D.x, worldPointTo2D.y };
	float areaDistance = worldPointTo2D.z / zShift;

	// Nothing to do if given 3D point is behind the camera
	if (areaDistance < gCamera->NearClip())  return;

	// Convert pixel coordinates to 0->1 coordinates as used by the shader
	area2DCentre.x /= gViewportWidth;
	area2DCentre.y /= gViewportHeight;



	// Using new helper function here - it calculates the world space units covered by a pixel at a certain distance from the camera.
	// Use this to find the size of the 2D area we need to cover the world space size requested
	CVector2 pixelSizeAtPoint = gCamera->PixelSizeInWorldSpace(areaDistance, gViewportWidth, gViewportHeight);
	CVector2 area2DSize = { areaSize.x / pixelSizeAtPoint.x, areaSize.y / pixelSizeAtPoint.y };

	// Again convert the result in pixels to a result to 0->1 coordinates
	area2DSize.x /= gViewportWidth;
	area2DSize.y /= gViewportHeight;



	// Send the area top-left and size into the constant buffer - the 2DQuad vertex shader will use this to create a quad in the right place
	gPostProcessingConstants.area2DTopLeft = area2DCentre - 0.5f * area2DSize; // Top-left of area is centre - half the size
	gPostProcessingConstants.area2DSize = area2DSize;

	// Manually calculate depth buffer value from Z distance to the 3D point and camera near/far clip values. Result is 0->1 depth value
	// We've never seen this full calculation before, it's occasionally useful. It is derived from the material in the Picking lecture
	// Having the depth allows us to have area effects behind normal objects
	gPostProcessingConstants.area2DDepth = gCamera->FarClip() * (areaDistance - gCamera->NearClip()) / (gCamera->FarClip() - gCamera->NearClip());
	gPostProcessingConstants.area2DDepth /= areaDistance;

	// Pass over this post-processing area to shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);



	gD3DContext->Draw(4, 0);

	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->Draw(4, 0);
}


// Perform an post process from "scene texture" to back buffer within the given four-point polygon and a world matrix to position/rotate/scale the polygon
void PolygonPostProcess(PostProcess postProcess, const std::array<CVector3, 4> & points, const CMatrix4x4& worldMatrix)
{
	// First perform a full-screen copy of the scene to back-buffer
	FullScreenPostProcess(PostProcess::Copy, 0);

	gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[0], gDepthStencil);

	gD3DContext->PSSetShaderResources(0, 1, &gSceneTextureSRV[1]);


	// Now perform a post-process of a portion of the scene to the back-buffer (overwriting some of the copy above)
	// Note: The following code relies on many of the settings that were prepared in the FullScreenPostProcess call above, it only
	//       updates a few things that need to be changed for an area process. If you tinker with the code structure you need to be
	//       aware of all the work that the above function did that was also preparation for this post-process area step

	// Select shader/textures needed for required post-process
	SelectPostProcessShaderAndTextures(postProcess, 0);


	gD3DContext->OMSetBlendState(gAlphaBlendingState, nullptr, 0xffffff);
	// Loop through the given points, transform each to 2D (this is what the vertex shader normally does in most labs)
	for (unsigned int i = 0; i < points.size(); ++i)
	{
		CVector4 modelPosition = CVector4(points[i], 1);
		CVector4 worldPosition = modelPosition * worldMatrix;
		CVector4 viewportPosition = worldPosition * gCamera->ViewProjectionMatrix();

		gPostProcessingConstants.polygon2DPoints[i] = viewportPosition;
	}

	// Pass over the polygon points to the shaders (also sends the per-process settings prepared in UpdateScene function below)
	UpdateConstantBuffer(gPostProcessingConstantBuffer, gPostProcessingConstants);
	gD3DContext->VSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);
	gD3DContext->PSSetConstantBuffers(1, 1, &gPostProcessingConstantBuffer);

	// Select the special 2D polygon post-processing vertex shader and draw the polygon
	gD3DContext->VSSetShader(g2DPolygonVertexShader, nullptr, 0);




	gD3DContext->Draw(4, 0);

	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);

	gD3DContext->Draw(4, 0);


}


//**************************

bool PortalMove(CMatrix4x4 startMat)
{
	//int part = GetPartitionFromPt(startMat.Position());
	CVector3 moveVec(0, 0, 0);
	// For each portal of given partition
	//TPortalIter itPortal = Partitions[part].Portals.begin();

		// Calculate world space portal entrance polygon

		//TransformPortalShape((*itPortal)->Shape, (*itPortal)->InMatrix, portalPoly);

		// See if any intersection of movement and the two triangles in the portal polygon

	float dist = Length(gPortal->Position() - gCamera->Position());


	//		// If the intersection was within the distance moved
	//		if (dist < 10.0f)
	//		{
	//			// Update matrix to reflect movement and transfomation through portal
	//			CMatrix4x4 endMat = startMat;
	//			endMat.Move(moveVec);
	//			endMat *= InverseAffine(gPortalCamera->WorldMatrix() * gPortalCamera->WorldMatrix());
	//			return endMat;
	//		}


	//
	//

	//CMatrix4x4 endMat = startMat;
	//endMat.Move(moveVec);
	//return endMat;

				// If the intersection was within the distance moved
	if (dist < 10.0f)
	{
		// Update matrix to reflect movement and transfomation through portal
		CMatrix4x4 endMat = startMat;
		endMat.Move(moveVec);
		endMat *= InverseAffine(gPortalCamera->WorldMatrix() * gPortalCamera->WorldMatrix());
		return true;
	}





	CMatrix4x4 endMat = startMat;
	endMat.Move(moveVec);
	return false;


}

// Rendering the scene
void RenderScene()
{
	//IMGUI
//*******************************
// Prepare ImGUI for this frame
//*******************************

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//*******************************

	//// Common settings ////


	CVector2 pixelPt;
	CVector3  test = (gCamera->PixelFromWorldPt(gLights[0].model->Position(), gViewportWidth, gViewportHeight));

	pixelPt.x = test.x;
	pixelPt.y = test.y;

	gCamera->PixelFromWorldPt(gLights[0].model->Position(), gViewportWidth, gViewportHeight);

	// Set up the light information in the constant buffer
	// Don't send to the GPU yet, the function RenderSceneFromCamera will do that
	gPerFrameConstants.light1Colour = gLights[0].colour * gLights[0].strength;
	gPerFrameConstants.light1Position = gLights[0].model->Position();
	//gPerFrameConstants.lightPixelPosition = pixelPt;



	gPerFrameConstants.light2Colour = gLights[1].colour * gLights[1].strength;
	gPerFrameConstants.light2Position = gLights[1].model->Position();

	gPerFrameConstants.ambientColour = gAmbientColour;
	gPerFrameConstants.specularPower = gSpecularPower;
	gPerFrameConstants.cameraPosition = gCamera->Position();

	gPerFrameConstants.viewportWidth = static_cast<float>(gViewportWidth);
	gPerFrameConstants.viewportHeight = static_cast<float>(gViewportHeight);



	//// Portal scene rendering ////

	// Set the portal texture and portal depth buffer as the targets for rendering
	// The portal texture will later be used on models in the main scene
	gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[2], gPortalDepthStencilView);

	// Clear the portal texture to a fixed colour and the portal depth buffer to the far distance
	gD3DContext->ClearRenderTargetView(gSceneRenderTarget[2], &gBackgroundColor.r);
	gD3DContext->ClearDepthStencilView(gPortalDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Setup the viewport for the portal texture size
	D3D11_VIEWPORT vp;
	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	// Render the scene for the portal
	RenderSceneFromCamera(gPortalCamera);

	if (TVList.empty())
	{
		ImagePostProcessing(PostProcess::Copy, 3, 2, 0);
	}
	else
	{
		for (int i = 0; i < TVList.size(); i++)
		{
			ImagePostProcessing(TVList[i], 3, 2, i);
		}
	}
	ID3D11ShaderResourceView* nullSRV = nullptr;
	gD3DContext->PSSetShaderResources(1, 1, &nullSRV);
	gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
	////--------------- Main scene rendering ---------------////

	// Set the target for rendering and select the main depth buffer.
	// If using post-processing then render to the scene texture, otherwise to the usual back buffer
	// Also clear the render target to a fixed colour and the depth buffer to the far distance

	//gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[4], gDepthStencil);
	//gD3DContext->ClearRenderTargetView(gSceneRenderTarget[4], &gBackgroundColor.r);
	//gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);
	//RenderSceneFromCamera(gCamera);


	if (!currentList.empty())
	{
		gD3DContext->OMSetRenderTargets(1, &gSceneRenderTarget[0], gDepthStencil);
		gD3DContext->ClearRenderTargetView(gSceneRenderTarget[0], &gBackgroundColor.r);
	}
	else
	{
		gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, gDepthStencil);
		gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &gBackgroundColor.r);
	}

	gD3DContext->ClearDepthStencilView(gDepthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);




	// Setup the viewport to the size of the main window

	//gD3DContext->PSSetShaderResources(1, 1, &gDepthShaderView);
	//gD3DContext->PSSetSamplers(1, 1, &gPointSampler);

	vp.Width = static_cast<FLOAT>(gViewportWidth);
	vp.Height = static_cast<FLOAT>(gViewportHeight);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gD3DContext->RSSetViewports(1, &vp);

	// Render the scene from the main camera
	//ColourRGBA white = { 1,1,1 };
	//gD3DContext->ClearRenderTargetView(gBackBufferRenderTarget, &white.r);
	//RenderDepthBufferFromLight();
	RenderSceneFromCamera(gCamera);




	////--------------- Scene completion ---------------////




	// Run any post-processing steps
	if (!currentList.empty())
	{
		for (int i = 0; i < currentList.size(); i++)
		{
			if (gCurrentPostProcessMode == PostProcessMode::Fullscreen)
			{
				FullScreenPostProcess(currentList[i], i);
			}
			//ID3D11ShaderResourceView* nullSRV = nullptr;
			gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
		}
		if (gCurrentPostProcessMode == PostProcessMode::Area)
		{
			// Pass a 3D point for the centre of the affected area and the size of the (rectangular) area in world units
			AreaPostProcess(gCurrentPostProcess, ModelSelected->Position(), { 10, 10 });
		}

		else if (gCurrentPostProcessMode == PostProcessMode::Polygon)
		{
			// An array of four points in world space - a tapered square centred at the origin			

			for (int i = 0; i < postProcessTing.size(); i++)
			{
				//CVector3 polyPos = CVector3( postProcessTing[i]->points[0] + postProcessTing[i]->points[1] + postProcessTing[i]->points[2] + postProcessTing[i]->points[3]) / 12;
				CVector3 polyPos = postProcessTing[i]->polyMatrix.GetPosition();
				float x = gCamera->Position().x - polyPos.x;
				float y = gCamera->Position().y - polyPos.y;
				float z = gCamera->Position().z - polyPos.z;


				float dist = sqrt((x * x) + (y * y) + (z * z));

				postProcessTing[i]->distance = dist;
			}

			for (int j = 0; j < postProcessTing.size(); j++)
			{
				for (int i = 0; i < postProcessTing.size() - 1; i++)
				{
					if (postProcessTing[i]->distance < postProcessTing[i + 1]->distance)
					{
						std::swap(postProcessTing[i], postProcessTing[i + 1]);
					}
				}
			}



			for (int i = 0; i < postProcessTing.size(); i++)
			{
				PolygonPostProcess(postProcessTing[i]->process[0], postProcessTing[i]->points, postProcessTing[i]->polyMatrix);
				gD3DContext->PSSetShaderResources(0, 1, &nullSRV);
			}
		}


		// These lines unbind the scene texture from the pixel shader to stop DirectX issuing a warning when we try to render to it again next frame
			//ID3D11ShaderResourceView* nullSRV = nullptr;
		gD3DContext->PSSetShaderResources(0, 1, &nullSRV);

	}

	//IMGUI
	//*******************************
	// Draw ImGUI interface
	//*******************************
	// You can draw ImGUI elements at any time between the frame preparation code at the top
	// of this function, and the finalisation code below

	//ImGui::ShowDemoWindow();

	ImGui::Begin("Main Menu", 0, ImGuiWindowFlags_AlwaysAutoResize);
	//ImGui::SliderFloat("Colour", &gPostProcessingConstants.tintColour.x, 1, 20);
	// Generate a dummy default palette. The palette will persist and can be edited.
	static bool saved_palette_init = true;
	static ImVec4 saved_palette[32] = {};
	if (saved_palette_init)
	{
		for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
		{
			ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f, saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
			saved_palette[n].w = 1.0f; // Alpha
		}
		saved_palette_init = false;
	}

	static ImVec4 backup_colour;
	bool open_popup = ImGui::ColorButton("MyColor##3b", color, 1);
	ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
	open_popup |= ImGui::Button("Palette");
	if (open_popup)
	{
		ImGui::OpenPopup("mypicker");
		backup_colour = color;
	}
	if (ImGui::BeginPopup("mypicker"))
	{
		ImGui::Text("Colour Picker");
		ImGui::Separator();
		ImGui::ColorPicker4("##picker", (float*)& color, 1 | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
		ImGui::Separator();
		ImGui::BeginGroup(); // Lock X position
		ImGui::Text("Current");
		ImGui::ColorButton("##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
		ImGui::Text("Previous");
		ImGui::ColorButton("##current", backup_colour, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
		ImGui::Separator();

		ImGui::ColorPicker4("##picker2", (float*)& color2, 1 | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
		ImGui::Separator();
		ImGui::Text("Current2");
		ImGui::Text("Previous");
		ImGui::ColorButton("##current", backup_colour, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
		ImGui::Separator();
		ImGui::ColorButton("##current", color2, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
		ImGui::Separator();
		ImGui::SameLine();


		gPostProcessingConstants.tintColour.x = color.x;
		gPostProcessingConstants.tintColour.y = color.y;
		gPostProcessingConstants.tintColour.z = color.z;
		gPostProcessingConstants.tintColour2.x = color2.x;
		gPostProcessingConstants.tintColour2.y = color2.y;
		gPostProcessingConstants.tintColour2.z = color2.z;

		ImGui::EndGroup();
		ImGui::EndPopup();
	}
	else
	{
		gPostProcessingConstants.tintColour.x = color.x;
		gPostProcessingConstants.tintColour.y = color.y;
		gPostProcessingConstants.tintColour.z = color.z;
		gPostProcessingConstants.tintColour2.x = color2.x;
		gPostProcessingConstants.tintColour2.y = color2.y;
		gPostProcessingConstants.tintColour2.z = color2.z;
	}

	ImGui::SliderInt("Blur", &gPostProcessingConstants.kernalSize, 3, KernelMaxSize);
	//ImGui::SliderFloat("Colour Green", &gPostProcessingConstants.tintColour.y, 0, 1);
	//ImGui::SliderFloat("Colour Blue", &gPostProcessingConstants.tintColour.z, 0, 1);

	if (ImGui::TreeNode("Screen Post Processes"))
	{
		if (ImGui::Button("Tint", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Tint, currentList.push_back(gCurrentPostProcess);
		}

		if (ImGui::Button("Blur", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Blur, currentList.push_back(gCurrentPostProcess), gCurrentPostProcess = PostProcess::SecondBlur, currentList.push_back(gCurrentPostProcess);
		}

		if (ImGui::Button("Burn", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::Burn, currentList.push_back(gCurrentPostProcess);
		}

		if (ImGui::Button("Water", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::UnderWater, currentList.push_back(gCurrentPostProcess);
		}

		if (ImGui::Button("Clear", ImVec2(100, 20)))
		{
			gCurrentPostProcess = PostProcess::None, currentList.clear();
		}


		//gPostProcessingConstants.blurLevel = 3;




		ImGui::TreePop();
	}

	if (ImGui::TreeNode("TV Post Processes"))
	{
		if (ImGui::Button("Tint", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::Tint);
		}

		if (ImGui::Button("Retro", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::Retro);
		}
		if (ImGui::Button("Burn", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::Burn);
		}

		if (ImGui::Button("Grey Noise", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::GreyNoise);
		}

		if (ImGui::Button("Blur", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::Blur);
			TVList.push_back(PostProcess::SecondBlur);
		}

		if (ImGui::Button("Water", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::UnderWater);
		}

		if (ImGui::Button("Clear TV Screen", ImVec2(100, 20)))
		{
			TVList.push_back(PostProcess::Copy);
			TVList.clear();
		}

		ImGui::TreePop();
	}



	ImGui::End();
	//*******************************



	////--------------- Scene completion ---------------////

	//IMGUI
	//*******************************
	// Finalise ImGUI for this frame
	//*******************************
	ImGui::Render();
	gD3DContext->OMSetRenderTargets(1, &gBackBufferRenderTarget, nullptr);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


	// When drawing to the off-screen back buffer is complete, we "present" the image to the front buffer (the screen)
	// Set first parameter to 1 to lock to vsync
	gSwapChain->Present(lockFPS ? 1 : 0, 0);


	CVector3 test4;
	CVector2 entityPixel;

	MousePixel.x = GetMouseX();
	MousePixel.y = GetMouseY();

	float nearestDistanceMove = 50.0f;
	for (int i = 0; i < allModels.size(); i++)
	{
		MousePixel.x = GetMouseX();
		MousePixel.y = GetMouseY();

		test4 = gCamera->PixelFromWorldPt(allModels[i]->Position(), gViewportWidth, gViewportHeight);
		entityPixel.x = test4.x;
		entityPixel.y = test4.y;

		float pixelDistance = Distance(MousePixel, entityPixel);
		if (pixelDistance < nearestDistanceMove)
		{

			MoveNearestEntity = allModels[i];
			nearestDistanceMove = pixelDistance;
			break;
		}
		else
		{
			MoveNearestEntity = 0;
		}

	}











}


//--------------------------------------------------------------------------------------
// Scene Update
//--------------------------------------------------------------------------------------


// Update models and camera. frameTime is the time passed since the last frame
void UpdateScene(float frameTime)
{
	//***********

	// Select post process on keys
	if (KeyHit(Key_F1))  gCurrentPostProcessMode = PostProcessMode::Fullscreen;
	if (KeyHit(Key_F2))  gCurrentPostProcessMode = PostProcessMode::Area;
	if (KeyHit(Key_F3))  gCurrentPostProcessMode = PostProcessMode::Polygon;

	if (KeyHit(Key_1))  gCurrentPostProcess = PostProcess::Tint, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_2))  gCurrentPostProcess = PostProcess::GreyNoise, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_3))  gCurrentPostProcess = PostProcess::Burn, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_4))  gCurrentPostProcess = PostProcess::Distort, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_5))  gCurrentPostProcess = PostProcess::Spiral, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_6))   gCurrentPostProcess = PostProcess::HeatHaze, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_7))  gCurrentPostProcess = PostProcess::Blur, currentList.push_back(gCurrentPostProcess), gCurrentPostProcess = PostProcess::SecondBlur, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_8))  gCurrentPostProcess = PostProcess::Bloom, currentList.push_back(gCurrentPostProcess)/*, gCurrentPostProcess = PostProcess::Blur, currentList.push_back(gCurrentPostProcess),
		gCurrentPostProcess = PostProcess::SecondBlur, currentList.push_back(gCurrentPostProcess)*/;
	if (KeyHit(Key_9))   gCurrentPostProcess = PostProcess::Copy, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_0))   gCurrentPostProcess = PostProcess::None, currentList.clear();
	if (KeyHit(Key_Numpad0))  gCurrentPostProcess = PostProcess::Depth, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_Numpad1))  gCurrentPostProcess = PostProcess::CellShading, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_Numpad2))  gCurrentPostProcess = PostProcess::Invert, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_Numpad3))  gCurrentPostProcess = PostProcess::Retro, currentList.push_back(gCurrentPostProcess);
	if (KeyHit(Key_Numpad4))  gCurrentPostProcess = PostProcess::UnderWater, currentList.push_back(gCurrentPostProcess);

	if (KeyHeld(Key_Period) && !KeyHeld(Mouse_RButton))  zShift += 0.5f;
	if (KeyHeld(Key_Comma) && !KeyHeld(Mouse_RButton))   zShift -= 0.5f;




	bool newCamPos = PortalMove(gCamera->WorldMatrix());

	if (newCamPos)
	{
		for (int i = TVList.size() - 1; i > 0; i--)
		{
			currentList.insert(currentList.begin(), TVList[i]);
			gCamera->SetPosition(gPortalCamera->Position());
			gCamera->SetRotation(gPortalCamera->Rotation());
			//gTvPostProcess = PostProcess(rand() % int(PostProcess::AmountOfPosts) + int(PostProcess::None));
		}
	}




	//gCamera->SetRotation = newCamPos;

	// Post processing settings - all data for post-processes is updated every frame whether in use or not (minimal cost)

	// Colour for tint shader
	//gPostProcessingConstants.tintColour = { 0.0f, 1.0f, 1.0f };

	// Noise scaling adjusts how fine the grey noise is.
	const float grainSize = 140; // Fineness of the noise grain
	gPostProcessingConstants.noiseScale = { gViewportWidth / grainSize, gViewportHeight / grainSize };

	// The noise offset is randomised to give a constantly changing noise effect (like tv static)
	gPostProcessingConstants.noiseOffset = { Random(0.0f, 1.0f), Random(0.0f, 1.0f) };

	// Set and increase the burn level (cycling back to 0 when it reaches 1.0f)
	const float burnSpeed = 0.2f;
	gPostProcessingConstants.burnHeight = fmod(gPostProcessingConstants.burnHeight + burnSpeed * frameTime, 1.0f);

	// Set the level of distortion
	gPostProcessingConstants.distortLevel = 0.03f;

	// Set and increase the amount of spiral - use a tweaked cos wave to animate
	static float wiggle = 0.0f;
	const float wiggleSpeed = 1.0f;
	gPostProcessingConstants.spiralLevel = ((1.0f - cos(wiggle)) * 4.0f);
	wiggle += wiggleSpeed * frameTime;

	// Update heat haze timer
	gPostProcessingConstants.heatHazeTimer += frameTime;

	// Set Bloom Level
	gPostProcessingConstants.bloomLevel = 10;

	// Set Blur Level

	static float HueTimer = 0.0f;
	const float HueSpeed = 0.1f;
	gPostProcessingConstants.hueTimer = ((1.0f - cos(HueTimer)) * 4.0f);
	HueTimer += HueSpeed * frameTime;

	// Set and increase the amount of Underwater - use a tweaked cos wave to animate
	static float UnderWaterTimer = 0.0f;
	const float UnderWaterSpeed = 0.5f;
	gPostProcessingConstants.underWaterLevel = ((1.0f - cos(UnderWaterTimer)) * 4.0f);
	UnderWaterTimer += UnderWaterSpeed * frameTime;




	//***********


	// Orbit one light - a bit of a cheat with the static variable [ask the tutor if you want to know what this is]
	static float lightRotate = 0.0f;
	static float portalRotate = 0.0f;
	static bool go = true;
	gLights[0].model->SetPosition({ 20 + cos(lightRotate) * gLightOrbitRadius, 10, 20 + sin(lightRotate) * gLightOrbitRadius });
	gPortalCamera->SetPosition({ 0 + cos(portalRotate * 7) * gPortalOrbitRadius, 40, 0 + sin(portalRotate * 7) * gPortalOrbitRadius });
	gPortalCamera->SetRotation({ gPortalCamera->Rotation().x, 40 + cos(-portalRotate / 2.8f) * -gPortalOrbitRadius, 0 });
	if (go)  lightRotate -= gLightOrbitSpeed * frameTime;
	if (go)  portalRotate -= gPortalOrbitSpeed * frameTime;
	if (KeyHit(Key_L))  go = !go;

	// Control of camera
	gCamera->Control(frameTime, Key_Up, Key_Down, Key_Left, Key_Right, Key_W, Key_S, Key_A, Key_D);

	// mouse stuff

	MousePixel.x = GetMouseX();
	MousePixel.y = GetMouseY();



	//Move entites witht the mouse in the scene
	if (KeyHeld(Mouse_RButton) && MoveNearestEntity != 0)
	{
		ModelSelected = MoveNearestEntity;

		MousePixel.x = GetMouseX();
		MousePixel.y = GetMouseY();

		CVector3 modelRotation = ModelSelected->Rotation();
		if (KeyHeld(Key_Comma))
		{
			modelRotation.y += 1 * frameTime;
			ModelSelected->SetRotation(modelRotation);
		}
		else if (KeyHeld(Key_Period))
		{
			modelRotation.y += -1 * frameTime;
			ModelSelected->SetRotation(modelRotation);
		}


		int newMouseWheelPos = 0;

		CVector3 worldpt = gCamera->WorldPtFromPixel(MousePixel, gViewportWidth, gViewportHeight); //Mouse world pos
		CVector3 rayCast = Normalise(worldpt - gCamera->Position());   //World point to camera direction 
		CVector3 camPos = gCamera->Position(); //Main cam pos

		//t = -camPos.y / rayCast.y;

		t = Length(ModelSelected->Position() - gCamera->Position());

		newMouseWheelPos = GetMouseWheel();
		if (oldMouseWheelPos < newMouseWheelPos)
		{
			t += 500 * frameTime;
		}
		else if (oldMouseWheelPos > newMouseWheelPos)
		{
			t += -500 * frameTime;
		}

		oldMouseWheelPos = newMouseWheelPos;
		CVector3 newPos = gCamera->Position() + t * rayCast;
		if (ModelSelected != 0)
		{
			//Move to newPos decided by mouse position


			ModelSelected->SetPosition(newPos);


		}
		if (ModelSelected == gTv)
		{
			gPortal->SetPosition({ gTv->Position() });
			gPortal->SetRotation(CVector3(gTv->Rotation().x, gTv->Rotation().y, gTv->Rotation().z));
			//gPortal->SetPosition({ gPortal->Position().x, gPortal->Position().y,  gPortal->Position().z -7.6f });*/

			//gPortal->SetWorldMatrix(gTv->WorldMatrix());
			gPortal->SetPosition(gTv->Position() + (gTv->WorldMatrix().GetZAxis() * 5.1));
		}
		MoveNearestEntity = 0;
	}
	MoveNearestEntity = 0;








	// Toggle FPS limiting
	if (KeyHit(Key_P))  lockFPS = !lockFPS;

	// Show frame time / FPS in the window title //
	const float fpsUpdateTime = 0.5f; // How long between updates (in seconds)
	static float totalFrameTime = 0;
	static int frameCount = 0;
	totalFrameTime += frameTime;
	++frameCount;
	if (totalFrameTime > fpsUpdateTime)
	{
		// Displays FPS rounded to nearest int, and frame time (more useful for developers) in milliseconds to 2 decimal places
		float avgFrameTime = totalFrameTime / frameCount;
		std::ostringstream frameTimeMs;
		frameTimeMs.precision(2);
		frameTimeMs << std::fixed << avgFrameTime * 1000;
		std::string windowTitle = "CO3303 Week 14: Area Post Processing - Frame Time: " + frameTimeMs.str() +
			"ms, FPS: " + std::to_string(static_cast<int>(1 / avgFrameTime + 0.5f));
		SetWindowTextA(gHWnd, windowTitle.c_str());
		totalFrameTime = 0;
		frameCount = 0;
	}
}
