//
//  tav_types.h
//  tav_gl4
//
//  Created by Sven Hahne on 07.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "headers/gl_header.h"
#include "headers/global_vars.h"
#include "GLUtils/TextureManager.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
enum coordType
{
	POSITION = 0,
	NORMAL = 1,
	TEXCOORD = 2,
	COLOR = 3,
	TEXCORMOD = 4,
	VELOCITY = 5,
	AUX0 = 6,
	AUX1 = 7,
	AUX2 = 8,
	AUX3 = 9,
	MODMATR = 10,
	COORDTYPE_COUNT = 11
};

extern std::vector<std::string> stdAttribNames;
extern std::vector<std::string> stdRecAttribNames;
extern std::vector<std::string> stdMatrixNames;
extern std::vector<int> coTypeStdSize;
extern std::vector<int> coTypeFragSize;
extern std::vector<int> recCoTypeFragSize;

enum texCordGenType
{
	PLANE_PROJECTION = 0
};
enum shaderType
{
	VERTEX = 0, GEOMETRY = 1, FRAGMENT = 2, SHTP_COUNT = 3
};

typedef struct
{
	glm::vec3 p0;
	glm::vec3 p1;
	float overl = 0.f;
	float dist = 1.f;
	int srcCamId = 0;
	int ctxId = 0;
	unsigned int id = 0;

	glm::vec2 lowLeft;	// in pixel bezieht sich GLFW Context
	glm::vec2 lowRight;
	glm::vec2 upRight;
	glm::vec2 upLeft;
	glm::vec2 texOffs;		// bezieht sich auf den Fbo
	glm::vec2 texSize;

	float lowBlend;
	float rightBlend;
	float upBlend;
	float leftBlend;

	glm::vec2 fTexOffs;		// bezieht sich auf den Fbo
	glm::vec2 fTexSize;		// bezieht sich auf den Fbo

	glm::mat4 deDist;
	bool update;

	glm::vec2 size;
	glm::vec2 offs;
	glm::vec2 cornerSize;
	glm::mat4 perspMat;
	glm::mat4 invPerspMat;
	glm::vec4 texMod;
	glm::vec2 corners[3][4];
} fboView;

typedef struct
{
	int unitNr;
	int texNr;
	GLenum target;
	std::string name;
} auxTexPar;

typedef struct
{
	float* model_matrix;
	glm::mat4 model_matrix_mat4;
	float* view_matrix;
	glm::mat4 view_matrix_mat4;
	float* projection_matrix;
	glm::mat4 projection_matrix_mat4;
	float* normal_matrix;
	glm::mat3 normal_matrix_mat3;
	float* mvp;
	glm::mat4 mvp_mat4;
	glm::mat4* multicam_mvp_mat4 = 0;
	glm::mat4* multicam_proj_mat4 = 0;
	glm::mat4* multicam_modmat_src = 0;
	float* multicam_model_matrix;
	float* multicam_view_matrix;
	float* multicam_projection_matrix;
	float* multicam_normal_matrix;
	glm::vec3 viewerVec;
	int nrCams;
	int camId = 0;
	int activeMultiCam = 0;
	glm::vec2 actFboSize;
	glm::vec3* roomDimen;
	bool usesFbo = false;
	float near = 1.f;
	float far = 100.f;
	float fov = 0.4f;
} camPar;

typedef struct
{
	int scrWidth;
	int scrHeight;
	float near;
	float far;
	sceneStructMode scnStructMode;
} spData;

typedef struct
{
	float relBlend = 0.f;
} scBlendData;

}
