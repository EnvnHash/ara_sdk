//
//  SPShadow.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPShadow.h"

namespace tav
{
SPShadow::SPShadow(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	usesShadowMaps = true;
	hasMultiCamGeo = true; // hierdurch wird keine automatischer geo shader generiert
						   // und der source code für die shader "roh", also ohne anfügen der in und outs übersetzt
	near = 1.f;
	far = 100.f;

	lightProp = new LightProperties();
	lightProp->setAmbient(0.1f, 0.1f, 0.1f);
	lightProp->setColor(0.8f, 0.8f, 0.8f);
	lightProp->setSpecular(0.2f, 0.2f, 0.2f);
	lightProp->setPosition(-4.0f, 1.0f, 20.0);
	lightProp->setStrength(20.0f);

	scrWidth = sData->scrWidth;
	scrHeight = sData->scrHeight;

	scale_bias_matrix = glm::mat4(glm::vec4(0.5f, 0.0f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.5f, 0.0f, 0.0f),
			glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
			glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
}

SPShadow::~SPShadow()
{
}

//--------------------------------------------------------------------------------

void SPShadow::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();

	if (!_multiCam)
	{
		a.setUniformSeparateMVP();
		a.addCode("out vec4 $O_Color;"
				"out glm::vec3 $O_Normal;"
				"out vec4 $O_shadow_coord;"
				"out glm::vec3 $O_world_coord;"
				"out glm::vec3 $O_eye_coord;"
				"uniform glm::mat4 shadow_matrix;"
				"mat4 selModelMatr;"
				"void main() {"
				"$O_Color = color;"
				"selModelMatr = useInstancing == 0 ? $S_modelMatrix : modMatr;"
				"vec4 world_pos = selModelMatr * position;"
				"vec4 eye_pos = $S_viewMatrix * world_pos;"
				"vec4 clip_pos = $S_projectionMatrix * eye_pos;"
				"$O_world_coord = world_pos.xyz;"
				"$O_eye_coord = eye_pos.xyz;"
				"$O_shadow_coord = shadow_matrix * world_pos;"
				"$O_Normal = mat3($S_viewMatrix * $S_modelMatrix) * normal;"
				"gl_Position = clip_pos;"
				"}");
	}
	else
	{
		a.addCode("out vec4 vs_Color;\n"
				"out glm::vec3 vs_Normal;\n"
				"out vec4 vs_world_pos;\n"
				"uniform glm::mat4 modelMatrix;\n"
				"mat4 selModelMatr;\n"
				"void main() {\n"
				"vs_Color = color;\n"
				"selModelMatr = useInstancing == 0 ? modelMatrix : modMatr;\n"
				"vs_world_pos = selModelMatr * position;\n"
				"vs_Normal = mat3(modelMatrix) * normal;\n"
				"gl_Position = vs_world_pos;\n"
				"}");
	}
}

//--------------------------------------------------------------------------------

void SPShadow::defineGeom(bool _multiCam)
{
	if (_multiCam)
	{
		a.setWorkShader(GEOMETRY);
		std::string code =
				"layout(triangles, invocations=" + std::to_string(nrCams)
						+ ") in;\n"
								"layout(triangle_strip, max_vertices=3) out;\n"

								"in vec4 vs_Color[];\n"
								"in glm::vec3 vs_Normal[];\n"
								"in vec4 vs_world_pos[];\n"

								"out vec4 gs_Color;\n"
								"out glm::vec3 gs_Normal;\n"
								"out vec4 gs_shadow_coord;\n" // könnte auch nach hierhin bewegt werden
								"out glm::vec3 gs_world_coord;\n"
								"out glm::vec3 gs_eye_coord;\n"

								"uniform glm::mat4 shadow_matrix;\n"

								"uniform glm::mat4 model_matrix_g["
						+ std::to_string(nrCams) + "];\n"
								"uniform glm::mat4 view_matrix_g["
						+ std::to_string(nrCams) + "];\n"
								"uniform glm::mat4 projection_matrix_g["
						+ std::to_string(nrCams)
						+ "];\n"

								"void main() {\n"
								"for (int i=0; i<gl_in.length(); i++)\n"
								"{\n"
								"gl_ViewportIndex = gl_InvocationID;\n"
								"gs_Color = vs_Color[i];\n"
								"gs_Normal = (view_matrix_g[gl_InvocationID] * vec4(vs_Normal[i], 0.0)).xyz;\n"
								"gs_shadow_coord = shadow_matrix * vs_world_pos[i];"

								"gs_world_coord = vs_world_pos[i].xyz;\n"
								"gs_eye_coord = (view_matrix_g[gl_InvocationID] * vs_world_pos[i]).xyz;\n"
								"gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * gl_in[i].gl_Position;\n"
//            "gl_Position = projection_matrix_g[gl_InvocationID] * view_matrix_g[gl_InvocationID] * model_matrix_g[gl_InvocationID] * gl_in[i].gl_Position;\n"
								"EmitVertex();\n"
								"}"
								"}";
		a.addCode(code);
	}
}

//--------------------------------------------------------------------------------

void SPShadow::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);

	if (!_multiCam)
	{
		a.addCode("uniform sampler2DShadow depth_texture;"
				"uniform glm::vec3 LPosition;"
				"uniform glm::vec3 ambient;"
				"uniform glm::vec3 LColor;"
				"uniform glm::vec3 LSpecular;"
				"uniform float strength;" // material_specular_power
				"out vec4 $S_FragColor;"
				"void main() {"
				"    glm::vec3 N = $I_Normal;"
				"    glm::vec3 L = normalize(LPosition - $I_world_coord);"
				"    glm::vec3 R = reflect(-L, N);"
				"    glm::vec3 E = normalize($I_eye_coord);"
				"    float NdotL = dot(N, L);"
				"    float EdotR = dot(-E, R);"
				"    float diffuse = max(NdotL, 0.0);"
				"    float specular = max(pow(EdotR, strength),0.0);"
				"    float f = textureProj(depth_texture, $I_shadow_coord);"
				"    $S_FragColor = vec4(ambient + f * (LColor * diffuse + LSpecular * specular), 1.0);"
				"}");

	}
	else
	{
		a.addCode("in vec4 gs_Color;\n"
				"in glm::vec3 gs_Normal;\n"
				"in vec4 gs_shadow_coord;\n"
				"in glm::vec3 gs_world_coord;\n"
				"in glm::vec3 gs_eye_coord;\n"

				"uniform sampler2DShadow depth_texture;\n"
				"uniform glm::vec3 LPosition;\n"
				"uniform glm::vec3 ambient;\n"
				"uniform glm::vec3 LColor;\n"
				"uniform glm::vec3 LSpecular;\n"
				"uniform float strength;\n" // material_specular_power
				"out vec4 $S_FragColor;\n"

				"void main() {\n"
				"    glm::vec3 N = gs_Normal;\n"
				"    glm::vec3 L = normalize(LPosition - gs_world_coord);\n"
				"    glm::vec3 R = reflect(-L, N);\n"
				"    glm::vec3 E = normalize(gs_eye_coord);\n"
				"    float NdotL = dot(N, L);\n"
				"    float EdotR = dot(-E, R);\n"
				"    float diffuse = max(NdotL, 0.0);\n"
				"    float specular = max(pow(EdotR, strength),0.0);\n"
				"    float f = textureProj(depth_texture, gs_shadow_coord);\n"
				"    $S_FragColor = vec4(ambient + f * (LColor * diffuse + LSpecular * specular), 1.0);\n"
				"}");
	}
}

//--------------------------------------------------------------------------------

void SPShadow::preRender(SceneNode* _scene, double time, double dt)
{
	//if(shMapInited) shMap->render(_scene, time, dt);    // generate shadow map
}

//--------------------------------------------------------------------------------

void SPShadow::sendPar(int nrCams, camPar& cp, OSCData* osc, double time)
{
	if (shMapInited)
	{
		shadow_matrix = scale_bias_matrix * shMap->getLightProjMatr()
				* shMap->lightViewMatr();
		shader->setUniformMatrix4fv("shadow_matrix", &shadow_matrix[0][0], 1);
		lightProp->sendToShader(shader->getProgram());

		// needs glbindsampler?
		glBindTexture(GL_TEXTURE_2D, shMap->getDepthImg());
	}

	// calc half std::vector
	sendModelMatrix(cp);
	sendOscPar(cp, osc);

	if (!shMapInited)
	{
		shMap = new ShadowMapStd(&cp, scrWidth, scrHeight,
				lightProp->getLightPosition(), near, far, shCol);
		shMapInited = true;
	}

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
