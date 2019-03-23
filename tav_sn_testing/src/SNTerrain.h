//
// SNTerrain.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once


#include <iostream>
#include <cmath>
#include <glm/gtx/rotate_vector.hpp>

#include <GeoPrimitives/Quad.h>
#include <GLUtils/GLMCamera.h>
#include <GLUtils/SkyBox.h>
#include <GLUtils/VAO.h>

#ifdef HAVE_NITE2
#include <NiTE/NISkeleton.h>
#endif

#include <Shaders/Shaders.h>

#ifdef WIDTH_AUDIO
#include <PAudio.h>
#endif

#include <SceneNode.h>

namespace tav
{
class SNTerrain: public SceneNode
{
public:
	SNTerrain(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTerrain();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo =
			nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods);

#ifdef HAVE_NITE2
	void updateAnimation(double time, camPar* cp);

private:
	PAudio* pa;
	ShaderCollector* shCol;
	Shaders* passOne;
	Shaders* passTwo;
	Shaders* testShader;
	VAO* vaoOne;
	VAO* vaoTwo;
	VAO* vaoThree;

	SkyBox* skyBox;

	NISkeleton* nis;

	TextureManager** textures;

	// gl_Position will be transformed to the buffer
	const char* TRANSFORM_VARYING = "v_vertex";
	const char* HEIGHT_VARYING = "actHeight";

	// Map metrics etc.

	// The space when traveling horizontal form one pixel to the next in meters.
	//const GLfloat       HORIZONTAL_PIXEL_SPACING = 60.0f;
	const GLfloat HORIZONTAL_PIXEL_SPACING = 60.0f;
	// The space when traveling all color pixel in meters.
	//const GLfloat       VERTICAL_PIXEL_RANGE = 10004.0f;
	const GLfloat VERTICAL_PIXEL_RANGE = 10004.0f;
	// The scale to convert the real world meters in virtual world scale.
	//const GLfloat       METERS_TO_VIRTUAL_WORLD_SCALE = 1.0f;
	const GLfloat METERS_TO_VIRTUAL_WORLD_SCALE = 5.0f;
	// Detail level at the beginning of the map.
	const GLuint MINIMUM_DETAIL_LEVEL = 4;
	// Additional detail level in the first pass. Adjust in GeometryPassOne max_vertices = 4^(firstPassDetailLevel+1)
	const GLuint DETAIL_LEVEL_FIRST_PASS = 1;
	// FOV radius
	const GLfloat FOV_RADIUS = 1.41f;
//        const GLfloat       FOV_RADIUS = 10000.0f;
	// Number of quadrants when going to the next detail level.
	// hoher quadrant step und hoher maxTessLevel => das detaillevel fällt schnell ab
	GLint QUADRANT_STEP = 2;
	GLuint maxTessLevel = 4; // experimentell gefundener wert

	// The extend of the map in s coordinate direction. This is basically the width of the texture.
	GLfloat g_sMapExtend;
	// The extend of the map in t coordinate direction. Basically the height.
	GLfloat g_tMapExtend;
	// Number of points in s direction.
	GLuint g_sNumPoints;
	// Number of points in t direction.
	GLuint g_tNumPoints;

	glm::mat4 g_textureToWorldMatrix;
	glm::mat4 g_worldToTextureMatrix;
	glm::mat4 g_worldToTextureNormalMatrix;
	glm::mat4 textureToWorldNormalMatrix;

	// Variable to query how much primitives were written.
	GLuint g_transformFeedbackQuery = 0;

	GLfloat g_width = 0;
	GLfloat g_height = 0;

	// The maximum detail level which is 2^s = sMapExtend.
	GLuint sMaxDetailLevel = 0;
	// The maximum detail level which is 2^t = tMapExtend.
	GLuint tMaxDetailLevel = 0;
	// The overall maximum detail level from s and t.
	GLuint overallMaxDetailLevel = 0;
	// Step for s and t direction.
	GLfloat detailStepS = 0;
	GLfloat detailStepT = 0;
	GLfloat* map = 0;
	GLuint* indices = 0;

	glm::vec3 lightDirection;

	// Field of view
	glm::mat4 rotationMatrix;
	glm::mat4 tmvpMatrix;

	glm::vec4 positionTextureSpace;
	glm::vec3 directionTextureSpace;
	glm::vec3 leftNormalTextureSpace;
	glm::vec3 rightNormalTextureSpace;
	glm::vec3 backNormalTextureSpace;

	glm::vec4 camPosTextureSpace;
	glm::vec4 camDirTextureSpace;
	float camTextureFOV;

	GLuint primitivesWritten = 0;
	GLuint heightWritten = 0;

	GLfloat heightScale = 1.f;
	float terrainScaleFact;

	glm::mat4 movIncrMatr;
	glm::mat4 movementMatr;
	glm::mat4 invMovementMatr;
	glm::mat4 movRotMatr;
	glm::mat4 invMovRotMatr;
	glm::mat4 finMovRot;

	float movSpeed = 0.005f;
	glm::vec3 movDir;
	float actHeight = 0.f;
	float move = 0.f;

	int frameNr = -1;

	glm::vec3 leftH;
	glm::vec3 rightH;

	glm::vec3 upH;
	glm::vec3 lowerH;

	float angle = 0.f;
	float rotateX = 0.f;
	float zDiffRaiseAmt = 0.f;

	float dsScale = 1.0f;
	bool showWire = false;
#endif

};
}
