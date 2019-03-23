/*
 *  FBO.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 25.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 *  keep things easy all atachments have the same type
 */

#include "pch.h"
#include "GLUtils/FBO.h"

namespace tav
{

FBO::FBO(ShaderCollector* _shCol, int width, int height, int _mipMapLevels) :
		tex_width(width), tex_height(height), tex_depth(1), shCol(_shCol), type(
				GL_RGBA8), target(GL_TEXTURE_2D), nrAttachments(1), hasDepthBuf(
				false), mipMapLevels(1), depthBuf(0), wrapMode(GL_REPEAT), layered(
				false), nrSamples(1)
{
	init();
}

//---------------------------------------------------------------

FBO::FBO(ShaderCollector* _shCol, int width, int height, GLenum _type,
		GLenum _target, bool _depthBuf, int _nrAttachments, int _mipMapLevels,
		int _nrSamples, GLenum _wrapMode, bool _layered) :
		tex_width(width), tex_height(height), tex_depth(1), shCol(_shCol), type(
				_type), target(_target), hasDepthBuf(_depthBuf), nrAttachments(
				_nrAttachments), mipMapLevels(_mipMapLevels), nrSamples(
				_nrSamples), depthBuf(0), wrapMode(_wrapMode), layered(_layered)
{
	init();
}

//---------------------------------------------------------------

FBO::FBO(ShaderCollector* _shCol, int width, int height, int depth,
		GLenum _type, GLenum _target, bool _depthBuf, int _nrAttachments,
		int _mipMapLevels, int _nrSamples, GLenum _wrapMode, bool _layered) :
		tex_width(width), tex_height(height), tex_depth(depth), shCol(_shCol), type(
				_type), target(_target), hasDepthBuf(_depthBuf), nrAttachments(
				_nrAttachments), mipMapLevels(_mipMapLevels), nrSamples(
				_nrSamples), depthBuf(0), wrapMode(_wrapMode), layered(_layered)
{
	init();
}

//---------------------------------------------------------------

FBO::~FBO()
{
	delete quad;
}

//---------------------------------------------------------------

void FBO::init()
{
	csVp = new GLint[4];
	for (int i = 0; i < 4; i++)
		csVp[i] = 0;

	mipMapLevels = std::max(1, mipMapLevels);
	if (target == GL_TEXTURE_RECTANGLE)
		mipMapLevels = 1;

	int maxNrAttachments;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxNrAttachments);
	if (nrAttachments > maxNrAttachments)
		printf("tav::FBO Error: trying to attach more textures to FBO than possible by hardware. Max: %d\n",
				maxNrAttachments);
	nrAttachments = std::min(nrAttachments, maxNrAttachments);

	textures = new GLuint[nrAttachments];
	bufModes = new GLenum[nrAttachments];

	f_tex_width = static_cast<float>(tex_width);
	f_tex_height = static_cast<float>(tex_height);
	f_tex_depth = static_cast<float>(tex_depth);

	std::vector<GLubyte> emptyData;
	std::vector<GLushort> emptyDataS;
	std::vector<GLuint> emptyDataI;
	std::vector<GLfloat> emptyDataF;

	switch (target)
	{
	case GL_TEXTURE_1D:
		emptyData = std::vector<GLubyte>(tex_width * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * 4, 0);
		break;
	case GL_TEXTURE_2D:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * 4, 0);
		break;
	case GL_TEXTURE_2D_MULTISAMPLE:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * 4, 0);
		break;
	case GL_TEXTURE_1D_ARRAY:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * 4, 0);
		break;
	case GL_TEXTURE_RECTANGLE:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * 4, 0);
		break;
	case GL_TEXTURE_3D:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * tex_depth * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * tex_depth * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * tex_depth * 4,0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * tex_depth * 4, 0);
		break;
	case GL_TEXTURE_2D_ARRAY:
		emptyData = std::vector<GLubyte>(tex_width * tex_height * tex_depth * 4, 0);
		emptyDataS = std::vector<GLushort>(tex_width * tex_height * tex_depth * 4, 0);
		emptyDataI = std::vector<GLuint>(tex_width * tex_height * tex_depth * 4, 0);
		emptyDataF = std::vector<GLfloat>(tex_width * tex_height * tex_depth * 4, 0);
		break;
	default:
		break;
	}

	// generate fbo textures
	for (short i = 0; i < nrAttachments; i++)
	{
		if (type != GL_DEPTH32F_STENCIL8)
			bufModes[i] = GL_COLOR_ATTACHMENT0 + i;

		glGenTextures(1, &textures[i]);
		glBindTexture(target, textures[i]);

		switch (target)
		{
		case GL_TEXTURE_1D:
			glTexStorage1D(target, mipMapLevels, type, tex_width);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage1D(target, 0, 0, tex_width, getExtType(type),
						getFormat(type), &emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage1D(target, 0, 0, tex_width, getExtType(type),
						getFormat(type), &emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage1D(target, 0, 0, tex_width, getExtType(type),
						getFormat(type), &emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage1D(target, 0, 0, tex_width, getExtType(type),
						getFormat(type), &emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage1D(target, 0, 0, tex_width, getExtType(type),
						getFormat(type), &emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		case GL_TEXTURE_2D:
			glTexStorage2D(target, mipMapLevels, type, tex_width, tex_height);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		case GL_TEXTURE_2D_MULTISAMPLE:
			glTexImage2DMultisample(target, nrSamples, type, tex_width,
					tex_height, GL_FALSE);
			break;

		case GL_TEXTURE_1D_ARRAY:
			glTexStorage2D(target, mipMapLevels, type, tex_width, tex_height);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		case GL_TEXTURE_RECTANGLE:
			glTexStorage2D(target, mipMapLevels, type, tex_width, tex_height);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
						getExtType(type), getFormat(type), &emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		case GL_TEXTURE_3D:
			glTexStorage3D(target, mipMapLevels, type, tex_width, tex_height, tex_depth);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		case GL_TEXTURE_2D_ARRAY:
			glTexStorage3D(target, mipMapLevels, type, tex_width, tex_height,
					tex_depth);
			switch (getFormat(type))
			{
			case GL_UNSIGNED_BYTE:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyData[0]);
				break;
			case GL_UNSIGNED_SHORT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataS[0]);
				break;
			case GL_UNSIGNED_INT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataI[0]);
				break;
			case GL_FLOAT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataF[0]);
				break;
			case GL_HALF_FLOAT:
				glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
						tex_depth, getExtType(type), getFormat(type),
						&emptyDataF[0]);
				break;
			default:
				break;
			}
			break;

		default:
			break;
		}

		emptyData.clear();
		emptyDataS.clear();
		emptyDataI.clear();
		emptyDataF.clear();

		if (target != GL_TEXTURE_2D_MULTISAMPLE)
		{
			if (mipMapLevels == 1)
			{
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
			{
				glGenerateMipmap(target);
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}

			if (target != GL_TEXTURE_RECTANGLE)
			{
				glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapMode);
				glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapMode);
			}
		}

		if (target == GL_TEXTURE_3D)
			glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrapMode);

		glBindTexture(target, 0);
	}

	// generate depth buffer if needed
	if (hasDepthBuf)
	{
		glGenTextures(1, &depthBuf);
		glBindTexture(target, depthBuf);

		switch (target)
		{
		case GL_TEXTURE_1D:
			glTexStorage1D(target, mipMapLevels, GL_DEPTH32F_STENCIL8,
					tex_width);
			glTexSubImage1D(target, 0, 0, tex_width, GL_DEPTH_COMPONENT,
					GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_2D:
			glTexStorage2D(target, mipMapLevels, GL_DEPTH24_STENCIL8, tex_width,
					tex_height);
//			glTexStorage2D(target, mipMapLevels, GL_DEPTH32F_STENCIL8, tex_width, tex_height);
			glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
					GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_2D_MULTISAMPLE:
			glTexStorage2DMultisample(target, nrSamples, GL_DEPTH24_STENCIL8,
					tex_width, tex_height, GL_FALSE);
//			glTexStorage2D(target, mipMapLevels, GL_DEPTH32F_STENCIL8, tex_width, tex_height);

			glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
					GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_1D_ARRAY:
			glTexStorage2D(target, mipMapLevels, GL_DEPTH24_STENCIL8, tex_width,
					tex_height);
			glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
					GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_RECTANGLE:
			glTexStorage2D(target, mipMapLevels, GL_DEPTH32F_STENCIL8,
					tex_width, tex_height);
			glTexSubImage2D(target, 0, 0, 0, tex_width, tex_height,
					GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_3D:
			glTexStorage3D(target, mipMapLevels, GL_DEPTH32F_STENCIL8,
					tex_width, tex_height, tex_depth);
			glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
					tex_depth, GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		case GL_TEXTURE_2D_ARRAY:
			glTexStorage3D(target, mipMapLevels, GL_DEPTH24_STENCIL8, tex_width,
					tex_height, tex_depth);
			glTexSubImage3D(target, 0, 0, 0, 0, tex_width, tex_height,
					tex_depth, GL_DEPTH_COMPONENT, GL_FLOAT, &emptyDataF[0]);
			break;
		}

		//glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE); // with this depth buffer doesnt work
		//glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	}

	// generate FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// Attach the texture to the fbo
	// iterate through the different types
	for (short i = 0; i < nrAttachments; i++)
	{
		switch (target)
		{
		case GL_TEXTURE_1D:
			glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textures[i], 0);
			break;
		case GL_TEXTURE_2D:
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textures[i], 0);
			break;
		case GL_TEXTURE_2D_MULTISAMPLE:
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textures[i], 0);
			break;
		case GL_TEXTURE_1D_ARRAY:
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures[i], 0);
			break;
		case GL_TEXTURE_3D:
			if (layered)
				glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textures[i], 0, i);
			else
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures[i], 0);
			break;
		case GL_TEXTURE_2D_ARRAY:
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures[i], 0);
			break;
		case GL_TEXTURE_RECTANGLE:
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, textures[i], 0);
			break;
		}
	}

	if (hasDepthBuf)
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				depthBuf, 0);

	/*
	 // if there is a depth buffer also generate a stencil buffer
	 if (genBuf[GL_DEPTH32F_STENCIL8] && !genBuf[StencilBuf])
	 {
	 glGenRenderbuffers(1, &textures[StencilBuf]);
	 glBindRenderbuffer(GL_RENDERBUFFER, textures[StencilBuf]);
	 glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX, tex_width, tex_height);
	 glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL, GL_RENDERBUFFER, textures[ColorBuf]);
	 }

	 // generate depth and stencil renderbuffers
	 glGenRenderbuffersEXT(1, &textures[GL_DEPTH32F_STENCIL8]);
	 glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, textures[GL_DEPTH32F_STENCIL8]);
	 glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,GL_DEPTH24_STENCIL8, tex_width, tex_height);
	 glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, textures[GL_DEPTH32F_STENCIL8]);
	 glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, textures[GL_DEPTH32F_STENCIL8]);
	 */

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if ( GL_FRAMEBUFFER_COMPLETE != status)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			std::cout << "FBO Error: Attachment Point Unconnected" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			std::cout << "FBO Error: Missing Attachment" << std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			std::cout << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"
					<< std::endl;
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			std::cout << "FBO Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"
					<< std::endl;
			break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
			//                    std::cout << "Framebuffer Object %d Error: Dimensions do not match" << std::endl;
			//                    break;
			//                case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
			//                    std::cout << "Framebuffer Object %d Error: Formats" << std::endl;
			//                    break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			std::cout
					<< "Framebuffer Object %d Error: Unsupported Framebuffer Configuration"
					<< std::endl;
			break;
		default:
			break;
		}

	}

	// clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// vertexbuffer für den texture quad beim alphaClear machen
	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 0.f);

	colShader = shCol->getStdCol();
	clearShader = shCol->getStdClear(layered, tex_depth);
	toAlphaShader = shCol->getStdTex();
}

//---------------------------------------------------------------

void FBO::assignTex(int attachmentNr, GLuint tex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentNr,
			target, tex, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//---------------------------------------------------------------

void FBO::bind()
{
	// hier muss hin, wenn FBO gebunden ist, der nicht 0, speicher diesen
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	glGetBooleanv(GL_MULTISAMPLE, &lastMultiSample);
	//glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);

	//printf ("last bound fbo: %d \n", lastBoundFbo);

	glGetIntegerv(GL_VIEWPORT, csVp);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, tex_width, tex_height);
	glViewportIndexedf(0, 0, 0, f_tex_width, f_tex_height);
	glScissor(0, 0, tex_width, tex_height); // wichtig!!!

	if (target == GL_TEXTURE_2D_MULTISAMPLE)
	{
		glEnable(GL_MULTISAMPLE);
		//glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	}

	if (hasDepthBuf && nrAttachments == 0)
	{
		glDrawBuffer(GL_NONE);
	}
	else if (nrAttachments > 1)
	{
		glDrawBuffers(nrAttachments, bufModes);
	}
	//printf (" now binding fbo nr: %d \n", fbo);
}

//---------------------------------------------------------------

void FBO::unbind()
{
	//glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &lastBoundFbo);
	//glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);

	//printf ("FBO:unbind rebinding: %d \n", lastBoundFbo);

	glBindFramebuffer(GL_FRAMEBUFFER, lastBoundFbo);

	if (target == GL_TEXTURE_2D_MULTISAMPLE)
		if (!lastMultiSample)
		{
			glDisable(GL_MULTISAMPLE);
			//glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		}

	glViewport(csVp[0], csVp[1], csVp[2], csVp[3]);
	glViewportIndexedf(0, csVp[0], csVp[1], csVp[2], csVp[3]);
	glScissor(csVp[0], csVp[1], csVp[2], csVp[3]); // wichtig!!!

	if (hasDepthBuf && nrAttachments == 0)
		glDrawBuffer(GL_BACK);

	// if using mimaplevels, generate new mipmaps
	if (mipMapLevels > 1)
	{
		for (short i = 0; i < nrAttachments; i++)
		{
			glBindTexture(target, textures[i]);
			glGenerateMipmap(target);
			glBindTexture(target, 0);
		}
	}
}

//---------------------------------------------------------------
// OpenGLES 3.0
void FBO::blit(int scrWidth, int scrHeigt)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, tex_width, tex_height, 0, 0, scrWidth, scrHeigt,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//---------------------------------------------------------------

void FBO::clearMultCol(float _r, float _g, float _b, float _a)
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);	// multiply src and dst colors

	if ( lastClearCol.r != _r || lastClearCol.g != _g || lastClearCol.b != _b || lastClearCol.a != _a){
		quad->setColor(_r, _g, _b, _a);
		lastClearCol = glm::vec4(_r, _g, _b, _a);
	}

	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");
	quad->draw();
	colShader->end();

	if (hasDepthBuf)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clearAlpha(float alpha, float col)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	clearCol = glm::vec4(col, col, col, 1.f - alpha);

	clearShader->begin();
	clearShader->setIdentMatrix4fv("m_pvm");
	clearShader->setUniform4fv("clearCol", &clearCol[0]);
	quad->draw();
	clearShader->end();

	if (hasDepthBuf)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clearToAlpha(float alpha)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	toAlphaShader->begin();
	toAlphaShader->setIdentMatrix4fv("m_pvm");
	toAlphaShader->setUniform1f("alpha", alpha);
	quad->draw();
	toAlphaShader->end();

	if (hasDepthBuf || type == GL_DEPTH32F_STENCIL8)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clearToColor(float _r, float _g, float _b, float _a)
{
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);

	//glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	quad->setColor(_r, _g, _b, _a);

	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");
	quad->draw();
	colShader->end();

	if (hasDepthBuf)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clear()
{
	if (nrAttachments > 0 && type != GL_DEPTH32F_STENCIL8)
	{
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	if (hasDepthBuf || type == GL_DEPTH32F_STENCIL8)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clearDepth()
{
	if (hasDepthBuf || type == GL_DEPTH32F_STENCIL8)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::clearWhite()
{
	if (nrAttachments > 0 && type != GL_DEPTH32F_STENCIL8)
	{
		glClearColor(1.0, 1.0, 1.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	if (hasDepthBuf || type == GL_DEPTH32F_STENCIL8)
	{
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearDepth(1.0f);
	}
}

//---------------------------------------------------------------

void FBO::cleanUp()
{
	// delete framebuffers
	glDeleteFramebuffers(1, &fbo);
}

//---------------------------------------------------------------

GLuint FBO::getColorImg()
{
	GLuint tex = 0;

	if (nrAttachments > 0 && type != GL_DEPTH32F_STENCIL8)
		tex = textures[0];

	return tex;
}

//---------------------------------------------------------------

GLuint FBO::getColorImg(int _index)
{
	GLuint tex = 0;

	if (nrAttachments > 0 && type != GL_DEPTH32F_STENCIL8)
		tex = textures[_index];

	return tex;
}

//---------------------------------------------------------------

GLuint FBO::getDepthImg()
{
	GLuint tex = 0;
	if (hasDepthBuf)
		tex = depthBuf;
	return tex;
}

//---------------------------------------------------------------

GLuint FBO::getWidth()
{
	return tex_width;
}

//---------------------------------------------------------------

GLuint FBO::getHeight()
{
	return tex_height;
}

//---------------------------------------------------------------

GLuint FBO::getFbo()
{
	return fbo;
}

//---------------------------------------------------------------

GLuint FBO::getDepth()
{
	return tex_depth;
}

//---------------------------------------------------------------

void FBO::setMinFilter(GLenum _type)
{
	for (short i = 0; i < nrAttachments; i++)
	{
		glBindTexture(target, textures[i]);
		glTexParameteri(target, GL_TEXTURE_MIN_FILTER, _type);
	}
}

//---------------------------------------------------------------

void FBO::setMagFilter(GLenum _type)
{
	for (short i = 0; i < nrAttachments; i++)
	{
		glBindTexture(target, textures[i]);
		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, _type);
	}
}

//---------------------------------------------------------------

void FBO::set3DLayer(int attachment, int offset)
{
	if (target == GL_TEXTURE_3D && (unsigned int)offset < tex_depth)
	{
		//glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+attachment, textures[attachment], 0, offset);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glFramebufferTexture3D(GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + attachment, target, textures[attachment],
				0, offset);

		if (hasDepthBuf)
		{
			glFramebufferTexture3D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, target, depthBuf, 0, offset);
			glFramebufferTexture3D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, target, depthBuf, 0, offset);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
}

//---------------------------------------------------------------

void FBO::printFramebufferLimits()
{
	int res;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &res);
	printf("Max Color Attachments: %d\n", res);

	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &res);
	printf("Max Framebuffer width: %d, height: %d\n", res, res);
}

//---------------------------------------------------------------

GLenum FBO::getExtType(GLenum inType)
{
	GLenum extType = 0;

	switch (inType)
	{
	case GL_R8:
		extType = GL_RED;
		break;
	case GL_R8_SNORM:
		extType = GL_RED;
		break;
	case GL_R16:
		extType = GL_RED;
		break;
	case GL_R16_SNORM:
		extType = GL_RED;
		break;
	case GL_RG8:
		extType = GL_RG;
		break;
	case GL_RG8_SNORM:
		extType = GL_RG;
		break;
	case GL_RG16:
		extType = GL_RG;
		break;
	case GL_RG16_SNORM:
		extType = GL_RG;
		break;
	case GL_R3_G3_B2:
		extType = GL_RGB;
		break;
	case GL_RGB4:
		extType = GL_RGB;
		break;
	case GL_RGB5:
		extType = GL_RGB;
		break;
	case GL_RGB565:
		extType = GL_RGB;
		break;
	case GL_RGB8:
		extType = GL_RGB;
		break;
	case GL_RGB8_SNORM:
		extType = GL_RGB;
		break;
	case GL_RGB10:
		extType = GL_RGB;
		break;
	case GL_RGB12:
		extType = GL_RGB;
		break;
	case GL_RGB16:
		extType = GL_RGB;
		break;
	case GL_RGB16_SNORM:
		extType = GL_RGB;
		break;
	case GL_RGBA2:
		extType = GL_RGBA;
		break;
	case GL_RGBA4:
		extType = GL_RGBA;
		break;
	case GL_RGB5_A1:
		extType = GL_RGBA;
		break;
	case GL_RGBA8:
		extType = GL_BGRA;
		break;
	case GL_RGBA8_SNORM:
		extType = GL_RGBA;
		break;
	case GL_RGB10_A2:
		extType = GL_RGBA;
		break;
	case GL_RGB10_A2UI:
		extType = GL_RGBA;
		break;
	case GL_RGBA12:
		extType = GL_RGBA;
		break;
	case GL_RGBA16:
		extType = GL_RGBA;
		break;
	case GL_RGBA16_SNORM:
		extType = GL_RGBA;
		break;
	case GL_SRGB8:
		extType = GL_RGB;
		break;
	case GL_SRGB8_ALPHA8:
		extType = GL_RGBA;
		break;
	case GL_R16F:
		extType = GL_RED;
		break;
	case GL_RG16F:
		extType = GL_RG;
		break;
	case GL_RGB16F:
		extType = GL_RGB;
		break;
	case GL_RGBA16F:
		extType = GL_RGBA;
		break;
	case GL_R32F:
		extType = GL_RED;
		break;
	case GL_RG32F:
		extType = GL_RG;
		break;
	case GL_RGB32F:
		extType = GL_RGB;
		break;
	case GL_RGBA32F:
		extType = GL_RGBA;
		break;
	case GL_R11F_G11F_B10F:
		extType = GL_RGB;
		break;
	case GL_RGB9_E5:
		extType = GL_RGB;
		break;
	case GL_R8I:
		extType = GL_RED_INTEGER;
		break;
	case GL_R8UI:
		extType = GL_RED_INTEGER;
		break;
	case GL_R16I:
		extType = GL_RED_INTEGER;
		break;
	case GL_R16UI:
		extType = GL_RED_INTEGER;
		break;
	case GL_R32I:
		extType = GL_RED_INTEGER;
		break;
	case GL_R32UI:
		extType = GL_RED_INTEGER;
		break;
	case GL_RG8I:
		extType = GL_RG_INTEGER;
		break;
	case GL_RG8UI:
		extType = GL_RG_INTEGER;
		break;
	case GL_RG16I:
		extType = GL_RG_INTEGER;
		break;
	case GL_RG16UI:
		extType = GL_RG_INTEGER;
		break;
	case GL_RG32I:
		extType = GL_RG_INTEGER;
		break;
	case GL_RG32UI:
		extType = GL_RG_INTEGER;
		break;
	case GL_RGB8I:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGB8UI:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGB16I:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGB16UI:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGB32I:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGB32UI:
		extType = GL_RGB_INTEGER;
		break;
	case GL_RGBA8I:
		extType = GL_RGBA_INTEGER;
		break;
	case GL_RGBA8UI:
		extType = GL_RGBA_INTEGER;
		break;
	case GL_RGBA16I:
		extType = GL_RGBA_INTEGER;
		break;
	case GL_RGBA16UI:
		extType = GL_RGBA_INTEGER;
		break;
	case GL_RGBA32I:
		extType = GL_RGBA_INTEGER;
		break;
	case GL_RGBA32UI:
		extType = GL_RGBA_INTEGER;
		break;
	}

	return extType;
}

GLenum FBO::getFormat(GLenum inType)
{
	GLenum format = 0;

	switch (inType)
	{
	case GL_R8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_R8_SNORM:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_R16:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_R16_SNORM:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RG8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RG8_SNORM:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RG16:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RG16_SNORM:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_R3_G3_B2:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB4:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB5:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB565:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB8_SNORM:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB10:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB12:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB16:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RGB16_SNORM:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RGBA2:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA4:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB5_A1:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA8_SNORM:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB10_A2:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB10_A2UI:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA12:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA16:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RGBA16_SNORM:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_SRGB8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_SRGB8_ALPHA8:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_R16F:
		format = GL_HALF_FLOAT;
		break;
	case GL_RG16F:
		format = GL_HALF_FLOAT;
		break;
	case GL_RGB16F:
		format = GL_HALF_FLOAT;
		break;
	case GL_RGBA16F:
		format = GL_HALF_FLOAT;
		break;
	case GL_R32F:
		format = GL_FLOAT;
		break;
	case GL_RG32F:
		format = GL_FLOAT;
		break;
	case GL_RGB32F:
		format = GL_FLOAT;
		break;
	case GL_RGBA32F:
		format = GL_FLOAT;
		break;
	case GL_R11F_G11F_B10F:
		format = GL_FLOAT;
		break;
	case GL_RGB9_E5:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_R8I:
		format = GL_BYTE;
		break;
	case GL_R8UI:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_R16I:
		format = GL_SHORT;
		break;
	case GL_R16UI:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_R32I:
		format = GL_INT;
		break;
	case GL_R32UI:
		format = GL_UNSIGNED_INT;
		break;
	case GL_RG8I:
		format = GL_BYTE;
		break;
	case GL_RG8UI:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RG16I:
		format = GL_SHORT;
		break;
	case GL_RG16UI:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RG32I:
		format = GL_INT;
		break;
	case GL_RG32UI:
		format = GL_UNSIGNED_INT;
		break;
	case GL_RGB8I:
		format = GL_BYTE;
		break;
	case GL_RGB8UI:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGB16I:
		format = GL_SHORT;
		break;
	case GL_RGB16UI:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RGB32I:
		format = GL_INT;
		break;
	case GL_RGB32UI:
		format = GL_UNSIGNED_INT;
		break;
	case GL_RGBA8I:
		format = GL_BYTE;
		break;
	case GL_RGBA8UI:
		format = GL_UNSIGNED_BYTE;
		break;
	case GL_RGBA16I:
		format = GL_SHORT;
		break;
	case GL_RGBA16UI:
		format = GL_UNSIGNED_SHORT;
		break;
	case GL_RGBA32I:
		format = GL_INT;
		break;
	case GL_RGBA32UI:
		format = GL_UNSIGNED_INT;
		break;
	}

	return format;
}
}
