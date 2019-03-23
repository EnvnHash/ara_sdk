//
//  SPPerlinAudioSphere.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 14.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPPerlinAudioSphere.h"

namespace tav
{
SPPerlinAudioSphere::SPPerlinAudioSphere(spData* _sData,
		ShaderCollector* _shCol) :
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

SPPerlinAudioSphere::~SPPerlinAudioSphere()
{
	delete lightProp;
}

//--------------------------------------------------------------------------------

void SPPerlinAudioSphere::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();

	a.setUniformSeparateMVP();
	std::string code = "//SPPerlinAudioSphere Prototype\n"
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

void SPPerlinAudioSphere::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPPerlinAudioSphere::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("//SPPerlinAudioSphere Directional Light Prototype\n"
			"uniform vec3 ambient;\n"
			"uniform vec3 LColor;\n"
			"uniform vec3 halfVector;\n"
			"uniform vec3 LDirection;\n"      // direction toward the light
			"uniform float shininess;\n"// exponent for sharping highlights
			"uniform float strength;\n"// extra factor to adjust shininess
			"uniform float brightness;\n"// extra factor to adjust shininess
			"uniform float oscAlpha;\n"// extra factor to adjust shininess
			"uniform sampler2D texs[4];\n"
			"uniform sampler3D perlinTex;\n"
			"float pi = 3.1415926535897932384626433832795;\n"
			"float amp;\n"
			"float amp2;\n"
			"float amp3;\n"
			"float newNoise = 0.0;\n"

			"out vec4 $S_FragColor;\n"
			"void main() {\n"
			// compute cosine of the directions, using dot products,
			// to see how much light would be reflected
			// calculate normal in both directions
			// "float diffuse = abs(dot($I_Normal, LDirection));\n"
			// "float specular = abs(dot($I_Normal, halfVector));\n"

			// surfaces facing away from the light (negative dot products)
			// won’t be lit by the directional light
			// "if (diffuse == 0.0)\n"
			// "    specular = 0.0;\n"
			/// "else\n"
			// "    specular = pow(specular, shininess);\n" // sharpen the highlight
			//  "vec3 scatteredLight = ambient + LColor * diffuse;\n"
			//  "vec3 reflectedLight = LColor * specular * strength;\n"

			"amp3 = texture(texs[0], vec2($I_TexCoord.x * 0.1, $I_TexCoord.z)).r * 0.5 + 0.5;\n"
			"amp2 = texture(texs[0], vec2($I_TexCoord.y * 0.5, $I_TexCoord.z)).r * 0.5 + 0.5;\n"
			"amp2 *= 2.0;\n"
			"newNoise = texture(perlinTex, vec3($I_TexCoord.xy * amp3, amp2) ).r;\n"
			//  "newNoise = 1.0;\n"
			"amp = (texture(texs[0], vec2(2.0 * $I_TexCoord.x * newNoise, $I_TexCoord.z)).r * 0.5 + 0.5) * 1.5;\n"

			// "vec4 tc = texture(texs[int($I_TexCoord.z)], $I_TexCoord.xy);\n"

			// das hier ist kritische... vielleicht doch verschiedene shader...
			//"vec3 rgb = min((tc.rgb + $I_Color.rgb) * scatteredLight + reflectedLight, vec3(1.0));\n"
			"float alpha = $I_Color.a * oscAlpha;\n"
			"if (alpha < 0.001) discard;\n"// transparenz wird vom depth_test zunichte gemacht
										   // deshalb wird das fragment ganz gecancelt, wenn transparent
										   // transparenz effekt nicht optimal, aber ok...
//                  "$S_FragColor = vec4($I_Color.rgb, 0.5);\n"
			"$S_FragColor = vec4($I_Color.rgb, alpha) * amp * amp2 * 0.8 * brightness;\n"
			"$S_FragColor *= $S_FragColor;\n"
			"$S_FragColor.a *= 0.7;\n"
			"}");

}

//--------------------------------------------------------------------------------

void SPPerlinAudioSphere::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPPerlinAudioSphere::sendPar(int nrCams, camPar& cp, OSCData* osc,
		double time)
{
	lightProp->sendToShader(shader->getProgram());

	halfVector = glm::normalize(lightDir + cp.viewerVec);
	//lightProp->setHalfVector(halfVector.x, halfVector.y, halfVector.z);
	lightProp->setHalfVector(0.f, 1.f, 0.f); // zum testen

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
