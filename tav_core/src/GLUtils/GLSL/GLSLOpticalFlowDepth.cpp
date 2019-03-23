//
//  GLSLOpticalFlowDepth.cpp
//  tav_core
//
//  Created by Sven Hahne on 13/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLSLOpticalFlowDepth.h"

#define STRINGIFY(A) #A

namespace tav
{
GLSLOpticalFlowDepth::GLSLOpticalFlowDepth(ShaderCollector* _shCol, int _width,
		int _height) :
		shCol(_shCol), width(_width), height(_height), srcId(0), lambda(0.1f), median(
				3.f), bright(4.f)
{
	texShader = _shCol->getStdTex();
	texture = new PingPongFbo(shCol, width, height, GL_RGBA16F, GL_TEXTURE_2D,
			false, 2, 1, GL_CLAMP_TO_EDGE);
	texture->clear();
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 0.f);

	initShaders();
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::update()
{
	texture->dst->bind();
	texture->dst->clear();

	flowShader->begin();
	flowShader->setIdentMatrix4fv("m_pvm");

	flowShader->setUniform2f("scale", 1.f, 2.f);
	flowShader->setUniform2f("offset", 1.f / static_cast<float>(width),
			1.f / static_cast<float>(height));
	flowShader->setUniform1f("amp", bright);
	flowShader->setUniform1f("lambda", lambda);
	flowShader->setUniform1i("tex0", 0);
	flowShader->setUniform1i("tex1", 1);
	flowShader->setUniform1i("last", 2);
	flowShader->setUniform1f("median", median);
	flowShader->setUniform1f("maxDepth", 1.f);
	flowShader->setUniform1f("maxDist", maxDist);
	flowShader->setUniform1f("diffAmp", diffAmp);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcId);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lastSrcId);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texture->getSrcTexId());

	quad->draw();

	texture->dst->unbind();
	texture->swap();
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::initShaders()
{
	std::string shdr_Header = "#version 410\n\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color;

					uniform mat4 m_pvm; out vec2 tex_coord; out vec4 col;

					void main() { col = color; tex_coord = texCoord; gl_Position = m_pvm * position; });

	vert = shdr_Header + vert;

	std::string frag =
			STRINGIFY(
					uniform sampler2D tex0; uniform sampler2D tex1; uniform sampler2D last;

					uniform vec2 scale; uniform vec2 offset; uniform float maxDist; uniform float maxDepth; uniform float amp; uniform float lambda; uniform float median; uniform float diffAmp;

					in vec2 tex_coord; in vec4 col; layout (location = 0) out vec4 color; layout (location = 1) out vec4 diff;

					vec4 getColorCoded(float x, float y, vec2 scale) { vec2 xout = vec2( max(x,0.), abs(min(x,0.)) ) *scale.x; vec2 yout = vec2( max(y,0.), abs(min(y,0.)) ) *scale.y; float dirY = 1.0; if (yout.x > yout.y) dirY=0.90; return vec4(xout.xy, max(yout.x,yout.y), dirY); }

					vec4 getGrayScale(vec4 col) {
					//float gray = dot(vec3(col.x, col.y, col.z), vec3(0.3, 0.59, 0.11));
					float gray = col.r / maxDepth; return vec4(gray,gray,gray,1.0); }

					vec4 texture2DRectGray(sampler2D tex, vec2 coord) { return getGrayScale(texture(tex, coord)); }

					void main() { float srcDepth = texture(tex0, tex_coord).r / maxDepth; float lastDepth = texture(tex1, tex_coord).r / maxDepth;

					vec4 a = texture2DRectGray(tex0, tex_coord); vec4 b = texture2DRectGray(tex1, tex_coord); vec2 x1 = vec2(offset.x,0.); vec2 y1 = vec2(0.,offset.y);

					//get the difference
					vec4 curdif = b-a;

					//calculate the gradient
					//for X________________
					vec4 gradx = texture2DRectGray(tex1, tex_coord+x1) -texture2DRectGray(tex1, tex_coord-x1); gradx += texture2DRectGray(tex0, tex_coord+x1) -texture2DRectGray(tex0, tex_coord-x1);

					//for Y________________
					vec4 grady = texture2DRectGray(tex1, tex_coord+y1) -texture2DRectGray(tex1, tex_coord-y1); grady += texture2DRectGray(tex0, tex_coord+y1) -texture2DRectGray(tex0, tex_coord-y1);

					vec4 gradmag = sqrt((gradx*gradx)+(grady*grady)+vec4(lambda));

					vec4 vx = curdif*(gradx/gradmag); vec4 vy = curdif*(grady/gradmag);

					//vec4 lastFrame = texture(last, tex_coord);

					//    color = vec4(vx.r, vy.r, 0.0, 1.0) * 3.0;
					color = vec4(pow(abs(vx.r) * 6.0, 1.7) * sign(vx.r) * amp, pow(abs(vy.r) * 6.0, 1.7) * sign(vy.r) * amp, (srcDepth - lastDepth) * amp, 1.0);
					//	color = srcDepth > maxDist ? vec4(0.0, 0.0, 0.0, 1.0) : color;

					float diffV = pow(abs(srcDepth - lastDepth) * diffAmp, 2.0);
					//diffV = srcDepth > maxDist ? 0.0 : diffV;
					//diffV = diffV  ? 1.0 : 0.0;
					diff = vec4(diffV, diffV, diffV, 1.0); });

	frag = shdr_Header + frag;

	flowShader = shCol->addCheckShaderText("GLSLOpticalFlowDepth", vert.c_str(),
			frag.c_str());

//        uBlock = new UniformBlock(flowShader->getProgram(), "data");
//        uBlock->addVarName("scale", &corners[i][0], GL_FLOAT_VEC2);
}

//---------------------------------------------------------

GLuint GLSLOpticalFlowDepth::getLastTexId()
{
	return srcId;
}

//---------------------------------------------------------

GLuint GLSLOpticalFlowDepth::getDiffTexId()
{
	return texture->src->getColorImg(1);
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setCurTexId(GLuint _id)
{
	lastSrcId = srcId;
	srcId = _id;
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setLastTexId(GLuint _id)
{
	lastSrcId = _id;
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setMedian(float _median)
{
	median = _median;
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setBright(float _val)
{
	bright = _val;
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setMaxDist(float _val)
{
	maxDist = _val;
}

//---------------------------------------------------------

void GLSLOpticalFlowDepth::setDiffAmp(float _val)
{
	diffAmp = _val;
}

//---------------------------------------------------------

GLuint GLSLOpticalFlowDepth::getResTexId()
{
	return texture->getSrcTexId();
}

//---------------------------------------------------------

GLSLOpticalFlowDepth::~GLSLOpticalFlowDepth()
{
	flowShader->remove();
	delete flowShader;
	delete quad;
	delete texture;
}
}
