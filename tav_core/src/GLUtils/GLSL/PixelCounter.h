/*
 * PixelCounter.h
 *
 *  Created on: Jul 10, 2018
 *      Author: sven
 */

#ifndef GLSL_PIXELCOUNTER_H_
#define GLSL_PIXELCOUNTER_H_

#include "Shaders/ShaderCollector.h"

namespace tav
{

class PixelCounter
{
public:
	PixelCounter(unsigned int texWidth, unsigned int texHeight,
		unsigned int texOffsX, unsigned int texOffsY, std::string _criterio,
		ShaderCollector* _shCol, unsigned int _work_group_size = 128);
	virtual ~PixelCounter();

	void initShader();
	void count(GLuint texId);
	float getResultFloat();
	unsigned int getResultUint();


private:
	ShaderCollector* 	shCol;
	Shaders*			countShader;

	GLuint 				initBuffer;
	GLuint 				atomicsBuffer;
	GLuint 				counted;
	GLuint 				numPixels;
	GLuint* 			pbos;

	std::string 		criterio;
	std::string 		srcTexSizeName;
	std::string 		srcTexOffsName;

	glm::ivec2			texSize;
	glm::ivec2			texOffs;

	unsigned int 		work_group_size;
	unsigned int 		nrPboBufs;
	unsigned int 		pboIndex;
	unsigned int 		nextPboIndex;

	float				countedFloat;

	bool				inited=false;
};

} /* namespace tav */

#endif /* GLSL_PIXELCOUNTER_H_ */
