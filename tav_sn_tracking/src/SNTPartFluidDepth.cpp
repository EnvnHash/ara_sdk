//
//  SNTPartFluidDepth.cpp
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

#include "SNTPartFluidDepth.h"

using namespace std;

namespace tav
{
SNTPartFluidDepth::SNTPartFluidDepth(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrParticle(800000),
flWidth(512), flHeight(512), emitPartPerUpdate(8000), thinDownSampleFact(8),
fblurSize(512),
noiseTexSize(512),
captureIntv(120.0),
inActAddNoiseInt(40.0),  // wenn nix los emit noise
actDrawMode(DRAW),  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
depthThresh(1755.f),
fastBlurAlpha(0.4f),
optFlowBlurAlpha(0.76f),
fluidColorSpeed(1.f),
fluidColTexForce(0.325f),
fluidDissip(0.8f),
fluidVelTexThres(0.8f),
fluidVelTexRadius(0.08f),
fluidVelTexForce(0.74f),
fluidSmoke(0.457f),
fluidSmokeWeight(0.03f),
fluidVelDissip(0.93f),
partColorSpeed(1.35f),
partFdbk(0.63f),
partFriction(0.56f),
partAlpha(0.34f),
partBright(0.25f),
partVeloBright(0.16f),
veloBlend(0.05f)
{
	kin = static_cast<KinectInput*>(scd->kin);
	osc = static_cast<OSCData*>(scd->osc);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// - add OSC Parameter --

	addPar("depthThresh", &depthThresh); // 10.f, 5000.f, 1.f, 1755.f, OSCData::LIN);
	addPar("fastBlurAlpha", &fastBlurAlpha); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("optFlowBlurAlpha", &optFlowBlurAlpha); //0.f, 1.f, 0.0001f, 0.76f, OSCData::LIN);
	addPar("fluidColorSpeed", &fluidColorSpeed); //0.f, 3.f, 0.001f, 1.f, OSCData::LIN);
	addPar("fluidColTexForce", &fluidColTexForce); //0.0f, 1.5f, 0.001f, 0.325f, OSCData::LIN);
	addPar("fluidDissip", &fluidDissip); // 0.0f, 1.f, 0.001f, 0.8f, OSCData::LIN);
	addPar("fluidVelTexThres", &fluidVelTexThres); // 0.f, 5.f, 0.0001f, 0.8f, OSCData::LIN);
	addPar("fluidVelTexRadius", &fluidVelTexRadius); // 0.f, 0.5f, 0.0001f, 0.08f, OSCData::LIN);
	addPar("fluidVelTexForce", &fluidVelTexForce); // 0.0f, 3.f, 0.001f, 0.74f, OSCData::LIN);
	addPar("fluidSmoke", &fluidSmoke); // 0.f, 1.f, 0.001f, 0.457f, OSCData::LIN);
	addPar("fluidSmokeWeight", &fluidSmokeWeight); // 0.f, 1.f, 0.001f, 0.03f, OSCData::LIN);
	addPar("fluidVelDissip", &fluidVelDissip); // 0.0f, 1.f, 0.001f, 0.93f, OSCData::LIN);
	addPar("partColorSpeed", &partColorSpeed); // 0.f, 4.f, 0.001f, 1.35f, OSCData::LIN);
	addPar("partFdbk", &partFdbk); // 0.f, 1.f, 0.001f, 0.63f, OSCData::LIN);
	addPar("partFriction", &partFriction); // 0.f, 1.f, 0.001f, 0.56f, OSCData::LIN);
	addPar("partAlpha", &partAlpha); // 0.f, 1.f, 0.001f, 0.34f, OSCData::LIN);
	addPar("partBright", &partBright); // 0.f, 1.f, 0.001f, 0.25f, OSCData::LIN);
	addPar("partVeloBright", &partVeloBright); // 0.f, 1.f, 0.001f, 0.16f, OSCData::LIN);
	addPar("veloBlend", &veloBlend); // 0.f, 1.f, 0.0001f, 0.05f,w	 OSCData::LIN);

	// - Partikel System --

	particleFbo = new FBO(shCol, 1024, 768);
	particleFbo->clear();

	ps = new GLSLParticleSystemFbo(shCol, nrParticle, float(scd->screenWidth) / float(scd->screenHeight));
	ps->setFriction(0.f);
	ps->setLifeTime(8.f);
	ps->setAging(true);
	ps->setAgeFading(true);
	ps->setEmitTexThres(0.8f); // die geblurrte textur enthaelt viel "grau muell" deshalb relativ hoher threshold

	// EmitData muss explizit geschickt werden, dadurch Optimierung mit UniformBlocks möglich
	data.emitOrg = glm::vec3(0.f, 0.f, 0.f);
	data.emitVel = normalize(glm::vec3(0.f, 1.f, 0.f));
	data.emitCol = glm::vec4(1.f, 1.f, 1.f, 1.f);
	data.posRand = 0.f; // wenn emit per textur und > 0 -> mehr gpu
	data.dirRand = 0.1f;
	data.speed = 0.01f;
	data.colRand = 0.f;
	data.extVelAmt = 0.5f;
	data.ageFadeCurv = 0.6f;
//        data.size = 0.06f;
	ps->setEmitData(&data);


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
					   0.f, 0.f, 0.f, 0.f);    // color will be replace when rendering with blending on

	rotateQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
						  glm::vec3(0.f, 0.f, 1.f),
						  0.f, 0.f, 0.f, 0.f,
						  nullptr, 1, true);

	// --- Shaders ---

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();

	initShaders();

	timer.showFps(true);
}

//---------------------------------------------------------------

void SNTPartFluidDepth::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if(inited)
	{
		if (_tfo)
		{
			_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		switch(actDrawMode)
		{
			case RAW_DEPTH :
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
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

				fluidSim->draw();

				// draw particles with fbo for feedback
				particleFbo->bind();
				particleFbo->clearAlpha(partFdbk);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				ps->draw( cp->mvp );
				particleFbo->unbind();

				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);

				/*
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
				rawQuad->draw();
*/

				glBindTexture(GL_TEXTURE_2D, particleFbo->getColorImg());
				rawQuad->draw();

				break;

			default:
				break;
		}


		/*
		 glEnable(GL_BLEND);
		 glBlendFunc(GL_SRC_ALPHA, GL_ONE);

		 // draw raw silhouette
		 blendTexShader->begin();
		 blendTexShader->setIdentMatrix4fv("m_pvm");
		 blendTexShader->setUniform1i("tex", 0);
		 blendTexShader->setUniform1f("alpha", 0.6);
		 glActiveTexture(GL_TEXTURE0);
		 glBindTexture(GL_TEXTURE_2D, fblur->getResult());
		 rotateQuad->draw();
		 */

		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//if (doCapture) startCaptureFrame(renderMode, time);
	}
}

//---------------------------------------------------------------

void SNTPartFluidDepth::update(double time, double dt)
{

//	std::cout << depthThresh << std::endl;

	if (kin && kin->isReady())
	{
		if (!inited)
		{
			kin->setCloseRange(true);
			kin->setDepthVMirror(true);

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
			// wenn nix los, emit particles
			//if(inActEmitTime > inActAddNoiseInt) inactivityEmit = true; else inactivityEmit = false;

			//--- Proc Depth Image and Update the Fluid Simulator --
			// upload depth image, wird intern gecheckt, ob neues Bild erforderlich oder nicht
			//kin->uploadDepthImg(false);

			if (kin->uploadDepthImg(false))
			{
//                	if(kin->getDepthUplFrameNr(false) - frameNr > 1)
//                		printf("frameNr: %d \n", kin->getDepthUplFrameNr(false) );

				frameNr = kin->getDepthUplFrameNr(false);

				// -- threshold the depth image --

				glDisable(GL_BLEND);

				threshFbo->bind();
				threshFbo->clear();

				depthThres->begin();
				depthThres->setIdentMatrix4fv("m_pvm");
				depthThres->setUniform1i("kinDepthTex", 0);
				depthThres->setUniform1f("depthThres", depthThresh);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

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


				//--- Emit Particles --

				// emit Color
				float peclTime = time * partColorSpeed;
				float colPos2 = static_cast<float>(std::sin(peclTime * (std::sin(peclTime * 0.01) * 0.5 + 0.5) * 0.2)
												   * std::sin(peclTime * 0.02)) * 0.5f + 0.5f;
				data.emitCol = std::fmax(1.f - colPos2 *2.f, 0.f) * partEmitCol[0]
								+ std::fmax(1.f - std::abs(colPos2 *2.f -1.f), 0.f) *partEmitCol[1]
								+ std::fmax((colPos2 -0.5f) *2.f, 0.f) *partEmitCol[2];

				data.emitCol *= partBright;
				data.emitCol.a = partAlpha;
				data.extVelAmt = veloBlend;

				ps->setEmitData(&data);
				ps->setFriction(partFriction);
				ps->setVeloBright(partVeloBright);

				//ps->emit(emitPartPerUpdate, fblur->getResult(), fblurSize, fblurSize);
				ps->emit(emitPartPerUpdate, fblur2nd->getResult(), fblurSize, fblurSize, optFlowBlur->getResult());
			}


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
			fluidSim->update();

			ps->update(time, fluidSim->getVelocityTex());


			//---
			/*
			//if (inactivityEmit)
			//{
			//data.posRand = (std::sin(time * 1.24) * 0.5f + 0.5f) * 0.2f;
			//data.emitVel = glm::vec3(std::sin(time * 1.23), std::sin(time * 1.93), std::sin(time * 1.33));
			//data.speed = 0.6f;

			//data.emitOrg = glm::vec3(std::sin(time * 1.13) * std::sin(time * 0.3),
			//std::sin(time * 0.612) * std::sin(time * 1.32),
			//std::sin(time * 0.812) * std::sin(time * 0.822) * 0.5f - 0.5f);

			//ps->emit(static_cast<int>( std::fmax( std::sin(time * 2.83f) * std::sin(time * 1.23f) * std::sin(time * 0.783f), 0.f ) * 50.f) , data);
			//}

			 */
		}
	}
}

//---------------------------------------------------------------

void SNTPartFluidDepth::mouseTest(double time)
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

//---------------------------------------------------------------

void SNTPartFluidDepth::startCaptureFrame(int renderMode, double time)
{
	lastCaptureTime = time;

	// start write thread
	if(capture_Thread)
	{
		delete capture_Thread;
		capture_Thread = 0;
	}

	capture_Thread = new boost::thread(&SNTPartFluidDepth::captureFrame, this, renderMode, time);
}

//---------------------------------------------------------------

void SNTPartFluidDepth::captureFrame(int renderMode, double time)
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

//---------------------------------------------------------------

void SNTPartFluidDepth::initShaders()
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

	stdVert = "// SNTPartFluidDepth depth threshold vertex shader\n" +shdr_Header +stdVert;
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

	frag = "// SNTPartFluidDepth depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNTPartFluidDepth_thres", stdVert.c_str(), frag.c_str());

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

	vert = "// SNTPartFluidDepth edge detect vertex shader\n" +shdr_Header +vert;

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

	frag = "// SNTPartFluidDepth edge detect fragment shader\n"+shdr_Header+frag;

	edgeDetect = shCol->addCheckShaderText("SNTPartFluidDepth_edge", vert.c_str(), frag.c_str());

	//------------------------------------------------------------

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

	frag = "// SNTPartFluidDepth subtract frag shader\n"+shdr_Header+frag;

	subtrShadr = shCol->addCheckShaderText("SNTPartFluidDepth_subtr", stdVert.c_str(), frag.c_str());

	//------------------------------------------------------------

	frag = STRINGIFY(uniform sampler2D tex;
					 in vec2 tex_coord;
					 layout (location = 0) out vec4 color;
					 void main()
					 {
						 color = vec4(texture(tex, tex_coord).r > 0.7 ? 1.0 : 0.0);
					 });

	frag = "// SNTPartFluidDepth second thresh frag shader\n"+shdr_Header+frag;

	blurThres = shCol->addCheckShaderText("SNTPartFluidDepth_thres2", stdVert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNTPartFluidDepth::onKey(int key, int scancode, int action, int mods)
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

//---------------------------------------------------------------

void SNTPartFluidDepth::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = (ypos / scd->screenHeight);
}

//---------------------------------------------------------------

SNTPartFluidDepth::~SNTPartFluidDepth()
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
