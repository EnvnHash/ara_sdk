/*
 * ThreshFbo.h
 *
 *  Created on: May 24, 2018
 *      Author: sven
 */

#ifndef THRESHFBO_H_
#define THRESHFBO_H_

#include <GLUtils/FBO.h>
#include <GeoPrimitives/Quad.h>
#include "headers/sceneData.h"

namespace tav
{

class ThreshFbo
{
public:
	ThreshFbo(sceneData* _scd, GLuint _width, GLuint _height, GLenum _type,
		GLenum _target, bool _depthBuf, int _nrAttachments, int _mipMapLevels,
		int _nrSamples, GLenum _wrapMode, bool _layered);
	virtual ~ThreshFbo();
	void initShader();
	void proc(GLenum target, GLuint texId, float threshLow, float threshHigh);
	void clear() { fbo->bind(); fbo->clear(); fbo->unbind(); }
	GLuint getResult() { return fbo->getColorImg(); }
	unsigned int getWidth() { return fbo->getWidth(); }
	unsigned int getHeight() { return fbo->getHeight(); }
public:
    FBO*					fbo;
    Quad*					quad;
	ShaderCollector*		shCol;
	Shaders*				threshShdr;

};

} /* namespace tav */

#endif /* THRESHFBO_H_ */
