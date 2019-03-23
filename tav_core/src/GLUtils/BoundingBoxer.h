#pragma once

#include "GLUtils/FBO.h"
#include "Shaders/ShaderCollector.h"
#include "SceneNode.h"

namespace tav {

class BoundingBoxer
{
public:
	BoundingBoxer(ShaderCollector* shCol);
	~BoundingBoxer(void);
	void initShader();
	void begin();
	void end();
	void sendModelMat(GLfloat* matPtr);
	void draw();

	glm::vec3* getBoundMin();
	glm::vec3* getBoundMax();
	glm::vec3* getCenter();
	Shaders* getShader();

private:
	FBO*				fbo;
	Shaders*			boxCalcShader;
	ShaderCollector*	shCol;

	glm::vec3			boundMin;
	glm::vec3			boundMax;
	glm::vec3			center;

	GLfloat*			result;

};

}
