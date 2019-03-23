/*
 * ShaderPipeline.cpp
 *
 *  Created on: May 11, 2018
 *      Author: sven
 */

#include "pch.h"
#include "ShaderPipeline.h"

namespace tav
{

ShaderPipeline::ShaderPipeline(Shaders* _shdr, GLenum shaderType)
{
	GLint status = GL_FALSE, len = 10;

	glGenProgramPipelines(1, &programPipeline);

	glBindProgramPipeline(programPipeline);
	glUseProgramStages(programPipeline, shaderType, _shdr->getProgram());

	glValidateProgramPipeline(programPipeline);
	glGetProgramPipelineiv(programPipeline, GL_VALIDATE_STATUS, &status);

	if (status != GL_TRUE)
	{
		GLint logLength;
		glGetProgramPipelineiv(programPipeline, GL_INFO_LOG_LENGTH, &logLength);
		char *log = new char[logLength];
		glGetProgramPipelineInfoLog(programPipeline, logLength, 0, log);
		printf("ShaderPipeline ERROR: Shader pipeline not valid:\n%s\n", log);
		delete[] log;
	}

	unbind();

	getGlError();
}

//------------------------------------------------------------------------------------

void ShaderPipeline::bind()
{
	glBindProgramPipeline(programPipeline);
}

//------------------------------------------------------------------------------------

void ShaderPipeline::unbind()
{
	glBindProgramPipeline(0);
}

//------------------------------------------------------------------------------------

ShaderPipeline::~ShaderPipeline()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
