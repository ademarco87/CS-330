///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  SetupSceneLights()
 *
 *  Adds and configures light sources, including sunlight.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Sunlight as a directional light
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.5f, -1.0f, -0.5f)); // Sunlight direction
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f));       // Soft ambient light
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(1.0f, 0.9f, 0.7f));       // Warm sunlight
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f));      // Bright highlights
}

void SceneManager::DefineMaterials()
{
	OBJECT_MATERIAL defaultMaterial;
	defaultMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);  // Light gray
	defaultMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Shiny
	defaultMaterial.shininess = 32.0f;                           // Moderately shiny
	defaultMaterial.tag = "default";

	m_objectMaterials.push_back(defaultMaterial);

	OBJECT_MATERIAL planeMaterial;
	planeMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);  // Light gray
	planeMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // Reflective white
	planeMaterial.shininess = 64.0f;                           // High shininess for reflection
	planeMaterial.tag = "planeMaterial";

	m_objectMaterials.push_back(planeMaterial);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/

void SceneManager::PrepareScene()
{
	DefineMaterials();
	// Load the textures directly in PrepareScene
	if (!CreateGLTexture("textures/gravel.jpg", "gravelTexture"))
	{
		std::cerr << "Failed to load gravel texture!" << std::endl;
	}
	if (!CreateGLTexture("textures/tree.jpg", "treeTexture"))
	{
		std::cerr << "Failed to load Tree texture!" << std::endl;
	}
	if (!CreateGLTexture("textures/hedge.jpg", "hedgeTexture"))
	{
		std::cerr << "Failed to load hedge texture!" << std::endl;
	}
	if (!CreateGLTexture("textures/sky.jpg", "skyTexture"))
	{
		std::cerr << "Failed to load sky texture!" << std::endl;
	}

	// Bind the loaded textures to texture units
	BindGLTextures();

	// Load the meshes required for the scene
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	// Set the sky color (clear color)
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Black background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Disable depth testing for sky rendering
	glDisable(GL_DEPTH_TEST);

	// *** Draw Sky Sphere *** //
	scaleXYZ = glm::vec3(-50.0f, -50.0f, -50.0f);  // Large sphere surrounding the scene
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);     // Centered at the origin

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.53f, 0.81f, 0.98f, 1.0f);
	SetShaderTexture("skyTexture");
	SetTextureUVScale(1.05f, 1.05f);  // Slight UV scale tweak to minimize seams
	m_basicMeshes->DrawSphereMesh();  // Draw the sky sphere

	// Re-enable depth testing for the rest of the scene
	glEnable(GL_DEPTH_TEST);

	//*** Draw Ground Plane (gravel path) ***//
	scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);  // Large ground plane
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);  // Centered at the origin
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.7f, 1.0f);  // Light gravel color
	SetShaderMaterial("planeMaterial");
	SetShaderTexture("gravelTexture");
	SetTextureUVScale(100.0f, 100.0f);
	m_basicMeshes->DrawPlaneMesh();  // Draw the ground

	// Define positions for the cone trees
	glm::vec3 frontConePositions[] = {
		glm::vec3(-5.0f, 0.0f, 3.0f),  // Front-left tree in front of the left hedge
		glm::vec3(5.0f, 0.0f, 3.0f),   // Front-right tree in front of the right hedge
		glm::vec3(-5.0f, 0.0f, 7.0f),  // Back-left tree behind the left hedge
		glm::vec3(5.0f, 0.0f, 7.0f)    // Back-right tree behind the right hedge
	};

	// Loop through the positions to draw the cone trees
	for (size_t i = 0; i < sizeof(frontConePositions) / sizeof(frontConePositions[0]); ++i) {
		positionXYZ = frontConePositions[i]; // Set the position of the current tree

		// Apply a consistent scale for the trees
		scaleXYZ = glm::vec3(1.0f, 4.5f, 1.0f);  // Standard scale for all trees

		// Set the transformations for the tree
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

		// Set the tree color to green
		SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);
		SetTextureUVScale(50.0f, 50.0f);
		SetShaderTexture("treeTexture");

		// Draw the cone mesh to represent the tree
		m_basicMeshes->DrawConeMesh();
	}

	//*** Draw Hedges ***//
	scaleXYZ = glm::vec3(2.0f, 1.75f, 3.5f);  // Proportional hedges
	glm::vec3 hedgePositions[] = {
		glm::vec3(-5.0f, 0.875f, 5.0f),  // Left hedge
		glm::vec3(5.0f, 0.875f, 5.0f),    // Right hedge
		glm::vec3(5.0f, 0.85f, 9.5f),   // front left
		glm::vec3(-5.0f, 0.85f, 9.5f), // front right
		glm::vec3(5.0f, 0.85f, 1.0f), // back left
		glm::vec3(-5.0f, 0.85f, 1.0f) // back right
	};
	for (const auto& position : hedgePositions) {
		positionXYZ = position;
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(0.1f, 0.5f, 0.1f, 1.0f);  // Dark green for hedges
		SetShaderTexture("hedge");
		m_basicMeshes->DrawBoxMesh();
	}

	//*** Draw Spiral Tree (centerpiece) ***//
	scaleXYZ = glm::vec3(1.5f, 3.0f, 1.5f);  // Size for spiral tree cones
	float spiralHeight = 0.0f;
	for (int i = 0; i < 5; ++i) {
		positionXYZ = glm::vec3(0.0f, spiralHeight, -15.0f);  // Centered at -15 Z
		YrotationDegrees = i * 45.0f;  // Increment rotation for spiral
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);  // Green color
		SetShaderTexture("treeTexture");
		m_basicMeshes->DrawConeMesh();
		spiralHeight += 1.0f;  // Increment height for each cone
	}

	// Fix orientation and position for Circular Hedge (Torus)
	scaleXYZ = glm::vec3(7.5f, 5.5f, 6.5f); // Flatten vertically, stretch horizontally
	positionXYZ = glm::vec3(0.0f, 0.25f, -15.0f); // Position it around the tree base
	XrotationDegrees = 90.0f; // Rotate 90 degrees around the X-axis
	YrotationDegrees = 0.0f;  // No rotation around Y-axis
	ZrotationDegrees = 0.0f;  // No rotation around Z-axis

	// Apply transformations and draw
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.2f, 0.6f, 0.2f, 1.0f); // Medium green color
	SetShaderTexture("treeTexture");
	m_basicMeshes->DrawTorusMesh();

	// Render the Sun (Visible Light Source)
	scaleXYZ = glm::vec3(5.0f, 5.0f, 5.0f);  // Scale for the sun
	positionXYZ = glm::vec3(-30.0f, 20.0f, -70.0f);  // Position of the sun in the sky

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.9f, 0.5f, 1.0f);  // Bright yellowish color for the sun
	m_basicMeshes->DrawSphereMesh();  // Draw the sun sphere

	// Render the Sun (Core Sphere)
	glm::vec3 sunPosition = glm::vec3(-30.0f, 20.0f, -70.0f);  // Position of the sun
	glm::vec3 sunCoreScale = glm::vec3(5.0f, 5.0f, 5.0f);      // Core sun size

	// Core Sun Sphere
	SetTransformations(sunCoreScale, 0.0f, 0.0f, 0.0f, sunPosition);
	SetShaderColor(1.0f, 0.9f, 0.5f, 1.0f);  // Bright yellowish core
	m_basicMeshes->DrawSphereMesh();

	// Render the Sun Glow (Outer Sphere)
	glm::vec3 sunGlowScale = glm::vec3(10.0f, 10.0f, 10.0f);  // Larger size for glow

	SetTransformations(sunGlowScale, 0.0f, 0.0f, 0.0f, sunPosition);
	SetShaderColor(1.0f, 0.9f, 0.5f, 0.3f);  // Semi-transparent glow
	m_basicMeshes->DrawSphereMesh();

	// *** Draw 50 Static Trees in the Background *** //
	glm::vec3 treePositions[50] = {
		{-30.0f, 0.0f, -30.0f}, {-25.0f, 0.0f, -35.0f}, {-20.0f, 0.0f, -40.0f},
		{-15.0f, 0.0f, -45.0f}, {-10.0f, 0.0f, -50.0f}, {-5.0f, 0.0f, -35.0f},
		{0.0f, 0.0f, -40.0f},   {5.0f, 0.0f, -45.0f},   {10.0f, 0.0f, -50.0f},
		{15.0f, 0.0f, -30.0f},  {20.0f, 0.0f, -35.0f},  {25.0f, 0.0f, -40.0f},
		{30.0f, 0.0f, -50.0f},  {-35.0f, 0.0f, -30.0f}, {-40.0f, 0.0f, -35.0f},
		{-45.0f, 0.0f, -40.0f}, {-50.0f, 0.0f, -45.0f}, {-40.0f, 0.0f, -50.0f},
		{-35.0f, 0.0f, -40.0f}, {-20.0f, 0.0f, -50.0f}, {-10.0f, 0.0f, -45.0f},
		{0.0f, 0.0f, -35.0f},   {10.0f, 0.0f, -30.0f},  {20.0f, 0.0f, -50.0f},
		{30.0f, 0.0f, -45.0f},  {40.0f, 0.0f, -40.0f},  {45.0f, 0.0f, -35.0f},
		{50.0f, 0.0f, -50.0f},  {35.0f, 0.0f, -30.0f},  {25.0f, 0.0f, -35.0f},
		{15.0f, 0.0f, -40.0f},  {5.0f, 0.0f, -45.0f},   {-5.0f, 0.0f, -50.0f},
		{-15.0f, 0.0f, -30.0f}, {-25.0f, 0.0f, -35.0f}, {-35.0f, 0.0f, -40.0f},
		{-45.0f, 0.0f, -50.0f}, {-50.0f, 0.0f, -45.0f}, {-40.0f, 0.0f, -50.0f},
		{-30.0f, 0.0f, -35.0f}, {-20.0f, 0.0f, -40.0f}, {-10.0f, 0.0f, -45.0f},
		{0.0f, 0.0f, -50.0f},   {10.0f, 0.0f, -40.0f},  {20.0f, 0.0f, -30.0f},
		{30.0f, 0.0f, -35.0f},  {40.0f, 0.0f, -45.0f},  {50.0f, 0.0f, -50.0f},
	};

	glm::vec3 treeScales[50] = {
		{1.0f, 2.0f, 1.0f}, {1.2f, 2.4f, 1.2f}, {0.9f, 1.8f, 0.9f},
		{1.1f, 2.2f, 1.1f}, {0.8f, 1.6f, 0.8f}, {1.3f, 2.6f, 1.3f},
		{1.0f, 2.0f, 1.0f}, {1.4f, 2.8f, 1.4f}, {0.7f, 1.4f, 0.7f},
		{1.2f, 2.4f, 1.2f}, {0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f},
		{1.0f, 2.0f, 1.0f}, {1.5f, 3.0f, 1.5f}, {0.8f, 1.6f, 0.8f},
		{1.3f, 2.6f, 1.3f}, {1.1f, 2.2f, 1.1f}, {1.2f, 2.4f, 1.2f},
		{0.9f, 1.8f, 0.9f}, {1.4f, 2.8f, 1.4f}, {1.0f, 2.0f, 1.0f},
		{1.2f, 2.4f, 1.2f}, {1.3f, 2.6f, 1.3f}, {1.0f, 2.0f, 1.0f},
		{1.1f, 2.2f, 1.1f}, {1.5f, 3.0f, 1.5f}, {0.9f, 1.8f, 0.9f},
		{1.0f, 2.0f, 1.0f}, {1.2f, 2.4f, 1.2f}, {1.1f, 2.2f, 1.1f},
		{1.3f, 2.6f, 1.3f}, {0.8f, 1.6f, 0.8f}, {1.0f, 2.0f, 1.0f},
		{1.2f, 2.4f, 1.2f}, {0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f},
		{1.0f, 2.0f, 1.0f}, {1.3f, 2.6f, 1.3f}, {1.1f, 2.2f, 1.1f},
		{1.2f, 2.4f, 1.2f}, {1.0f, 2.0f, 1.0f}, {1.1f, 2.2f, 1.1f},
		{1.2f, 2.4f, 1.2f}, {1.3f, 2.6f, 1.3f}, {1.0f, 2.0f, 1.0f},
		{1.2f, 2.4f, 1.2f}, {0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f},
	};

	for (size_t i = 0; i < 50; ++i) {
		positionXYZ = treePositions[i];
		scaleXYZ = treeScales[i];
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);  // Green trees
		SetShaderTexture("treeTexture");
		m_basicMeshes->DrawConeMesh();
	}
	// *** Draw 40 Static Filler Trees Spread Over Larger Areas *** //
	glm::vec3 fillerTreePositions[40] = {
		// Left Area (20 Trees)
		{-30.0f, 0.0f, -40.0f}, {-35.0f, 0.0f, -25.0f}, {-25.0f, 0.0f, -35.0f},
		{-40.0f, 0.0f, -30.0f}, {-30.0f, 0.0f, -20.0f}, {-38.0f, 0.0f, -27.0f},
		{-28.0f, 0.0f, -23.0f}, {-33.0f, 0.0f, -32.0f}, {-27.0f, 0.0f, -18.0f},
		{-31.0f, 0.0f, -29.0f}, {-36.0f, 0.0f, -24.0f}, {-26.0f, 0.0f, -22.0f},
		{-39.0f, 0.0f, -26.0f}, {-32.0f, 0.0f, -31.0f}, {-29.0f, 0.0f, -19.0f},
		{-37.0f, 0.0f, -33.0f}, {-34.0f, 0.0f, -21.0f}, {-25.0f, 0.0f, -28.0f},
		{-30.0f, 0.0f, -17.0f}, {-28.0f, 0.0f, -25.0f},

		// Right Area (20 Trees)
		{30.0f, 0.0f, -40.0f}, {35.0f, 0.0f, -25.0f}, {25.0f, 0.0f, -35.0f},
		{40.0f, 0.0f, -30.0f}, {30.0f, 0.0f, -20.0f}, {38.0f, 0.0f, -27.0f},
		{28.0f, 0.0f, -23.0f}, {33.0f, 0.0f, -32.0f}, {27.0f, 0.0f, -18.0f},
		{31.0f, 0.0f, -29.0f}, {36.0f, 0.0f, -24.0f}, {26.0f, 0.0f, -22.0f},
		{39.0f, 0.0f, -26.0f}, {32.0f, 0.0f, -31.0f}, {29.0f, 0.0f, -19.0f},
		{37.0f, 0.0f, -33.0f}, {34.0f, 0.0f, -21.0f}, {25.0f, 0.0f, -28.0f},
		{30.0f, 0.0f, -17.0f}, {28.0f, 0.0f, -25.0f}
	};

	glm::vec3 fillerTreeScales[40] = {
		// Left Area Scales
		{1.2f, 2.4f, 1.2f}, {1.4f, 2.8f, 1.4f}, {1.0f, 2.0f, 1.0f},
		{1.3f, 2.6f, 1.3f}, {0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f},
		{1.0f, 2.0f, 1.0f}, {1.5f, 3.0f, 1.5f}, {1.2f, 2.4f, 1.2f},
		{1.3f, 2.6f, 1.3f}, {1.0f, 2.0f, 1.0f}, {1.2f, 2.4f, 1.2f},
		{0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f}, {1.3f, 2.6f, 1.3f},
		{1.0f, 2.0f, 1.0f}, {1.5f, 3.0f, 1.5f}, {1.2f, 2.4f, 1.2f},
		{1.1f, 2.2f, 1.1f}, {1.0f, 2.0f, 1.0f},

		// Right Area Scales
		{1.2f, 2.4f, 1.2f}, {1.4f, 2.8f, 1.4f}, {1.0f, 2.0f, 1.0f},
		{1.3f, 2.6f, 1.3f}, {0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f},
		{1.0f, 2.0f, 1.0f}, {1.5f, 3.0f, 1.5f}, {1.2f, 2.4f, 1.2f},
		{1.3f, 2.6f, 1.3f}, {1.0f, 2.0f, 1.0f}, {1.2f, 2.4f, 1.2f},
		{0.9f, 1.8f, 0.9f}, {1.1f, 2.2f, 1.1f}, {1.3f, 2.6f, 1.3f},
		{1.0f, 2.0f, 1.0f}, {1.5f, 3.0f, 1.5f}, {1.2f, 2.4f, 1.2f},
		{1.1f, 2.2f, 1.1f}, {1.0f, 2.0f, 1.0f}
	};

	for (size_t i = 0; i < 40; ++i) {
		positionXYZ = fillerTreePositions[i];
		scaleXYZ = fillerTreeScales[i];
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.2f, 0.7f, 0.2f, 1.0f);  // Green trees
		SetShaderTexture("treeTexture");
		m_basicMeshes->DrawConeMesh();
	}

	//*** End of Scene Rendering ***//
}

