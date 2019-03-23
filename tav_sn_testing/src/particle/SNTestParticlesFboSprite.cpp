//
// SNTestParticlesFboSprite.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestParticlesFboSprite.h"

#define STRINGIFY(A) #A

using namespace glm;

namespace tav
{

SNTestParticlesFboSprite::SNTestParticlesFboSprite(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"LitSphere"), intrv(0.1)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	ps = new GLSLParticleSystemFbo(shCol, 100000,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setFriction(0.f);
	ps->setLifeTime(4.f);
	ps->setAging(true);

	data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
	data.emitVel = normalize(glm::vec3(-0.1f, 0.f, 0.f));
	data.emitCol = glm::vec4(1.f, 1.f, 0.f, 1.f);
	data.colRand = 1.f;
	data.dirRand = 2.f;
	data.speed = 0.2f;
	data.size = 20.f;
	data.sizeRand = 0.7f;
	data.angleRand = 0.2f;
	data.lifeRand = 0.2f;
	data.colInd = 1.f;
	ps->setEmitData(&data);

	initSpriteShader();
}

//----------------------------------------------------

void SNTestParticlesFboSprite::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	if(!inited)
	{
		// ps->init(_tfo);
		inited = true;
		initSpriteShader();
	}

	// draw as points
	//ps->draw( cp->mvp );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// draw as sprite
	spriteShader->begin();
	spriteShader->setUniformMatrix4fv("m_pvm", cp->mvp);
	spriteShader->setUniform2iv("cellSize", ps->getCellSizePtr(), 1);
	ps->bindTexBufs(spriteShader);
	ps->draw();
}

//----------------------------------------------------

void SNTestParticlesFboSprite::update(double time, double dt)
{
	int nrEmit = 10;

	if (time - lastTime > intrv)
	{
		ps->emit(nrEmit);
		lastTime = time;
	}

	ps->update(time);
}

//----------------------------------------------------

void SNTestParticlesFboSprite::initSpriteShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout(location = 0) in vec4 position;
	uniform mat4 m_pvm;
	void main() {
		gl_Position = position;
	});
	vert = "// SNTestParticlesFboSprite  Draw Point Sprite Shader vertex shader\n" + shdr_Header + vert;

	std::string geom = STRINGIFY(uniform sampler2D pos_tex;
	uniform sampler2D vel_tex;
	uniform sampler2D col_tex;
	uniform sampler2D aux0_tex;

	uniform ivec2 cellSize;
	uniform mat4 m_pvm;

	out vec4 fsColor;

	vec4 pPos;
	vec4 pVel;
	vec4 pCol;
	vec4 pAux0;
	ivec2 texCoord;

	void main()
	{
		ivec2 baseTexCoord = ivec2(gl_in[0].gl_Position.xy);

		// read the textures
		for (int y=0;y<cellSize.y;y++)
		{
			for (int x=0;x<cellSize.x;x++)
			{
				texCoord = baseTexCoord + ivec2(x, y);
				pPos = texelFetch(pos_tex, texCoord, 0);
				pAux0 = texelFetch(aux0_tex, texCoord, 0);

				// switching doesn´t work, since we are double buffering...
				// check if the particle is living and if it´s not the last
				// one in the cell, since this is used for offset writing
				if ( pAux0.x > 0.001 )
				{
					//pVel = texture(vel_tex, texCoord);
					pCol = texelFetch(col_tex, texCoord, 0);
					gl_PointSize = pAux0.y;

					fsColor = pCol * min(pAux0.x, 1.0);
					gl_Position = m_pvm * pPos;

					EmitVertex();
					EndPrimitive();
				}
			}
		}
	});
	geom = "// SNTestParticlesFboSprite Draw Point Sprite Shader\n" + shdr_Header
			+ "layout (points) in;\n layout (points, max_vertices = "+std::to_string(ps->getMaxGeoAmpPoints())+") out;"+ geom;

	std::string frag = STRINGIFY(// GLSLParticleSystemFBO Emit Shader
			layout (location = 0) out vec4 color;
	in vec4 fsColor;
	vec4 black = vec4(0.0);
	void main()
	{
		vec2 temp = gl_PointCoord - vec2(0.5);
		float f = dot(temp, temp);
		if (f > 0.25) discard;
		color = mix(fsColor, black, smoothstep(0.0, 0.25, f));
		color.a = (0.25 - f) * 4.0;
	});

	frag = "// SNTestParticlesFboSprite Draw Point Sprite Shader\n"
			+ shdr_Header + frag;

	spriteShader = shCol->addCheckShaderText("partFboDrawSprite", vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestParticlesFboSprite::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestParticlesFboSprite::~SNTestParticlesFboSprite()
{
	delete ps;
}

}
