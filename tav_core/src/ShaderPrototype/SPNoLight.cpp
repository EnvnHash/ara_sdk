//
//  SPNoLight.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "SPNoLight.h"

namespace tav
{
SPNoLight::SPNoLight(spData* _sData, ShaderCollector* _shCol) :
		ShaderProto(_sData, _shCol)
{
	debug = false;
}

SPNoLight::~SPNoLight()
{
}

//--------------------------------------------------------------------------------

void SPNoLight::defineVert(bool _multiCam)
{
	a.setVertStdInput();
	a.setEnableInstancing();
	a.setInstModMatr();

	a.setUniformSeparateMVP();
	std::string code = "//SPNoLight Prototype\n"
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
					"}";

	a.addCode(code);
}

//--------------------------------------------------------------------------------

void SPNoLight::defineGeom(bool _multiCam)
{
}

//--------------------------------------------------------------------------------

void SPNoLight::defineFrag(bool _multiCam)
{
	a.setWorkShader(FRAGMENT);
	a.addCode("//SPNoLight Directional Light Prototype\n"
			"uniform float brightness;\n"    // extra factor to adjust shininess
			"uniform float oscAlpha;\n"// extra factor to adjust shininess
			"uniform sampler2D texs[4];\n"
			"out vec4 $S_FragColor;\n"
			"void main() {\n"

			// don’t modulate the underlying color with reflected light,
			// only with scattered light
			"vec4 tc = texture(texs[int($I_TexCoord.z)], $I_TexCoord.xy);\n"

			// das hier ist kritische... vielleicht doch verschiedene shader...
			"vec3 rgb = min((tc.rgb + $I_Color.rgb), vec3(1.0));\n"
			"float alpha = $I_Color.a * tc.a * oscAlpha;\n"
			"if (alpha < 0.001) discard;\n"// transparenz wird vom depth_test zunichte gemacht
			// deshalb wird das fragment ganz gecancelt, wenn transparent
			// transparenz effekt nicht optimal, aber ok...

			"$S_FragColor = vec4(rgb, alpha) * brightness;\n"
			"}");

}

//--------------------------------------------------------------------------------

void SPNoLight::preRender(SceneNode* _scene, double time, double dt)
{
}

//--------------------------------------------------------------------------------

void SPNoLight::sendPar(int nrCams, camPar& cp, OSCData* osc, double time)
{
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
