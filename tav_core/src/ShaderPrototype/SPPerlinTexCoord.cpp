//
//  SPPerlinTexCoord.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPPerlinTexCoord.h"

namespace tav
{
SPPerlinTexCoord::SPPerlinTexCoord(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	debug = false;
	lightProp = new LightProperties();
	lightProp->setAmbient(0.1f, 0.1f, 0.1f);
	lightDir = glm::normalize(glm::vec3(-2.5f, 0.0f, 1.0f)); // from the object and not to the object
	lightProp->setDirection(lightDir.x, lightDir.y, lightDir.z); // by scaling this, the intensity varies
	lightProp->setHalfVector(lightDir.x, lightDir.y, lightDir.z);
	lightProp->setColor(1.0f, 1.0f, 1.0f);
	lightProp->setShininess(70.0f);
	lightProp->setStrength(0.5f);
}

//--------------------------------------------------------------------------------

SPPerlinTexCoord::~SPPerlinTexCoord()
{
	// delete lightProp;
}

//--------------------------------------------------------------------------------

void SPPerlinTexCoord::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();

	a.setUniformSeparateMVP();
	std::string code = "//SPPerlinTexCoord Prototype\n"
			"out vec4 $O_Color;\n"
			"out vec3 $O_Normal;\n"
			"out vec4 $O_TexCoord;\n";

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
					"$O_Normal = mat3($S_viewMatrix * $S_modelMatrix) * mapNorm;\n"
					"gl_Position = $S_projectionMatrix * $G_MVM * mapPos;\n"
					// "gl_Position = vec4(min(max(gl_Position.x, -1.0), 1.0), min(max(gl_Position.y, -1.0), 1.0), min(max(gl_Position.z, -10.0), 0.0), gl_Position.w);\n"
					"}";

	a.addCode(code);
}

//--------------------------------------------------------------------------------

void SPPerlinTexCoord::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPPerlinTexCoord::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("//SPPerlinTexCoord Directional Light Prototype\n"
			"uniform vec3 ambient;\n"
			"uniform vec3 LColor;\n"
			"uniform vec3 halfVector;\n"
			"uniform vec3 LDirection;\n"      // direction toward the light
			"uniform float shininess;\n"// exponent for sharping highlights
			"uniform float strength;\n"// extra factor to adjust shininess
			"uniform float brightness;\n"// extra factor to adjust shininess
			"uniform float oscAlpha;\n"// extra factor to adjust shininess
			"uniform float time2;\n"
			"uniform float chanOffs;\n"
			"uniform float toRad;\n"
			"uniform sampler2D tex;\n"
			"uniform sampler3D perlinTex;\n"
			"uniform sampler2D fotoTex;\n"
			"float pi = 3.1415926535897932384626433832795;\n"
			"float xScale;\n"
			"float amp;\n"
			"float ampX;\n"
			"float ampY;\n"
			"float zOffs;\n"
			"vec2 texCoPerl;\n"

			"out vec4 $S_FragColor;\n"
			"void main() {\n"
			"zOffs = $I_TexCoord.z / 2.0 + 0.25;\n"
			"amp = texture(tex, vec2(0.05, chanOffs)).r * 0.5;\n"
			"ampX = texture(tex, vec2($I_TexCoord.x, chanOffs)).r * 0.5;\n"
			"ampY = texture(tex, vec2($I_TexCoord.y, chanOffs)).r * 0.5;\n"
			"ampY = texture(tex, vec2($I_TexCoord.y, chanOffs)).r * 0.5;\n"

			"xScale = (ampX * 4.0);"
			"vec2 ampVec = vec2(ampX, ampY);\n"
			"vec2 modTex = $I_TexCoord.xy + ampVec;\n"

			"vec2 texCoPerl = vec2(texture(perlinTex, vec3(modTex, time2 * 0.01)).r, \n"
			"texture(perlinTex, vec3(modTex, time2 * 0.01 + 0.2)).r);\n"

			"texCoPerl = mix($I_TexCoord.xy, texCoPerl, abs(amp)* 2.0) + ampVec - vec2(0.0, 0.25);\n"
//        		"texCoPerl = mix($I_TexCoord.xy, texCoPerl, amp + 0.5) + ampVec;\n"

			"vec2 normTex = texCoPerl * 2.0 - vec2(1.0);\n"
			"float texAngle = atan(normTex.y, normTex.x);\n"
			"float texRad = sqrt((normTex.x * normTex.x) + (normTex.y * normTex.y));\n"
			"vec2 texCoPerlRad = vec2(texAngle / pi * 0.5, texRad * 0.8);\n "

			"vec2 mixCoPerlRad = mix(texCoPerl, texCoPerlRad, toRad);\n "

			"vec4 foto = texture(fotoTex, mixCoPerlRad + zOffs) * $I_Color * brightness;\n"

			"float alpha = (foto.r + foto.g + foto.b) * 0.333 * oscAlpha;\n"
			"if (alpha < 0.001) discard;\n"// transparenz wird vom depth_test zunichte gemacht
										   // deshalb wird das fragment ganz gecancelt, wenn transparent
										   // transparenz effekt nicht optimal, aber ok...
			"$S_FragColor = vec4(foto.rgb, alpha);\n"
			"}");

}

//--------------------------------------------------------------------------------

void SPPerlinTexCoord::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPPerlinTexCoord::sendPar(int nrCams, camPar& cp, OSCData* osc,
		double time)
{
	lightProp->sendToShader(shader->getProgram());

	halfVector = glm::normalize(lightDir + cp.viewerVec);
	//lightProp->setHalfVector(halfVector.x, halfVector.y, halfVector.z);
	lightProp->setHalfVector(0.f, 1.f, 0.f); // zum testen

	//
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

	shader->setUniform1f("time2", time);
	shader->needsTime = true;
}
}
