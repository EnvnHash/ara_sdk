//
// SNAudioFotoPerl3D.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAudioFotoPerl3D.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{

SNAudioFotoPerl3D::SNAudioFotoPerl3D(sceneData* _scd,
		std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs, "PerlinTexCoord")
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	pa = (PAudio*) scd->pa;
	osc = (OSCData*) scd->osc;

	// setup chanCols
	getChanCols(&chanCols);

	quadAr = new QuadArray*[scd->nrChannels];
	for (short i = 0; i < scd->nrChannels; i++)
	{
		quadAr[i] = new QuadArray(80, 80, -1.f, -1.f, 2.f, 2.f, 1.f, 1.f, 1.f,
				1.f);
		quadAr[i]->scale(1.5f, 1.5f, 1.f);
		quadAr[i]->setColor(chanCols[i].r, chanCols[i].g, chanCols[i].b,
				chanCols[i].a);
	}

	numDiffTex = 4;
	img = new TextureManager*[numDiffTex];
	for (int i = 0; i < numDiffTex; i++)
		img[i] = new TextureManager();

	img[0]->loadTexture2D((*scd->dataPath) + "textures/mann.jpg");
	img[1]->loadTexture2D((*scd->dataPath) + "textures/mann2.jpg");
	img[2]->loadTexture2D((*scd->dataPath) + "textures/mann3.jpg");
	img[3]->loadTexture2D((*scd->dataPath) + "textures/mann4.jpg");

	audioTex2DNrChans = int(float(scd->nrChannels / 2) + 0.5f) * 2; // make sure it´s power of two

	noiseTex = new Noise3DTexGen(shCol, false, 4, 256, 256, 64, 4.f, 4.f, 8.f);

	addPar("alpha", &alpha);
	addPar("steady", &steady);
	addPar("planeOffs", &planeOffs);
	addPar("planeZRot", &planeZRot);
	addPar("toRad", &toRad);
}

//----------------------------------------------------

void SNAudioFotoPerl3D::draw(double time, double dt, camPar* cp,
		Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->setSceneNodeColors(chanCols);
		_tfo->disableDepthTest();

		// special assignement for PerlinTexCoord
		useTextureUnitInd(0, scd->audioTex->getTex2D(), _shader, _tfo);
		_tfo->addTexture(1, noiseTex->getTex(), GL_TEXTURE_3D, "perlinTex");
		_tfo->addTexture(2, img[0]->getId(), GL_TEXTURE_2D, "fotoTex");
	}

	glClear (GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	//glBlendFunc(GL_DST_COLOR, GL_ZERO); //Blending formula with these factors : srcColor*destColor+0
	//   glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);

	sendStdShaderInit(_shader);
	useTextureUnitInd(0, scd->audioTex->getTex2D(), _shader, _tfo);

	_shader->setUniform1i("texNr", 0);
	_shader->setUniform1f("toRad", toRad);

	glActiveTexture (GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scd->audioTex->getTex2D());

	for (short i = 0; i < scd->nrChannels; i++)
	{

		glm::mat4 offsMat = glm::rotate(_modelMat,
				float(i) / float(scd->nrChannels) * planeZRot,
				glm::vec3(0.f, 0.f, 1.f));
		offsMat = glm::translate(offsMat,
				glm::vec3(0.f, 0.f,
						-(float(i) / float(scd->nrChannels)) * planeOffs
								- 0.4f));
		glm::mat4 m_pvm = cp->projection_matrix_mat4 * cp->view_matrix_mat4
				* offsMat;

		_shader->setUniform1f("oscAlpha",
				osc->alpha * alpha
						* (steady + (1.f - steady) * pa->getMedAmp(i) * 4.f));
		_shader->setUniformMatrix4fv("modelMatrix", &offsMat[0][0]);
		_shader->setUniform1i("perlinTex", 3);
		_shader->setUniform1i("fotoTex", 4);
		_shader->setUniform1f("chanOffs",
				float(i) / float(pa->maxInputChannels)
						+ 0.5f / float(pa->maxInputChannels));

		glActiveTexture (GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_3D, noiseTex->getTex());

		glActiveTexture (GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, img[i % numDiffTex]->getId());

		quadAr[i]->draw(_tfo);
	}
}

//----------------------------------------------------

void SNAudioFotoPerl3D::update(double time, double dt)
{
	//   scd->audioTex->update(pa->getActFft(), pa->waveData.blockCounter);
	scd->audioTex->update(pa->getPll(), pa->waveData.blockCounter);
	scd->audioTex->mergeTo2D(pa->waveData.blockCounter);
}

//----------------------------------------------------

void SNAudioFotoPerl3D::onKey(int key, int scancode, int action, int mods)
{
}

//----------------------------------------------------

SNAudioFotoPerl3D::~SNAudioFotoPerl3D()
{
}

}
