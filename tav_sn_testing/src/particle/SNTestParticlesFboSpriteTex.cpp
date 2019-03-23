//
// SNTestParticlesFboSpriteTex.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestParticlesFboSpriteTex.h"

#define STRINGIFY(A) #A

using namespace glm;
namespace tav
{
SNTestParticlesFboSpriteTex::SNTestParticlesFboSpriteTex(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs,"LitSphere"), intrv(0.5), texElementsPerSide(4)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	ps = new GLSLParticleSystemFbo(shCol, 200000,
			float(_scd->screenWidth) / float(_scd->screenHeight));
	ps->setFriction(0.f);
	ps->setLifeTime(8.f);
	ps->setAging(true);

	data.emitVel = normalize(glm::vec3(0.f, 1.f, 0.f));
	data.emitCol = glm::vec4(1.f, 0.f, 1.f, 1.f);
	data.posRand = 0.01f; // wenn emit per textur und > 0 -> mehr gpu
	data.dirRand = 2.f;
	data.speed = 0.01f;
	data.angle = 0.3f;
	data.angleRand = 2.f;
	data.size = 10.f;
	data.sizeRand = 1.f;
	data.texNr = 1;
	data.maxNrTex = texElementsPerSide * texElementsPerSide;
	data.colRand = 0.4f;


	ps->setEmitData(&data);

	quad = _scd->stdQuad;

	// light settings
	diffuse = glm::vec4(0.8f, 1.f, 0.8f, 1.f);
	specular = glm::vec4(0.8f);
	lightPos = glm::vec3(1.f, 1.f, 1.f);

	texNrElemStep = 1.f / float(texElementsPerSide);
	elemPerSide = float(texElementsPerSide);

	// groesse der Textur macht sich EXTREM in den fps bemerkbar!!!!
	partTex = new TextureManager();
	partTex->loadTexture2D((*scd->dataPath)+"textures/marriott/stars_one.png");

	partTexNorm = new TextureManager();
	partTexNorm->loadTexture2D((*scd->dataPath)+"textures/marriott/stars_one_hm.tif");

	litsphereTexture = new TextureManager();
	litsphereTexture->loadTexture2D((*scd->dataPath)+"textures/marriott/droplet_01.png");

	emitTex = new TextureManager();
	emitTex->loadTexture2D((*_scd->dataPath)+"/textures/test_emit.png");

	stdTex = _scd->shaderCollector->getStdTex();
	initSpriteShader();
}

//----------------------------------------------------

void SNTestParticlesFboSpriteTex::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE);
	}

	if(!spriteShader) initSpriteShader();

	glEnable(GL_BLEND);
	glEnable(GL_POINT_SPRITE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	// draw as sprite
	spriteShader->begin();
	spriteShader->setUniformMatrix4fv("m_pvm", cp->mvp);
	spriteShader->setUniform2iv("cellSize", ps->getCellSizePtr(), 1);
	spriteShader->setUniform1f("elemStep", texNrElemStep);
	spriteShader->setUniform1f("elemPerSide", elemPerSide);
	spriteShader->setUniform1i("tex", ps->getNrAttachments()); // = 4

	partTex->bind(ps->getNrAttachments());

	ps->bindTexBufs(spriteShader);
	ps->draw();

}

//----------------------------------------------------

void SNTestParticlesFboSpriteTex::update(double time, double dt)
{
	int nrEmit = 100;

	if (time - lastTime > intrv)
	{
		//ps->emit(nrEmit);
		ps->emit(nrEmit, emitTex->getId(), emitTex->getWidth(), emitTex->getHeight());
		lastTime = time;
	}

	ps->update(time);
}

//----------------------------------------------------

void SNTestParticlesFboSpriteTex::initSpriteShader()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout(location = 0) in vec4 position;
	uniform mat4 m_pvm;
	void main() {
		gl_Position = position;
	});

	vert = "// SNTestParticlesFboSpriteTex  Draw Point Sprite Shader vertex shader\n" + shdr_Header + vert;


	//--------------------------------------------

	std::string geom = STRINGIFY(
			uniform sampler2D pos_tex;
	uniform sampler2D vel_tex;
	uniform sampler2D col_tex;
	uniform sampler2D aux0_tex;

	uniform ivec2 cellSize;
	uniform mat4 m_pvm;

	out GS_VS_DATA {
		vec4 pos;
		vec4 color;
		vec4 aux0;
	} outData;

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
				pAux0 = texelFetch(aux0_tex, texCoord, 0);

				// switching doesn´t work, since we are double buffering...
				// check if the particle is living and if it´s not the last
				// one in the cell, since this is used for offset writing
				if ( pAux0.x > 0.001 )
				{
					pPos = texelFetch(pos_tex, texCoord, 0);
					//pVel = texture(vel_tex, texCoord);
					pCol = texelFetch(col_tex, texCoord, 0);
					gl_PointSize = pAux0.y;

					outData.color = pCol * min(pAux0.x, 1.0);
					outData.aux0 = pAux0;
					outData.pos = pPos;
					gl_Position = m_pvm * pPos;

					EmitVertex();
					EndPrimitive();
				}
			}
		}
	});

	geom = "// SNTestParticlesFboSpriteTex Draw Point Sprite Shader\n" + shdr_Header
			+ "layout (points) in;\n layout (points, max_vertices = "+std::to_string(ps->getMaxGeoAmpPoints())+") out;"+ geom;

	//---------------------------------

	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	uniform sampler2D tex;
	uniform float elemStep;
	uniform float elemPerSide;

	in GS_VS_DATA {
		vec4 pos;
		vec4 color;
		vec4 aux0;
	} inData;

	vec4 procColor;
	vec4 normal;
	vec2 texCoord;

	void main()
	{
		// apply rotation

		// center
		//texCoord = gl_PointCoord - vec2(0.5);

		/*
		// convert to angle, radius
		texCoord = vec2(atan(texCoord.y, texCoord.x),
				sqrt(texCoord.x * texCoord.x + texCoord.y * texCoord.y));

		// rotate
		texCoord.x += inData.aux0.z;

		// convert back to x, y and move to normalized space
		texCoord = vec2(cos(texCoord.x) * texCoord.y + 0.5,
				sin(texCoord.x) * texCoord.y + 0.5);

		// apply texSelection
		texCoord = texCoord * elemStep
				+ vec2(elemStep * mod(inData.aux0.w, elemPerSide),
						elemStep * floor(inData.aux0.w / elemPerSide));
*/
		texCoord = vec2(elemStep * mod(inData.aux0.w, elemPerSide),
						elemStep * floor(inData.aux0.w / elemPerSide));

		// get Bump Map
		//procColor = texture(tex, texCoord);
		procColor = vec4(texCoord.xy, 0.0, 1.0);
		//* inData.color;

		color = procColor;
//		color = vec4(1.0);
		//color.a *= 0.5;
	});

	frag = "// SNTestParticlesFboSpriteTex Draw Point Sprite Shader\n" + shdr_Header + frag;

	spriteShader = shCol->addCheckShaderText("testPartFboDrawSpriteTex", vert.c_str(), geom.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTestParticlesFboSpriteTex::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTestParticlesFboSpriteTex::~SNTestParticlesFboSpriteTex()
{
	delete emitTex;
	delete litsphereTexture;
	delete partTex;
	delete partTexNorm;
	delete ps;
}

}
