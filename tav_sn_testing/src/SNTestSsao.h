//
// SNTestSsao.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <GLUtils/TextureManager.h>
#include <GLUtils/Vertex.h>
#include <GLUtils/Geometry.h>
//#include "GLUtils/CameraControl.h"
#include <GLUtils/MersenneTwister.h>
#include "math_utils.h"
#include <Shaders/ShaderBuffer.h>
#include <GLUtils/Noise3DTexGen.h>
#include <GeoPrimitives/Quad.h>

#include <glm/ext.hpp>
#include <glm/gtx/color_space.hpp>

#include <GLUtils/SSAO.h>

namespace tav
{

class SNTestSsao : public SceneNode
{
public:
	static const int gridX = 3;
	static const int gridY = 3;
	static const int gridZ = 3;

	SNTestSsao(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestSsao();

	void init(TFO* _tfo = nullptr);
	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);

	void initShaders();
	void initSceneShdr();
	bool initScene();

	std::string initComputeUpdtShdr();
	GLuint createShaderPipelineProgram(GLuint target, const char* src);

	void updateOffsPos(double time);
	void startThreads(double time, double dt);
	void stopThreads(double time, double dt);
	void cleanUp();
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:
	SSAO*				ssao;
	TextureManager* 	litsphereTex;
	Projection 			projection;

	glm::uint  			sceneTriangleIndices;
	glm::uint  			sceneObjects;

    Noise3DTexGen*		noiseTex;
    Quad*				quad;

	ShaderBuffer<glm::vec4>* modu_pos;
	ShaderBuffer<glm::vec4>* m_vel;
	ShaderBuffer<glm::vec4>* ref_pos;

	Shaders*			draw_scene;
	Shaders*			texShdr;
    ShaderCollector*	shCol;

	bool                inited = false;

	std::string			vertShdr;
	std::string 		basicVert;
	std::string 		shdr_Header;

	GLuint				defaultVAO;
    GLuint 				m_programPipeline;
    GLuint 				m_updateProg;
    GLuint 				scene_vbo;
    GLuint 				scene_ibo;
    GLuint 				scene_ubo;
    GLuint 				hbao_ubo;

    GLint 				lastBoundFbo;

	unsigned int		sphereIndOffs;

	float				propo;
	double				lastUpdt;

	float				spSize;
	float				perlForce;
	float				randAmt;
	float				posRandSpd;
	float				posRandAmtX;
	float				posRandAmtY;
};
}
