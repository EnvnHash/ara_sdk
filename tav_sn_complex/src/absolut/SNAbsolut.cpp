//
//  SNAbsolut.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNAbsolut.h"

using namespace std;

namespace tav
{
SNAbsolut::SNAbsolut(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), inited(false),
			psInited(false), maxNrPart(800), nrPartLeaves(800), flWidth(150), flHeight(150), numLeaveTexs(6),
			scaleFact(1.f), zPos(0.f), fblurSize(512), actDrawMode(DRAW), depthThresh(1755.f), fastBlurAlpha(0.4f),
			optFlowBlurAlpha(0.76f), numFrases(7), destinyTime(25.0),
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
			veloBlend(0.05f),
			reposFact(0.001f),
			bottlePos(-0.02f),
			activityThres(1000.f),
			logoDist(1.f),
			breakTime(3.0),
			timeToSetOff(1.5),
			fadeOutFact(0.1f)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);

	// get onKey function
	winMan = static_cast<GWindowManager*>(_scd->winMan);
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });


	texs = new TextureManager*[numLeaveTexs];
	leaveTexUnits = new GLint[numLeaveTexs];

	for (int i=0;i<numLeaveTexs;i++){
		texs[i] = new TextureManager();
		texs[i]->loadTexture2D( ((*_scd->dataPath)+"/textures/absolut/farn_0"+std::to_string(i)+".png").c_str(), 1);
		leaveTexUnits[i] = i;
	}


	frase_from_mat = glm::mat4(1.f);
	frase_to_mat = glm::mat4(1.f);

	frase_solid_tex = new TextureManager*[numFrases];
	frase_glow_tex = new TextureManager*[numFrases];
	frases_alpha = new float[numFrases];

	for (int i=0;i<numFrases;i++)
	{
		frase_solid_tex[i] = new TextureManager();
		frase_glow_tex[i] = new TextureManager();
		frases_alpha[i] = 0.f;

		frase_solid_tex[i]->loadTexture2D( ((*_scd->dataPath)+"/textures/absolut/frase_solid_0"+std::to_string(i)+".png").c_str(), 1);
		frase_glow_tex[i]->loadTexture2D( ((*_scd->dataPath)+"/textures/absolut/frase_glow_0"+std::to_string(i)+".png").c_str(), 1);
	}

	backTex = new TextureManager();
	backTex->loadTexture2D((*_scd->dataPath)+"/textures/absolut/absolut_fondo.jpg", 6);

	frontTex = new TextureManager();
	frontTex->loadTexture2D((*_scd->dataPath)+"/textures/absolut_vodka_flasche.png");

	// activity_dot
	redDotTex = new TextureManager();
	redDotTex->loadTexture2D((*_scd->dataPath)+"/textures/red_dot.png");

	// absolut logo
	absolut_logo_tex = new TextureManager();
	absolut_logo_tex->loadTexture2D((*_scd->dataPath)+"/textures/absolut/absolut_logo.png", 1);

	kin = static_cast<KinectInput*>(scd->kin);
	osc = static_cast<OSCData*>(scd->osc);


	ps = new GLSLParticleSystem2(shCol, maxNrPart, scd->screenWidth, scd->screenHeight);
	ps->setFriction(0.6f);
	ps->setLifeTime(4.f);
	ps->setCheckBounds(false);
	ps->setAging(false);
	ps->setAgeFading(false);
	ps->setAgeSizing(false);
	ps->setVelTexAngleStepSize(1.f / static_cast<float>(flWidth));
	ps->setReturnToOrg(true, 0.005f);
	ps->init();

	fluidSim = new GLSLFluid(false, shCol);
	fluidSim->allocate(flWidth, flHeight, 0.5f);
	fluidSim->dissipation = 0.95f;
	fluidSim->velocityDissipation = 0.98f;
	fluidSim->setGravity(glm::vec2(0.0f, 0.0f));

	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.3f,
			static_cast<float>(flHeight) * 0.3f);
	gotActivity = true;

	optFlow = new GLSLOpticalFlow(shCol, kin->getDepthWidth(), kin->getDepthHeight());
	optFlow->setMedian(0.8f);

	bwActFbo = new FBO(shCol, kin->getDepthWidth(), kin->getDepthHeight(),
		GL_R16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_BORDER, false);


	histo = new GLSLHistogram(shCol,
			kin->getDepthWidth(),
			kin->getDepthHeight(),
			GL_R32F,
			2,
			128, // hist width
			false,
			true,
			1.f); // maxValPerChan

	camPos = glm::vec3(0.f, 0.f, 1.f);
	kCamPos = glm::vec3(0.f, 0.f, 1.f);


	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);

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
	texAlphaShader = shCol->getStdTexAlpha();

	initShaders();

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
	addPar("fluidVelDissip", &fluidVelDissip); // 0.0f, 1.f, 0.001f, 0.93f, OSCData::LIN);gotActivity
	addPar("partColorSpeed", &partColorSpeed); // 0.f, 4.f, 0.001f, 1.35f, OSCData::LIN);
	addPar("partFdbk", &partFdbk); // 0.f, 1.f, 0.001f, 0.63f, OSCData::LIN);
	addPar("partFriction", &partFriction); // 0.f, 1.f, 0.001f, 0.56f, OSCData::LIN);
	addPar("partAlpha", &partAlpha); // 0.f, 1.f, 0.001f, 0.34f, OSCData::LIN);
	addPar("partBright", &partBright); // 0.f, 1.f, 0.001f, 0.25f, OSCData::LIN);
	addPar("partVeloBright", &partVeloBright); // 0.f, 1.f, 0.001f, 0.16f, OSCData::LIN);
	addPar("veloBlend", &veloBlend); // 0.f, 1.f, 0.0001f, 0.05f, OSCData::LIN);
	addPar("reposFact", &reposFact);
	addPar("leavesZPos", &zPos);
	addPar("bottlePos", &bottlePos);
	addPar("activityThres", &activityThres);
	addPar("logoDist", &logoDist);

    fraseBlendFromAlphaMed = new Median<float>(2.f);
	fraseBlendToAlphaMed = new Median<float>(2.f);
}

//----------------------------------------------------

SNAbsolut::~SNAbsolut()
{
	delete ps;
	delete fluidSim;
	delete [] texs;
	delete [] leaveTexUnits;
	delete backTex;
	delete frontTex;
	delete redDotTex;
	delete optFlow;
	delete bwActFbo;
	delete histo;
	delete absolut_logo_tex;
}

//----------------------------------------------------

void SNAbsolut::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (inited)
	{
		glDisable(GL_DEPTH_TEST);

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

			case OPT_FLOW_BLUR_BW:
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, bwActFbo->getColorImg());
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
				{

					// draw background texture
					texShader->begin();
					texShader->setIdentMatrix4fv("m_pvm");
					texShader->setUniform1i("tex", 0);
					backTex->bind(0);
					quad->draw();


					glClearDepth(1.0f);
					glClear(GL_DEPTH_BUFFER_BIT);

					glEnable(GL_DEPTH_TEST);
					glDisable(GL_CULL_FACE);
					glEnable(GL_BLEND);

					// draw leaves particles
					glm::mat4 lOffsMat = glm::translate( glm::vec3(0.f, 0.f, zPos) );
					glm::mat4 leavesMat = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * lOffsMat;

					Shaders* quadShdr = ps->getStdShader(GLSLParticleSystem2::QUADS);

					quadShdr->begin();
					quadShdr->setUniform1iv("texs", &leaveTexUnits[0], numLeaveTexs);
					quadShdr->setUniformMatrix4fv("m_pvm", &leavesMat[0][0]);
					quadShdr->setUniform1f("numPart", float(maxNrPart));
					quadShdr->setUniform1i("numTex", numLeaveTexs);

					for (auto i=0;i<numLeaveTexs;i++)
					{
						glActiveTexture(GL_TEXTURE0+i);
						glBindTexture(GL_TEXTURE_2D, texs[i]->getId());
					}

					ps->draw();




					///////////// frases solid

				//	if(gotActivity) {

						// draw leaves particles
						frase_from_mat = cp->projection_matrix_mat4 * cp->view_matrix_mat4
							* glm::translate( glm::vec3(0.f, 0.f, 0.f) )
							* glm::scale( glm::vec3(1.f, 1.f, 0.f) );

						frase_to_mat = cp->projection_matrix_mat4 * cp->view_matrix_mat4
							* glm::translate( glm::vec3(0.f, 0.f, 0.f) )
							* glm::scale( glm::vec3(fraseBlendToAlphaMed->get(), fraseBlendToAlphaMed->get(), 0.f) );

						// draw glow from texture
						texAlphaShader->begin();
						texAlphaShader->setUniformMatrix4fv("m_pvm", &frase_from_mat[0][0]);
						texAlphaShader->setUniform1i("tex", 0);
						texAlphaShader->setUniform1f("alpha", fraseBlendFromAlpha);
						frase_solid_tex[fraseBlendFrom]->bind(0);
						quad->draw();

						// draw glow from texture
						texAlphaShader->begin();
						texAlphaShader->setUniformMatrix4fv("m_pvm", &frase_to_mat[0][0]);
						texAlphaShader->setUniform1i("tex", 0);
						texAlphaShader->setUniform1f("alpha", fraseBlendToAlpha);
						frase_solid_tex[fraseBlendTo]->bind(0);
						quad->draw();

				//	}

					//////////


/*
					if (gotActivity){

						glDisable(GL_DEPTH_TEST);

						// draw red dot
						glm::mat4 redDotMat = glm::translate( glm::vec3(0.75f, 0.75f, 0.f) )
							* glm::scale(glm::vec3(0.2f * (cp->actFboSize.y / cp->actFboSize.x), 0.1f, 1.f));

						texShader->begin();
						texShader->setUniformMatrix4fv("m_pvm", &redDotMat[0][0]);
						texShader->setUniform1i("tex", 0);
						redDotTex->bind(0);
						quad->draw();

					}
*/
					glDisable(GL_DEPTH_TEST);



					///////////// frases glow

				//	if(gotActivity){

						// draw glow from texture
						texAlphaShader->begin();
						texAlphaShader->setUniformMatrix4fv("m_pvm", &frase_from_mat[0][0]);
						texAlphaShader->setUniform1i("tex", 0);
						texAlphaShader->setUniform1f("alpha", fraseBlendFromAlpha);
						frase_glow_tex[fraseBlendFrom]->bind(0);
						quad->draw();

						// draw glow from texture
						texAlphaShader->begin();
						texAlphaShader->setUniformMatrix4fv("m_pvm", &frase_to_mat[0][0]);
						texAlphaShader->setUniform1i("tex", 0);
						texAlphaShader->setUniform1f("alpha", fraseBlendToAlpha);
						frase_glow_tex[fraseBlendTo]->bind(0);
						quad->draw();
				//	}

					//////////


					// draw absolut logo
					float logoProp = float(absolut_logo_tex->getHeight()) / float(absolut_logo_tex->getWidth());
					logoProp *= cp->actFboSize.x / cp->actFboSize.y;

					float logoSize = 0.3f;
					glm::mat4 absLogo = glm::translate( glm::vec3(0.f, -0.6f, 0.f) )
						* glm::scale(glm::vec3(logoSize, logoSize* logoProp * logoDist, 1.f));

					texShader->begin();
					texShader->setUniformMatrix4fv("m_pvm", &absLogo[0][0]);
					texShader->setUniform1i("tex", 0);
					absolut_logo_tex->bind(0);
					quad->draw();

				}

				break;

			default:
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

void SNAbsolut::update(double time, double dt)
{
	// osc par
	ps->setReturnToOrg(true, reposFact);

	if (kin && kin->isReady())
	{
		if (!inited)
		{
			data.emitOrg = glm::vec3(0.f, 0.f, -0.05f);
			data.posRand = glm::vec3(1.f, 1.f, 0.05f);
			data.emitVel = glm::vec3(0.0f, 1.f, 0.f);
			data.speed = 0.f;
			data.size = 0.04f;
			data.sizeRand = 0.06f;
			data.texUnit = 1;
			data.texRand = 1.f;
			data.angleRand = 1.f;
			data.colInd = 1.f;

			ps->emit(time, nrPartLeaves, data, true);


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
			// draw glow from texture
			texAlphaShader->begin();
			texAlphaShader->setIdentMatrix4fv("m_pvm");
			texAlphaShader->setUniform1i("tex", 0);
			texAlphaShader->setUniform1f("alpha", fraseBlendFromAlpha);
			frase_solid_tex[fraseBlendFrom]->bind(0);
			quad->draw();
			// - FastBlurs --

			fblur = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
			fblur2nd = new FastBlurMem(0.84f, shCol, fblurSize, fblurSize);
			optFlowBlur = new FastBlurMem(0.84f, shCol, kin->getDepthWidth(),
										  kin->getDepthHeight(), GL_RGB16F);

			inited = true;

		} else
		{

			// upload depth image, wird intern gecheckt, ob neues Bild erforderlich oder nicht
			if (kin->uploadDepthImg(false))
			{
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


				// generate bw version of the blured optical flow for measuring the activity
				bwActFbo->bind();
				bwActFbo->clear();

				optFlow2BwShdr->begin();
				optFlow2BwShdr->setIdentMatrix4fv("m_pvm");
				optFlow2BwShdr->setUniform1i("tex", 0);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());

				rawQuad->draw();
				bwActFbo->unbind();

				// start Histogram for Activity calculation
				histo->proc(bwActFbo->getColorImg());

				// update the fluid simulation
				fluidSim->setVelTexThresh(fluidVelTexThres);
				fluidSim->setSmokeBuoyancy(fluidSmoke);
				fluidSim->setVelTexRadius(fluidVelTexRadius);
				fluidSim->dissipation = std::pow(fluidDissip, 0.2) * 0.1f + 0.9f;
				fluidSim->velocityDissipation = std::pow(fluidVelDissip, 0.2) * 0.1f + 0.9f;

				fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
				fluidSim->update();
			}
		}
	}



	if (!blockChange){

		// activity
		if (histo->getEnergySum(0.1f) > activityThres) {

			// if there was no activity before start counting
			if (!gotActivity)
			{
				gotActivity = true;
				initActivityTime = time;

			} else {

				unsigned int id = std::min(static_cast<unsigned int>(progress * double(numFrases-1)), numFrases-1);
				if (fraseBlendFrom != id && id != 0)
				{
					blockStart = time;
					blockChange = true;
				}

				updateFade(time);
			}


		} else {

			// inactivity
			if (gotActivity) {

				// continue updating
				updateFade(time);

				if(!requestOff)
				{
					offTime = time;
					requestOff = true;

				} else
				{

					if ((time - offTime) > timeToSetOff)
					{

						gotActivity = false;
						requestOff = false;
						wasSetOffOn = time;
					}
				}
			} else {

				// fade out alpha if there is any
				fraseBlendFromAlpha = std::fmax(fraseBlendFromAlpha - (time - wasSetOffOn) * fadeOutFact, 0.f);
				fraseBlendToAlpha = std::fmax(fraseBlendToAlpha - (time - wasSetOffOn) * fadeOutFact, 0.f);

				fraseBlendFromAlphaMed->update( fraseBlendFromAlpha );
				fraseBlendToAlphaMed->update( fraseBlendToAlpha );
			}
		}

	} else {

		if (time - blockStart > breakTime)
		{
			blockChange = false;
			initActivityTime += breakTime;

		} else {

			// wegen median alphas weiter blenden
			fraseBlendFromAlphaMed->update(fraseBlendFromAlpha);
			fraseBlendToAlphaMed->update( fraseBlendToAlpha );

		}
	}

	// update particle system
	ps->update(time, false, fluidSim->getVelocityTex());
}

//----------------------------------------------------

void SNAbsolut::updateFade(double time){

	// how long did we have activity??
	progress = std::min( (time - initActivityTime) / destinyTime, 1.0 );


	fraseBlendFrom = std::min(static_cast<unsigned int>(progress * double(numFrases-1)), numFrases-1);
	fraseBlendTo = std::min(fraseBlendFrom +1, numFrases-1);


	frasesMix = float(progress * double(numFrases-1)) - float(fraseBlendFrom);

	fraseBlendFromAlpha = 1.f -frasesMix;
	fraseBlendToAlpha = frasesMix;
	fraseBlendFromAlphaMed->update(fraseBlendFromAlpha);
	fraseBlendToAlphaMed->update(fraseBlendToAlpha);

	//std::cout << "progress: " << progress  << " blendFrom: " << fraseBlendFrom << "  to " << fraseBlendTo << " mix: " << frasesMix << std::endl;

}

//----------------------------------------------------

void SNAbsolut::initShaders()
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

	stdVert = "// SNAutLeaveVel depth threshold vertex shader\n" +shdr_Header +stdVert;
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

	frag = "// SNAutLeaveVel depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNAutLeaveVel_thres", stdVert.c_str(), frag.c_str());

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

	vert = "// SNAutLeaveVel edge detect vertex shader\n" +shdr_Header +vert;

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

	frag = "// SNAutLeaveVel edge detect fragment shader\n"+shdr_Header+frag;

	edgeDetect = shCol->addCheckShaderText("SNAutLeaveVel_edge", vert.c_str(), frag.c_str());

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

	frag = "// SNAutLeaveVel subtract frag shader\n"+shdr_Header+frag;

	subtrShadr = shCol->addCheckShaderText("SNAutLeaveVel_subtr", stdVert.c_str(), frag.c_str());

	//------------------------------------------------------------

	frag = STRINGIFY(uniform sampler2D tex;
					 in vec2 tex_coord;
					 layout (location = 0) out vec4 color;
					 void main()
					 {
						 color = vec4(texture(tex, tex_coord).r > 0.7 ? 1.0 : 0.0);
					 });

	frag = "// SNAutLeaveVel second thresh frag shader\n"+shdr_Header+frag;

	blurThres = shCol->addCheckShaderText("SNAutLeaveVel_thres2", stdVert.c_str(), frag.c_str());


	//------------------------------------------------------------

	frag = STRINGIFY(uniform sampler2D tex;
					 in vec2 tex_coord;
					 layout (location = 0) out vec4 color;
					 void main() {
						 vec4 texCol = texture(tex, tex_coord);
						 color = vec4(texCol.r + texCol.g, 0.0, 0.0, 1.0);
					 });

	frag = "// SNAutLeaveVel optical flow bw frag shader\n"+shdr_Header+frag;

	optFlow2BwShdr = shCol->addCheckShaderText("SNAutLeaveVel_optFlowBlur", stdVert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNAbsolut::onKey(int key, int scancode, int action, int mods)
{
	// trapez korrektur schnell und schmutzig
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
			case GLFW_KEY_6 : actDrawMode = OPT_FLOW_BLUR_BW;
				printf("actDrawMode = OPT_FLOW_BLUR_BW \n");
				break;
			case GLFW_KEY_7 : actDrawMode = FLUID_VEL;
				printf("actDrawMode = FLUID_VEL \n");
				break;
			case GLFW_KEY_8 : actDrawMode = DRAW;
				printf("actDrawMode = DRAW \n");
				break;
		}
	}
}

//----------------------------------------------------

void SNAbsolut::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = 1.f - (ypos / scd->screenHeight);
}
}
