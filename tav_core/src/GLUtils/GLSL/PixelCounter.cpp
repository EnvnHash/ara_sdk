/*
 * PixelCounter.cpp
 *
 *  Created on: Jul 10, 2018
 *      Author: sven
 */

#include "PixelCounter.h"

#define STRINGIFY(A) #A

namespace tav
{

PixelCounter::PixelCounter(unsigned int texWidth, unsigned int texHeight,
		unsigned int texOffsX, unsigned int texOffsY, std::string _criterio,
		ShaderCollector* _shCol, unsigned int _work_group_size):
	shCol(_shCol), texSize(glm::ivec2(texWidth, texHeight)), texOffs(glm::ivec2(texOffsX, texOffsY)),
	criterio(_criterio), srcTexSizeName("srcTexSize"), srcTexOffsName("srcTexOffs"),
	work_group_size(_work_group_size), numPixels(texWidth * texHeight), nrPboBufs(2),
	pboIndex(0), nextPboIndex(1), countedFloat(0.f)
{
	initShader();

	// generate  pbos for download
	pbos = new GLuint[nrPboBufs];
	glGenBuffers(nrPboBufs, pbos);
	for (int i=0;i<nrPboBufs;i++)
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[i]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(GLuint), NULL, GL_STREAM_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	// generate an atomic counter
	glGenBuffers(1, &atomicsBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicsBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// create init buffer
	glGenBuffers(1, &initBuffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, initBuffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// map the buffer, userCounters will point to the buffers data
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, initBuffer);
	GLuint* countPtr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
	                                         GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
	// set the memory to zeros, resetting the values in the buffer
	memset(countPtr, 0, sizeof(GLuint) );
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, initBuffer);


}

//-----------------------------------------------------------------------

void PixelCounter::initShader()
{
	std::string src = shCol->getShaderHeader();
	src += "layout(local_size_x = "+std::to_string(work_group_size)+", local_size_y = 1, local_size_z = 1) in;\n";
	src += "uniform ivec2 "+srcTexSizeName+";\n";
	src += "uniform ivec2 "+srcTexOffsName+";\n";

	src += STRINGIFY(
	layout(binding = 0) uniform atomic_uint count;\n
	uniform sampler2D srcTex;\n
	uniform uint numPixels;\n
	\n
	void main(){\n
		uint i = gl_GlobalInvocationID.x;\n
		if (i >= numPixels) return;\n);

	src += "if(texelFetch(srcTex, ivec2("
			"(i % "+srcTexSizeName+".x) + "+srcTexOffsName+".x,\n "
			" i / "+srcTexSizeName+".x + "+srcTexOffsName+".y), 0)"+criterio+")\n "

			"atomicCounterIncrement(count);\n"
			"}";

	countShader = shCol->addCheckShaderText("PixelCounter", src.c_str());
}

//-----------------------------------------------------------------------

void PixelCounter::count(GLuint texId)
{
	unsigned int nrWorkgroups = (unsigned int)((float)(texSize.x * texSize.y)
			/ (float) work_group_size + 0.5f);

	// ------- reset counter ---------

	glBindBuffer(GL_COPY_READ_BUFFER, initBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, atomicsBuffer);

	glCopyBufferSubData(
			GL_COPY_READ_BUFFER, 	// from
			GL_COPY_WRITE_BUFFER,		// to
			0,				// readoffs
			0,				// writeoffs
			sizeof(GLuint));

	// ------- run the shader ---------

	countShader->begin();

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicsBuffer);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texId);

	countShader->setUniform1i("srcTex", 0);
	countShader->setUniform2iv(srcTexSizeName, &texSize[0]);
	countShader->setUniform2iv(srcTexOffsName, &texOffs[0]);
	countShader->setUniform1ui("numPixels", numPixels);

	glDispatchCompute(nrWorkgroups, 1, 1);

	// We need to block here on compute completion to ensure that the
	// computation is done before we continue
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, 0);

	countShader->end();

	// ------- read result ---------

	// cpu peak here

	glBindBuffer(GL_COPY_READ_BUFFER, atomicsBuffer);
	glBindBuffer(GL_COPY_WRITE_BUFFER, pbos[pboIndex]);

	glCopyBufferSubData(
			GL_COPY_READ_BUFFER, 	// from
			GL_COPY_WRITE_BUFFER,		// to
			0,				// readoffs
			0,				// writeoffs
			sizeof(GLuint));

	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

	// map the PBO to process its data by CPU
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[pboIndex]);

	GLuint* ptr = (GLuint*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

	if (ptr) counted = *ptr;

	countedFloat = static_cast<float>(counted)
					/  static_cast<float>(texSize.x * texSize.y);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	// back to conventional pixel operation
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	// download the value via pbos to avoid cpu peaks
	pboIndex = (pboIndex + 1) % nrPboBufs;
	if (pboIndex == 0) inited = true;

}

//-----------------------------------------------------------------------

float PixelCounter::getResultFloat(){

	if (inited){
		return static_cast<float>(counted) /  static_cast<float>(texSize.x * texSize.y);
	} else {
		return 0.f;
	}
}

//-----------------------------------------------------------------------

unsigned int PixelCounter::getResultUint(){
	return counted;
}

//-----------------------------------------------------------------------

PixelCounter::~PixelCounter()
{
	glDeleteBuffers(1, &atomicsBuffer);

}

} /* namespace tav */
