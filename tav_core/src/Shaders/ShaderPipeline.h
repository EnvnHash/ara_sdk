/*
 * ShaderPipeline.h
 *
 *  Created on: May 11, 2018
 *      Author: sven
 */

#ifndef SHADERPIPELINE_H_
#define SHADERPIPELINE_H_

#include "headers/gl_header.h"
#include "Shaders/Shaders.h"

namespace tav
{

class ShaderPipeline
{
public:
	ShaderPipeline(Shaders* _shdr, GLenum shaderType);
	virtual ~ShaderPipeline();

	void bind();
	void unbind();

private:
	GLuint		programPipeline;
};

} /* namespace tav */

#endif /* SHADERPIPELINE_H_ */
