//
// SNTestEnvironmentMapping.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestEnvironmentMapping.h"

namespace tav
{
SNTestEnvironmentMapping::SNTestEnvironmentMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), actMode(ENV_REFR)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	quad = new Quad(-0.5f, -0.5f, 1.f, 1.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on

	switch(actMode)
	{
	case ONLY_ENV:
		envShdr = shCol->addCheckShader("envMap", "shaders/basic_envMap.vert",
				"shaders/basic_envMap.frag");
		break;
	case ENV_REFR:
		envShdr = shCol->addCheckShader("envMapRefr", "shaders/basic_envMapRefr.vert",
				"shaders/basic_envMapRefr.frag");
		break;
	default:
		break;
	}

	sphere = new Sphere(1.f, 64);

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube( ((*(scd->dataPath))+"textures/skyboxsun5deg2.png").c_str() );

	bumpTex = new TextureManager();
	bumpTex->loadTexture2D( (*(scd->dataPath))+"textures/noise_p.jpg");


	cam = new GLMCamera(GLMCamera::FRUSTUM,
			scd->screenWidth, scd->screenHeight,
			-1.0f, 1.0f, -1.0f, 1.0f,                // left, right, bottom, top
			0.f, 0.f, 1.f,                           // camPos
			0.f, 0.f, 0.f,   // lookAt
			0.f, 1.f, 0.f,
			1.f, 100.f);

	cam->setModelMatr(glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -1.f)));
}


SNTestEnvironmentMapping::~SNTestEnvironmentMapping()
{
	delete quad;
}

//----------------------------------------------------

void SNTestEnvironmentMapping::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	sendStdShaderInit(_shader);


	glm::mat3 normalMatr = glm::mat3( glm::transpose( glm::inverse( cam->getModelViewMatr() ) ) );

	envShdr->begin();
	envShdr->setUniformMatrix4fv("viewMatrix", cam->getViewMatrPtr() ); // world to view transformation
	envShdr->setUniformMatrix4fv("modelMatrix", cam->getModelMatrPtr() ); // world to view transformation
	envShdr->setUniformMatrix4fv("m_pvm", cam->getMVPPtr());
	envShdr->setUniformMatrix3fv("m_normal", &normalMatr[0][0]);
	envShdr->setUniform3fv("eyePosW", &cam->getCamPos()[0]);
	envShdr->setUniform1i("cubeMap", 0);

	switch(actMode)
	{
	case ENV_REFR:
		envShdr->setUniform1i("refrMap", 0);
		envShdr->setUniform1i("decalMap", 1);
		// bumpTex->bind(1);
		// Vacuum 1.0, Air 1.0003, Water 1.3333
		// Glass 1.5, Plastic 1.5, Diamond 2.417
		envShdr->setUniform1f("etaRatio", 1.5);
		break;
	default:
		break;
	}

	cubeTex->bind(0);

	//        quad->draw(_tfo);
	sphere->draw();
}

//----------------------------------------------------

void SNTestEnvironmentMapping::update(double time, double dt)
{}
}
