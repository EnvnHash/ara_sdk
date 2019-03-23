//
//  SNTFluidDepth.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//
//  funktioniert mit dem rohen tiefenbild der kinect (ohne normalisierung) 16bit integer: 0 - 65536
//
//  Hintergrund und Vordergrund werden über einen einfachen Tiefen-Schwellenwert getrennt
//  Die Idee, das Tiefenbild mit einem Referenztiefenbild zu "subtrahiert" geht nicht, weil das Tiefenbild
//  zu sehr rauscht...
//  Der Tiefenschwellenwert wird
//

#include "SNTFluidDepth.h"

using namespace std;

namespace tav
{
SNTFluidDepth::SNTFluidDepth(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrParticle(800000),
			flWidth(512), flHeight(512), emitPartPerUpdate(8000), thinDownSampleFact(8),
			fblurSize(512),
			noiseTexSize(512),
			captureIntv(120.0),
			inActAddNoiseInt(40.0),  // wenn nix los emit noise
			actDrawMode(DRAW)  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
{
    kin = static_cast<KinectInput*>(scd->kin);
    kinRepro = static_cast<KinectReproTools*>(scd->kinRepro);
    osc = static_cast<OSCData*>(scd->osc);
    winMan = static_cast<GWindowManager*>(scd->winMan);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// - add OSC Parameter --

	addPar("fastBlurAlpha",  &fastBlurAlpha);
	addPar("optFlowBlurAlpha",  &optFlowBlurAlpha);
	addPar("fluidColorSpeed",  &fluidColorSpeed);
	addPar("fluidColTexForce",  &fluidColTexForce);
	addPar("fluidDissip",  &fluidDissip);
	addPar("fluidVelTexThres",  &fluidVelTexThres);
	addPar("fluidVelTexRadius",  &fluidVelTexRadius);
	addPar("fluidVelTexForce",  &fluidVelTexForce);
	addPar("fluidSmoke",  &fluidSmoke);
	addPar("fluidSmokeWeight",  &fluidSmokeWeight);
	addPar("fluidVelDissip",  &fluidVelDissip);
	addPar("fluidTimeStep",  &fluidTimeStep);
	addPar("veloBlend",  &veloBlend);
	addPar("depthAmp",  &depthAmp);
	addPar("alpha",  &alpha);

	fluidAddCol = new glm::vec4[3];
	fluidAddCol[0] = glm::vec4(25.f, 39.f, 175.f, 255.f) / 255.f;
	fluidAddCol[1] = glm::vec4(247.f, 159.f, 35.f, 255.f) / 255.f;
	fluidAddCol[2] = glm::vec4(134.f, 26.f, 255.f, 255.f) / 255.f;

	partEmitCol = new glm::vec4[3];
	partEmitCol[0] = glm::vec4(182.f, 189.f, 251.f, 255.f) / 255.f;
	partEmitCol[1] = glm::vec4(181.f, 248.f, 227.f, 255.f) / 255.f;
	partEmitCol[2] = glm::vec4(248.f, 241.f, 181.f, 255.f) / 255.f;

	// - Fluid System --

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.999f;
	fluidSim->velocityDissipation = 0.999f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

	// forceScale nur zum Mouse test
	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.125f,
			static_cast<float>(flHeight) * 0.125f);


	// --- Geo Primitives ---

	rawQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on

	rotateQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f,
			nullptr, 1, true);

	redQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 0.f, 0.f, 1.f);


	// --- Shaders ---

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
	texShaderAlpha = shCol->getStdTexAlpha();

	initShaders();
	initWhiteShdr();

	timer.showFps(true);
}


void SNTFluidDepth::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(inited)
	{
		if (_tfo)
		{
			_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/*
        kin->uploadDepthImg(true);
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true));

		redQuad->draw();
		*/
		float ap = alpha * osc->alpha;

		switch(actDrawMode)
		{
		case RAW_DEPTH :

			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, 0));
			glBindTexture(GL_TEXTURE_2D, kinRepro->getDepthTransTexId(1));

			rawQuad->draw();

			break;

		case DEPTH_THRESH:
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
			rawQuad->draw();
			break;

		case DEPTH_BLUR:
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
			rawQuad->draw();
			break;

		case OPT_FLOW:
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
			rawQuad->draw();
			break;

		case OPT_FLOW_BLUR:
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
			rawQuad->draw();
			break;

		case FLUID_VEL:
			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());
			rawQuad->draw();
			break;

		case DRAW:


			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
			rotateQuad->draw();

			texShaderAlpha->begin();
			texShaderAlpha->setIdentMatrix4fv("m_pvm");
			texShaderAlpha->setUniform1i("tex", 0);
			texShaderAlpha->setUniform1f("alpha", ap );
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());
			rotateQuad->draw();
			//fluidSim->draw();
			break;

		default:
			break;
		}
	}
}


void SNTFluidDepth::update(double time, double dt)
{
	if (kin && kin->isReady())
	{
		if (!inited)
		{
			//  kin->setCloseRange(true);
			//  kin->setDepthVMirror(true);

			threshFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
					GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);
			threshFbo->clear();


			// - Optical Flow --

			optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(),
					kin->getDepthHeight());
			optFlow->setMedian(0.8f);
			optFlow->setBright(0.3f);

			// - FastBlurs --

			fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
			fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
			optFlowBlur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(),
					kin->getDepthHeight(), GL_RGB16F);

			inited = true;
		} else
		{


		//	kin->uploadDepthImg(false, 0);

		//	if(kin->getDepthUplFrameNr(false) !=  frameNr )
		//	{
				frameNr = kin->getDepthUplFrameNr(false);

				threshFbo->bind();
				threshFbo->clear();

				whiteShdr->begin();
				whiteShdr->setIdentMatrix4fv("m_pvm");
				whiteShdr->setUniform1i("tex", 0);
				whiteShdr->setUniform1f("whiteAdj", depthAmp);

				glActiveTexture(GL_TEXTURE0);
//				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, 0));
				glBindTexture(GL_TEXTURE_2D, kinRepro->getDepthTransTexId(1));

				rawQuad->draw();

				threshFbo->unbind();

				// -- apply blur on silhoutte--

				fblur->setAlpha(fastBlurAlpha);
				fblur->proc(threshFbo->getColorImg());

				fblur2nd->setAlpha(fastBlurAlpha);
				fblur2nd->proc(fblur->getResult());

				// -- calculate optical flow, the result will be used to add velocity to the fluid --

				optFlow->update(fblur2nd->getResult(), fblur2nd->getLastResult());

				optFlowBlur->setAlpha(optFlowBlurAlpha);
				optFlowBlur->proc(optFlow->getResTexId());
		//	}


			//--- Update Fluid, muss jeden Frame passieren, sonst flimmer effekte ----

			// set Fluid Color
			float timeMult = time * fluidColorSpeed;
			float colPos = static_cast<float>(std::sin(timeMult * (std::sin(timeMult * 0.01) * 0.5 + 0.5) * 0.2)) * 0.5f + 0.5f;
			glm::vec4 eCol = std::fmax(1.f - colPos *2.f, 0.f) * fluidAddCol[0]
							 + std::fmax(1.f - std::abs(colPos *2.f -1.f), 0.f) *fluidAddCol[1]
							 + std::fmax((colPos -0.5f) *2.f, 0.f) * fluidAddCol[2];

			fluidSim->setVelTexThresh(fluidVelTexThres);
			fluidSim->setSmokeBuoyancy(fluidSmoke);
			fluidSim->setVelTexRadius(fluidVelTexRadius);
			fluidSim->dissipation = std::pow(fluidDissip, 0.2) * 0.1f + 0.9f;
			fluidSim->velocityDissipation = std::pow(fluidVelDissip, 0.2) * 0.1f + 0.9f;

			fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
			fluidSim->addColor(optFlowBlur->getResult(), glm::vec3(eCol), fluidColTexForce, true);
			fluidSim->setTimeStep(fluidTimeStep);
			fluidSim->update();
		}
	}
}


void SNTFluidDepth::mouseTest(double time)
{
	// mouse add force to debug
	// Adding temporal Force, in pixel relative to flWidth and flHeight
	glm::vec2 m = glm::vec2(mouseX * static_cast<float>(fluidSim->getWidth()),
			mouseY * static_cast<float>(fluidSim->getHeight()));
	glm::vec2 d = (m - oldM) * 5.f;
	oldM = m;
	glm::vec2 c = glm::normalize(forceScale - m);

	glm::vec4 eCol = glm::vec4(25.f / 255.f,
			39.f / 255.f,
			175.f / 255.f,
			0.01f);

	float tm = static_cast<float>(std::sin(time));

	fluidSim->addTemporalForce(m,                           // pos
			d,                           // vel
			eCol,
			2.0f);
	fluidSim->update();
}


void SNTFluidDepth::startCaptureFrame(int renderMode, double time)
{
	lastCaptureTime = time;

	// start write thread
	if(capture_Thread)
	{
		delete capture_Thread;
		capture_Thread = 0;
	}

	capture_Thread = new boost::thread(&SNTFluidDepth::captureFrame, this, renderMode, time);
}


void SNTFluidDepth::captureFrame(int renderMode, double time)
{
	// start SaveThread
	char fileName [100];
	sprintf(fileName,
			((*scd->dataPath)+"capture/lenovo%d%d%f.jpg").c_str(),
			static_cast<int>( ( static_cast<double>(savedNr) / 10.0 ) ),
			savedNr % 10,
			time);

	//        cv::flip(capturePic1Cam, fCapturePic1Cam, 0);
	//        cv::imwrite(fileName, fCapturePic1Cam);

	// start scp thread
	if (!inactivityEmit)
	{
		int i = system(("cp "+std::string(fileName)+" /media/ingeneria1-pc").c_str());
		if (i != 0) printf ("Image Capture Error!!!! scp upload failed\n");

		printf("image written \n");
	}
}


void SNTFluidDepth::initWhiteShdr()
{
	std::string shdr_Header = "#version 410 core\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	out vec2 tex_coord;

	uniform mat4 m_pvm;

	void main() {
		tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});
	vert = "// SNTKinectShadowV2 vertex shader\n" + shdr_Header + vert;


	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	uniform float alpha;
	uniform float whiteAdj;
	uniform sampler2D tex;
	in vec2 tex_coord;
	void main() {
		float texCol = texture(tex, tex_coord).r * whiteAdj;
		color = vec4(texCol, texCol, texCol, 1.0);
	});
	frag = "// SNTKinectShadowV2 Shader\n" + shdr_Header + frag;

	whiteShdr = shCol->addCheckShaderText("SNTFluidDepth_white", vert.c_str(), frag.c_str());
}


void SNTFluidDepth::initShaders()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
	std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	uniform mat4 m_pvm;
	out vec2 tex_coord;
	void main()
	{
		tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});

	stdVert = "// SNTFluidDepth depth threshold vertex shader\n" +shdr_Header +stdVert;
	std::string frag = STRINGIFY(uniform sampler2D kinDepthTex; // 16 bit -> float
	uniform float depthThres;
	in vec2 tex_coord;
	layout (location = 0) out vec4 color;
	float outVal;
	void main()
	{
		outVal = texture(kinDepthTex, tex_coord).r;
		outVal = outVal > 50.0 ? (outVal < depthThres ? 1.0 : 0.0) : 0.0;
		color = vec4(outVal);
	});

	frag = "// SNTFluidDepth depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNTFluidDepth_thres", stdVert.c_str(), frag.c_str());

	//----/----/----/----/----/----/----/----/----/----/----/----/----/----/----/----

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	uniform float stepX;
	uniform float stepY;
	uniform mat4 m_pvm;
	out vec2 le;
	out vec2 ri;
	out vec2 tex_coord;
	void main()
	{
		le = texCoord + vec2(-stepX, 0.0);
		ri = texCoord + vec2(stepX, 0.0);
		tex_coord = texCoord;
		gl_Position = m_pvm * position;
	});

	vert = "// SNTFluidDepth edge detect vertex shader\n" +shdr_Header +vert;

	frag = STRINGIFY(uniform sampler2D kinDepthTex; // 16 bit -> float
	in vec2 le;
	in vec2 ri;
	in vec2 tex_coord;
	layout (location = 0) out vec4 color;
	layout (location = 1) out vec4 shape;
	bool leftDet;
	bool rightDet;
	float outVal;
	void main()
	{
		vec4 center = texture(kinDepthTex, tex_coord);
		vec4 left = texture(kinDepthTex, le);
		vec4 right = texture(kinDepthTex, ri);

		leftDet = (left.r == 0.0 && center.r > 0.0) || (left.r > 0.0 && center.r == 0.0);
		rightDet = (right.r == 0.0 && center.r > 0.0) || (right.r > 0.0 && center.r == 0.0);

		outVal = leftDet || rightDet ? 1.0 : 0.0;
		outVal = tex_coord.x > 0.015 ? outVal : 0;
		outVal = tex_coord.x < 0.995 ? outVal : 0;

		color = vec4(outVal);
	});

	frag = "// SNTFluidDepth edge detect fragment shader\n"+shdr_Header+frag;

	edgeDetect = shCol->addCheckShaderText("SNTFluidDepth_edge", vert.c_str(), frag.c_str());

	//----

	frag = STRINGIFY(uniform sampler2D act;
	uniform sampler2D last;
	in vec2 tex_coord;
	layout (location = 0) out vec4 color;
	void main()
	{
		float actV = texture(act, tex_coord).r;
		float lastV = texture(last, tex_coord).r;
		float sub = (actV - lastV);
		color = vec4(sub > 0.3 ? sub : 0.0);
	});

	frag = "// SNTFluidDepth subtract frag shader\n"+shdr_Header+frag;

	subtrShadr = shCol->addCheckShaderText("SNTFluidDepth_subtr", stdVert.c_str(), frag.c_str());

	//----

	frag = STRINGIFY(uniform sampler2D tex;
	in vec2 tex_coord;
	layout (location = 0) out vec4 color;
	void main()
	{
		color = vec4(texture(tex, tex_coord).r > 0.7 ? 1.0 : 0.0);
	});

	frag = "// SNTFluidDepth second thresh frag shader\n"+shdr_Header+frag;

	blurThres = shCol->addCheckShaderText("SNTFluidDepth_thres2", stdVert.c_str(), frag.c_str());
}


void SNTFluidDepth::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_1 : actDrawMode = RAW_DEPTH;
		printf("actDrawMode = RAW_DEPTH \n");
		break;
		case GLFW_KEY_2 : actDrawMode = DEPTH_THRESH;
		printf("actDrawMode = DEPTH_THRESH \n");
		break;
		case GLFW_KEY_3 : actDrawMode = DEPTH_BLUR;
		printf("actDrawMode = DEPTH_BLUR \n");
		break;
		case GLFW_KEY_4 : actDrawMode = OPT_FLOW;
		printf("actDrawMode = OPT_FLOW \n");
		break;
		case GLFW_KEY_5 : actDrawMode = OPT_FLOW_BLUR;
		printf("actDrawMode = OPT_FLOW_BLUR \n");
		break;
		case GLFW_KEY_6 : actDrawMode = FLUID_VEL;
		printf("actDrawMode = FLUID_VEL \n");
		break;
		case GLFW_KEY_7 : actDrawMode = DRAW;
		printf("actDrawMode = DRAW \n");
		break;
		}
	}
}


void SNTFluidDepth::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = (ypos / scd->screenHeight);
}


SNTFluidDepth::~SNTFluidDepth()
{
	delete userMapConv;
	delete userMapRGBA;
	delete ps;
	delete fluidSim;
	delete fluidAddCol;
	delete partEmitCol;
	delete rawQuad;
	delete rotateQuad;
	delete edgePP;
	delete optFlow;
}
}
