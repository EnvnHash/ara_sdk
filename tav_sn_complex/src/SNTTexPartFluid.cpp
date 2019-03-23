//
// SNTTexPartFluid.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  useDepthHisto muss aus sein!!!
//  wenn kinect auf dem kopf installiert im setup.xml kinMirror auf 1
//  und die KinectReproTools hFlip(1)

#include "SNTTexPartFluid.h"

using namespace std;

namespace tav
{
SNTTexPartFluid::SNTTexPartFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), inited(false), psInited(false), maxNrPart(262144),
			nrPartLeaves(60), flWidth(120), flHeight(120), scaleFact(1.f), zPos(-0.5f),
			nrPartTex(4), actMode(KinectReproTools::USE_ROT_WARP), hFlip(false),
			emitPartPerUpdate(200), kinDeviceNr(0)
{
	kin = static_cast<KinectInput*>(scd->kin);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(shCol);

	partTex = new TextureManager[nrPartTex];
	partTexNorm = new TextureManager[nrPartTex];
	litSphereTex = new TextureManager();
	for (short i=0;i<1;i++)
	{
		partTex->loadTexture2D((*scd->dataPath)+"textures/marriott/stars_one.tif");
		partTexNorm->loadTexture2D((*scd->dataPath)+"textures/marriott/stars_one_hm.tif");
	}
	litSphereTex->loadTexture2D((*scd->dataPath)+"textures/marriott/droplet_01.png");


	calibFileName = (*scd->dataPath)+"calib_cam/texPartFluid.yml";
	kinRepro = new KinectReproTools(winMan, kin, shCol, scd->screenWidth,
			scd->screenHeight, *scd->dataPath, kinDeviceNr,
			(*scd->dataPath)+"calib_cam/xtion_fisheye_depth.yml");
	kinRepro->noUnwarp();
	kinRepro->loadCalib(calibFileName);
	kinRepro->setMode(actMode);
	kinRepro->setHFlip(hFlip);

	// init ParticleSystem
	//        ps = new GLSLParticleSystem2(shCol, maxNrPart,
	//                                     scd->screenWidth, scd->screenHeight);
	ps = new GLSLParticleSystemFbo(shCol, maxNrPart,
			float(scd->screenWidth) / float(scd->screenHeight));

	/*
        ps->setFriction(0.f);
        ps->setLifeTime(2.0f);
        ps->setAging(true);
        ps->setAgeFading(true);
//        ps->setCheckBounds(false);
//        ps->setVelTexAngleStepSize(1.f / static_cast<float>(flWidth));
//        ps->init();
	 */

	// init Fluid Simulation
	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.99f;
	fluidSim->velocityDissipation = 0.98f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.3f,
			static_cast<float>(flHeight) * 0.3f);

	// init optical flow
	optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(kinDeviceNr),
			kin->getDepthHeight(kinDeviceNr));
	optFlow->setMedian(0.8f);
	optFlow->setBright(1.f);

	fBlur = new FastBlurMem(0.1f, shCol,
			kin->getDepthWidth(kinDeviceNr), kin->getDepthHeight(kinDeviceNr));

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
	quadShader = shCol->addCheckShader("part2Quad", "shaders/part_quad.vert", "shaders/part_quad.geom",
			"shaders/part_quad.frag");
	quadShader->link();
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);

	glm::mat4 scMat = glm::scale(glm::mat4(1.f), glm::vec3(0.5, 1.0, 1.0));

	camPos = glm::vec3(0.f, 0.f, 1.f);
}

//----------------------------------------------------

SNTTexPartFluid::~SNTTexPartFluid()
{
	delete ps;
	delete fluidSim;
}

//----------------------------------------------------

void SNTTexPartFluid::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	float zDeep;

	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	if (inited)
	{
		switch (actMode)
		{
		case KinectReproTools::CALIB_ROT_WARP :
			kinRepro->drawCalib();
			break;

		case KinectReproTools::CHECK_RAW_COLOR :
			kinRepro->drawCheckColor();
			break;

		case KinectReproTools::CHECK_RAW_DEPTH :
			kinRepro->drawCheckDepth();
			break;

		case KinectReproTools::CHECK_ROT_WARP :
			zDeep = osc->feedback * 5000.f;
			cout << "zDeep: " << zDeep << endl;
			kinRepro->setThreshZDeep(zDeep);
			kinRepro->setThreshZNear(osc->alpha * 3500.f);
			kinRepro->setRotXOffs((osc->blurOffs - 1.f) * M_PI * 1.f);

			kinRepro->drawCheckRotWarp();
			break;

		case KinectReproTools::CHECK_ROT_WARP_LOADED :
			kinRepro->drawCheckRotWarp();
			break;

		case KinectReproTools::USE_ROT_WARP :

			GLMCamera* cam = new GLMCamera(GLMCamera::FRUSTUM,
					scd->screenWidth, scd->screenHeight,
					-1.f * scaleFact, 1.f * scaleFact,
					-1.f * scaleFact, 1.f * scaleFact,
					camPos.x, camPos.y, camPos.z);
			glm::mat4 modMatr = glm::mat4(1.f);
			modMatr = glm::translate(modMatr, glm::vec3(0.f, 0.f, zPos));
			cam->setModelMatr(modMatr);
			break;
		}
	}

	if (_tfo)
	{
		glEnable(GL_RASTERIZER_DISCARD);
		_shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//----------------------------------------------------

void SNTTexPartFluid::update(double time, double dt)
{
	if(!inited)
	{
		data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
		data.posRand = 0.1f;
		data.emitVel = glm::vec3(0.0f, 1.f, 0.f);
		data.speed = 0.3f;
		data.size = 0.05f;
		data.sizeRand = 0.03f;
		data.texUnit = 1;
		data.texRand = 1.f;
		data.angleRand = 1.f;
		data.colInd = 1.f;
		data.maxNrTex = 16;

		inited = true;
	}

	kinRepro->update();

	switch (actMode)
	{
	case KinectReproTools::CHECK_RAW_DEPTH :
		break;
	case KinectReproTools::CHECK_RAW_COLOR :
		break;
	case KinectReproTools::CALIB_ROT_WARP :
		break;
	case KinectReproTools::CHECK_ROT_WARP :
		break;
	case KinectReproTools::CHECK_ROT_WARP_LOADED :
		kinRepro->setRotXOffs(0.f);
		break;
	case KinectReproTools::USE_ROT_WARP :

		if(kinRepro->getFrameNr() != lastFBlurFrame)
		{
			lastFBlurFrame = kinRepro->getFrameNr();
			fBlur->proc(kinRepro->getRotDepth());
			//                    fBlur->proc(kinRepro->getUnwarpTex());
		}

		optFlow->update(fBlur->getResult(), fBlur->getLastResult());

		// osc par
		fluidSim->velocityDissipation = 0.999f;
		//                fluidSim->velocityDissipation = std::pow(osc->blurFboAlpha, 0.25);
		fluidSim->addVelocity(optFlow->getResTexId(), 5.f);
		//                fluidSim->addVelocity(optFlow->getResTexId(), osc->blurFdbk * 10.f);
		fluidSim->update();

		//data.posRand = 0.f;
		data.speed = 0.5f;
		data.emitCol = glm::vec4(1.f, 1.f, 1.f, 1.f);

		/*
                // emit particles with emit texture and depth info
                ps->emit(emitPartPerUpdate,
                         &data,
                         optFlow->getResTexId(),
//                         fBlur->getResult(),
                         kin->getDepthWidth(kinDeviceNr),
                         kin->getDepthHeight(kinDeviceNr));

                //renderPartFbo();

                ps->update(time, fluidSim->getVelocityTex());
//                ps->update(time);
		 */

		break;
	}
}

//----------------------------------------------------

void SNTTexPartFluid::onKey(int key, int scancode, int action, int mods)
{
	// trapez korrektur schnell und schmutzig
	if (action == GLFW_PRESS)
	{
		if(mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_1 :
				actMode = KinectReproTools::CALIB_ROT_WARP;
				printf("SNTTexPartFluid::CALIB_ROT_WARP \n");
				break;
			case GLFW_KEY_2 :
				actMode = KinectReproTools::CHECK_RAW_DEPTH;
				printf("SNTTexPartFluid::CHECK_RAW_DEPTH \n");
				break;
			case GLFW_KEY_3 :
				actMode = KinectReproTools::CHECK_ROT_WARP;
				printf("SNTTexPartFluid::CHECK_ROT_WARP \n");
				break;
			case GLFW_KEY_4 :
				actMode = KinectReproTools::CHECK_ROT_WARP_LOADED;
				kinRepro->loadCalib(calibFileName);
				printf("SNTTexPartFluid::CHECK_ROT_WARP_LOADED \n");
				break;
			case GLFW_KEY_5 : actMode = KinectReproTools::USE_ROT_WARP;
			printf("SNTTexPartFluid::USE_ROT_WARP \n");
			break;
			break;
			case GLFW_KEY_S : kinRepro->saveCalib(calibFileName); break;
			case GLFW_KEY_D : kinRepro->useDepthDeDist = !kinRepro->useDepthDeDist;
			}

			kinRepro->setMode(actMode);
		}
	}
}

//----------------------------------------------------

void SNTTexPartFluid::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = 1.f - (ypos / scd->screenHeight);
}
}
