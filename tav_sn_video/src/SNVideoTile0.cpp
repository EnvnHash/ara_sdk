//
// SNVideoTile0.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNVideoTile0.h"

namespace tav
{
SNVideoTile0::SNVideoTile0(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	osc = (OSCData*) scd->osc;

#ifdef HAVE_OPENCV
	VideoTextureCv** vts = static_cast<VideoTextureCv**>(scd->videoTextures);
	vt = static_cast<VideoTextureCv*>(vts[0]);
#endif

	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	std::vector < coordType > instAttribs;
	instAttribs.push_back(MODMATR);
	instAttribs.push_back(TEXCORMOD);

	nrInstances = 3;
	fNrInst = static_cast<float>(nrInstances);
	stepSize = 2.f / fNrInst;

	quad = new QuadArray(4, 4, -1.0f, -1.0f, 2.f, 2.f, 1.f, 1.f, 1.f, 1.f,
			&instAttribs, nrInstances * nrInstances);
	quad->rotate(M_PI, 1.f, 0.f, 0.f);
}

//------------------------------------------------------------------------------------

void SNVideoTile0::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	sendStdShaderInit(_shader);
	_shader->setUniform1i("useInstancing", 1);

#ifdef HAVE_OPENCV
	vt->updateDt(dt * scd->sceneSpeed * osc->videoSpeed,
			static_cast<bool>(osc->startStopVideo));
	vt->loadFrameToTexture(time);

	useTextureUnitInd(0, vt->getTex(), _shader, _tfo);
#endif

	if (!isInited)
	{
		updateModMatr(dt);
		isInited = true;
	}

	quad->drawInstanced(nrInstances * nrInstances, _tfo, 1.f);
}

//------------------------------------------------------------------------------------

void SNVideoTile0::update(double time, double dt)
{
}

//------------------------------------------------------------------------------------

void SNVideoTile0::updateModMatr(float dt)
{
	// Setup Matrices Map the buffer
	GLfloat* matrices = (GLfloat*) quad->getMapBuffer(MODMATR);

	for (auto y = 0; y < nrInstances; y++)
	{
		for (auto x = 0; x < nrInstances; x++)
		{
			int ind = y * nrInstances + x;

			// move up and down
			glm::mat4 scaleMatr = glm::scale(
					glm::vec3(1.f / fNrInst, 1.f / fNrInst, 1.f));
			glm::mat4 thisMatr = glm::translate(
					glm::vec3(
							static_cast<float>(x) * stepSize - 1.f
									+ (stepSize * 0.5f),
							static_cast<float>(y) * stepSize - 1.f
									+ (stepSize * 0.5f), 0.f));
			thisMatr = thisMatr * scaleMatr;

			for (int i = 0; i < 4; i++)
				for (int j = 0; j < 4; j++)
					matrices[ind * 16 + i * 4 + j] = thisMatr[i][j];
		}
	}

	quad->unMapBuffer();

	// make the texcormods
	GLfloat* texCorMods = (GLfloat*) quad->getMapBuffer(TEXCORMOD);
	for (auto y = 0; y < nrInstances; y++)
	{
		for (auto x = 0; x < nrInstances; x++)
		{
			int ind = y * nrInstances + x;

//            glm::vec4 texMod = glm::vec4(cp[instInd].texOffsX, cp[instInd].texOffsY,
//                                         cp[instInd].height * 0.1f, cp[instInd].width);
			glm::vec4 texMod = glm::vec4(getRandF(-1.f, 1.f),   // offsx
			getRandF(-1.f, 1.f),   // offsy
			getRandF(0.2f, 3.f),   // scale
			1.f);

			for (int i = 0; i < 4; i++)
				texCorMods[ind * 4 + i] = texMod[i];
		}
	}

	quad->unMapBuffer();
}

//------------------------------------------------------------------------------------

SNVideoTile0::~SNVideoTile0()
{
	delete quad;
}

}
