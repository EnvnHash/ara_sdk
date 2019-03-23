//
// SNTestNiTE.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
// SNTestNiTE läuft in der KinectInput Klasse
//  Noch nicht blendfähig!!!!
//

#include "SNTestNiTE.h"

namespace tav
{
SNTestNiTE::SNTestNiTE(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    			SceneNode(_scd, _sceneArgs)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = static_cast<Quad*>(_scd->stdHFlipQuad);

	kin = static_cast<KinectInput*>(scd->kin);

	nis = kin->getNis();
	nis->setUpdateImg(true);
	nis->setUpdateSkel(true);

	shdr = shCol->getStdTex();
}

//---------------------------------------------------------------

SNTestNiTE::~SNTestNiTE()
{
	delete shdr;
	delete quad;

#ifdef HAVE_OPENNI2
	delete nis;
#endif

}

//---------------------------------------------------------------

void SNTestNiTE::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

// use internal shader
	if ( kin->isNisInited() )
	{
		if (!inited)
		{
			resImgWidth = nis->resImgWidth;
			resImgHeight = nis->resImgHeight;
			inited = true;
		}

		if (inited)
		{
			kin->uploadDepthImg(true);

			//uploadImg();

			shdr->begin();
			shdr->setUniform1i("tex", 0);
			shdr->setIdentMatrix4fv("m_pvm");
			glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, texId);
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true));
			quad->draw();

			nis->drawDebug(shCol, cp);
		}
	}
}

//---------------------------------------------------------------

void SNTestNiTE::update(double time, double dt)
{
}

//---------------------------------------------------------------

void SNTestNiTE::uploadImg()
{
	if (!texInited)
	{
		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		if (generateMips) mimapLevels = 5; else mimapLevels = 1;

		// define inmutable storage
		glTexStorage2D(GL_TEXTURE_2D,
				mimapLevels,                 // nr of mipmap levels
				GL_RGBA8,
				resImgWidth,
				resImgHeight);

		glTexSubImage2D(GL_TEXTURE_2D,              // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				resImgWidth,                 // width and height
				resImgHeight,
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				nis->getResImg());

		glBindTexture(GL_TEXTURE_2D, 0);
		texInited = true;
	}

	glBindTexture(GL_TEXTURE_2D, texId);
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // First mipmap level
			0, 0,                       // x and y offset
			resImgWidth,              // width and height
			resImgHeight,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			nis->getResImg());
}

//---------------------------------------------------------------

GLuint SNTestNiTE::getTexId()
{
	return texId;
}

}
