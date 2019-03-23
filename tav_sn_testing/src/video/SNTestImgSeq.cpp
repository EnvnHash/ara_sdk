/*
 * SNTestImgSeq.cpp
 *
 *  Created on: Jul 23, 2018
 *      Author: sven
 */

#include "SNTestImgSeq.h"

namespace tav
{

SNTestImgSeq::SNTestImgSeq(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    SceneNode(_scd, _sceneArgs,"NoLight")
{
	quad = _scd->stdQuad;
	texShader = _scd->shaderCollector->getStdTex();

	imgSeq = new ImgSeqPlayer(((*_scd->dataPath)+"/movies/gam_mustakis/entrada").c_str(), 30);
}


//----------------------------------------------------

void SNTestImgSeq::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	imgSeq->bind(0);

	quad->draw();
}

//----------------------------------------------------

void SNTestImgSeq::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestImgSeq::bind(unsigned int texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
}

//----------------------------------------------------

SNTestImgSeq::~SNTestImgSeq()
{
	delete quad;
}


} /* namespace tav */
