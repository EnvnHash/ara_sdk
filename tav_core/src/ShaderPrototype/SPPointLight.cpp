//
//  SPPointLight.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPPointLight.h"

namespace tav
{
SPPointLight::SPPointLight(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	lightProp = new LightProperties();
	lightProp->setAmbient(0.1f, 0.1f, 0.1f);
	lightProp->setColor(1.0f, 1.0f, 1.0f);
	lightProp->setPosition(-1.2f, 1.2f, 2.8f);
	lightProp->setEyeDirection(0.0f, 1.0f, 0.0f); // for the highlight, sollte cameraPos - lookAt sein
	lightProp->setShininess(0.5f);
	lightProp->setStrength(1.42f);
	lightProp->setConstantAttenuation(0.0f);
	lightProp->setLinearAttenuation(0.0f);
	lightProp->setQuadraticAttenuation(0.14f);
}

SPPointLight::~SPPointLight()
{
}

//--------------------------------------------------------------------------------

void SPPointLight::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();
	a.setUniformSeparateMVP();
	a.addCode(
			"out vec4 $O_Color;"
					"out glm::vec3 $O_Normal;"
					"out vec4 $O_Position;"
					"mat4 $G_MVM;"
					"void main() {"
					"$O_Color = color;"
					"$G_MVM = $S_viewMatrix * (useInstancing == 0 ? $S_modelMatrix : modMatr);"
					"$O_Normal = mat3($S_modelMatrix * $S_viewMatrix) * normal;"
					"$O_Position = $G_MVM * position;"
					"gl_Position = $S_projectionMatrix * ($G_MVM * position);"
					"}");
}

//--------------------------------------------------------------------------------

void SPPointLight::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPPointLight::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("uniform glm::vec3 ambient;"
			"uniform glm::vec3 LColor;"
			"uniform glm::vec3 LPosition;"   // location of the light, eye space
			"uniform float shininess;"// exponent for sharping highlights
			"uniform float strength;"// extra factor to adjust shininess
			"uniform glm::vec3 eyeDirection;"
			"uniform float constantAttenuation;"
			"uniform float linearAttenuation;"
			"uniform float quadraticAttenuation;"
			"out vec4 $S_FragColor;"
			"void main() {"
			// find the direction and distance of the light,
			// which changes fragment to fragment for a local light
			"vec3 lightDirection = LPosition - glm::vec3($I_Position);"
			"float lightDistance = length(lightDirection);"
			// normalize the light direction std::vector, so
			// that a dot products give cosines
			"lightDirection = lightDirection / lightDistance;"
			// model how much light is available for this fragment
			"float attenuation = 1.0 / (constantAttenuation + linearAttenuation * lightDistance + quadraticAttenuation * lightDistance * lightDistance);"
			// the direction of maximum highlight also changes per fragment
			"vec3 halfVector = normalize(lightDirection + eyeDirection);"
			"float diffuse = max(0.0, dot($I_Normal, lightDirection));"
			"float specular = max(0.0, dot($I_Normal, halfVector));"
			"if (diffuse == 0.0) specular = 0.0; else specular = pow(specular, shininess) * strength;"
			"vec3 scatteredLight = ambient + LColor * diffuse * attenuation;"
			"vec3 reflectedLight = LColor * specular * attenuation;"
			"vec3 rgb = min($I_Color.rgb * scatteredLight + reflectedLight, glm::vec3(1.0));"
			"$S_FragColor = vec4(rgb, $I_Color.a);"
			"}");
}

//--------------------------------------------------------------------------------

void SPPointLight::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPPointLight::sendPar(int nrCams, camPar& cp, OSCData* osc, double time)
{
	lightProp->sendToShader(shader->getProgram());

	sendModelMatrix(cp);

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

	sendOscPar(cp, osc);
}
}
