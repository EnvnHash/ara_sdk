//
// SNTunnelCoke.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTunnelCoke.h"

using namespace std;

namespace tav
{

SNTunnelCoke::SNTunnelCoke(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
		SceneNode(_scd, _sceneArgs)
{
	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	rawQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

	skyBox = new SkyBoxBlend((*scd->dataPath) + "textures/coke_skybox2.jpg", this_cp->nrCams);
}

//----------------------------------------------------

void SNTunnelCoke::draw(double time, double dt, camPar* cp, Shaders* _shader,
		TFO* _tfo)
{
	if (!inited)
	{
		this_cp = cp;
		inited = true;
	}

	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	skyBox->draw(time * 0.4f, cp, _shader, _tfo);
	glClear(GL_DEPTH_BUFFER_BIT);
}

//----------------------------------------------------

void SNTunnelCoke::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTunnelCoke::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

SNTunnelCoke::~SNTunnelCoke()
{
	delete quad;
}

}
