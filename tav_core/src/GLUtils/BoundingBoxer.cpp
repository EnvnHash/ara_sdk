#include "pch.h"
#include "BoundingBoxer.h"

#define STRINGIFY(A) #A

namespace tav {

/**
	\brief BoundingBoxer, helper class for calculating the dimensions of SceneNodes

	begin() has to be called, then the sceneNode drawn and finally end()
	after that the results are available with getBoundMin(), getBoundMax() and getCenter()

**/

BoundingBoxer::BoundingBoxer(ShaderCollector* _shCol) :
	shCol(_shCol), boundMin(glm::vec3(0.f)), boundMax(glm::vec3(0.f)),
	center(glm::vec3(0.f)) {

	fbo = new FBO(_shCol, 6, 1, GL_R32F, GL_TEXTURE_1D,
		false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	fbo->setMinFilter(GL_NEAREST);
	fbo->setMagFilter(GL_NEAREST);

	result = new GLfloat[6];
	
	initShader();
}

//---------------------------------------------------------------

void BoundingBoxer::initShader(){
	
	std::string vert = shCol->getShaderHeader() + "// Bounding Boxer Vertex Shader \n"; 
	vert += "layout(location = 0) in vec4 position; \n"
		"uniform mat4 m_model;\n"
		"void main() { \n"
		"gl_Position = m_model * position; \n"
	"}";


	// Iterate six times over each vertex, output -x, x, -y, y, -z, z as color
	std::string geom = shCol->getShaderHeader() + "// Bounding Boxer Geometry Shader \n";
	
	geom += STRINGIFY(
		layout(triangles, invocations=1) in; \n
		layout(points, max_vertices = 18) out; \n
		out vec4 o_col;		
		void main() { \n
			float outVal;
			for (int i=0; i<gl_in.length(); i++) { \n
				for (int j=0; j<6; j++) { \n
					outVal = j == 0 ? -gl_in[i].gl_Position.x : j == 1 ? gl_in[i].gl_Position.x :  
						j == 2 ? -gl_in[i].gl_Position.y : j == 3 ? gl_in[i].gl_Position.y :
						j == 4 ? -gl_in[i].gl_Position.z : j == 5 ? gl_in[i].gl_Position.z : 0.0;
					o_col = vec4(outVal, 0.0, 0.0, 1.0);
					gl_Position = vec4( float(j) / 3.0 - 1.0 + 0.08333, 0.0, 0.0, 1.0 ); 
					EmitVertex(); 
					EndPrimitive(); \n
				}\n
			}\n 
		});


	std::string frag = shCol->getShaderHeader() + "// Bounding Boxer Fragment Shader \n"; 
	
	frag += STRINGIFY(
	layout(location=0) out vec4 fragColor;\n
	in vec4 o_col; \n
	void main() { fragColor = o_col; }
	);
	
	boxCalcShader = shCol->addCheckShaderText("BoundingBoxer", vert.c_str(), geom.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void BoundingBoxer::begin() {

	fbo->bind();
	
	glClearColor(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 
		-std::numeric_limits<float>::max(), 0.f);
	glClear(GL_COLOR_BUFFER_BIT);

	boxCalcShader->begin();
	boxCalcShader->setIdentMatrix4fv("m_model");

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendEquation(GL_MAX);
	glBlendFunc(GL_ONE, GL_ONE);

}

//---------------------------------------------------------------

void BoundingBoxer::end() {

	boxCalcShader->end();
	fbo->unbind();

	glBindTexture(GL_TEXTURE_1D, fbo->getColorImg());
	glGetTexImage(GL_TEXTURE_1D, 0, GL_RED, GL_FLOAT, (void*) &result[0]);
	
	glBindTexture(GL_TEXTURE_1D, 0);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	boundMin.x = -result[0]; boundMin.y = -result[2]; boundMin.z = -result[4];
	boundMax.x = result[1]; boundMax.y = result[3]; boundMax.z = result[5];
	
	for (int i=0; i<3; i++) {
		if (boundMin[i] >= std::numeric_limits<float>::max() || boundMin[i] <= -std::numeric_limits<float>::max()) boundMin[i] = 0.f;
		if (boundMax[i] >= std::numeric_limits<float>::max() || boundMax[i] <= -std::numeric_limits<float>::max()) boundMax[i] = 0.f;
	}

	for (unsigned int i=0;i<3;i++)
		center[i] = (-boundMin[i] + boundMax[i]) * 0.5f + boundMin[i];
}

//---------------------------------------------------------------

void BoundingBoxer::sendModelMat(GLfloat* matPtr){

	boxCalcShader->setUniformMatrix4fv("m_model", matPtr);
}

//---------------------------------------------------------------

void BoundingBoxer::draw() {

}

//---------------------------------------------------------------

glm::vec3* BoundingBoxer::getBoundMin() {
	
	return &boundMin;
}

//---------------------------------------------------------------

glm::vec3* BoundingBoxer::getBoundMax(){

	return &boundMax;
}

//---------------------------------------------------------------

glm::vec3*BoundingBoxer::getCenter() {

	return &center;
}

//---------------------------------------------------------------

Shaders* BoundingBoxer::getShader(){

	return boxCalcShader;
}

//---------------------------------------------------------------

BoundingBoxer::~BoundingBoxer(void){

	delete fbo;
	delete [] result;
	//shCol->deleteShader("BoundingBoxer");
}

}
