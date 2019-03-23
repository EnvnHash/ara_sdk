/*
 *  FBO.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 25.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved..
 *
 */

#pragma once

#include <utility>

#include "headers/gl_header.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"

namespace tav
{
class FBO
{
public:
	FBO(ShaderCollector* _shCol, int width, int height, int _mipMapLevels = 0);
	FBO(ShaderCollector* _shCol, int width, int height, GLenum _type,
			GLenum _target, bool _depthBuf, int _nrAttachments,
			int _mipMapLevels, int _nrSamples, GLenum _wrapMode, bool _layered);
	FBO(ShaderCollector* _shCol, int width, int height, int depth, GLenum _type,
			GLenum _target, bool _depthBuf, int _nrAttachments,
			int _mipMapLevels, int _nrSamples, GLenum _wrapMode, bool _layered);
	~FBO();
	void init();
	void bind();
	void unbind();
	void blit(int scrWidth, int scrHeigt);
	void frontFaceCW();
	void frontFaceCCW();
	void clearMultCol(float _r, float _g, float _b, float _a);
	void clearAlpha(float alpha, float col = 0.f);
	void clearToAlpha(float alpha);
	void clearToColor(float _r, float _g, float _b, float _a);
	void clearWhite();
	void clear();
	void clearDepth();
	void cleanUp();
	GLuint getColorImg();
	GLuint getColorImg(int _index);
	GLuint getDepthImg();
	GLuint getWidth();
	GLuint getHeight();
	GLuint getFbo();
	GLuint getDepth();
	GLenum getTarget() { return target; }
	void printFramebufferLimits();
	GLenum getExtType(GLenum inType);
	GLenum getFormat(GLenum inType);
	void assignTex(int attachmentNr, GLuint tex);
	void setMinFilter(GLenum _type);
	void setMagFilter(GLenum _type);
	void set3DLayer(int attachment, int offset);

private:
	bool hasDepthBuf;
	bool layered;
	GLboolean lastMultiSample;

	GLuint fbo;

	GLuint tex_width;
	GLuint tex_height;
	GLuint tex_depth;
	GLfloat f_tex_width;
	GLfloat f_tex_height;
	GLfloat f_tex_depth;

	GLint* csVp;

	GLuint* textures;
	GLuint* texBufs;
	GLuint depthBuf;
	GLenum type;
	GLenum target;
	GLenum wrapMode;

	Quad* quad;

	ShaderCollector* shCol;
	Shaders* colShader;
	Shaders* toAlphaShader;
	Shaders* clearShader;

	int mipMapLevels;
	int nrAttachments;
	int nrChan;
	int lastBoundFbo;
	int nrSamples;

	GLenum* bufModes;
	glm::vec4 clearCol;
	glm::vec4 lastClearCol;
};
}
