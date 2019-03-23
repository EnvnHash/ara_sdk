//
//  GLSLFluid.cpp
//  Tav_App
//
//  adapted from ofxFluid.cpp
//  Created by Patricio Gonzalez Vivo on 9/29/11
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "pch.h"
#include "GLUtils/GLSL/GLSLFluid.h"

namespace tav
{
GLSLFluid::GLSLFluid(bool _bObstacles, ShaderCollector* _shCol) :
		bObstacles(_bObstacles), shCol(_shCol)
{
	initShaders(bObstacles);
	stdTexShader = _shCol->getStdTex();

	cellSize = 1.25f;
	gradientScale = 1.00f / cellSize;
	ambientTemperature = 0.0f;
	numJacobiIterations = 40;
	timeStep = 0.125f;
	smokeBuoyancy = 0.1f;
	smokeWeight = 0.05f;

	applyVelTextureRadius = 0.08f;

	gForce = glm::vec2(0, -0.98);
	applyImpMultCol = glm::vec3(1.f, 1.f, 1.f);
	whiteCol = glm::vec3(1.f, 1.f, 1.f);

	camMat = glm::scale(glm::mat4(1.f), glm::vec3(1.f, -1.f, 1.f));
}

//------------------------------------------------------------------------------------

void GLSLFluid::allocate(int _width, int _height, float _scale)
{
	quad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 0.f);

	width = _width;
	height = _height;
	scale = _scale;

	gridWidth = width * scale;
	gridHeight = height * scale;

	pingPong = new PingPongFbo(shCol, gridWidth, gridHeight, GL_RGBA16F,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	dissipation = 0.999f;

	velocityBuffer = new PingPongFbo(shCol, gridWidth, gridHeight, GL_RGB16F,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	velocityDissipation = 0.9f;

	temperatureBuffer = new PingPongFbo(shCol, gridWidth, gridHeight, GL_RGB16F,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	//        temperatureBuffer->clear(ambientTemperature);
	temperatureDissipation = 0.99f;

	pressureBuffer = new PingPongFbo(shCol, gridWidth, gridHeight, GL_RGB16F,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
	pressureDissipation = 0.9f;

	obstaclesFbo = new FBO(shCol, gridWidth, gridHeight, GL_RGB8, GL_TEXTURE_2D,
			false, 1, 1, 1, GL_REPEAT, false);
	divergenceFbo = new FBO(shCol, gridWidth, gridHeight, GL_RGB16F,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

	int num = 1;
	textures = new FBO*[num];

	// In any case it will allocate the total amount of textures with the internalFormat needed
	for (int i = 0; i < num; i++)
		textures[i] = new FBO(shCol, width, height, GL_RGBA8, GL_TEXTURE_2D,
				false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);

	colorAddPct = 0.0;
	velocityAddPct = 0.0;
}

//------------------------------------------------------------------------------------
// input in pixeln, links unten ist 0|0

void GLSLFluid::addTemporalForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col,
		float _rad, float _temp, float _den)
{
	punctualForce f;

	f.pos = _pos * scale;
	f.vel = _vel;
	f.color = _col;
	f.rad = _rad;
	f.temp = _temp;
	f.den = _den;

	temporalForces.push_back(f);
}

//------------------------------------------------------------------------------------

void GLSLFluid::addTemporalVelocity(glm::vec2 _pos, glm::vec2 _vel, float _rad,
		float _temp, float _den)
{
	punctualForce f;

	f.pos = _pos * scale;
	f.vel = _vel;
	f.rad = _rad;
	f.temp = _temp;
	f.den = _den;

	temporalVelocity.push_back(f);
}

//------------------------------------------------------------------------------------

void GLSLFluid::addConstantForce(glm::vec2 _pos, glm::vec2 _vel, glm::vec4 _col,
		float _rad, float _temp, float _den)
{
	punctualForce f;

	f.pos = _pos * scale;
	f.vel = _vel;
	f.color = _col;
	f.rad = _rad;
	f.temp = _temp;
	f.den = _den;

	constantForces.push_back(f);
}

//------------------------------------------------------------------------------------

void GLSLFluid::setObstacles(GLint texNr)
{
	obstaclesFbo->bind();
	obstaclesFbo->clear();

	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	stdTexShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texNr);

	quad->draw();
	stdTexShader->end();

	obstaclesFbo->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid::setObstacles(TextureManager* _tex)
{
	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	obstaclesFbo->bind();
	_tex->bind();
	quad->draw();
	stdTexShader->end();
	obstaclesFbo->unbind();
}

//--- add Color with a gl Texture ---------------------------------------------------------------------------------

void GLSLFluid::addColor(GLint _tex, glm::vec3 _multCol, float _pct, bool _asBW)
{
	applyImpMultCol = _multCol;
	colorAddTex = _tex;
	colorAddPct = _pct;
	colorAddPctAsBW = _asBW;
}

//--- add Velocity with a gl Texture ---------------------------------------------------------------------------------

void GLSLFluid::addVelocity(GLint _tex, float _pct)
{
	velocityAddTex = _tex;
	velocityAddPct = _pct;
}

//------------------------------------------------------------------------------------

void GLSLFluid::clear(float _alpha)
{
	pingPong->clear();
	velocityBuffer->clear();
	temperatureBuffer->clear();
	pressureBuffer->clear();

	obstaclesFbo->bind();
	obstaclesFbo->clear();
	divergenceFbo->bind();
	divergenceFbo->clear();

	temperatureBuffer->clear(ambientTemperature);
}

//------------------------------------------------------------------------------------

void GLSLFluid::update()
{
	glDisable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//  Scale Obstacles
	if (bObstacles)
	{
		obstaclesFbo->bind();
		stdTexShader->begin();
		stdTexShader->setIdentMatrix4fv("m_pvm");
		stdTexShader->setUniform2f("tScale", 1.f, 1.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[0]->getColorImg());
		quad->draw();

		obstaclesFbo->unbind();
	}

	//  Pre-Compute
	advect(velocityBuffer, velocityDissipation);
	advect(temperatureBuffer, temperatureDissipation);
	advect(pingPong, dissipation);

	if (useBuoyancy)
		applyBuoyancy();    // only useful for smoke, write to velo buf

	if (colorAddPct > 0.0)
	{
		applyImpulse(temperatureBuffer, colorAddTex, whiteCol, colorAddPct,
				false);
		applyImpulse(pingPong, colorAddTex, applyImpMultCol, colorAddPct, false,
				colorAddPctAsBW);

		// könnte sein, dass bei jeden update die textur interpretiert werden muss, sonst gibt es flimmer effekte
		// colorAddPct = 0.0;
	}

	if (velocityAddPct > 0.0)
	{
		applyImpulse(velocityBuffer, velocityAddTex, whiteCol, velocityAddPct,
				true);
		// könnte sein, dass bei jeden update die textur interpretiert werden muss, sonst gibt es flimmer effekte
		// velocityAddPct = 0.0;
	}

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_BLEND);

	if ((int) temporalVelocity.size() != 0)
	{
		for (int i = 0; i < int(temporalVelocity.size()); i++)
		{
			if (glm::length(temporalVelocity[i].vel) != 0)
			{
				glm::vec4 val = glm::vec4(temporalVelocity[i].vel.x,
						temporalVelocity[i].vel.y, 0.f, 1.f);
				applyImpulse(velocityBuffer, temporalVelocity[i].pos, val,
						temporalVelocity[i].rad);
			}
		}

		temporalVelocity.clear();
	}

	// proc forces
	std::vector<punctualForce> forces[2] =
	{ constantForces, temporalForces };
	for (int j = 0; j < 2; j++)
	{
		for (int i = 0; i < int(forces[j].size()); i++)
		{
			glm::vec4 val = glm::vec4(forces[j][i].temp, forces[j][i].temp,
					forces[j][i].temp, 1.f);
			applyImpulse(temperatureBuffer, forces[j][i].pos, val,
					forces[j][i].rad);

			if (glm::length(forces[j][i].color) != 0)
				applyImpulse(pingPong, forces[j][i].pos,
						forces[j][i].color * forces[j][i].den,
						forces[j][i].rad);

			if (glm::length(forces[j][i].vel) != 0)
			{
				val = glm::vec4(forces[j][i].vel.x, forces[j][i].vel.y, 0.f,
						1.f);
				applyImpulse(velocityBuffer, forces[j][i].pos, val,
						forces[j][i].rad);
			}
		}
	}
	temporalForces.clear();

	//  Compute
	computeDivergence();

	pressureBuffer->src->bind();
	pressureBuffer->src->clear();
	pressureBuffer->src->unbind();

	// start the jacobi shader
	for (int i = 0; i < numJacobiIterations; i++)
		jacobi();

	subtractGradient();
}

//------------------------------------------------------------------------------------

void GLSLFluid::draw()
{
	//    if(bObstacles){
	//        textures[0].draw(x,y,_width,_height);
	//    }

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	stdTexShader->setUniform1i("tex", 0);
	stdTexShader->setUniform2f("tScale", 1.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pingPong->getSrcTexId());

	quad->draw();

	stdTexShader->end();
}

//------------------------------------------------------------------------------------

void GLSLFluid::drawVelocity()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	stdTexShader->setUniform1i("tex", 0);
	stdTexShader->setUniform2f("tScale", 1.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
	quad->draw();

	stdTexShader->end();
}

//------------------------------------------------------------------------------------

void GLSLFluid::advect(PingPongFbo* _buffer, float _dissipation)
{
	_buffer->dst->bind();

	shader->begin();
	shader->setUniform1f("TimeStep", timeStep);
	shader->setUniform2f("scr", gridWidth, gridHeight);
	shader->setUniform1f("Dissipation", _dissipation);
	shader->setUniform2f("tScale", 1.f, 1.f);
	shader->setIdentMatrix4fv("m_pvm");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
	shader->setUniform1i("VelocityTexture", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _buffer->getSrcTexId());
	shader->setUniform1i("backbuffer", 1);

	if (bObstacles)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, obstaclesFbo->getColorImg());
		shader->setUniform1i("tex0", 2);
	}

	quad->draw();

	//shader->end();

	_buffer->dst->unbind();
	_buffer->swap();
}

//------------------------------------------------------------------------------------

void GLSLFluid::jacobi()
{
	pressureBuffer->dst->bind();

	jacobiShader->begin();
	jacobiShader->setUniform1f("Alpha", -cellSize * cellSize);
	jacobiShader->setUniform1f("InverseBeta", 0.25f);
	jacobiShader->setUniform2f("step", 1.f / gridWidth, 1.f / gridHeight);
	jacobiShader->setUniform2f("tScale", 1.f, 1.f);
	jacobiShader->setIdentMatrix4fv("m_pvm");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressureBuffer->getSrcTexId());
	jacobiShader->setUniform1i("Pressure", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, divergenceFbo->getColorImg());
	jacobiShader->setUniform1i("Divergence", 1);

	if (bObstacles)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, obstaclesFbo->getColorImg());
		jacobiShader->setUniform1i("tex0", 2);
	}

	quad->draw();

	// jacobiShader->end();
	pressureBuffer->dst->unbind();
	pressureBuffer->swap();
}

//------------------------------------------------------------------------------------

void GLSLFluid::subtractGradient()
{
	velocityBuffer->dst->bind();

	subtractGradientShader->begin();
	subtractGradientShader->setUniform1f("GradientScale", gradientScale);
	subtractGradientShader->setIdentMatrix4fv("m_pvm");
	subtractGradientShader->setUniform2f("tScale", 1.f, 1.f);
	subtractGradientShader->setUniform2f("step", 1.f / gridWidth,
			1.f / gridHeight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
	subtractGradientShader->setUniform1i("Velocity", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pressureBuffer->getSrcTexId());
	subtractGradientShader->setUniform1i("Pressure", 1);

	if (bObstacles)
	{
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, obstaclesFbo->getColorImg());
		subtractGradientShader->setUniform1i("tex0", 2);
	}

	quad->draw();

	//subtractGradientShader->end();
	velocityBuffer->dst->unbind();
	velocityBuffer->swap();

}

//------------------------------------------------------------------------------------

void GLSLFluid::computeDivergence()
{
	divergenceFbo->bind();

	computeDivergenceShader->begin();
	computeDivergenceShader->setUniform1f("HalfInverseCellSize",
			0.5f / cellSize);
	computeDivergenceShader->setIdentMatrix4fv("m_pvm");
	computeDivergenceShader->setUniform2f("tScale", 1.f, 1.f);
	computeDivergenceShader->setUniform2f("step", 1.f / gridWidth,
			1.f / gridHeight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
	computeDivergenceShader->setUniform1i("Velocity", 0);

	if (bObstacles)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, obstaclesFbo->getColorImg());
		computeDivergenceShader->setUniform1i("tex0", 2);
	}

	quad->draw();

	computeDivergenceShader->end();
	divergenceFbo->unbind();
}

//------------------------------------------------------------------------------------

void GLSLFluid::applyImpulse(PingPongFbo* _buffer, GLint _baseTex,
		glm::vec3& _multVec, float _pct, bool _isVel, bool _asBW)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	if (_isVel)
		glBlendFunc(GL_ONE, GL_ONE);

	_buffer->src->bind();
	applyTextureShader->begin();
	applyTextureShader->setIdentMatrix4fv("m_pvm");
	applyTextureShader->setUniform2f("tScale", 1.f, 1.f);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _baseTex);
	applyTextureShader->setUniform1i("tex1", 0);

	applyTextureShader->setUniform1f("pct", _pct);
	applyTextureShader->setUniform1f("velThresh", applyVelTexThresh);
	applyTextureShader->setUniform1i("isVel", int(_isVel));
	applyTextureShader->setUniform1f("asBW", float(_asBW));
	applyTextureShader->setUniform3fv("multCol", &_multVec[0]);
	applyTextureShader->setUniform1f("Radius", applyVelTextureRadius);

	quad->draw();

	applyTextureShader->end();
	_buffer->src->unbind();

	if (_isVel)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_BLEND);
}

//------------------------------------------------------------------------------------

void GLSLFluid::applyImpulse(PingPongFbo* _buffer, glm::vec2 _force,
		glm::vec4 _value, float _radio, bool _isVel)
{
	glEnable(GL_BLEND);
	_buffer->src->bind();
	applyImpulseShader->begin();

	applyImpulseShader->setUniform2f("Point", _force.x, _force.y);
	applyImpulseShader->setUniform1f("Radius", _radio);
	applyImpulseShader->setUniform2f("tScale", gridWidth, gridHeight);
	applyImpulseShader->setUniform3f("Value", _value.x, _value.y, _value.z);
	applyImpulseShader->setUniform1i("isVel", (_isVel) ? 1 : 0);
	applyImpulseShader->setIdentMatrix4fv("m_pvm");

	quad->draw();

	applyImpulseShader->end();
	_buffer->src->unbind();
	glDisable(GL_BLEND);
}

//------------------------------------------------------------------------------------

void GLSLFluid::applyBuoyancy()
{
	velocityBuffer->dst->bind();

	applyBuoyancyShader->begin();
	applyBuoyancyShader->setUniform1f("AmbientTemperature", ambientTemperature);
	applyBuoyancyShader->setUniform1f("TimeStep", timeStep);
	applyBuoyancyShader->setUniform1f("Sigma", smokeBuoyancy);
	applyBuoyancyShader->setUniform1f("Kappa", smokeWeight);
	applyBuoyancyShader->setUniform2f("Gravity", (float) gForce.x,
			(float) gForce.y);
	applyBuoyancyShader->setUniform2f("tScale", 1.f, 1.f);
	applyBuoyancyShader->setIdentMatrix4fv("m_pvm");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocityBuffer->getSrcTexId());
	applyBuoyancyShader->setUniform1i("Velocity", 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, temperatureBuffer->getSrcTexId());
	applyBuoyancyShader->setUniform1i("Temperature", 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pingPong->getSrcTexId());
	applyBuoyancyShader->setUniform1i("Density", 2);

	quad->draw();

	applyBuoyancyShader->end();

	velocityBuffer->dst->unbind();
	velocityBuffer->swap();
}

//------------------------------------------------------------------------------------

// With begin(int) and end(int) the textures allocated can be filled with data
void GLSLFluid::begin(int _texNum)
{
	if ((_texNum < nTextures) && (_texNum >= 0))
	{
		textures[_texNum]->bind();
		textures[_texNum]->clear();
	}
}

//------------------------------------------------------------------------------------

void GLSLFluid::end(int _texNum)
{
	if ((_texNum < nTextures) && (_texNum >= 0))
	{
		textures[_texNum]->unbind();
	}
}

//------------------------------------------------------------------------------------

GLint GLSLFluid::getResTex()
{
	return pingPong->getSrcTexId();
}

//------------------------------------------------------------------------------------

GLint GLSLFluid::getVelocityTex()
{
	return velocityBuffer->getSrcTexId();;
}

//------------------------------------------------------------------------------------

int GLSLFluid::getWidth()
{
	return width;
}

//------------------------------------------------------------------------------------

int GLSLFluid::getHeight()
{
	return height;
}

//------------------------------------------------------------------------------------

int GLSLFluid::getVelTexWidth()
{
	return gridWidth;
}

//------------------------------------------------------------------------------------

int GLSLFluid::getVelTexHeight()
{
	return gridHeight;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setVelTexThresh(float _velTexThresh)
{
	applyVelTexThresh = _velTexThresh;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setUseBuoyancy(bool _val)
{
	useBuoyancy = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setAmbientTemp(float _val)
{
	ambientTemperature = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setSmokeBuoyancy(float _val)
{
	smokeBuoyancy = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setSmokeWeight(float _val)
{
	smokeWeight = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setVelTexRadius(float _val)
{
	applyVelTextureRadius = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::setTimeStep(float _val)
{
	timeStep = _val;
}

//------------------------------------------------------------------------------------

void GLSLFluid::initShaders(bool obstacles)
{
	std::string frag;

	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string stdVert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; uniform vec2 tScale; out vec2 tex_coord; out vec4 col; void main() { col = color; tex_coord = texCoord * tScale; gl_Position = m_pvm * position; });
	stdVert = "// GLSLFluid vertex shader\n" + shdr_Header + stdVert;

	if (bObstacles)
	{
		frag =
				STRINGIFY(uniform sampler2D tex0;         // Real obstacles
						uniform sampler2D backbuffer; uniform sampler2D VelocityTexture;

						uniform float TimeStep; uniform float Dissipation; uniform vec2 scr; uniform int bObstacles; in vec2 tex_coord; layout(location = 0) out vec4 fragColor;

						void main(){ float solid = texture(tex0, tex_coord).r; if (solid > 0.1) { fragColor = vec4(0.0,0.0,0.0,0.0); return; } vec2 u = texture(VelocityTexture, tex_coord).rg; vec2 coord = tex_coord * scr - TimeStep * u; fragColor = Dissipation * texture(backbuffer, coord / scr); });
		frag = "// GLSLFluid advect frag shader\n" + shdr_Header + frag;
		shader = shCol->addCheckShaderText("fluidAdvect", stdVert.c_str(),
				frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Pressure; uniform sampler2D Divergence; uniform sampler2D tex0; uniform float Alpha; uniform float InverseBeta; uniform vec2 step; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main() { vec4 pN = texture(Pressure, tex_coord + vec2(0.0, step.y)); vec4 pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)); vec4 pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)); vec4 pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)); vec4 pC = texture(Pressure, tex_coord);

						glm::vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb; glm::vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb; glm::vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb; glm::vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;

						if (oN.x > 0.1) pN = pC; if (oS.x > 0.1) pS = pC; if (oE.x > 0.1) pE = pC; if (oW.x > 0.1) pW = pC;

						vec4 bC = texture(Divergence, tex_coord); fragColor = (pW + pE + pS + pN + Alpha * bC) * InverseBeta; });
		frag = "// GLSLFluid jacobi frag shader\n" + shdr_Header + frag;
		jacobiShader = shCol->addCheckShaderText("fluidJacobi", stdVert.c_str(),
				frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Velocity; uniform sampler2D Pressure; uniform sampler2D tex0; uniform vec2 step; uniform float GradientScale; in vec2 tex_coord; layout(location = 0) out vec4 fragColor;

						void main(){ vec3 oC = texture(tex0, tex_coord).rgb; if (oC.x > 0.1) { fragColor.gb = oC.yz; return; }

						float pN = texture(Pressure, tex_coord + vec2(0.0, step.y)).r; float pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)).r; float pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)).r; float pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)).r; float pC = texture(Pressure, tex_coord).r;

						vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb; vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb; vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb; vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;

						vec2 obstV = vec2(0.0,0.0); vec2 vMask = vec2(1.0,1.0);

						if (oN.x > 0.1) { pN = pC; obstV.y = oN.z; vMask.y = 0.0; } if (oS.x > 0.1) { pS = pC; obstV.y = oS.z; vMask.y = 0.0; } if (oE.x > 0.1) { pE = pC; obstV.x = oE.y; vMask.x = 0.0; } if (oW.x > 0.1) { pW = pC; obstV.x = oW.y; vMask.x = 0.0; }

						vec2 oldV = texture(Velocity, tex_coord).rg; vec2 grad = vec2(pE - pW, pN - pS) * GradientScale; vec2 newV = oldV - grad;

						fragColor.rg = (vMask * newV) + obstV; });
		frag = "// GLSLFluid subtract gradient frag shader\n" + shdr_Header
				+ frag;
		subtractGradientShader = shCol->addCheckShaderText("fluidSubtrGrad",
				stdVert.c_str(), frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Velocity; uniform sampler2D Pressure; uniform sampler2D tex0; uniform vec2 step; uniform float GradientScale; in vec2 tex_coord; layout(location = 0) out vec4 fragColor;

						void main(){ vec3 oC = texture(tex0, tex_coord).rgb; if (oC.x > 0.1) { fragColor.gb = oC.yz; return; }

						float pN = texture(Pressure, tex_coord + vec2(0.0, step.y)).r; float pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)).r; float pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)).r; float pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)).r; float pC = texture(Pressure, tex_coord).r;

						vec3 oN = texture(tex0, tex_coord + vec2(0.0, step.y)).rgb; vec3 oS = texture(tex0, tex_coord + vec2(0.0, -step.y)).rgb; vec3 oE = texture(tex0, tex_coord + vec2(step.x, 0.0)).rgb; vec3 oW = texture(tex0, tex_coord + vec2(-step.x, 0.0)).rgb;

						vec2 obstV = vec2(0.0,0.0); vec2 vMask = vec2(1.0,1.0);

						if (oN.x > 0.1) { pN = pC; obstV.y = oN.z; vMask.y = 0.0; } if (oS.x > 0.1) { pS = pC; obstV.y = oS.z; vMask.y = 0.0; } if (oE.x > 0.1) { pE = pC; obstV.x = oE.y; vMask.x = 0.0; } if (oW.x > 0.1) { pW = pC; obstV.x = oW.y; vMask.x = 0.0; }

						vec2 oldV = texture(Velocity, tex_coord).rg; vec2 grad = vec2(pE - pW, pN - pS) * GradientScale; vec2 newV = oldV - grad;

						fragColor.rg = (vMask * newV) + obstV; });
		frag = "// GLSLFluid divergence frag shader\n" + shdr_Header + frag;
		computeDivergenceShader = shCol->addCheckShaderText("fluidDiverg",
				stdVert.c_str(), frag.c_str());
		// computeDivergenceShader = shCol->addCheckShader("fluidDiverg", "shaders/fluid.vert", "shaders/fluidDiverg.frag");

	}
	else
	{
		frag =
				STRINGIFY(uniform sampler2D tex0;         // Real obstacles
						uniform sampler2D backbuffer; uniform sampler2D VelocityTexture; uniform float TimeStep; uniform float Dissipation; uniform vec2 scr; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main(){ vec2 u = texture(VelocityTexture, tex_coord).rg; vec2 coord = tex_coord * scr - TimeStep * u; fragColor = Dissipation * texture(backbuffer, coord / scr); });
		frag = "// GLSLFluid advect frag no obstacles shader\n" + shdr_Header
				+ frag;
		shader = shCol->addCheckShaderText("fluidNoObst", stdVert.c_str(),
				frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Pressure; uniform sampler2D Divergence; uniform sampler2D tex0;

						uniform float Alpha; uniform float InverseBeta; uniform vec2 step; in vec2 tex_coord; layout(location = 0) out vec4 fragColor;

						void main(){ vec4 pN = texture(Pressure, tex_coord + vec2(0.0, step.y)); vec4 pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)); vec4 pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)); vec4 pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)); vec4 pC = texture(Pressure, tex_coord); vec4 bC = texture(Divergence, tex_coord); fragColor = (pW + pE + pS + pN + Alpha * bC) * InverseBeta; });
		frag = "// GLSLFluid jacobi frag no obstacles shader\n" + shdr_Header
				+ frag;
		jacobiShader = shCol->addCheckShaderText("fluidJacobiNoObst",
				stdVert.c_str(), frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Velocity; uniform sampler2D Pressure; uniform sampler2D tex0; uniform vec2 step; uniform float GradientScale; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main(){ float pN = texture(Pressure, tex_coord + vec2(0.0, step.y)).r; float pS = texture(Pressure, tex_coord + vec2(0.0, -step.y)).r; float pE = texture(Pressure, tex_coord + vec2(step.x, 0.0)).r; float pW = texture(Pressure, tex_coord + vec2(-step.x, 0.0)).r; float pC = texture(Pressure, tex_coord).r;

						vec2 obstV = vec2(0.0,0.0); vec2 vMask = vec2(1.0,1.0); vec2 oldV = texture(Velocity, tex_coord).rg; vec2 grad = vec2(pE - pW, pN - pS) * GradientScale; vec2 newV = oldV - grad;

						fragColor.rg = (vMask * newV) + obstV; });
		frag = "// GLSLFluid subtract gradient frag no obstacles shader\n"
				+ shdr_Header + frag;
		subtractGradientShader = shCol->addCheckShaderText(
				"fluidSubtrGradNoObst", stdVert.c_str(), frag.c_str());

		//----------------------------------------------------------------------------

		frag =
				STRINGIFY(
						uniform sampler2D Velocity; uniform sampler2D tex0; uniform vec2 step; uniform float HalfInverseCellSize; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main(){ vec2 vN = texture(Velocity, tex_coord + vec2(0.0,step.y)).rg; vec2 vS = texture(Velocity, tex_coord + vec2(0.0,-step.y)).rg; vec2 vE = texture(Velocity, tex_coord + vec2(step.x,0.0)).rg; vec2 vW = texture(Velocity, tex_coord + vec2(-step.x,0.0)).rg; fragColor.r = HalfInverseCellSize * (vE.x - vW.x + vN.y - vS.y); });
		frag = "// GLSLFluid subtract gradient frag no obstacles shader\n"
				+ shdr_Header + frag;
		computeDivergenceShader = shCol->addCheckShaderText("fluidDivergNoObst",
				stdVert.c_str(), frag.c_str());
	}

	frag =
			STRINGIFY(
					uniform sampler2D tex1; uniform float pct; uniform float Radius; uniform int isVel; uniform float velThresh; uniform float asBW; uniform vec3 multCol; vec4 outCol; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main() { vec4 newFrame = texture(tex1, tex_coord); outCol = asBW > 0.0 ? vec4(multCol.rgb * vec3(abs(newFrame.r + newFrame.g + newFrame.b) *asBW), asBW) * pct : vec4(multCol.rgb * newFrame.rgb, newFrame.a) * pct; if (isVel==1) { float intens = abs(outCol.r) + abs(outCol.g); if (intens < velThresh) discard; fragColor = vec4(outCol.r, outCol.g, 0.0, 1.0);

					} else { float intens = abs(outCol.r + outCol.g + outCol.b) * 0.3333;
					//  if (intens < Radius) outCol = vec4(0);
					if (intens < Radius) discard; fragColor = outCol;
					// weiche kante
					fragColor.a *= min((intens - Radius) / (1.0 - Radius), 0.35) / 0.35; } });
	frag = "// GLSLFluid applyTexture fragment shader\n" + shdr_Header + frag;
	applyTextureShader = shCol->addCheckShaderText("fluidApplyTex",
			stdVert.c_str(), frag.c_str());

	//----------------------------------------------------------------------------

	frag =
			STRINGIFY(
					uniform vec2 Point; uniform float Radius; uniform vec3 Value; uniform int isVel; in vec2 tex_coord; layout(location = 0) out vec4 fragColor; void main(){ float d = distance(Point, tex_coord); if (d < Radius) { float a = (Radius - d) * 0.5; a = min(a, 1.0); fragColor = vec4(Value, a); } else { fragColor = vec4(0); } });
	frag = "// GLSLFluid applyImpulse fragment shader\n" + shdr_Header + frag;
	applyImpulseShader = shCol->addCheckShaderText("fluidApplyImpulse",
			stdVert.c_str(), frag.c_str());

	//----------------------------------------------------------------------------

	frag =
			STRINGIFY(
					uniform sampler2D Velocity; uniform sampler2D Temperature; uniform sampler2D Density; uniform float AmbientTemperature; uniform float TimeStep; uniform float Sigma; uniform float Kappa; uniform vec2 Gravity;

					in vec2 tex_coord; layout(location = 0) out vec4 fragColor;

					void main(){ float T = texture(Temperature, tex_coord).r; vec2 V = texture(Velocity, tex_coord).rg; fragColor.rg = V; if (T > AmbientTemperature) { float D = texture(Density, tex_coord).r; fragColor.rg += (TimeStep * (T - AmbientTemperature) * Sigma - D * Kappa ) * Gravity; } });
	frag = "// GLSLFluid applyBuoyancy fragment shader\n" + shdr_Header + frag;
	applyBuoyancyShader = shCol->addCheckShaderText("fluidApplyBuoyancy",
			stdVert.c_str(), frag.c_str());
}

//------------------------------------------------------------------------------------

void GLSLFluid::cleanUp()
{
	jacobiShader->remove();
	subtractGradientShader->remove();
	computeDivergenceShader->remove();
	applyTextureShader->remove();
	applyImpulseShader->remove();
	applyBuoyancyShader->remove();

	if (pingPong)
		pingPong->cleanUp();
	if (velocityBuffer)
		velocityBuffer->cleanUp();
	if (temperatureBuffer)
		temperatureBuffer->cleanUp();
	if (pressureBuffer)
		pressureBuffer->cleanUp();
	if (obstaclesFbo)
		obstaclesFbo->cleanUp();
	if (divergenceFbo)
		divergenceFbo->cleanUp();
}

//------------------------------------------------------------------------------------

GLSLFluid::~GLSLFluid()
{
	delete jacobiShader;
	delete subtractGradientShader;
	delete computeDivergenceShader;
	delete applyTextureShader;
	delete applyImpulseShader;
	delete applyBuoyancyShader;

	delete pingPong;
	delete velocityBuffer;
	delete temperatureBuffer;
	delete pressureBuffer;
	delete obstaclesFbo;
	delete divergenceFbo;
}
}
