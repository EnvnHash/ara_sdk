//
//  SNTerrain.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  needs a height field, size and proportion doesn´t matter
//  texture should be tileable
//
//  init:
//  - generate a field of points depending on the size of the texture
//  each field represents 128 x 128 pixels of texture
//
// first pass:
//  - apply a transformation matrix on that field
//  - via a geometry shader subdivide the fields depending on DETAIL_LEVEL_FIRST_PASS
//  (e.g. DETAIL_LEVEL_FIRST_PASS=2 means (2^2 +1) ^2 points per Field)
//  - skip all points that are outside the field of view
//  - via modulo assure that all coordinates lie within the unit square
//
//  second pass:
//  - via tesselation shader subdivide the patches received from the output of the first pass
//  depending on the distance to the camera
//  - via geometry shader read from the height and normal map and generate normals and
//  extruded vertices
//  - also generate texture coordinates, since all coordinates from the input of the geometry
//  shader lie within the unit square the texture coordinates will be generated from the positions
//  and be multiplied by the size of the height/normalmap texture
//  - the same rotation matrix that was applied to the positions in the first pass will be applied
//  to read position of the normal/heightmap and the texture coordinate
//  depending on the resulting height at the point of the camera an minimum height will be set
//
//
//


#include "SNTerrain.h"

using namespace glm;

namespace tav
{
SNTerrain::SNTerrain(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
#ifdef HAVE_NITE2
	heightScale = 5.f;
	terrainScaleFact = 40.f;

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// nis = kin->getNis();

	g_width = scd->screenWidth;
	g_height = scd->screenHeight;

	// hier helligkeit steuern
	lightDirection = normalize(glm::vec3(0.5f, 1.f, 0.5f));

	// load Textures
	textures = new TextureManager*[3];
	for (auto i = 0; i < 3; i++)
		textures[i] = new TextureManager();

	textures[0]->loadTextureRect(
			(*scd->dataPath) + "textures/t0_heightmap.tga");
	textures[1]->loadTexture2D((*scd->dataPath) + "textures/texture_land.jpg");
	textures[2]->loadTextureRect((*scd->dataPath) + "textures/t0_normmap.tga");

	skyBox = new SkyBox((*scd->dataPath) + "textures/skyboxsun5deg2.png", 1);

	// get the size of the height map texture
	g_sMapExtend = textures[0]->getWidthF();   // heightMap width
	g_tMapExtend = textures[0]->getHeightF();   // heightMap height

	// divide the texture in fields of 128 x 128 pixels, round down
	g_sNumPoints = sMaxDetailLevel = static_cast<GLuint>(ceilf(
			g_sMapExtend / 128.f));
	g_tNumPoints = tMaxDetailLevel = static_cast<GLuint>(ceilf(
			g_tMapExtend / 128.f));
	overallMaxDetailLevel = std::min(sMaxDetailLevel, tMaxDetailLevel);

//        std::cout << "sMaxDetailLevel: " << sMaxDetailLevel << std::endl;
//        std::cout << "tMaxDetailLevel: " << tMaxDetailLevel << std::endl;
//        std::cout << "overallMaxDetailLevel: " << overallMaxDetailLevel << std::endl;

	// Do checking of calculated parameters
	if (MINIMUM_DETAIL_LEVEL > overallMaxDetailLevel)
		printf("Detail level to high %d > %d\n", MINIMUM_DETAIL_LEVEL,
				overallMaxDetailLevel);

	if (MINIMUM_DETAIL_LEVEL + DETAIL_LEVEL_FIRST_PASS > overallMaxDetailLevel)
		printf("First pass detail level to high %d > %d\n",
				MINIMUM_DETAIL_LEVEL + DETAIL_LEVEL_FIRST_PASS,
				overallMaxDetailLevel);

	// calculate the width and height of one Texturefield
	detailStepS = 1.f / (sMaxDetailLevel - 1);
	detailStepT = 1.f / (tMaxDetailLevel - 1);

//        std::cout << "detailStepS: " << detailStepS << std::endl;
//        std::cout << "detailStepT: " << detailStepT << std::endl;

	// Generate the flat terrain mesh. one base point (left lower) of each texture field
	map = (GLfloat*) malloc(g_sNumPoints * g_tNumPoints * 2 * sizeof(GLfloat));
	indices = (GLuint*) malloc(g_sNumPoints * g_tNumPoints * sizeof(GLuint));

	GLfloat f_sNumPoints = (GLfloat) (g_sNumPoints - 1);
	GLfloat f_tNumPoints = (GLfloat) (g_tNumPoints - 1);

	for (auto t = 0; t < g_tNumPoints; t++)
	{
		for (auto s = 0; s < g_sNumPoints; s++)
		{
			map[t * g_sNumPoints * 2 + s * 2 + 0] = (GLfloat) s / f_sNumPoints
					- 0.5f;
			map[t * g_sNumPoints * 2 + s * 2 + 1] = (GLfloat) t / f_tNumPoints
					- 0.5f;
			//cout << map[t * g_sNumPoints *2 +s *2 +0] << ":" << map[t * g_sNumPoints *2 +s *2 +1] << std::endl;
			indices[t * g_sNumPoints + s + 0] = t * g_sNumPoints + s;
		}
	}

	// Pass one
	passOne = new Shaders("shaders/TerrainPassOne.vert.glsl",
			"shaders/TerrainPassOne.geom.glsl",
			"shaders/TerrainPassOne.frag.glsl", true);

	// add the transform variable ...
	glTransformFeedbackVaryings(passOne->getProgram(), 1,
			(const GLchar**) &TRANSFORM_VARYING, GL_SEPARATE_ATTRIBS);
	passOne->link();

	passTwo = new Shaders();
	passTwo->addVertSrc("shaders/TerrainPassTwo.vert.glsl", true);
	passTwo->addContrSrc("shaders/TerrainPassTwo.cont.glsl", true);
	passTwo->addEvalSrc("shaders/TerrainPassTwo.eval.glsl", true);
	passTwo->addGeomSrc("shaders/TerrainPassTwo.geom.glsl", true);
	passTwo->addFragSrc("shaders/TerrainPassTwo.frag.glsl", true);
	passTwo->create();

	// add the transform variable for reading the height
//        glTransformFeedbackVaryings(passTwo->getProgram(), 1,
//                                    (const GLchar**) &HEIGHT_VARYING,
//                                    GL_SEPARATE_ATTRIBS);

	passTwo->link();

	testShader = new Shaders("shaders/basic_col.vert", "shaders/basic_col.frag",
			true);
	testShader->link();

	// One time matrix calculations to convert between texture and world space
	g_textureToWorldMatrix = glm::mat4();       // identity matrix
	textureToWorldNormalMatrix = glm::mat4();   // identity matrix

	// Skip this scale for the normal matrix

	g_textureToWorldMatrix = glm::scale(g_textureToWorldMatrix,
			glm::vec3(1.0f, 1.0f, -1.0f));
	textureToWorldNormalMatrix = glm::scale(textureToWorldNormalMatrix,
			glm::vec3(1.0f, 1.0f, -1.0f));

	// No need for the translation matrix in the normal matrix

	g_worldToTextureMatrix = g_textureToWorldMatrix;
	g_worldToTextureMatrix = inverse(g_worldToTextureMatrix);

	g_worldToTextureNormalMatrix = textureToWorldNormalMatrix;
	g_worldToTextureNormalMatrix = inverse(g_worldToTextureNormalMatrix);

	camPosTextureSpace = glm::vec4(0.f, 0.f, 0.f, 1.f);
	camDirTextureSpace = glm::vec4(0.f, 0.f, -1.f, 0.f);
	camTextureFOV = 80.0f;

	// generate vaos
	// pass one
	vaoOne = new VAO("position:2f", GL_DYNAMIC_DRAW);
	vaoOne->initData(g_sNumPoints * g_tNumPoints * 2, map);
	vaoOne->setElemIndices(g_sNumPoints * g_tNumPoints, indices);

	// pass two
	vaoTwo = new VAO("position:2f", GL_DYNAMIC_DRAW);
	vaoTwo->initData(
			g_sNumPoints * g_tNumPoints
					* (GLuint) std::pow(4, DETAIL_LEVEL_FIRST_PASS + 1) * 4);

	// for reading the height at the actual position
//        vaoThree = new VAO("position:1f", GL_DYNAMIC_DRAW);
//        vaoThree->initData(g_sNumPoints * g_tNumPoints * (GLuint) pow(4, DETAIL_LEVEL_FIRST_PASS + 1) * 4);

	glPatchParameteri(GL_PATCH_VERTICES, 4);

	// movement
	movIncrMatr = glm::mat4();
	invMovementMatr = glm::mat4();

	movRotMatr = glm::mat4();
	invMovRotMatr = glm::mat4();

	movDir = glm::vec3(0.f, 0.f, 1.f);
	movSpeed = 0.001f;

	actHeight = 0.f;

	QUADRANT_STEP = 2;
	maxTessLevel = 2;
#endif
}

//---------------------------------------------------------------

SNTerrain::~SNTerrain()
{
#ifdef HAVE_NITE2
	delete passOne;
	delete passTwo;
	delete vaoOne;
	delete vaoTwo;
	// delete vaoThree;
	free(map);
	free(indices);
	delete testShader;
	delete skyBox;
#endif
}

//---------------------------------------------------------------

void SNTerrain::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
#ifdef HAVE_NITE2

	// draw a skybox
	//  mat4 finInvRot = rotate(invMovRotMatr, rotateX, glm::vec3(1.f, 0.f, 0.f));
	skyBox->setMatr(&invMovRotMatr);
	skyBox->draw(time, cp, _shader);
	/*
	 // get ni input
	 if (kin->isNisInited())
	 {
	 if (frameNr != kin->getNisFrameNr())
	 {
	 frameNr = kin->getNisFrameNr();

	 //kin->lockDepthMtx();
	 
	 short usr = nis->getNearestUser();
	 if (usr != -1)
	 {
	 leftH = nis->getJoint(usr, nite::JOINT_LEFT_HAND);
	 rightH = nis->getJoint(usr, nite::JOINT_RIGHT_HAND);
	 upH = nis->getJoint(usr, nite::JOINT_LEFT_SHOULDER );
	 lowerH = nis->getJoint(usr, nite::JOINT_LEFT_HIP );
	 
	 // speed
	 movSpeed = (
	 movSpeed * 10.f
	 + (
	 std::pow( std::fmax( std::fabs( leftH.x - rightH.x ) - 0.5f, 0.f ), 2.f )
	 * -0.01f
	 )
	 ) / 11.f;
	 //                    std::cout << "dist hand raw: " << leftH.x - rightH.x << " moveSpeed: " << movSpeed << std::endl;
	 //                    std::cout << "dist hand: " << std::fmax( std::fabs( leftH.x - rightH.x ) - 0.5f, 0.f ) << " moveSpeed: " << movSpeed << std::endl;
	 
	 // rotation X-Axis
	 float zDiff = ((upH.z - lowerH.z) - 0.008f) * 5.f;

	 zDiffRaiseAmt = (zDiffRaiseAmt * 7.f + zDiff * 0.8f) / 8.f;
	 actHeight = std::fmax( std::fmin( actHeight + zDiffRaiseAmt, 1.f ), 0.f );
	 
	 rotateX = (rotateX * 10.f + zDiff * 200.f) / 11.f;
	 rotateX = std::fmax( std::fmin(rotateX, 20.f), -20.f);

	 // rotation Y axis
	 float newAngle = std::atan2(leftH.y - rightH.y, leftH.x - rightH.x);
	 
	 if (newAngle != 0.f)
	 {
	 float cutLowerValues = 0.5f;
	 if (newAngle >= 0.f) {
	 newAngle = std::fmax(M_PI - newAngle, cutLowerValues) - cutLowerValues;
	 } else {
	 newAngle = std::fmin(-M_PI - newAngle, -cutLowerValues) + cutLowerValues;
	 }
	 
	 newAngle *= -10.f;
	 
	 angle = (angle * 20.f + newAngle) / 21.f;
	 
	 movRotMatr = glm::rotate(movRotMatr, angle, glm::vec3(0.f, 1.f, 0.f));
	 movDir = glm::rotateY(movDir, -angle);
	 }
	 }
	 
	 //kin->unLockDepthMtx();
	 }
	 }
	 */

	updateAnimation(dt, cp);

	// -
	// Pass one.

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glDisable (GL_BLEND);
	glBlendFunc(GL_ONE, GL_ZERO); // ohne dass kein antialiasing

	//glFrontFace(GL_CCW);        // counter clockwise definition means front, as default
	//glDepthMask(GL_TRUE);

	if (showWire)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Disable any rasterization
	glEnable (GL_RASTERIZER_DISCARD);

	// pass one render the base points, drop all points that are not visible
	// subdivide the field depending on the DETAIL_LEVEL_FIRST_PASS
	// (initially 2) means for each base point there will be (2^2 +1) ^2 points
	// later forming a grid of triangles
	passOne->begin();

	passOne->setUniform1f("u_halfDetailStepS", detailStepS * 0.5f);
	passOne->setUniform1f("u_halfDetailStepT", detailStepT * 0.5f);

	passOne->setUniform1ui("u_detailLevel", DETAIL_LEVEL_FIRST_PASS);
	passOne->setUniform1f("u_fovRadius", FOV_RADIUS);

	passOne->setUniformMatrix4fv("u_movMatr", (GLfloat*) &movementMatr[0][0],
			1);
	passOne->setUniformMatrix4fv("u_rotMatr", (GLfloat*) &finMovRot[0][0], 1);

//        std::cout << "mov mat: " << glm::to_string(movementMatr) << std::endl;
//        std::cout << glm::to_string(positionTextureSpace) << std::endl;
//        std::cout << "left normal: " << glm::to_string(leftNormalTextureSpace) << std::endl;
//        std::cout << "right normal: " << glm::to_string(rightNormalTextureSpace) << std::endl;
//        std::cout << "back normal: " << glm::to_string(backNormalTextureSpace) << std::endl;

	passOne->setUniform4fv("u_positionTextureSpace",
			(GLfloat*) &positionTextureSpace[0], 1);
	passOne->setUniform3fv("u_leftNormalTextureSpace",
			(GLfloat*) &leftNormalTextureSpace[0], 1);
	passOne->setUniform3fv("u_rightNormalTextureSpace",
			(GLfloat*) &rightNormalTextureSpace[0], 1);
	passOne->setUniform3fv("u_backNormalTextureSpace",
			(GLfloat*) &backNormalTextureSpace[0], 1);

	// Bind to vertices used in render pass two. To this buffer is written.
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vaoTwo->getVBO(POSITION));

	// We need to know, how many primitives are written. So start the query.
	if (g_transformFeedbackQuery == 0)
		glGenQueries(1, &g_transformFeedbackQuery);

//        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_transformFeedbackQuery);

	// Start the operation ...
	glBeginTransformFeedback (GL_POINTS);

	// ... render the elements ...
	vaoOne->drawElements(GL_POINTS, nullptr, GL_TRIANGLES);

	// ... and stop the operation.
	glEndTransformFeedback();

	// Now, we can also stop the query.
	//  glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	glDisable(GL_RASTERIZER_DISCARD);

	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

	passOne->end();

	// Now get the number of primitives written in the first render pass.
	// glGetQueryObjectuiv(g_transformFeedbackQuery, GL_QUERY_RESULT, &primitivesWritten);
	// std::cout << primitivesWritten << std::endl;

	// -

	// Pass two
	passTwo->begin();

	passTwo->setUniform3fv("u_lightDirection", (GLfloat*) &lightDirection[0],
			1);

	passTwo->setUniform1ui("u_maxTessellationLevel", maxTessLevel);

//        passTwo->setUniform1ui("u_maxTessellationLevel", overallMaxDetailLevel - (MINIMUM_DETAIL_LEVEL + DETAIL_LEVEL_FIRST_PASS) -10);
//        passTwo->setUniform1ui("u_maxTessellationLevelS", sMaxDetailLevel);
//        passTwo->setUniform1ui("u_maxTessellationLevelT", tMaxDetailLevel);
	passTwo->setUniform1i("u_quadrantStep", QUADRANT_STEP);
//        passTwo->setUniform1f("u_quadrantStep", QUADRANT_STEP);

	passTwo->setUniform1f("u_detailStepS", detailStepS);
	passTwo->setUniform1f("u_detailStepT", detailStepT);

	float sSize[2] =
	{ static_cast<float>(scd->screenWidth),
			static_cast<float>(scd->screenHeight) };

	passTwo->setUniform2fv("screen_size", sSize);

	// camera matrix
	passTwo->setUniformMatrix4fv("u_tmvpMatrix", (GLfloat*) &tmvpMatrix[0][0],
			1);

	// movement & rotation matrix
	passTwo->setUniformMatrix4fv("u_movMatr", (GLfloat*) &invMovementMatr[0][0],
			1);
	passTwo->setUniformMatrix4fv("u_rotMatr", (GLfloat*) &invMovRotMatr[0][0],
			1);

	// position of the camera
	passTwo->setUniform4fv("u_positionTextureSpace",
			(GLfloat*) &positionTextureSpace[0], 1);

	passTwo->setUniform1i("u_heightMapTexture", 0);
	textures[0]->bind(0);

	passTwo->setUniform1i("u_colorMapTexture", 1);
	textures[1]->bind(1);

	passTwo->setUniform1i("u_normalMapTexture", 2);
	textures[2]->bind(2);

	/*
	 testShader->begin();
	 testShader->setUniformMatrix4fv("m_pvm", cp->mvp, 1);
	 glPointSize(2.f);
	 vaoTwo->draw(GL_POINTS, 0, primitivesWritten, nullptr);
	 testShader->end();
	 */

//        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vaoThree->getVBO(POSITION));
//        glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_transformFeedbackQuery);
//        glBeginTransformFeedback(GL_TRIANGLES);
	primitivesWritten = 5500;

	glPatchParameteri(GL_PATCH_VERTICES, 4);

	vaoTwo->draw(GL_PATCHES, 0, primitivesWritten, nullptr, GL_PATCHES);

//        glEndTransformFeedback();
//        glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
//        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

//        glGetQueryObjectuiv(g_transformFeedbackQuery, GL_QUERY_RESULT, &heightWritten);
	//cout << heightWritten << std::endl;

	/*
	 GLfloat* pos = (GLfloat*) vaoThree->getMapBuffer(POSITION);
	 
	 // look for an entry other than 0
	 int i=0; bool found = false;
	 while (i<heightWritten && !found)
	 {
	 found = pos[i] != 0.f;
	 i++;
	 }
	 actHeight = pos[i];
	 vaoThree->unMapBuffer();
	 */
#endif

}

//---------------------------------------------------------------
#ifdef HAVE_NITE2

void SNTerrain::updateAnimation(double time, camPar* cp)
{

//        movRotMatr = glm::rotate(movRotMatr, -2.f, glm::vec3(0.f, 1.f, 0.f));
//        movDir = glm::rotateY(movDir, 2.f);

	movIncrMatr = glm::translate(movIncrMatr, movDir * movSpeed);

	movementMatr = glm::mat4();
	movementMatr = movIncrMatr * movementMatr;

	finMovRot = glm::rotate(movRotMatr, rotateX, glm::vec3(1.f, 0.f, 0.f));

	invMovementMatr = inverse(movementMatr);
	invMovRotMatr = inverse(finMovRot);

	// get the mvp matrix vom the actual camera settings
	tmvpMatrix = cp->mvp_mat4 * g_textureToWorldMatrix;

	// scale and zoom into
	// height of heightmap interpretation -> dirty!!!
	glm::vec3 scaleModel = glm::vec3(terrainScaleFact, heightScale,
			terrainScaleFact);
	tmvpMatrix = glm::rotate(tmvpMatrix, rotateX, glm::vec3(1.f, 0.f, 0.f));
	tmvpMatrix = glm::translate(tmvpMatrix,
			glm::vec3(0.f, heightScale * (actHeight * -1.f + -0.5f), 0.f));
	tmvpMatrix = glm::scale(tmvpMatrix, scaleModel);

	// Position
	positionTextureSpace = g_worldToTextureMatrix * camPosTextureSpace;

	// Left normal of field of view
	rotationMatrix = glm::mat4();
	rotationMatrix = glm::rotate(rotationMatrix,
			camTextureFOV * (g_width / g_height) / 2.0f + 90.0f,
			glm::vec3(0.f, 1.f, 0.f));

	leftNormalTextureSpace = glm::vec3(rotationMatrix * camDirTextureSpace);
	leftNormalTextureSpace = glm::vec3(
			g_worldToTextureNormalMatrix
					* glm::vec4(leftNormalTextureSpace.x,
							leftNormalTextureSpace.y, leftNormalTextureSpace.z,
							0.f));

	// Right normal of field of view
	rotationMatrix = glm::mat4();
	rotationMatrix = glm::rotate(rotationMatrix,
			-camTextureFOV * (g_width / g_height) / 2.0f - 90.0f,
			glm::vec3(0.f, 1.f, 0.f));
	rightNormalTextureSpace = glm::vec3(rotationMatrix * camDirTextureSpace);
	rightNormalTextureSpace = glm::vec3(
			g_worldToTextureNormalMatrix
					* glm::vec4(rightNormalTextureSpace.x,
							rightNormalTextureSpace.y,
							rightNormalTextureSpace.z, 0.f));

	// Back normal of field of view
	rotationMatrix = glm::mat4();
	rotationMatrix = glm::rotate(rotationMatrix, 180.0f,
			glm::vec3(0.f, 1.f, 0.f));
	backNormalTextureSpace = glm::vec3(rotationMatrix * camDirTextureSpace);
	backNormalTextureSpace = glm::vec3(
			g_worldToTextureNormalMatrix
					* glm::vec4(backNormalTextureSpace.x,
							backNormalTextureSpace.y, backNormalTextureSpace.z,
							0.f));
}
#endif
//---------------------------------------------------------------

void SNTerrain::update(double time, double dt)
{
}

//---------------------------------------------------------------

void SNTerrain::onKey(int key, int scancode, int action, int mods)
{
#ifdef HAVE_NITE2

	if (action == GLFW_PRESS || action == GLFW_REPEAT)
	{
		switch (key)
		{
		case GLFW_KEY_RIGHT:
			movRotMatr = glm::rotate(movRotMatr, 1.f, glm::vec3(0.f, 1.f, 0.f));
			movDir = glm::rotateY(movDir, -1.f);
			std::cout << glm::to_string(movDir) << std::endl;
			break;
		case GLFW_KEY_LEFT:
			movRotMatr = glm::rotate(movRotMatr, -1.f,
					glm::vec3(0.f, 1.f, 0.f));
			movDir = glm::rotateY(movDir, 1.f);

			//movRotMatr = glm:mat4(RotationBetweenVectors(movDir, glm::vec3(0.f, 0.f, 1.f)));
			std::cout << glm::to_string(movDir) << std::endl;
			break;
		case GLFW_KEY_UP:
			QUADRANT_STEP += 1;
			printf("QUADRANT_STEP: %d\n", QUADRANT_STEP);
			break;
		case GLFW_KEY_DOWN:
			QUADRANT_STEP -= 1;
			printf("QUADRANT_STEP: %d\n", QUADRANT_STEP);
			break;
		case GLFW_KEY_O:
			maxTessLevel += 1;
			printf("maxTessLevel: %d\n", maxTessLevel);
			break;
		case GLFW_KEY_L:
			maxTessLevel -= 1;
			printf("maxTessLevel: %d\n", maxTessLevel);
			break;
		case GLFW_KEY_W:
			showWire = !showWire;
			break;
		case GLFW_KEY_I:
			dsScale -= 0.01f;
			printf("dsScale: %f\n", dsScale);
			break;
		case GLFW_KEY_K:
			dsScale += 0.01f;
			printf("dsScale: %f\n", dsScale);
			break;
		}
	}
#endif
}
}

