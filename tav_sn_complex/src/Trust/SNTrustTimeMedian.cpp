//
// SNTrustTimeMedian.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTrustTimeMedian.h"

#define STRINGIFY(A) #A


namespace tav
{
SNTrustTimeMedian::SNTrustTimeMedian(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs,"NoLight")
{
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vt = vts[ static_cast<unsigned short>(_sceneArgs->at("vtex0")) ];

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

	addPar("alpha", &alpha );
    addPar("median", &median);

    tMed = new GLSLTimeMedian(shCol, vt->getWidth(),	vt->getHeight(), GL_RGBA8);
    stdTex = shCol->getStdTexAlpha();
}

//----------------------------------------------------

void SNTrustTimeMedian::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	stdTex->begin();
	stdTex->setUniformMatrix4fv("m_pvm", vt->getTransMatPtr());
	//stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniform1i("tex", 0);
	stdTex->setUniform1f("alpha", alpha);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tMed->getResTexId() );

	quad->draw(_tfo);
}

//----------------------------------------------------

void SNTrustTimeMedian::update(double time, double dt)
{
	vt->updateDt(time, false);
	actUplTexId = vt->loadFrameToTexture();

    // if there is a new frame copy it to an opengl array buffer
	if(actUplTexId != 0 && actUplTexId != lastTexId)
	{
		tMed->setMedian( median );
		tMed->update( actUplTexId );
  	  	lastTexId = actUplTexId;
	}
}

//----------------------------------------------------

SNTrustTimeMedian::~SNTrustTimeMedian()
{
	delete quad;
	delete tMed;
}

}
