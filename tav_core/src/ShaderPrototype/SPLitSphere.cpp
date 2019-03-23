//
//  SPLitSphere.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPLitSphere.h"

namespace tav
{
SPLitSphere::SPLitSphere(spData* _sData, ShaderCollector* _shCol,
		sceneData* _scd) :
		ShaderProto(_sData, _shCol), scd(_scd)
{
	debug = false;
	lightProp = new LightProperties();
	lightProp->setPosition(8.f, 24.f, 16.f);
	lightProp->setAmbient(0.05f, 0.05f, 0.05f);
	lightProp->setColor(0.8f, 0.8f, 0.8f);
	lightProp->setShininess(5.0f);
	lightProp->setSpecular(0.7f, 0.7f, 0.7f);

	cubeTex = new TextureManager();
	//cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skyboxsunLumina.png").c_str() );
	cubeTex->loadTextureCube(
			((*scd->dataPath) + "textures/skyboxsun5deg2.png").c_str());

	litsphereTexture = new TextureManager();
//        litsphereTexture->loadTexture2D( ((*scd->dataPath)+"textures/marriott/droplet_01.png").c_str() );
	litsphereTexture->loadTexture2D(
			((*scd->dataPath) + "textures/marriott/droplet_01_bw.png").c_str());

	normalTexture = new TextureManager();
	normalTexture->loadTexture2D(
			((*scd->dataPath) + "textures/bump_maps/Unknown-5.jpeg").c_str());
}

SPLitSphere::~SPLitSphere()
{
}

//--------------------------------------------------------------------------------

void SPLitSphere::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();

	a.setUniformSeparateMVP();
	std::string code = "//SPLitSphere Directional Light Prototype\n"
			"out vec4 $O_Color;\n"
			"out vec3 $O_Normal;\n"
			"out vec4 $O_TexCoord;\n"
			"out vec4 $O_Position;\n";

	if (sData->scnStructMode)
		code += "uniform samplerBuffer pos_tbo;\n"
				"uniform samplerBuffer nor_tbo;\n"
				"uniform samplerBuffer tex_tbo;\n"
				"uniform samplerBuffer col_tbo;\n";

	code += "uniform float mapScaleFact;\n"
			"uniform float relBlend;\n"
			"mat4 $G_MVM;\n"
			"vec4 mapPos;\n"
			"vec3 mapNorm;\n"
			"int mappedPos;\n"
			"void main() {\n";

	if (sData->scnStructMode)
	{
		code +=
				"mappedPos = int(float((gl_VertexID / 3) * 3) * mapScaleFact) + (gl_VertexID % 3);\n"
						"mapPos = mix(position, texelFetch(pos_tbo, mappedPos), relBlend);\n"
						"mapNorm = mix(normal, texelFetch(nor_tbo, mappedPos).xyz, relBlend);\n"
						"$O_Color = mix(color, texelFetch(col_tbo, mappedPos), relBlend);\n"
						"$O_TexCoord = mix(texCoord, texelFetch(tex_tbo, mappedPos), relBlend);\n";
	}
	else
	{
		code += "mapPos = position;\n"
				"mapNorm = normal;\n"
				"$O_Color = color;\n"
				"$O_TexCoord = texCoord;\n";
	}

	code +=
			"$G_MVM = $S_viewMatrix * (useInstancing == 0 ? $S_modelMatrix : modMatr);\n"
					"$O_Position = mapPos;\n"
					"$O_Normal = normalize($S_normalMatrix * mapNorm);\n"
					"gl_Position = $S_projectionMatrix * $G_MVM * mapPos;\n"
					"}";

	a.addCode(code);
}

//--------------------------------------------------------------------------------

void SPLitSphere::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPLitSphere::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("//SPLitSphere Directional Light Prototype\n"
			"uniform vec3 ambient;\n"
			"uniform vec3 LPosition;\n"
			"uniform vec3 LColor;\n"
			"uniform vec3 LSpecular;\n"
			"uniform float shininess;\n"     // exponent for sharping highlights
			"uniform float brightness;\n"// extra factor to adjust shininess
			"uniform float oscAlpha;\n"// extra factor to adjust shininess
			"uniform float blendVec;\n"

			"uniform sampler2D texs[1];\n"
			"uniform sampler2D litsphereTexture;\n"
			"uniform sampler2D normalTex;\n"
			"uniform samplerCube cubeMap;\n"
			"vec4 tex0;\n"
			"vec4 envColor;\n"
			"vec4 modNorm;\n"
			"vec4 color;\n"
			"out vec4 $S_FragColor;\n"
			"void main() {\n"

			"tex0 = texture(texs[0], $I_TexCoord.xy);\n"
			"modNorm = texture(normalTex, $I_TexCoord.xy);\n"
			"modNorm = vec4($I_Normal, 0.0) + modNorm * 0.25;\n"

			"vec3 L = normalize(LPosition - $I_Position.xyz);\n"
			"vec3 E = normalize(-$I_Position.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)\n"
			"vec3 R = normalize(-reflect(L, modNorm.xyz));\n"

			//calculate Ambient Term:
			"vec4 Iamb = vec4(ambient, 1.0);\n"

			//calculate Diffuse Term:
			//    vec4 Idiff = blendCol * max(dot($I_Normal.xyz, L), 0.0);
			"vec4 Idiff = vec4(LColor, 1.0) * max(dot(modNorm.xyz, L), 0.0);\n"
			"Idiff = clamp(Idiff, 0.0, 1.0);\n"

			// calculate Specular Term:
			"vec4 Ispec = vec4(LSpecular, 1.0) * pow(max(dot(R, E), 0.0),0.3 * shininess);\n"
			"Ispec = clamp(Ispec, 0.0, 1.0);\n"

			"vec4 shading = texture(litsphereTexture, normalize(modNorm).xy * vec2(0.5, 0.5) + vec2(0.5, 0.5));\n"
			//"vec4 shading2 = texture(litsphereTexture, (LPosition.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));\n"
			//"shading = shading * blendVec + shading2 * (1.0 - blendVec);\n"

			"envColor = texture(cubeMap, R);\n"

//                  "color = (envColor * blendVec) * $I_Color ;\n"
			"color = mix(Iamb + Idiff + Ispec, envColor, blendVec) * shading * $I_Color;\n"
//                  "color = ((Iamb + Idiff + Ispec + envColor * blendVec) * shading ) * tex0;\n"

			"float alpha = ($I_Color.a * tex0.a) * oscAlpha;\n"
			"if (alpha < 0.001) discard;\n"// transparenz wird vom depth_test zunichte gemacht
										   // deshalb wird das fragment ganz gecancelt, wenn transparent
										   // transparenz effekt nicht optimal, aber ok...
			"$S_FragColor = vec4(color.rgb, alpha) * brightness;\n"
			"}");

}

//--------------------------------------------------------------------------------

void SPLitSphere::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPLitSphere::sendPar(int nrCams, camPar& cp, OSCData* osc, double time)
{
	lightProp->sendToShader(shader->getProgram());

	sendModelMatrix(cp);
	sendOscPar(cp, osc);

	if (nrCams == 1)
	{
		sendNormalMatrix(cp);
		sendViewMatrix(cp);
		sendProjectionMatrix(cp);
	}
	else
	{
		shader->setUniformMatrix3fv("normal_matrix_g",
				cp.multicam_normal_matrix, nrCams);
		shader->setUniformMatrix4fv("model_matrix_g", cp.multicam_model_matrix,
				nrCams);
		shader->setUniformMatrix4fv("view_matrix_g", cp.multicam_view_matrix,
				nrCams);
		shader->setUniformMatrix4fv("projection_matrix_g",
				cp.multicam_projection_matrix, nrCams);
	}
	shader->setUniform1f("blendVec", 0.4f);

	glActiveTexture(GL_TEXTURE5);
	shader->setUniform1i("normalTex", 5);
	normalTexture->bind(5);

	glActiveTexture(GL_TEXTURE6);
	shader->setUniform1i("litsphereTexture", 6);
	litsphereTexture->bind(6);

	glActiveTexture(GL_TEXTURE7);
	shader->setUniform1i("cubeMap", 7);
	cubeTex->bind(7);

	// printf("litspeher send par \n");
}
}
