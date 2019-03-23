/*
 * ThreshFbo.cpp
 *
 *  Created on: May 24, 2018
 *      Author: sven
 */

#include "pch.h"
#include "ThreshFbo.h"

#define STRINGIFY(A) #A

namespace tav
{

ThreshFbo::ThreshFbo(sceneData* _scd, GLuint _width, GLuint _height, GLenum _type,
		GLenum _target, bool _depthBuf, int _nrAttachments, int _mipMapLevels,
		int _nrSamples, GLenum _wrapMode, bool _layered)
{
	shCol = (ShaderCollector*)_scd->shaderCollector;
	quad = _scd->stdQuad;

	fbo = new FBO(shCol, _width, _height, _type, _target, _depthBuf, _nrAttachments,
			_mipMapLevels, _nrSamples, _wrapMode, _layered);

	initShader();
}

//----------------------------------------------------

void ThreshFbo::initShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string stdVert = STRINGIFY(
		layout( location = 0 ) in vec4 position;\n
		layout( location = 1 ) in vec4 normal;\n
		layout( location = 2 ) in vec2 texCoord;\n
		layout( location = 3 ) in vec4 color;\n
		uniform mat4 m_pvm;\n
	);

	switch(fbo->getTarget())
	{
		case GL_TEXTURE_1D :
			stdVert += "out float tex_coord;\n";
		break;
		case GL_TEXTURE_1D_ARRAY :
			stdVert += "out vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D :
			stdVert += "out vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D_MULTISAMPLE :
			stdVert += "out vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_RECTANGLE :
			stdVert += "out vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D_ARRAY :
			stdVert += "out vec3 tex_coord;\n uniform float zCoord\n;";
		break;
		case GL_TEXTURE_3D :
			stdVert += "out vec3 tex_coord;\n uniform float zCoord\n;";
		break;
	}

	stdVert += "void main() {\n";

	switch(fbo->getTarget())
	{
		case GL_TEXTURE_1D :
			stdVert += "tex_coord = texCoord.x;\n";
		break;
		case GL_TEXTURE_1D_ARRAY :
			stdVert += "tex_coord = texCoord;\n";
		break;
		case GL_TEXTURE_2D :
			stdVert += "tex_coord = texCoord;\n";
		break;
		case GL_TEXTURE_2D_MULTISAMPLE :
			stdVert += "tex_coord = texCoord;\n";
		break;
		case GL_TEXTURE_RECTANGLE :
			stdVert += "tex_coord = texCoord;\n";
		break;
		case GL_TEXTURE_2D_ARRAY :
			stdVert += "tex_coord = vec3(texCoord, zCoord);\n";
		break;
		case GL_TEXTURE_3D :
			stdVert += "tex_coord = vec3(texCoord, zCoord);\n";
		break;
	}

	stdVert += "\t gl_Position = m_pvm * position;\n";
	stdVert += "}";

	stdVert = "// ThreshFbo vertex shader\n" +shdr_Header +stdVert;

	//----------------------------------------------------------------------

	std::string frag = "";

	switch(fbo->getTarget())
	{
		case GL_TEXTURE_1D :
			frag += "uniform sampler1D tex;\n in float tex_coord;\n";
		break;
		case GL_TEXTURE_1D_ARRAY :
			frag += "uniform sampler1DArray tex;\n in vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D :
			frag += "uniform sampler2D tex;\n in vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D_MULTISAMPLE :
			frag += "uniform sampler2D tex;\n in vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_RECTANGLE :
			frag += "uniform sampler2D tex;\n in vec2 tex_coord;\n";
		break;
		case GL_TEXTURE_2D_ARRAY :
			frag += "uniform sampler2DArray tex;\n in vec3 tex_coord;\n";
		break;
		case GL_TEXTURE_3D :
			frag += "uniform sampler3D tex;\n in vec3 tex_coord;\n";
		break;
	}

	frag += STRINGIFY(uniform float threshHigh;\n
					uniform float threshLow;\n
					layout (location = 0) out vec4 color;\n
					float outVal;\n
					void main(){\n
						outVal = texture(tex, tex_coord).r;\n
						outVal = outVal > threshLow ? (outVal < threshHigh ? 1.0 : 0.0) : 0.0;\n
						color = vec4(outVal);\n
					});

	frag = "// ThreshFbo fragment shader\n"+shdr_Header+frag;

	threshShdr = shCol->addCheckShaderText("ThreshFbo_"+std::to_string(fbo->getTarget()),
			stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

void ThreshFbo::proc(GLenum target, GLuint texId, float threshLow, float threshHigh)
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fbo->bind();
	fbo->clear();

	threshShdr->begin();
	threshShdr->setIdentMatrix4fv("m_pvm");
	threshShdr->setUniform1i("tex", 0);
	threshShdr->setUniform1f("threshLow", threshLow);
	threshShdr->setUniform1f("threshHigh", threshHigh);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(target, texId);

	quad->draw();

	fbo->unbind();
}

//----------------------------------------------------

ThreshFbo::~ThreshFbo()
{
	delete threshShdr;
	delete fbo;
}

} /* namespace tav */
