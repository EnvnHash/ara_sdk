//
// SNTestExtrude.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <iostream>
#include <SceneNode.h>
#include <Shaders/Shaders.h>
#include <Shaders/ShaderCollector.h>
#include <GeoPrimitives/Quad.h>

#ifdef HAVE_AUDIO
#include <PAudio.h>
#endif

#include <GLUtils/TextureBuffer.h>
#include <GLUtils/TFO.h>
#include <headers/tav_types.h>
#include <GLUtils/SSAO.h>

namespace tav
{

class SNTestExtrude : public SceneNode
{
public:
	SNTestExtrude(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTestExtrude();
	void initBaseLineShdr();
	void initMatrShdr();
	void initTex1DShdr();
	void initExtrShdr();

	void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
	void update(double time, double dt);
	void onKey(int key, int scancode, int action, int mods){};

private:

#ifdef HAVE_AUDIO
	PAudio*                     pa;
#endif

	Quad*                       quad;
	VAO*           				trigVao;
	VAO*           				baseVao;
	FBO*						matrFbo;
	FBO*						baseLineFbo;
	SSAO*						ssao;

	ShaderCollector*			shCol;
	Shaders*                    baseLineShdr;
	Shaders*                    calcMatrShdr;
	Shaders*                    extrShdr;
	Shaders*                    texShader;

	TextureBuffer*				baseFormPosBuf;
	TextureBuffer*				baseFormNormBuf;

	TextureManager*				litTex;
	TextureManager*				cubeTex;

	int                         lastBlock = -1;
	unsigned int				baseFormNrPoints;
	unsigned int				nrBaseLinePoints;
	float						baseFormRad=0.1f;
};
}
