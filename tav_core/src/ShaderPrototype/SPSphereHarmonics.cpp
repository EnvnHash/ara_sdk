//
//  SPSphereHarmonics.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
// not yet working!!

#include "pch.h"
#include "SPSphereHarmonics.h"

namespace tav
{
SPSphereHarmonics::SPSphereHarmonics(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	lightProp = new LightProperties();
	lightProp->setScaleFactor(0.8f);
}

SPSphereHarmonics::~SPSphereHarmonics()
{
}

//--------------------------------------------------------------------------------

void SPSphereHarmonics::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();
	a.setUniformSeparateMVP();
	a.addCode("out glm::vec3 $O_Color;\n"
			"uniform float ScaleFactor;"
			"const float C1 = 0.429043;"
			"const float C2 = 0.511664;"
			"const float C3 = 0.743125;"
			"const float C4 = 0.886227;"
			"const float C5 = 0.247708;"
			// Constants for Old Town Square lighting
			"const glm::vec3 L00 = glm::vec3( 0.871297, 0.875222, 0.864470);"
			"const glm::vec3 L1m1 = glm::vec3( 0.175058, 0.245335, 0.312891);"
			"const glm::vec3 L10 = glm::vec3( 0.034675, 0.036107, 0.037362);"
			"const glm::vec3 L11 = glm::vec3(-0.004629, -0.029448, -0.048028);"
			"const glm::vec3 L2m2 = glm::vec3(-0.120535, -0.121160, -0.117507);"
			"const glm::vec3 L2m1 = glm::vec3( 0.003242, 0.003624, 0.007511);"
			"const glm::vec3 L20 = glm::vec3(-0.028667, -0.024926, -0.020998);"
			"const glm::vec3 L21 = glm::vec3(-0.077539, -0.086325, -0.091591);"
			"const glm::vec3 L22 = glm::vec3(-0.161784, -0.191783, -0.219152);"
			"mat4 $G_MVM;\n"
			"void main() {\n"
			"   glm::vec3 tnorm = normalize(mat3($S_modelMatrix * $S_viewMatrix) * normal);"
			"   $O_Color = C1 * L22 *(tnorm.x * tnorm.x - tnorm.y * tnorm.y) +"
			"    C3 * L20 * tnorm.z * tnorm.z +"
			"    C4 * L00 -"
			"    C5 * L20 +"
			"    2.0 * C1 * L2m2 * tnorm.x * tnorm.y +"
			"    2.0 * C1 * L21 * tnorm.x * tnorm.z +"
			"    2.0 * C1 * L2m1 * tnorm.y * tnorm.z +"
			"    2.0 * C2 * L11 * tnorm.x +"
			"    2.0 * C2 * L1m1 * tnorm.y +"
			"    2.0 * C2 * L10 * tnorm.z;"
			"$O_Color *= ScaleFactor;"
			"gl_Position = $S_projectionMatrix * ($G_MVM * position);\n"
			"}");
}

//--------------------------------------------------------------------------------

void SPSphereHarmonics::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPSphereHarmonics::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("out vec4 $S_FragColor;\n"
			"void main() {"
			"$S_FragColor = vec4($I_Color, 1.0);"
			"}");
}

//--------------------------------------------------------------------------------

void SPSphereHarmonics::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPSphereHarmonics::sendPar(int nrCams, camPar& cp, OSCData* osc,
		double time)
{
	lightProp->sendToShader(shader->getProgram());

	sendModelMatrix(cp);
	sendOscPar(cp, osc);

	if (nrCams == 1)
	{
		sendViewMatrix(cp);
		sendProjectionMatrix(cp);
	}
	else
	{
		shader->setUniformMatrix4fv("model_matrix_g", cp.multicam_model_matrix,
				nrCams);
		shader->setUniformMatrix4fv("view_matrix_g", cp.multicam_view_matrix,
				nrCams);
		shader->setUniformMatrix4fv("projection_matrix_g",
				cp.multicam_projection_matrix, nrCams);
	}
}
}
