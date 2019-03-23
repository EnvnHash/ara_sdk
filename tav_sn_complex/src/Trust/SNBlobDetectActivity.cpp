/*
 * SNBlobDetectActivity.cpp
 *
 *  Created on: 19.01.2017
 *      Copyright by Sven Hahne
 */

#include "SNBlobDetectActivity.h"

#define STRINGIFY(A) #A

namespace tav
{

SNBlobDetectActivity::SNBlobDetectActivity(sceneData* _scd, std::map<std::string, float>* _sceneArgs) : SceneNode(_scd, _sceneArgs)
{
	VideoTextureCvActRange** vts = static_cast<VideoTextureCvActRange**>(scd->videoTextsActRange);
	vidRange = vts[0];

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f,
            nullptr, 1, true);

	stdTex = shCol->getStdTex();
}

//----------------------------------------------------

void SNBlobDetectActivity::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//vidRange->drawPosCol();
	//vidRange->drawHistogram();
	//vidRange->drawActivityQuad();

	stdTex->begin();
	//stdTex->setIdentMatrix4fv("m_pvm");
	stdTex->setUniformMatrix4fv("m_pvm", vidRange->getTransMatPtr());

	stdTex->setUniform1i("tex", 0);
	vidRange->bindActFrame(0);

	quad->draw(_tfo);

}

//----------------------------------------------------

void SNBlobDetectActivity::update(double time, double dt)
{
	vidRange->updateDt(time, false);
}

//----------------------------------------------------

SNBlobDetectActivity::~SNBlobDetectActivity()
{
}

} /* namespace tav */
