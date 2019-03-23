/*
 * NTreeSortCS.h
 *
 *  Created on: May 20, 2018
 *      Author: sven
 */

#ifndef GLSL_NTREESORTCS_H_
#define GLSL_NTREESORTCS_H_

#include "Shaders/ShaderCollector.h"
#include <glm/gtx/string_cast.hpp>

namespace tav
{

class NTreeSortCS
{
public:
	NTreeSortCS(unsigned int _sortTreePow, unsigned int _bufSize,
			std::string _criterio, ShaderCollector* _shCol,
			unsigned int _work_group_size = 128);
	NTreeSortCS(unsigned int _sortTreePow, GLint _texId, int texWidth, int texHeight,
			std::string _criterio, ShaderCollector* _shCol, int texOffsetX=0, int texOffsetY=0,
			unsigned int _work_group_size = 128);
	virtual ~NTreeSortCS();

	void initFirstPassSortTreeShader();
	void initFirstPassSortTexTreeShader();
	void initSortTreeShader();

	std::string getReadParameterStr(GLuint binding, std::string prefix);
	std::string getReadBodyStr(std::string indexName, std::string outputIndName);

	void setReadUniforms(Shaders* _shader, unsigned int nrPartToEmit);

	void bindSortBuffer();
	void bindSortBufferBase(GLuint unit);

	void sort(GLuint srcBuf=0, GLuint _texId=0);

	void setSrcTexId(GLint _texId) { texId = _texId; }
	float getTreeAt(unsigned int index);

private:
	Shaders*									firstPassSortShader=0;
	Shaders*									sortShader=0;
	ShaderCollector* 							shCol;

	GLuint 										sortBuffer;
	GLint 										texId;
	GLenum										texType;

	size_t 										m_size;
	size_t 										nr_hierachies;

	bool										srcIsTex;

	unsigned int 								bufSize;
	unsigned int								sortTreePow;
	unsigned int								sortTreeBufSize;
	unsigned int								work_group_size;
	unsigned int*								writeOffsets;

	std::string									criterio;
	std::string									shdr_prefix;
	std::string									srcTexSizeName;
	std::string									srcTexOffsName;
	std::string 								fpShaderName;

	glm::ivec2									texSize2D;
	glm::ivec2									texOffs2D;
};

} /* namespace tav */

#endif /* GLSL_NTREESORTCS_H_ */
