///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//Function to load textures into memory.
	bReturn = CreateGLTexture(
		"desktop.jpg",
		"desktop");

	bReturn = CreateGLTexture(
		"tab.png",
		"tab");

	bReturn = CreateGLTexture(
		"table.png",
		"table");

	// afer the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/

void SceneManager::DefineObjectMaterials()
{
	//Makes a non reflective material for the tabletop since it's wooden. 
	OBJECT_MATERIAL tableMaterial;
	tableMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	tableMaterial.ambientStrength = 0.5f;
	tableMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	tableMaterial.specularColor = glm::vec3(0.8f, 0.8f, 1.0f);
	tableMaterial.shininess = 1.0;
	tableMaterial.tag = "table";
	m_objectMaterials.push_back(tableMaterial);

	//Makes a black pastic material for the monitor's black plastic parts.
	OBJECT_MATERIAL blackPlasticMaterial;
	blackPlasticMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	blackPlasticMaterial.ambientStrength = 0.1f;
	blackPlasticMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	blackPlasticMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	blackPlasticMaterial.shininess = 32.0;
	blackPlasticMaterial.tag = "blackPlastic";
	m_objectMaterials.push_back(blackPlasticMaterial);

	//Makes a grey plastic material for the legs and base of the monitor.
	OBJECT_MATERIAL greyPlasticMaterial;
	greyPlasticMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	greyPlasticMaterial.ambientStrength = 0.3f;
	greyPlasticMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	greyPlasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	greyPlasticMaterial.shininess = 32.0;
	greyPlasticMaterial.tag = "greyPlastic";
	m_objectMaterials.push_back(greyPlasticMaterial);

	//Makes a reflective material for the monitor's screen. 
	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	screenMaterial.ambientStrength = 0.8f;
	screenMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.5f);
	screenMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	screenMaterial.shininess = 256.0f;
	screenMaterial.tag = "screen";
	m_objectMaterials.push_back(screenMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	//Configures light source and its params to best suite scene. 
	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f, 8.0f, 10.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 4.0f, 8.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.1f, 0.0f, 0.15f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.0f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);


	//Enables lighting to be used in the scene. 
	m_pShaderManager->setBoolValue("bUseLighting", true);
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
	// load the textures for the 3D scene
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	//Applies a texture
	SetShaderTexture("table");
	//Sets the shader material.
	SetShaderMaterial("table");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	//Deactivate textures so that other objects don't receive wood texture.
	m_pShaderManager->setIntValue("bUseTexture", false);
	/****************************************************************/

	//Student's code starts below: 

	//First draw the base of the monitor stand:

	//Determine size
	glm::vec3 baseCylinderScaleXYZ = glm::vec3(1.0f, .5f, 1.0f);
	float baseCylinderRadius = baseCylinderScaleXYZ.x;
	//Determine rotation
	float baseCylinderXrotationDegrees = 0.0f;
	float baseCylinderYrotationDegrees = 0.0f;
	float baseCylinderZrotationDegrees = 0.0f;
	//Determine position
	glm::vec3 baseCylinderPositionXYZ = glm::vec3(0.0f, 1.0f, 3.0f);
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets shader material. 
	SetShaderMaterial("greyPlastic");
	//Confirm transformations
	SetTransformations(
		baseCylinderScaleXYZ,
		baseCylinderXrotationDegrees,
		baseCylinderYrotationDegrees,
		baseCylinderZrotationDegrees,
		baseCylinderPositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//Create the monitor stand's legs: 

	//Leg 1: 

	//Determine size
	glm::vec3 prong1ScaleXYZ = glm::vec3(.2f, .2f, 2.6f);
	//Determine position
	glm::vec3 prong1PositionXYZ = baseCylinderPositionXYZ;
	prong1PositionXYZ.x -= 1.8f;
	prong1PositionXYZ.z += 1.0f;
	prong1PositionXYZ.y -= 0.3f;
	//Determine rotation
	float prong1XrotationDegrees = 30.0f;
	float prong1YrotationDegrees = -60.0f;
	float prong1ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets shader material. 
	SetShaderMaterial("greyPlastic");

	//Confirm transformations
	SetTransformations(
		prong1ScaleXYZ,
		prong1XrotationDegrees,
		prong1YrotationDegrees,
		prong1ZrotationDegrees,
		prong1PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Leg 2: 

	//Determine size
	glm::vec3 prong2ScaleXYZ = glm::vec3(.2f, .2f, 2.6f);
	//Determine position
	glm::vec3 prong2PositionXYZ = baseCylinderPositionXYZ;
	prong2PositionXYZ.x += 1.8f;
	prong2PositionXYZ.z += 1.0f;
	prong2PositionXYZ.y -= 0.3f;
	//Determine rotation
	float prong2XrotationDegrees = 30.0f;
	float prong2YrotationDegrees = 60.0f;
	float prong2ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets shader material. 
	SetShaderMaterial("greyPlastic");
	//Confirm transformations
	SetTransformations(
		prong2ScaleXYZ,
		prong2XrotationDegrees,
		prong2YrotationDegrees,
		prong2ZrotationDegrees,
		prong2PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Leg 3: 

	//Determine size
	glm::vec3 prong3ScaleXYZ = glm::vec3(.2f, .2f, 1.8f);
	//Determine position
	glm::vec3 prong3PositionXYZ = baseCylinderPositionXYZ;
	prong3PositionXYZ.z -= 1.5;
	prong3PositionXYZ.y -= 0.3f;
	//Determine rotation
	float prong3XrotationDegrees = -45.0f;
	float prong3YrotationDegrees = 0.0f;
	float prong3ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets shader material. 
	SetShaderMaterial("greyPlastic");
	//Confirm transformations
	SetTransformations(
		prong3ScaleXYZ,
		prong3XrotationDegrees,
		prong3YrotationDegrees,
		prong3ZrotationDegrees,
		prong3PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Now draw monitor stand post:

	//Determine size
	glm::vec3 postScaleXYZ = glm::vec3(baseCylinderScaleXYZ.x - .2f, 4.0f, 1.0f);
	//Determine position
	glm::vec3 postPositionXYZ = baseCylinderPositionXYZ;
	postPositionXYZ.y += postScaleXYZ.y / 2;
	//Determine rotation
	float postXrotationDegrees = 0.0f;
	float postYrotationDegrees = 0.0f;
	float postZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.7f, 0.7f, 0.7f, 1.0f);
	//Sets shader material. 
	SetShaderMaterial("greyPlastic");
	//Confirm transformations
	SetTransformations(
		postScaleXYZ,
		postXrotationDegrees,
		postYrotationDegrees,
		postZrotationDegrees,
		postPositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Now draw monitor:

	//Determine size
	glm::vec3 monitorScaleXYZ = glm::vec3(8.0f, 4.0f, 1.0f);
	//Determine position
	glm::vec3 monitorPositionXYZ = postPositionXYZ;
	monitorPositionXYZ.y += postScaleXYZ.y/2;
	monitorPositionXYZ.z += .1f;
	//Determine rotation
	float monitorXrotationDegrees = 0.0f;
	float monitorYrotationDegrees = 0.0f;
	float monitorZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(.2f, .2f, .2f, 1.0f);
	//Sets shader material.
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		monitorScaleXYZ,
		monitorXrotationDegrees,
		monitorYrotationDegrees,
		monitorZrotationDegrees,
		monitorPositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	//Draw screen:

	//Determine size
	glm::vec3 screenScaleXYZ = glm::vec3(3.5f, 0.0f, 1.7f);
	//Determine position
	glm::vec3 screenPositionXYZ = monitorPositionXYZ;
	screenPositionXYZ.z += 0.567f;
	//Determine rotation
	float screenXrotationDegrees = 90.0f;
	float screenYrotationDegrees = 0.0f;
	float screenZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(.9f, .9f, .9f, 1.0f);
	//Set Texture
	//Reactivate textures to apply them to current and future objects. 
	m_pShaderManager->setIntValue("bUseTexture", true);
	SetShaderTexture("desktop");
	//Sets shader material. 
	SetShaderMaterial("screen");
	//Confirm transformations
	SetTransformations(
		screenScaleXYZ,
		screenXrotationDegrees,
		screenYrotationDegrees,
		screenZrotationDegrees,
		screenPositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawPlaneMesh();

	// Determine size for the second texture (scaled down three times smaller)
	glm::vec3 smallScreenScaleXYZ = screenScaleXYZ / 3.0f; // Divide the original size by 3

	// Determine position for the second texture (adjust it to be on top of the first texture)
	glm::vec3 smallScreenPositionXYZ = screenPositionXYZ;
	// Adjust position slightly in the z-axis to layer it on top
	smallScreenPositionXYZ.z += 0.01f; // Slightly closer to the camera to prevent z-fighting
	smallScreenPositionXYZ.x -= 1.7f;
	smallScreenPositionXYZ.y -= 1.0f;

	// Set texture for the smaller texture
	SetShaderTexture("tab");

	//Sets shader material. 
	SetShaderMaterial("screen");

	// Confirm transformations for the smaller texture
	SetTransformations(
		smallScreenScaleXYZ,
		screenXrotationDegrees,
		screenYrotationDegrees,
		screenZrotationDegrees,
		smallScreenPositionXYZ
	);

	// Draw the smaller texture mesh
	m_basicMeshes->DrawPlaneMesh();

	//Now draw keyboard: 

	//Determine size
	glm::vec3 keyboardScaleXYZ = glm::vec3(6.0f, .2f, 1.5f);
	//Determine position
	glm::vec3 keyboardPositionXYZ = glm::vec3(0.0f, 0.0f, 7.0f);
	//Determine rotation
	float keyboardXrotationDegrees = 0.0f;
	float keyboardYrotationDegrees = 0.0f;
	float keyboardZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	m_pShaderManager->setIntValue("bUseTexture", false);
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		keyboardScaleXYZ,
		keyboardXrotationDegrees,
		keyboardYrotationDegrees,
		keyboardZrotationDegrees,
		keyboardPositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawBoxMesh();

	// Determine size of keys
	glm::vec3 keyScaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);

	// Calculate number of keys horizontally and vertically
	int horizontalSpace = static_cast<int>(keyboardScaleXYZ.x / (keyScaleXYZ.x + 0.1f));
	int verticalSpace = static_cast<int>(keyboardScaleXYZ.z / (keyScaleXYZ.z + 0.1f));

	// Draw keys
	for (int v = 0; v < verticalSpace; v++) {
		for (int h = 0; h < horizontalSpace; h++) {
			// Calculate position for each key
			glm::vec3 keyPositionXYZ = keyboardPositionXYZ;
			keyPositionXYZ.y += 0.1f; // Slightly above keyboard
			keyPositionXYZ.x = keyboardPositionXYZ.x - ((keyboardScaleXYZ.x / 2) - .15f) + ((keyScaleXYZ.x + 0.1f) * h);
			keyPositionXYZ.z = keyboardPositionXYZ.z - ((keyboardScaleXYZ.z / 2) - .15f) + ((keyScaleXYZ.z + 0.1f) * v);

			// Apply transformations
			SetTransformations(
				keyScaleXYZ,
				0.0f, 0.0f, 0.0f,
				keyPositionXYZ
			);

			// Draw key
			//SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
			//Sets matertial
			SetShaderMaterial("blackPlastic");
			m_basicMeshes->DrawBoxMesh();
		}
	}

	//Draw mouse. 

	//Determine size
	glm::vec3 mouseScaleXYZ = glm::vec3(0.4f, 0.2f, 0.7f);
	//Determine position
	glm::vec3 mousePositionXYZ = glm::vec3(4.0f, 0.0f, 9.0f);
	//Determine rotation
	float mouseXrotationDegrees = 0.0f;
	float mouseYrotationDegrees = 0.0f;
	float mouseZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		mouseScaleXYZ,
		mouseXrotationDegrees,
		mouseYrotationDegrees,
		mouseZrotationDegrees,
		mousePositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawHalfSphereMesh();

	//Draw the speakers. 

	//Speaker 1:

	//Draw the speaker base:
	//Determine size
	glm::vec3 speaker1ScaleXYZ = glm::vec3(.64f, 1.2f, .5f);
	//Determine position
	glm::vec3 speaker1PositionXYZ = glm::vec3(-4.0f, 0.1f, 6.0f);
	//Determine rotation
	float speaker1XrotationDegrees = 90.0f;
	float speaker1YrotationDegrees = 0.0f;
	float speaker1ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		speaker1ScaleXYZ,
		speaker1XrotationDegrees,
		speaker1YrotationDegrees,
		speaker1ZrotationDegrees,
		speaker1PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawTorusMesh();

	//Draw the speaker body:
	//Determine size
	glm::vec3 speakerBase1ScaleXYZ = glm::vec3(.7f, 3.f, .9f);
	//Determine position
	glm::vec3 speakerBase1PositionXYZ = speaker1PositionXYZ;
	speakerBase1PositionXYZ.y += .07f;
	speakerBase1PositionXYZ.z += .3f;
	//Determine rotation
	float speakerBase1XrotationDegrees = 0.0f;
	float speakerBase1YrotationDegrees = 0.0f;
	float speakerBase1ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		speakerBase1ScaleXYZ,
		speakerBase1XrotationDegrees,
		speakerBase1YrotationDegrees,
		speakerBase1ZrotationDegrees,
		speakerBase1PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();

	//Speaker 2:

	//Draw the speaker base:
	//Determine size
	glm::vec3 speaker2ScaleXYZ = glm::vec3(.64f, 1.2f, .5f);
	//Determine position
	glm::vec3 speaker2PositionXYZ = glm::vec3(4.0f, 0.1f, 6.0f);
	//Determine rotation
	float speaker2XrotationDegrees = 90.0f;
	float speaker2YrotationDegrees = 0.0f;
	float speaker2ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		speaker2ScaleXYZ,
		speaker2XrotationDegrees,
		speaker2YrotationDegrees,
		speaker2ZrotationDegrees,
		speaker2PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawTorusMesh();

	//Draw the speaker body:
	//Determine size
	glm::vec3 speakerBase2ScaleXYZ = glm::vec3(.7f, 3.f, .9f);
	//Determine position
	glm::vec3 speakerBase2PositionXYZ = speaker2PositionXYZ;
	speakerBase2PositionXYZ.y += .07f;
	speakerBase2PositionXYZ.z += .3f;
	//Determine rotation
	float speakerBase2XrotationDegrees = 0.0f;
	float speakerBase2YrotationDegrees = 0.0f;
	float speakerBase2ZrotationDegrees = 0.0f;
	//Determine color: grey
	//SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	//Sets matertial
	SetShaderMaterial("blackPlastic");
	//Confirm transformations
	SetTransformations(
		speakerBase2ScaleXYZ,
		speakerBase2XrotationDegrees,
		speakerBase2YrotationDegrees,
		speakerBase2ZrotationDegrees,
		speakerBase2PositionXYZ);
	//Draw the mesh
	m_basicMeshes->DrawCylinderMesh();
}
