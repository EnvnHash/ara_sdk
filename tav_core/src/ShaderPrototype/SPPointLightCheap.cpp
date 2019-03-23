//
//  SPPointLightCheap.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPPointLightCheap.h"

namespace tav
{
SPPointLightCheap::SPPointLightCheap(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	lightProp = new LightProperties();

	lightProp->setAmbient(0.1f, 0.1f, 0.1f);
	lightProp->setColor(1.0f, 1.0f, 1.0f);
	lightProp->setPosition(-0.9f, 1.42f, 2.8f);
	lightProp->setEyeDirection(0.0f, 0.0f, 0.0f);
	lightProp->setShininess(0.504f);
	lightProp->setStrength(0.4f);
	lightProp->setConstantAttenuation(0.0f);
	lightProp->setLinearAttenuation(0.0f);
	lightProp->setQuadraticAttenuation(0.14f);

}

SPPointLightCheap::~SPPointLightCheap()
{
}

//--------------------------------------------------------------------------------

void SPPointLightCheap::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();
	a.setUniformSeparateMVP();
	a.addCode("out vec4 $O_Color;\n"
			"out glm::vec3 $O_Normal;\n"
			"out glm::vec3 $O_LightDirection;\n"
			"out glm::vec3 $O_HalfVector;\n"
			"out float $O_Attenuation;\n"
			"uniform glm::vec3 LPosition;\n" // location of the light, eye space
			"uniform glm::vec3 eyeDirection;\n"
			"uniform float constantAttenuation;\n"
			"uniform float linearAttenuation;\n"
			"uniform float quadraticAttenuation;\n"
			"mat4 $G_MVM;\n"
			"void main() {\n"
			"$O_Color = color;\n"
			"$G_MVM = $S_viewMatrix * (useInstancing == 0 ? $S_modelMatrix : modMatr);\n"
			"$O_Normal = mat3($S_modelMatrix * $S_viewMatrix) * normal;\n"
			// Compute these in the vertex shader instead of the fragment shader
			"$O_LightDirection = LPosition - glm::vec3(position);\n"
			"float lightDistance = length($O_LightDirection);\n"
			"$O_LightDirection = $O_LightDirection / lightDistance;\n"
			"$O_Attenuation = 1.0 / (constantAttenuation + linearAttenuation * lightDistance + quadraticAttenuation * lightDistance * lightDistance);\n"
			"$O_HalfVector = normalize($O_LightDirection + eyeDirection);\n"
			"gl_Position = $S_projectionMatrix * ($G_MVM * position);\n"
			"}");
}

//--------------------------------------------------------------------------------

void SPPointLightCheap::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPPointLightCheap::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("uniform glm::vec3 ambient;\n"
			"uniform glm::vec3 LColor;\n"
			"uniform float shininess;\n"     // exponent for sharping highlights
			"uniform float strength;\n"// extra factor to adjust shininess
			"out vec4 $S_FragColor;\n"
			"void main() {"
			// LightDirection, HalfVector, and Attenuation are interpolated
			// now, from vertex shader calculations
			"float diffuse = max(0.0, dot($I_Normal, $I_LightDirection));\n"
			"float specular = max(0.0, dot($I_Normal, $I_HalfVector));\n"
			"if (diffuse == 0.0) specular = 0.0; else specular = pow(specular, shininess) * strength;\n"
			"vec3 scatteredLight = ambient + LColor * diffuse * $I_Attenuation;\n"
			"vec3 reflectedLight = LColor * specular * $I_Attenuation;\n"
			"vec3 rgb = min($I_Color.rgb * scatteredLight + reflectedLight, glm::vec3(1.0));\n"
			"$S_FragColor = vec4(rgb, $I_Color.a);\n"
			"}");
}

//--------------------------------------------------------------------------------

void SPPointLightCheap::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPPointLightCheap::sendPar(int nrCams, camPar& cp, OSCData* osc,
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
