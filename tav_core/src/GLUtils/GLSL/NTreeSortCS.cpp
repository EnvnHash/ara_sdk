/*
 * NTreeSortCS.cpp
 *
 *  Created on: May 20, 2018
 *      Author: sven
 *
 *  N-dimensional Tree sorting opengl buffers with Compute Shaders
 *  source can be a buffer or a texture
 *
 */

#include "pch.h"
#include "NTreeSortCS.h"

#define STRINGIFY(A) #A


namespace tav
{

NTreeSortCS::NTreeSortCS(unsigned int _sortTreePow, unsigned int _bufSize, std::string _criterio,
		ShaderCollector* _shCol, unsigned int _work_group_size) :
	sortTreePow(_sortTreePow), bufSize(_bufSize), criterio(_criterio), shCol(_shCol),
	work_group_size(_work_group_size), sortTreeBufSize(0), srcIsTex(false)
{
	// init an atomic counter buffer for the sorting tree. Each index holds the number of free
	// entries in its subentries.
	// the hierarchies are saved in a linearly 1D Manner e.g. in case of sortTreePow=3 it looks like:
	// h[0] : [0]
	// h[1] : [h[0].end +1], ... [h[0].end + (sortTreePow^1)]	=> [1]  - [2]
	// h[2] : [h[1].end +1], ... [h[1].end + (sortTreePow^2)]	=> [3]  - [6]
	//              [0]
	//       [1]             [2]
	//   [3]    [4]      [5]     [6]
	// [7][8] [9][10] [11][12] [13][14]

	m_size = (int)(std::log(_bufSize) / std::log(sortTreePow));
	nr_hierachies = m_size +1;
	writeOffsets = new unsigned int[nr_hierachies];

	// check how much hierarchies we need to get down to the bottom (requested size of particles)
	int itNrHier = 0;
	while (itNrHier < nr_hierachies)
	{
		writeOffsets[itNrHier] = sortTreeBufSize;
		sortTreeBufSize += std::pow(sortTreePow, itNrHier);
		itNrHier++;
	}

	// generate two buffer, one to hold the values, a second with zeros to clear the first by copying
	glGenBuffers(1, &sortBuffer);

	// bind the buffer and define its initial storage capacity
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sortTreeBufSize * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

	// unbind the buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	initFirstPassSortTreeShader();
	initSortTreeShader();
}

//-----------------------------------------------------------------------
// 2d textures

NTreeSortCS::NTreeSortCS(unsigned int _sortTreePow, GLint _texId, int texWidth, int texHeight,
		std::string _criterio, ShaderCollector* _shCol, int texOffsetX, int texOffsetY,
		unsigned int _work_group_size):
			sortTreePow(_sortTreePow), criterio(_criterio), shCol(_shCol),
			work_group_size(_work_group_size), sortTreeBufSize(0), srcIsTex(true),
			texSize2D(glm::ivec2(texWidth, texHeight)), texOffs2D(glm::ivec2(texOffsetX, texOffsetY)),
			texType(GL_TEXTURE_2D), texId(_texId), srcTexSizeName("srcTexSize"),
			srcTexOffsName("srcTexOffs")
{
	m_size = (int)(std::log(texWidth * texHeight) / std::log(sortTreePow));
	nr_hierachies = m_size +1;

	writeOffsets = new unsigned int[nr_hierachies];

	// check how much hierarchies we need to get down to the bottom (requested size of particles)
	int itNrHier = 0;
	while (itNrHier < nr_hierachies)
	{
		writeOffsets[itNrHier] = sortTreeBufSize;
		sortTreeBufSize += std::pow(sortTreePow, itNrHier);
		itNrHier++;
	}

	// generate a buffer to hold the values
	glGenBuffers(1, &sortBuffer);

	// bind the buffer and define its initial storage capacity
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sortTreeBufSize * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

	// unbind the buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	initFirstPassSortTexTreeShader();
	initSortTreeShader();
}

//-----------------------------------------------------------------------

void NTreeSortCS::initFirstPassSortTreeShader()
{
	std::string src = "#version 430\n";
	src += "layout(local_size_x = "+std::to_string(work_group_size)+", local_size_y = 1, local_size_z = 1) in;\n";

	src += STRINGIFY(
	layout(std430, binding=1) buffer SortBuf { uint sortBuffer[]; };\n	// is bound as a vec4 anyway...
	layout(std140, binding=2) buffer Pos { vec4 pos[]; };\n
	\n
	uniform uint numProcIndexes;\n
	uniform uint writeOffset;\n
	\n
	void main(){\n
		uint i = gl_GlobalInvocationID.x;\n
		if (i >= numProcIndexes) return;\n);

	src += "if(pos[i]"+criterio+") atomicAdd( sortBuffer[writeOffset +i], 1);\n";
	src += "}";

	firstPassSortShader = shCol->addCheckShaderText("NTreeSortCS_first", src.c_str());
}

//-----------------------------------------------------------------------

void NTreeSortCS::initFirstPassSortTexTreeShader()
{
	std::string src = shCol->getShaderHeader();
	src += "layout(local_size_x = "+std::to_string(work_group_size)+", local_size_y = 1, local_size_z = 1) in;\n";
	src += "uniform ivec2 "+srcTexSizeName+";\n";
	src += "uniform ivec2 "+srcTexOffsName+";\n";


	src += STRINGIFY(
	layout(std430, binding=1) buffer SortBuf { uint sortBuffer[]; };\n	// is bound as a vec4 anyway...
	uniform sampler2D srcTex;\n
	\n
	uniform uint numProcIndexes;\n
	uniform uint writeOffset;\n
	\n
	void main(){\n
		uint i = gl_GlobalInvocationID.x;\n
		if (i >= numProcIndexes) return;\n);

	src += "if(texelFetch(srcTex, ivec2("
			"(i % "+srcTexSizeName+".x) + "+srcTexOffsName+".x, "
			" i / "+srcTexSizeName+".x + "+srcTexOffsName+".y), 0)"+criterio+") "
			"atomicAdd( sortBuffer[writeOffset +i], 1);\n"
			"}";

	fpShaderName = "NTreeSortCS_tex_first";
	firstPassSortShader = shCol->addCheckShaderText(fpShaderName, src.c_str());
}

//-----------------------------------------------------------------------

void NTreeSortCS::initSortTreeShader()
{
	std::string src = "#version 430\n";
	src += "layout(local_size_x = "+std::to_string(work_group_size)+", local_size_y = 1, local_size_z = 1) in;\n";

	src += STRINGIFY(
	layout(std430, binding=1) buffer SortBuf { uint sortBuffer[]; };\n	// is bound as a vec4 anyway...
	\n
	uniform uint numProcIndexes;\n
	uniform uint readOffset;\n
	uniform uint writeOffset;\n
	\n
	void main(){\n
		uint i = gl_GlobalInvocationID.x;\n
		if (i >= numProcIndexes) return;\n\n);

		// always sum up two indexes to one in the higher hierarchy
	src += "sortBuffer[writeOffset +i] = ";
	for (int i=0; i<sortTreePow; i++){
		src += "sortBuffer[readOffset + i*"+std::to_string(sortTreePow)+" + "+std::to_string(i)+"] ";
		if (i < sortTreePow -1) src += " + ";
	}
	src += ";\n}";

	sortShader = shCol->addCheckShaderText("NTreeSortCS_"+std::to_string(sortTreePow)+"_sort_tree", src.c_str());
}

//-----------------------------------------------------------------------

std::string NTreeSortCS::getReadParameterStr(GLuint binding, std::string prefix)
{
	shdr_prefix = prefix;
	std::string bufName = prefix;
	bufName[0] = std::toupper(bufName[0]);

	std::string par = "layout(std430, binding="+std::to_string(binding)+") buffer "+bufName+"_SortBuf { uint "+prefix+"_sortBuffer[]; };\n";
		par += "uniform uint "+prefix+"_nrPartToEmit;\n";
		par += "uniform uint "+prefix+"_nr_hierarchies;\n";
		par += "uniform uint "+prefix+"_sortTreePow;\n\n";

		// get the index position where to write to from the sort tree
		// the position in the sort tree results from the emit-index
		// walk down the sortBuffer through each hierarchy
		// at each hierarchy take a decision to which side to step down further
		par += "uint "+prefix+"GetSortTreeInd(uint index, uint nr_hierarchies, uint sortTreePow){\n";

		par += "\t uint destInd = 0;\n"
		"\t uint writeHierOffs = 0;\n"
		"\t uint act_hierachy = 0;\n"
		"\t uint act_hier_offset = 0;\n"
		"\t uint searchIndex = index;\n"
		"\t uint j;\n"

		"\t while ( "+prefix+"_sortBuffer[destInd] > searchIndex && act_hierachy < (nr_hierarchies -1) ) {\n"
				// get the index of the first entry in the sort buffer corresponding to this hierachy
				"\t\t writeHierOffs += uint( pow(sortTreePow, act_hierachy) );\n"
				// get the actual index to continue reading in this hierarchy
				"\t\t destInd = writeHierOffs + act_hier_offset;\n"
				// step down hierarchy
				"\t\t act_hierachy++;\n"
				"\t\t j=0;\n"
				"\n"
				"\t\t while ( searchIndex >= "+prefix+"_sortBuffer[destInd +j] && j < sortTreePow ){\n"
					"\t\t\t searchIndex -= "+prefix+"_sortBuffer[destInd +j];\n"
					"\t\t\t j++;\n"
				"\t\t }\n"
				"\t\t\n"
				"\t\t if (j > sortTreePow) {\n"
					"\t\t\t destInd = -1;\n"
					"\t\t\t break;\n" // if we got an invalid index break, Not enough free particles!
				"\t\t } else {\n"
					// calculate the relative offset in the hierarchy (respect to the first entry of the hierarchy
					"\t\t\t act_hier_offset = (act_hier_offset +j) * sortTreePow;\n"
					"\t\t\t destInd += j;\n"
				"\t\t }\n"
			"\t }\n"

			"\t if (destInd == -1 || act_hierachy != (nr_hierarchies-1)) return -1;\n" // if we didn't reach the lowest hierarchie, we didn't find enough free particles

			// now do the last comparison in the particle buffer itself
			"\t destInd -= writeHierOffs;\n"
			"\t return destInd;\n"
	"}\n\n";

	return par;
}

//-----------------------------------------------------------------------

std::string NTreeSortCS::getReadBodyStr(std::string indexName, std::string outputIndName)
{
	std::string body = "";
	if(!srcIsTex) body += "if ("+indexName+" >= "+shdr_prefix+"_nrPartToEmit ) return;\n";
	body += "uint "+outputIndName+" = "+shdr_prefix+"GetSortTreeInd("+indexName+", "+shdr_prefix+"_nr_hierarchies, "+shdr_prefix+"_sortTreePow);\n";
	return body;
}

//-----------------------------------------------------------------------

void NTreeSortCS::setReadUniforms(Shaders* _shader, unsigned int nrPartToEmit)
{
	_shader->setUniform1ui(shdr_prefix+"_nrPartToEmit", nrPartToEmit);
	_shader->setUniform1ui(shdr_prefix+"_nr_hierarchies", nr_hierachies);
	_shader->setUniform1ui(shdr_prefix+"_sortTreePow", sortTreePow);
}

//-----------------------------------------------------------------------

void NTreeSortCS::bindSortBuffer()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
}

//-----------------------------------------------------------------------

void NTreeSortCS::bindSortBufferBase(GLuint unit)
{
	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, unit, sortBuffer);
}

//-----------------------------------------------------------------------

void NTreeSortCS::sort(GLuint srcBuf, GLuint _texId)
{
	GLuint zero = 0;
	unsigned int nrWorkgroups = (unsigned int)std::max(std::pow((double)sortTreePow,
																(double)nr_hierachies -1.0)
														/ (double) work_group_size, 1.0);

	// if we got a different texId during runtime, take this instead of saved
	GLuint dynTexId = _texId != 0 ? _texId : texId;

/*	// debug update
	if (srcIsTex)
		std::cout << "----------------------------- actual state texId: " << dynTexId << " ---------------" << std::endl;

	if (!srcIsTex)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, srcBuf);
		glm::vec4* pptr = (glm::vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(glm::vec4) * bufSize, GL_MAP_READ_BIT);

		for (int i=0; i<bufSize; i++)
			std::cout << "pos[" << i <<  "]" << glm::to_string(pptr[i]) << std::endl;

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}
*/

	// clear buffer
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortBuffer);
	glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, 0, sortTreeBufSize * sizeof(GLuint), GL_RED, GL_UNSIGNED_INT, &zero);

	//----------------------------------------------------------
	// First Pass
	// initial run with atomic counters, count the number of entries that fullfill the criterio in the lowest hierarchy

	if (!srcIsTex) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, srcBuf);

	/*
	if (srcIsTex){
		std::cout << "inital: ";
		std::cout << " writeoffset:" << writeOffsets[nr_hierachies -1];
		std::cout << " nrProcIndexes: " << std::pow(sortTreePow, nr_hierachies-1) << std::endl;
	}
	*/

	firstPassSortShader->begin();
	firstPassSortShader->setUniform1ui("writeOffset", writeOffsets[nr_hierachies -1]);
	firstPassSortShader->setUniform1ui("numProcIndexes", std::pow(sortTreePow, nr_hierachies-1));

	if (srcIsTex)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(texType, dynTexId);
		firstPassSortShader->setUniform1i("srcTex", 0);
		firstPassSortShader->setUniform2iv(srcTexSizeName, &texSize2D[0]);
		firstPassSortShader->setUniform2iv(srcTexOffsName, &texOffs2D[0]);
	}

	glDispatchCompute(nrWorkgroups, 1, 1);

	// We need to block here on compute completion to ensure that the
	// computation is done before we continue
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

/*
	// debug atomic counters
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, sortBuffer);
	GLuint* ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint) * sortTreeBufSize, GL_MAP_READ_BIT);

	for (int i=0; i<sortTreeBufSize; i++)
		std::cout << "firstRun sortBuffer[" << i <<  "]" << ptr[i] << std::endl;

	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
*/

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	if (!srcIsTex) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0); else glBindTexture(texType, 0);

	firstPassSortShader->end();

	//------ other passes ----------------------------------------------------

	sortShader->begin();

	// bind atomic buffer as shader_storage buffer for read and write access
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sortBuffer);

	for (int i=nr_hierachies-1; i>0; i--)
	{
		/*
		if (srcIsTex){
			std::cout << "actHierarchie: " << i;
			std::cout << " readOffset:" << writeOffsets[i];
			std::cout << " writeoffset:" << writeOffsets[i-1];
			std::cout << " numProcIndexes:" << std::pow(sortTreePow, i-1) << std::endl;
		}
		*/

		sortShader->setUniform1ui("readOffset", writeOffsets[i]);
		sortShader->setUniform1ui("writeOffset", writeOffsets[i-1]);
		sortShader->setUniform1ui("numProcIndexes", std::pow(sortTreePow, i-1));

		glDispatchCompute(nrWorkgroups, 1, 1);

		// We need to block here on compute completion to ensure that the
		// computation is done before we continue
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


/*		//----------------------------------------------------------
		// debug atomic counters
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
		GLuint* ptr = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * sortTreeBufSize, GL_MAP_READ_BIT);

		for (int i=0; i<sortTreeBufSize; i++)
			std::cout << "sortBuffer[" << i <<  "]" << ptr[i] << std::endl;

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		//----------------------------------------------------------
*/
	}

	glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, 0);

	sortShader->end();

	//----------------------------------------------------------

	/*if (srcIsTex){
		// debug atomic counters
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
		GLuint* tptr = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * sortTreeBufSize, GL_MAP_READ_BIT);
		if (srcIsTex) std::cout << "tex ";
		std::cout << "sortBuffer[" << 0 <<  "]" << tptr[0] << std::endl;

		for (int i=0; i<sortTreeBufSize; i++)
		{
			if(srcIsTex) std::cout << "tex: ";
			std::cout << "tex sortBuffer[" << i <<  "]" << tptr[i] << std::endl;
		}

		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		//----------------------------------------------------------

		std::cout << " ---------- finished build sort tree ------------------------------------------------" << std::endl;
	}*/

}

//-----------------------------------------------------------------------
// float from 0 -1 in relation to total size of texture

float NTreeSortCS::getTreeAt(unsigned int index)
{
	float out=0.f;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortBuffer);
	GLuint* tptr = (GLuint*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint) * sortTreeBufSize, GL_MAP_READ_BIT);
	out = (float)tptr[index] / texSize2D.x / texSize2D.y;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	return out;
}

//-----------------------------------------------------------------------

NTreeSortCS::~NTreeSortCS()
{
	// TODO Auto-generated destructor stub
}

} /* namespace tav */
