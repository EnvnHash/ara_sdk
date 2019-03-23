//
// SNAutumnLeaves.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAutumnLeaves.h"

using namespace std;

namespace tav
{
SNAutumnLeaves::SNAutumnLeaves(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrPartLeaves(400),
			flWidth(200), flHeight(200)
{
	kin = static_cast<KinectInput*>(scd->kin);
	shCol = static_cast<ShaderCollector*>(shCol);

	TextureManager** texs_unsorted = static_cast<TextureManager**>(scd->texObjs);
	texs = new TextureManager*[MAX_NUM_SIM_TEXS];
	for (int i=0;i<MAX_NUM_SIM_TEXS;i++)
		texs[i] = texs[ static_cast<GLuint>(_sceneArgs->at("tex"+std::to_string(i))) ];

	nrTestPart = nrPartLeaves;
	nis = kin->getNis();

	backTex = new TextureManager();
	backTex->loadFromFile((*scd->dataPath)+"/textures/p8-h1.jpg", GL_TEXTURE_2D, 9);

	ps = new GLSLParticleSystem(shCol, nrTestPart, scd->screenWidth, scd->screenHeight);
	ps->setFriction(0.6f);
	ps->setLifeTime(2.f);
	ps->setAging(false);
	ps->setAgeFading(false);
	ps->setAgeSizing(false);
	ps->setVelTexAngleStepSize(1.f / static_cast<float>(flWidth));
	ps->setReturnToOrg(true, 0.005f);
	ps->init();

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.95f;
	fluidSim->velocityDissipation = 0.92f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.3f,
			static_cast<float>(flHeight) * 0.3f);


	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();

	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);

	nrForceJoints = 15;
	oldJointPos = new glm::vec2[nrForceJoints];

	forceJoints = new nite::JointType[nrForceJoints];
	forceJoints[0] = nite::JOINT_HEAD;
	forceJoints[1] = nite::JOINT_NECK;

	forceJoints[2] = nite::JOINT_LEFT_SHOULDER;
	forceJoints[3] = nite::JOINT_RIGHT_SHOULDER;
	forceJoints[4] = nite::JOINT_LEFT_ELBOW;
	forceJoints[5] = nite::JOINT_RIGHT_ELBOW;
	forceJoints[6] = nite::JOINT_LEFT_HAND;
	forceJoints[7] = nite::JOINT_RIGHT_HAND;

	forceJoints[8] = nite::JOINT_TORSO;

	forceJoints[9] = nite::JOINT_LEFT_HIP;
	forceJoints[10] = nite::JOINT_RIGHT_HIP;
	forceJoints[11] = nite::JOINT_LEFT_KNEE;
	forceJoints[12] = nite::JOINT_RIGHT_KNEE;
	forceJoints[13] = nite::JOINT_LEFT_FOOT;
	forceJoints[14] = nite::JOINT_RIGHT_FOOT;
}

//----------------------------------------------------

SNAutumnLeaves::~SNAutumnLeaves()
{
	delete ps;
	delete fluidSim;
}

//----------------------------------------------------

void SNAutumnLeaves::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_DEPTH_BOUNDS_TEST_EXT);     // is needed for drawing cubes
	glDisable(GL_CULL_FACE);     // is needed for drawing cubes
	glDisable(GL_STENCIL_TEST);     // is needed for drawing cubes

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backTex->getId());
	quad->draw();

	ps->bindStdShader(GLSLParticleSystem::QUADS);
	for (auto i=0;i<MAX_NUM_SIM_TEXS;i++)
	{
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D, texs[i]->getId());
	}

	ps->draw();

	// restart the actual light shader
	//        _shader->begin();
	//        sendStdShaderInit(_shader);

	if (_tfo)
	{
		glEnable(GL_RASTERIZER_DISCARD);
		_shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//----------------------------------------------------

void SNAutumnLeaves::update(double time, double dt)
{
	if(!inited)
	{
		data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
		data.posRand = 1.f;
		data.emitVel = glm::vec3(0.0f, 1.f, 0.f);
		data.speed = 0.f;
		data.size = 0.06f;
		data.sizeRand = 0.06f;
		data.texUnit = 1;
		data.texRand = 1.f;
		data.angleRand = 1.f;
		data.colInd = 1.f;

		ps->emit(nrTestPart, data, true);

		inited = true;
	}

	// get new position
	if (nis->isInited())
	{
		const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
		for (int i=0; i<users.getSize(); ++i)
		{
			const nite::UserData& user = users[i];

			if (user.isVisible())
			{
				for (int jNr=0;jNr<nrForceJoints;jNr++)
				{
					glm::vec2 m = glm::vec2((nis->skelData->getJoint(i, forceJoints[jNr]).x * 0.5f + 0.5f) * flWidth,
							((nis->skelData->getJoint(i, forceJoints[jNr]).y * 0.5f + 0.5f)) * flHeight);
					glm::vec2 d = (m - oldJointPos[jNr]) * 1.f;
					glm::vec2 c = glm::normalize(forceScale - m);

					fluidSim->addTemporalVelocity(m, d, 3.0f);

					oldJointPos[jNr] = m;
				}
			}
		}
	}


	// ---- fluid update ---

	// Adding temporal Force, in pixel relative to flWidth and flHeight
	glm::vec2 m = glm::vec2(mouseX * flWidth, mouseY * flHeight);
	glm::vec2 d = (m - oldM) * 10.f;
	oldM = m;
	glm::vec2 c = glm::normalize(forceScale - m);

	float tm = static_cast<float>(std::sin(time));
	fluidSim->addTemporalForce(m,                           // pos
			d,                           // vel
			glm::vec4(std::fabs(c.x * tm),
					std::fabs(c.y * tm),
					std::fabs(0.5f * tm),
					1.f),
					3.0f);
	fluidSim->update();

	// update particle system
	ps->update(time, false, fluidSim->getVelocityTex());
}

//----------------------------------------------------

void SNAutumnLeaves::onKey(int key, int scancode, int action, int mods)
{}

//----------------------------------------------------

void SNAutumnLeaves::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = 1.f - (ypos / scd->screenHeight);
}

}
