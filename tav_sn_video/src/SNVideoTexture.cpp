//
// SNVideoTexture.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVideoTexture.h"

namespace tav
{
SNVideoTexture::SNVideoTexture(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
#ifdef HAVE_OPENCV
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);

	//tex0 = texs[ static_cast<GLuint>(_sceneArgs->at("tex0")) ];

	//vt = static_cast<VideoTextureCv*>(vts[0]);
	vt = vts[ static_cast<GLuint>(_sceneArgs->at("vtex0")) ];
#endif
	stdTex = shCol->getStdTexAlpha();

	//quad = new QuadArray(20, 20, -1.0f, -1.0f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f);
	//quad->rotate(M_PI, 1.f, 0.f, 0.f);


	alpha = 1.f;
	addPar("alpha", &alpha);
}

//------------------------------------------

void SNVideoTexture::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
//        useTextureUnitInd(0, vt->texIDs[0], _shader, _tfo);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);

	alpha = 1.f;
	stdTex->setUniform1f("alpha", alpha);

	glActiveTexture (GL_TEXTURE0);

#ifdef HAVE_OPENCV
	vt->bindActFrame(0);
#endif

	scd->stdHFlipQuad->draw();
	//quad->draw(_tfo);

	_shader->begin();
}

//------------------------------------------

void SNVideoTexture::update(double time, double dt)
{
#ifdef HAVE_OPENCV
	vt->updateDt(time, false);
	vt->loadFrameToTexture(time);
#endif
}

//------------------------------------------

SNVideoTexture::~SNVideoTexture()
{
	delete quad;
}
}
