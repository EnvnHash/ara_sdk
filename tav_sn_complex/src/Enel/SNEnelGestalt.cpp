//
//  SNEnelGestalt.cpp
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

#include "SNEnelGestalt.h"

using namespace std;
using namespace cv;

namespace tav
{
SNEnelGestalt::SNEnelGestalt(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrParticle(800000),
flWidth(256), flHeight(256), emitPartPerUpdate(6000), thinDownSampleFact(8),
fblurSize(256),
noiseTexSize(256),
captureIntv(120.0),
inActAddNoiseInt(40.0),  // wenn nix los emit noise
actDrawMode(DRAW),  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
depthThresh(3500.f),
fastBlurAlpha(0.45f),
contThresh(163.f),
lineFdbk(0.3f),
optFlowBlurAlpha(0.76f),
fluidColorSpeed(1.f),
fluidColTexForce(0.24f),
fluidDissip(0.8f),
fluidVelTexThres(0.8f),
fluidVelTexRadius(0.129f),
fluidVelTexForce(1.74f),
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
lineWidth(2.2f),
impFreq(25.408f),
impFreq2(3.8f),
emitContourInt(0.125), // seconds
silLifeTime(5.0),
nrSilhouttePoints(220),
nrSilInstances(23.f),
instScaleAmt(3.34f),
silMed(2.f),
timeOffsScale(9.6f),
timeScaleZoom(0.026f),
rotGradOffs(2.75f),
lineBlurAlpha(0.35f),
lineBlurOffs(1.35f),
sentFluidDist(0.01f),
exp(0.0089f),
dens(0.276f),
grAlpha(0.15f),
typoSpeed(0.03f),
typoMoveAmt(0.1f),
accumAlpha(0.05f),
switchFade(6.0),
godRaySpeed(0.02f),
colBase(glm::vec4(0.f, 0.f, 0.4f, 1.f)),
colPeak(glm::vec4(0.93f, 0.76f, 0.88f, 1.f)),
drawParticles(false),
trig(0.f),
clear(0.f)
{
	kin = static_cast<KinectInput*>(scd->kin);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	FreeTypeFont** fonts = static_cast<FreeTypeFont**>(scd->ft_fonts);

	// get onKey function
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	// - add OSC Parameter --

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
	addPar("veloBlend", &veloBlend); // 0.f, 1.f, 0.0001f, 0.05f, OSCData::LIN);


	addPar("depthThresh", &depthThresh); // 10.f, 5000.f, 1.f, 1755.f, OSCData::LIN);
	addPar("fastBlurAlpha", &fastBlurAlpha); // 0.f, 1.f, 0.0001f, 0.4f, OSCData::LIN);
	addPar("optFlowBlurAlpha", &optFlowBlurAlpha); //0.f, 1.f, 0.0001f, 0.76f, OSCData::LIN);
	addPar("contThresh", &contThresh);
	addPar("minArea", &params.minArea);
	addPar("maxArea", &params.maxArea);
	addPar("lineFdbk", &lineFdbk);
	addPar("lineWidth", &lineWidth); // 0.f, 1000.f, 0.0001f, 0.05f, OSCData::LIN);
	addPar("impFreq", &impFreq);
	addPar("impFreq2", &impFreq2);
	addPar("nrSilInstances", &nrSilInstances);
	addPar("instScaleAmt", &instScaleAmt);
	addPar("silMed", &silMed);
	addPar("timeOffsScale", &timeOffsScale);
	addPar("timeScaleZoom", &timeScaleZoom);
	addPar("rotGradOffs", &rotGradOffs);
	addPar("lineBlurAlpha", &lineBlurAlpha);
	addPar("lineBlurOffs", &lineBlurOffs);
	addPar("sentFluidDist", &sentFluidDist);

	addPar("typoSpeed", &typoSpeed);
	addPar("typoMoveAmt", &typoMoveAmt);
	addPar("accumAlpha", &accumAlpha);

    addPar("exp", &exp);
    addPar("dens", &dens);
    addPar("decay", &decay);
    addPar("weight", &weight);
    addPar("lightX", &lightX);
    addPar("lightY", &lightY);
    addPar("grAlpha", &grAlpha);
    addPar("godRaySpeed", &godRaySpeed);

	addPar("trig", &trig);
	addPar("clear", &clear);


    // ------- FBOs --------

	particleFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight, GL_RGBA8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	particleFbo->clear();

	gradFbo = new FBO(shCol, fblurSize, fblurSize);
	gradFbo->clear();

	lineFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);
	lineSnapFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);

//	typoFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);
	accumFbo = new FBO(shCol, _scd->screenWidth/2, _scd->screenHeight/2);
	diffFbo = new FBO(shCol, _scd->screenWidth/2, _scd->screenHeight/2, GL_R8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	typoMaskFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);

	// ------- Historgram -------------------

	histo = new GLSLHistogram(shCol, _scd->screenWidth, _scd->screenHeight,
			GL_R8, 2, 128, true, false, 1.f);


// muevate kommt als typo nach links neben das logo
	//muevate = new TextureManager();
	//muevate->loadTexture2D((*_scd->dataPath)+"/textures/muevate.png");

	//-----------------------------------------------

	lineBlur = new FastBlurMem(0.84f, shCol, scd->screenWidth/2, scd->screenHeight/2);

    // ------- Partikel System --------

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
	data.extVelAmt = 0.2f;
	data.ageFadeCurv = 0.6f;
//        data.size = 0.06f;
	ps->setEmitData(&data);

	//-----------------------------------------------

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


	nrEnelColors = 8;
	enel_colors = new glm::vec4[nrEnelColors];
	enel_colors[0] = glm::vec4(0.9f, 0.078f, 0.f, 1.f);			// rot
	enel_colors[1] = glm::vec4(1.f, 0.059f, 0.39f, 1.f);		// hell rot
	enel_colors[2] = glm::vec4(0.0196f, 0.3333f, 0.98f, 1.f);	// blau
	enel_colors[3] = glm::vec4(0.f, 0.549f, 0.353f, 1.f);		// dunkel grün
	enel_colors[4] = glm::vec4(1.f, 0.353f, 0.059f, 1.f);		// orange
	enel_colors[5] = glm::vec4(1.f, 0.2745f, 0.53f, 1.f);		// rosa
	enel_colors[6] = glm::vec4(0.255f, 0.725f, 0.9f, 1.f);		// hellblau
	enel_colors[7] = glm::vec4(0.333f, 0.745f, 0.353f, 1.f);	// hellgrün


	// ------------

	godRays = new GodRays(shCol, fblurSize, fblurSize, 60);

	// forceScale nur zum Mouse test
	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.125f,
						   static_cast<float>(flHeight) * 0.125f);


	// --- Geo Primitives ---
	rawQuad = static_cast<Quad*>(_scd->stdQuad);
	rotateQuad = static_cast<Quad*>(_scd->stdHFlipQuad);
	colQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 1.f, 1.f, 1.f);
	quadAr = new QuadArray(20, 20);


	// vao mit irgendwas drin, damit die sache mit dem shader storage buffer richtig funktioniert
	rawPointVAO = new VAO("position:4f", GL_STATIC_DRAW);
	GLfloat* pos = new GLfloat[(nrSilhouttePoints-1) *6 *4];
	memset(&pos[0], 0.f, (nrSilhouttePoints-1) *6 *4);
	rawPointVAO->upload(POSITION, pos, (nrSilhouttePoints-1) *6);

	//-----------------------------------------------
	// --- Shaders ---

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
	texAlphaShader = shCol->getStdTexAlpha();
	stdMultiTex = shCol->getStdTexMulti();
	stdColAlpha = shCol->getStdColAlpha();


	initShaders();
	initSentShdr();
	initAccumShdr();
	initDiffShdr();

	//-----------------------------------------------

	timer.showFps(true);
	silhoutte_frames.resize(0);

	//--------------------------------------------------

	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
	{
		// init sentences
		nrSentences = 4;
		sentPtr = 0;
		textBlocks = new NVTextBlock*[nrSentences];

		words = new std::string[nrSentences];
		words[0] = "Sólo esto podemos hoy decirte: lo que no somos, lo que no queremos.";
		words[2] =	"Si sta come d'autunno sugli alberi le foglie.";
		words[1] =	"Mi illumino d'immenso";
		words[3] =	"El amor que mueve el sol y las demás estrellas";


		for (unsigned int i=0;i<nrSentences;i++)
		{
			textBlocks[i] = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font0")) ] );
		}

	} else {
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{

	/*
	if (!textInited)
	{
		for (unsigned int i=0;i<nrSentences;i++)
		{
			textBlocks[i]->setAlign(CENTER_X);
			textBlocks[i]->setString(words[i], cp);
			textBlocks[i]->setTextColor(enel_colors[2].x, enel_colors[2].y, enel_colors[2].z, 1.f);
		}

		textInited = true;
	}
	*/

	if(inited)
	{
		if (_tfo)
		{
			_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		renderTypo(time, cp);

		switch(actDrawMode)
		{
			case RAW_DEPTH : // 1
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));
				rawQuad->draw();
				break;

			case DEPTH_THRESH: // 2
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, threshFbo->getColorImg());
				rawQuad->draw();
				break;

			case DEPTH_BLUR: // 3
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
				rawQuad->draw();
				break;

			case OPT_FLOW:	//4
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, optFlow->getResTexId());
				rawQuad->draw();
				break;

			case OPT_FLOW_BLUR: // 5
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, optFlowBlur->getResult());
				rawQuad->draw();
				break;

			case FLUID_VEL:	// 6
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());
				rawQuad->draw();
				break;

			case SENTENCES:	// 7
				/*
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());
				rawQuad->draw();
*/
				break;

			case DRAW: // 8
			{
				// gradient_textures
				gradFbo->bind();

				gradShader->begin();
				gradShader->setIdentMatrix4fv("m_pvm");
				gradShader->setUniform4fv("colBase", glm::value_ptr(eCol));
//				gradShader->setUniform4fv("colBase", glm::value_ptr(colBase));
				gradShader->setUniform4fv("colPeak", glm::value_ptr(colPeak));
				gradShader->setUniform1f("timeOffs", time * timeOffsScale);
				gradShader->setUniform1f("impFreq", impFreq);
				gradShader->setUniform1f("impFreq2", impFreq2);
				rawQuad->draw();

				gradFbo->unbind();

				//----------------------------------
				// renderSilhouttes

				if (reqSnapShot || reqDeleteSnapBuf)
				{
					lineSnapFbo->bind();

					if (reqDeleteSnapBuf)
					{
						lineSnapFbo->clear();
						reqDeleteSnapBuf = false;
					}

					renderSilhouettes(time);
					lineSnapFbo->unbind();

					reqSnapShot = false;
				}

				//----------------------------------

				lineFbo->bind();
				lineFbo->clear();

				// draw old snapshots
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lineSnapFbo->getColorImg());
				rawQuad->draw();

				renderSilhouettes(time);

				lineFbo->unbind();

				//----------------------------------
				// calculate blur

				lineBlur->setAlpha( lineBlurAlpha );
				lineBlur->setOffsScale( lineBlurOffs );
				lineBlur->proc( lineFbo->getColorImg() );
				//for (unsigned int i=0;i<4;i++)
				//	lineBlur->proc( lineBlur->getResult() );

				//-----------------------------------
				// draw particles into fbo for feedback

				particleFbo->bind();
				particleFbo->clear();
				//particleFbo->clearAlpha(partFdbk);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				ps->draw( cp->mvp );
				particleFbo->unbind();

				//------------------------------

				/*

				// draw Particles to accumulation buffer
				accumFbo->bind();

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				accumShader->begin();
				accumShader->setIdentMatrix4fv("m_pvm");
				accumShader->setUniform1i("tex", 0);
				accumShader->setUniform1i("maskTex", 1);
				accumShader->setUniform1f("burst", switchProg * 0.25f);
				accumShader->setUniform1f("alpha", accumAlpha);

				glActiveTexture(GL_TEXTURE0);
				//glBindTexture(GL_TEXTURE_2D, lineFbo->getColorImg());
				glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex() );

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());

				rawQuad->draw();


				// also accumulate the particleFbo
				//glActiveTexture(GL_TEXTURE0);
				//glBindTexture(GL_TEXTURE_2D, particleFbo->getColorImg());
				//rawQuad->draw();

				accumFbo->unbind();



				//---------------------------------------
				// measure progress

				diffFbo->bind();
				diffFbo->clear();

				glBlendFunc(GL_ONE, GL_ZERO);

				diffShader->begin();
				diffShader->setIdentMatrix4fv("m_pvm");
				diffShader->setUniform1i("tex", 0);
				diffShader->setUniform1i("refTex", 1);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, accumFbo->getColorImg());

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());

				rawQuad->draw();

				diffFbo->unbind();


//				glClear(GL_COLOR_BUFFER_BIT);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
*/
				//------------------------------

			//	procProgress(time, dt);

				//---------------------------------------
				// draw blur

				texAlphaShader->begin();
				texAlphaShader->setIdentMatrix4fv("m_pvm");
				texAlphaShader->setUniform1i("tex", 0);
				texAlphaShader->setUniform1f("alpha", 0.3f);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lineBlur->getResult());
				rawQuad->draw();

				//-----------------------------------------
				// draw particles

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, particleFbo->getColorImg());
				rawQuad->draw();

				//-------------------------------------
				// draw lines

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lineFbo->getColorImg());
				rawQuad->draw();

				//----------------------------------
				// draw lines again with godrays

				godRays->setExposure( exp );
				godRays->setDensity( dens );
				godRays->setDecay( decay );
				godRays->setWeight( weight );

				lightX = glm::perlin( glm::vec2(time * godRaySpeed + 0.1f, time * godRaySpeed + 0.12f ) );
				lightY = glm::perlin( glm::vec2(time * godRaySpeed, time * godRaySpeed + 0.06f) );

				godRays->setLightPosScr( lightX, lightY );
				godRays->setAlpha( grAlpha );

				godRays->bind();


				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, lineFbo->getColorImg());
				rawQuad->draw();

				//texShader->begin();
				//texShader->setIdentMatrix4fv("m_pvm");
			//	texShader->setUniform1i("tex", 0);
				//glBindTexture(GL_TEXTURE_2D, particleFbo->getColorImg());
				//rawQuad->draw();

				godRays->unbind();
				godRays->draw();



				//--------------------------------------------
				// draw accum fbo

				//renderTypoVelTex(time, cp);


			}
				break;

			default:
				break;
		}
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::procProgress(double time, double dt)
{
	// get the range of filling
	histo->proc(diffFbo->getColorImg());

	if (!block)
	{
		if (!startSwitch)
		{
			//std::cout << "histo->getMaximum(0) " << histo->getMaximum(0)<< std::endl;

			typoProgress = std::fmax( 1.f - ( std::fmin(histo->getMaximum(0), 1266.f) / 1266.f ), 0.f);
			//std::cout << "typoProgress " << typoProgress << std::endl;

			if ( typoProgress > 0.9f )
			{

				startSwitchTime = time;
				startSwitch = true;

			}

		} else {

			if (time - startSwitchTime < switchFade)
			{
				//std::cout << "switching " <<  switchProg << std::endl;

				switchProg = (time - startSwitchTime) / switchFade;
				typoProgress = std::fmin( switchProg + typoProgress, 1.f );

			} else {

				//std::cout << "stop switching " <<  switchProg << std::endl;

				stopSwitchTime = time;

				// init sentences
				sentPtr = (sentPtr +1) % nrSentences;


				accumFbo->bind();
				glClearColor(0.f, 0.f, 0.f, 0.f);
				glClear(GL_COLOR_BUFFER_BIT);
				accumFbo->unbind();

				accumFbo->clear();

				diffFbo->clear();
			//	typoFbo->clear();
				particleFbo->clear();

				startSwitch = false;
				block = true;
				switchProg = 0;

			}
		}
	} else {

		fadeDown += dt;

		//typoProgress = std::fmax( 1.f - ( std::fmin(histo->getMaximum(0), 666.f) / 666.f ), 0.f);
		//std::cout << "typoProgress " << typoProgress << std::endl;

	}
	//				std::cout << typoProgress<< std::endl;

}

//---------------------------------------------------------------

void SNEnelGestalt::renderTypo(double time, camPar* cp)
{
	/*
	typoPosMat = glm::translate(
			glm::vec3(
					glm::perlin( glm::vec2(time * typoSpeed) ) * typoMoveAmt,
					glm::perlin( glm::vec2(time * typoSpeed + 0.1f) ) * typoMoveAmt,
					0.f )
	);

	GLuint typoTex = textBlocks[sentPtr]->drawToTex(cp);

	typoFbo->bind();
	typoFbo->clear();

	stdMultiTex->begin();
	stdMultiTex->setIdentMatrix4fv("m_pvm");
	stdMultiTex->setUniform1i("tex", 0);
	stdMultiTex->setUniform1i("nrSamples", textBlocks[sentPtr]->getNrSamples());
	stdMultiTex->setUniform2fv("fboSize", &cp->actFboSize[0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, typoTex);

	rawQuad->draw();

	typoFbo->unbind();
*/
}

//---------------------------------------------------------------

void SNEnelGestalt::renderTypoVelTex(double time, camPar* cp)
{
	float scaleAmt = 0.6f + typoProgress;

	typoPosMat = glm::translate( glm::vec3(
					glm::perlin( glm::vec2(time * typoSpeed) ) * (typoMoveAmt * (1.f - typoProgress)) - 0.2f,
					glm::perlin( glm::vec2(time * typoSpeed + 0.1f) ) * (typoMoveAmt * (1.f - typoProgress)) - 0.1f,
					0.f ))
		* glm::scale( glm::vec3(scaleAmt, scaleAmt, 1.f) );

	glm::mat4 pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;



	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sentenceShader->begin();
	sentenceShader->setUniformMatrix4fv("m_model", &typoPosMat[0][0]);
//	sentenceShader->setIdentMatrix4fv("m_model");
	sentenceShader->setUniformMatrix4fv("m_pv", &pv[0][0]);


	sentenceShader->setUniform1i("tex", 0);
	sentenceShader->setUniform1i("fluidMaskTex", 1);
	sentenceShader->setUniform1i("fluidVelTex", 2);


	sentenceShader->setUniform1f("fluidDist", sentFluidDist * (1.f - typoProgress));
	sentenceShader->setUniform1i("nrSamples", textBlocks[sentPtr]->getNrSamples() );
	sentenceShader->setUniform2fv("fboSize", &cp->actFboSize[0]);
	sentenceShader->setUniform1f("alpha", 1.f);
//	sentenceShader->setUniform1f("alpha", std::pow(typoProgress, 2.f));
	sentenceShader->setUniform4fv("typoCol", &enel_colors[3][0]);


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getResTex());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());

	quadAr->draw();

}

//---------------------------------------------------------------

void SNEnelGestalt::renderSilhouettes(double time)
{
	if (silhoutte_frames.size() > 0)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		glLineWidth(lineWidth);

		fluidLineShdr->begin();
		rawPointVAO->bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gradFbo->getColorImg());

		fluidLineShdr->setUniform1i("numLinePoints", nrSilhouttePoints);
		fluidLineShdr->setUniform1f("numInstances", float(nrSilInstances));
		fluidLineShdr->setUniform1f("scaleAmt", instScaleAmt);
//		fluidLineShdr->setUniform4fv("colBase", glm::value_ptr(colBase));
		fluidLineShdr->setUniform4fv("colBase", glm::value_ptr(eCol));
		fluidLineShdr->setUniform4fv("colPeak", glm::value_ptr(colPeak));
		fluidLineShdr->setUniform1f("timeOffs", time * timeScaleZoom);
		fluidLineShdr->setUniform1i("gradTex", 0);
		fluidLineShdr->setUniform1f("rotGradOffs", rotGradOffs);


		for(std::vector<contPar>::iterator it = silhoutte_frames.back().begin(); it != silhoutte_frames.back().end(); ++it)
		{
			fluidLineShdr->setUniform1f("progress", float( (time - (*it).startTime) / (*it).lifeTime) );
			fluidLineShdr->setUniform4fv("center",  glm::value_ptr( (*it).center ) );
			//fluidLineShdr->setUniform1f("lineWidth", lineWidth);

			glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1, (*it).buf->getBuffer() );

			glDrawArraysInstanced(GL_LINE_LOOP, 0, nrSilhouttePoints, GLsizei(nrSilInstances));

			glBindBufferBase( GL_SHADER_STORAGE_BUFFER, 1,  0 );
		}

		rawPointVAO->unbind();
		fluidLineShdr->end();
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::update(double time, double dt)
{

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

			img = cv::Mat(fblurSize, fblurSize, CV_8UC3);
			inited = true;

		} else
		{
			//--- Proc Depth Image and Update the Fluid Simulator --
			// upload depth image, wird intern gecheckt, ob neues Bild erforderlich oder nicht

			if (kin->uploadDepthImg(false))
			{
				//frameNr = kin->getDepthUplFrameNr(false);

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

				// ------ apply blur on silhoutte --------
				fblur->setAlpha(fastBlurAlpha);
				fblur->proc(threshFbo->getColorImg());

				fblur2nd->setAlpha(fastBlurAlpha);
				fblur2nd->proc(fblur->getResult());

				// calculate contour
				//if (time - lastEmitContourTime > emitContourInt)
				//{
					procContours(fblur2nd->getResult(), time);
					//lastEmitContourTime = time;
				//}


				//-------------------------------------------------------

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
			float timeMult = std::fmod( time * fluidColorSpeed, float(nrEnelColors) );
			unsigned int lowCol = std::floor( timeMult );
			unsigned int highCol = (lowCol + 1) % nrEnelColors;
			float bAmt = std::fmod( timeMult, 1.f );

			eCol = glm::mix( enel_colors[lowCol], enel_colors[highCol], bAmt);


			fluidSim->setVelTexThresh(fluidVelTexThres);
			fluidSim->setSmokeBuoyancy(fluidSmoke);
			fluidSim->setVelTexRadius(fluidVelTexRadius);
			fluidSim->dissipation = fluidDissip;
			fluidSim->velocityDissipation = std::pow(fluidVelDissip, 0.2) * 0.1f + 0.9f;

			fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
			fluidSim->addColor(optFlowBlur->getResult(), glm::vec3(eCol), fluidColTexForce, true);
			fluidSim->update();

			ps->update(time, fluidSim->getVelocityTex());
		}
	}

	if (trig == 1.f)
	{
		reqSnapShot = true;
		std::cout << "body trig" << std::endl;
		trig = 0.f;
	}

	if (clear == 1.f)
	{
		reqDeleteSnapBuf = true;
		std::cout << "body clear" << std::endl;
		clear = 0.f;
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::procContours(GLuint texId, double time)
{
	std::vector<cv::Vec4i> hierarchy;
	cv::Mat threshold_output;


	// download image
	glBindTexture(GL_TEXTURE_2D, texId);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);


	/// Convert image to gray and blur it
	cv::cvtColor( img, src_gray, COLOR_BGR2GRAY );
//	blur( src_gray, src_gray, Size(3,3) );


	// Detect edges using Threshold
	cv::threshold( src_gray, threshold_output, int(contThresh), 255, THRESH_BINARY );

	// Find contours
	contours.clear();
	cv::findContours( threshold_output, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0) );


	// add new vector with silhoutte_frames
	silhoutte_frames.push_back( std::vector<contPar>() );
		std::vector<contPar>* newSet = &silhoutte_frames.back();


	// filter the contours
    for (size_t contourIdx = 0; contourIdx < contours.size(); contourIdx++)
    {
        Center center;
        center.confidence = 1;
        Moments moms = moments(Mat(contours[contourIdx]));

        {
            double area = moms.m00;
            center.area = area;
            if (params.filterByArea && (area < params.minArea || area >= params.maxArea))
                continue;
        }

        {
            double area = moms.m00;
            double perimeter = arcLength(Mat(contours[contourIdx]), true);
            double ratio = 4 * CV_PI * area / (perimeter * perimeter);
            center.circularity = ratio;
            if (params.filterByCircularity && (ratio < params.minCircularity || ratio >= params.maxCircularity))
                continue;
        }

        {
            double denominator = sqrt(pow(2 * moms.mu11, 2) + pow(moms.mu20 - moms.mu02, 2));
            const double eps = 1e-2;
            double ratio;
            if (denominator > eps)
            {
                double cosmin = (moms.mu20 - moms.mu02) / denominator;
                double sinmin = 2 * moms.mu11 / denominator;
                double cosmax = -cosmin;
                double sinmax = -sinmin;

                double imin = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmin - moms.mu11 * sinmin;
                double imax = 0.5 * (moms.mu20 + moms.mu02) - 0.5 * (moms.mu20 - moms.mu02) * cosmax - moms.mu11 * sinmax;
                ratio = imin / imax;
            }
            else ratio = 1;

            if (params.filterByInertia && (ratio < params.minInertiaRatio || ratio >= params.maxInertiaRatio))
                continue;

            center.confidence = ratio * ratio;
        }

        {
            vector < Point > hull;
            convexHull(Mat(contours[contourIdx]), hull);
            double area = contourArea(Mat(contours[contourIdx]));
            double hullArea = contourArea(Mat(hull));
            double ratio = area / hullArea;
            center.convexity = ratio;

            if (params.filterByConvexity && (ratio < params.minConvexity || ratio >= params.maxConvexity))
                continue;
        }

        center.location = Point2d(moms.m10 / moms.m00, moms.m01 / moms.m00);

        //if (params.filterByColor)
        //    if (binaryImage.at<uchar> (cvRound(center.location.y), cvRound(center.location.x)) != params.blobColor)
        //        continue;

        //compute blob radius
        {
            vector<double> dists;
            for (size_t pointIdx = 0; pointIdx < contours[contourIdx].size(); pointIdx++)
            {
                Point2d pt = contours[contourIdx][pointIdx];
                dists.push_back(norm(center.location - pt));
            }
            std::sort(dists.begin(), dists.end());
            center.radius = (dists[(dists.size() - 1) / 2] + dists[dists.size() / 2]) / 2.0;
        }


		// create a spline with the found contour
		Spline2D contourToInterp;
		for(std::vector<cv::Point>::iterator it = contours[contourIdx].begin(); it!= contours[contourIdx].end(); ++it)
			contourToInterp.push_back( glm::vec2( (*it).x, (*it).y ) );

		newSet->push_back( contPar() );
		newSet->back().lifeTime = silLifeTime;
		newSet->back().startTime = time;

		newSet->back().area = center.area;
		newSet->back().buf = new ShaderBuffer<glm::vec4>(nrSilhouttePoints); // memory leak?
		newSet->back().center = glm::vec4(
				(center.location.x / float(fblurSize)) * 2.f -1.f,
				(center.location.y / float(fblurSize)) * 2.f -1.f,
				0.f, 1.f);
		newSet->back().circularity = center.circularity;
		newSet->back().cpuBuf = new glm::vec4[nrSilhouttePoints];
		newSet->back().confidence = center.confidence;
		newSet->back().convexity = center.convexity;
		newSet->back().nrRawPoints = contours[contourIdx].size();
		newSet->back().radius = center.radius / double(fblurSize);



		glm::vec4* ptr = newSet->back().buf->map();

		// load the fixed number of points into the ShaderBuffer
		for ( unsigned int j=0; j<nrSilhouttePoints; j++)
		{
			float fInd = static_cast<float>(j) / static_cast<float>(nrSilhouttePoints-1);

			newSet->back().cpuBuf[j] = glm::vec4(
					(contourToInterp.sampleAt(fInd).x / float(fblurSize)) * 2.f -1.f,
					(contourToInterp.sampleAt(fInd).y / float(fblurSize)) * 2.f -1.f,
					0.f,
					1.f);

			(*ptr++) = newSet->back().cpuBuf[j];
		}

		newSet->back().buf->unmap();

    }

	// if there is a previous frame do tracking
	// if the assignment is clear and everything set, delete first (old) element
	// memory of contPar buffers is automatically freed
	if (silhoutte_frames.size() > 1)
	{
	//	trackBlobs();
		silhoutte_frames.erase(silhoutte_frames.begin());
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::trackBlobs()
{
	unsigned int nrParaToDiff = 7;

	std::vector< std::pair<unsigned int, std::vector<contPar>::iterator> >* voting =
			new std::vector< std::pair<unsigned int, std::vector<contPar>::iterator> >[silhoutte_frames[0].size()];

	// which shape from the old set [0] corresponds to the new set [1] ?
	// check everything against everything and calculate the differences
	for (std::vector<contPar>::iterator oldIt = silhoutte_frames[0].begin(); oldIt != silhoutte_frames[0].end(); ++oldIt)
	{
		unsigned int oldInd = oldIt - silhoutte_frames[0].begin();

		// create a map for this shape with all the differences to all the new shapes
		// center, radius, confidence, number of number of rawpoints
		std::vector< std::map<float, std::vector<contPar>::iterator> > par_differences;

		// we are checking for 7 parameters
		for (unsigned int i=0; i<nrParaToDiff; i++)
			par_differences.push_back( std::map<float, std::vector<contPar>::iterator>() );

		// iterate through all new shapes
		for (std::vector<contPar>::iterator newIt = silhoutte_frames[1].begin(); newIt != silhoutte_frames[1].end(); ++newIt)
		{
			// center difference
			par_differences[0][ std::fabs(glm::length( glm::vec3((*oldIt).center) - glm::vec3((*newIt).center) )) ] = newIt;

			// radius difference
			par_differences[1][ std::fabs( (*oldIt).radius - (*newIt).radius ) ] = newIt;

			// confidence difference
			par_differences[2][ std::fabs( (*oldIt).confidence - (*newIt).confidence ) ] = newIt;

			// nr raw points difference
			par_differences[3][ std::fabs( float((*oldIt).nrRawPoints - (*newIt).nrRawPoints) ) ] = newIt;

			// area difference
			par_differences[4][ std::fabs( (*oldIt).area - (*newIt).area ) ] = newIt;

			// circularity
			par_differences[5][ std::fabs( (*oldIt).circularity - (*newIt).circularity ) ] = newIt;

			// convexivity
			par_differences[6][ std::fabs( (*oldIt).convexity - (*newIt).convexity ) ] = newIt;
		}


		// for each new shape sum up all differences and save as voting
		for (std::vector<contPar>::iterator newIt = silhoutte_frames[1].begin(); newIt != silhoutte_frames[1].end(); ++newIt)
		{
			unsigned int newInd = newIt - silhoutte_frames[1].begin();
			voting[oldInd].push_back( std::make_pair(0, oldIt) );	// add a new entry for this old Shape

			for (unsigned int i=0; i<nrParaToDiff; i++)
				if ( (*par_differences[i].begin()).second == newIt )
					voting[oldInd].back().first++;
		}
	}

	// now that we got the voting, create a map to assign shapes from frame[0] to frame[1]
	// only the new shapes count, if there are more old ones than new ones, ignore them
	// [oldShape] = newShape
	unsigned int* shapeMap = new unsigned int[silhoutte_frames[0].size()];

	for (std::vector<contPar>::iterator oldIt = silhoutte_frames[0].begin(); oldIt != silhoutte_frames[0].end(); ++oldIt)
	{
		unsigned int maxRank = 0;
		unsigned int foundInd = 0;
		unsigned int oldInd = oldIt - silhoutte_frames[0].begin();

		// check all votings for the this shape, and get the highest ranking
		for (std::vector< std::pair<unsigned int, std::vector<contPar>::iterator> >::iterator vIt = voting[oldInd].begin();
			vIt != voting[oldInd].end(); ++vIt)
		{
			if ( (*vIt).first > maxRank)
			{
				maxRank = (*vIt).first;
				foundInd = (*vIt).second - silhoutte_frames[0].begin();
			}
		}

		shapeMap[oldInd] = foundInd;

		// kill the result from the votemap
		for (unsigned int i=0; i<silhoutte_frames[0].size(); i++)
		{
			for (std::vector< std::pair<unsigned int, std::vector<contPar>::iterator> >::iterator vIt = voting[i].begin();
				vIt != voting[i].end();)
			{
				if ( (*vIt).second == silhoutte_frames[1].begin() + foundInd )
				{
					voting[i].erase(vIt);

				} else ++vIt;
			}
		}
	}

	// now that we got the shape map, smooth the points of the new and old shapes
	for(unsigned int i=0; i<silhoutte_frames[1].size();i++)
	{
//		std::cout << "assigning old shape " << shapeMap[i] << " to new shape " << i << std::endl;
//		std::cout << std::endl;

/*
		// check if we have enough kalman filters
		if (kalmanFilters.size() < (i+1))
		{
			kalmanFilters.push_back( std::vector<CvKalmanFilter*>() );
			for ( unsigned int j=0; j<nrSilhouttePoints; j++)
			{
				kalmanFilters.back().push_back( new CvKalmanFilter(4, 2) );
				kalmanFilters.back().back()->update(silhoutte_frames[1][i].cpuBuf[j].x,
													silhoutte_frames[1][i].cpuBuf[j].x);
			}
		}

		// update kalman filters
		for ( unsigned int j=0; j<nrSilhouttePoints; j++)
			kalmanFilters[i][j]->predict();


		if( silhoutte_frames[0].size() > shapeMap[i] )
		{
			// get the offset of each of the shapepoint indices
			std::vector< std::map< float, unsigned int > > shapePointDist;

			for ( unsigned int newI=0; newI<nrSilhouttePoints; newI++)
			{
				shapePointDist.push_back( std::map<float, unsigned int>() );

				for ( unsigned int oldI=0; oldI<nrSilhouttePoints; oldI++)
					shapePointDist[newI][ glm::length( glm::vec2( kalmanFilters[i][oldI]->get(0), kalmanFilters[i][oldI]->get(1) )
											 		 - glm::vec2( silhoutte_frames[1][i].cpuBuf[newI])) ] = oldI;

					//shapePointDist[newI][glm::length(silhoutte_frames[0][shapeMap[i]].cpuBuf[oldI] - silhoutte_frames[1][i].cpuBuf[newI])] = oldI;
			}


			// filter points
			glm::vec4* ptr = silhoutte_frames[1][i].buf->map();
			for ( unsigned int j=0; j<nrSilhouttePoints; j++)
			{
				//std::cout << "shortest dist to new shape point " << j << " is from old: " << (*shapePointDist[j].begin()).second << std::endl;

				kalmanFilters[i][j]->update(silhoutte_frames[1][i].cpuBuf[ j].x,
											silhoutte_frames[1][i].cpuBuf[ j ].y);

//				kalmanFilters[i][j]->update(silhoutte_frames[1][i].cpuBuf[ (*shapePointDist[j].begin()).second ].x,
//											silhoutte_frames[1][i].cpuBuf[ (*shapePointDist[j].begin()).second ].y);

				(*ptr++) = glm::vec4(kalmanFilters[i][j]->get(0), kalmanFilters[i][j]->get(1), 0.f, 1.f);

				silhoutte_frames[1][i].cpuBuf[j] = (
						silhoutte_frames[1][i].cpuBuf[j]
						+ silhoutte_frames[0][shapeMap[i]].cpuBuf[ (*shapePointDist[j].begin()).second ] * silMed
					) / (1.f + silMed);

				(*ptr++) = silhoutte_frames[1][i].cpuBuf[j];

			}
			silhoutte_frames[1][i].buf->unmap();
		}
*/
	}

	//std::cout << std::endl;
}

//---------------------------------------------------------------

void SNEnelGestalt::initShaders()
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

	stdVert = "// SNEnelGestalt depth threshold vertex shader\n" +shdr_Header +stdVert;
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

	frag = "// SNEnelGestalt depth threshold fragment shader\n"+shdr_Header+frag;

	depthThres = shCol->addCheckShaderText("SNEnelGestalt_thres", stdVert.c_str(), frag.c_str());

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

	vert = "// SNEnelGestalt edge detect vertex shader\n" +shdr_Header +vert;

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

	frag = "// SNEnelGestalt edge detect fragment shader\n"+shdr_Header+frag;

	edgeDetect = shCol->addCheckShaderText("SNEnelGestalt_edge", vert.c_str(), frag.c_str());

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

	frag = "// SNEnelGestalt subtract frag shader\n"+shdr_Header+frag;

	subtrShadr = shCol->addCheckShaderText("SNEnelGestalt_subtr", stdVert.c_str(), frag.c_str());

	//------------------------------------------------------------

	frag = STRINGIFY(uniform sampler2D tex;
					 in vec2 tex_coord;
					 layout (location = 0) out vec4 color;
					 void main()
					 {
						 color = vec4(texture(tex, tex_coord).r > 0.7 ? 1.0 : 0.0);
					 });

	frag = "// SNEnelGestalt second thresh frag shader\n"+shdr_Header+frag;

	blurThres = shCol->addCheckShaderText("SNEnelGestalt_thres2", stdVert.c_str(), frag.c_str());

	initLineShader();
	initGradShader();
}

//------------------------------------------------------------

void SNEnelGestalt::initSentShdr()
{
	std::string shdr_Header = "#version 410\n";

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 2 ) in vec2 texCoord;
	uniform mat4 m_model;
	uniform mat4 m_pv;

	uniform float fluidDist;
	uniform sampler2D fluidVelTex;

	out vec2 tex_coord;
	out vec2 mod_tex_coord;

	void main()\n
	{\n

		vec4 mod_pos = m_model * position;
		vec2 mod_coord = vec2(  mod_pos.x * 0.5 + 0.5,
								mod_pos.y * 0.5 + 0.5 );
		tex_coord = texCoord;
		mod_tex_coord = mod_coord;

		vec4 vel = texture(fluidVelTex, mod_coord.xy);
		mod_pos.xy += vel.xy * fluidDist;

//		gl_Position = m_pv * position;
		gl_Position = m_pv * mod_pos;
	});

	vert = shdr_Header +vert;


	//----------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;

	uniform sampler2D tex;\n
	uniform sampler2D fluidMaskTex;

	uniform float alpha;\n
	uniform vec4 typoCol;\n
	in vec2 tex_coord;\n
	in vec2 mod_tex_coord;\n

	void main(){\n

		vec4 fluidCol = texture(fluidMaskTex, mod_tex_coord);
		float fluidBright = min( (fluidCol.r + fluidCol.g + fluidCol.b) * 10.0, 1.0);

		vec4 fontCol = texture(tex, tex_coord);
		float fontBright = min( (fontCol.r + fontCol.g + fontCol.b), 1.0);

		fragColor = fontCol * fluidBright; 	// the typo
		fragColor.a *= fontBright;
	});


	frag = "// SNEnelFluid sentence Shdr \n"+shdr_Header+frag;

	sentenceShader = shCol->addCheckShaderText("SNEnelFluid_sent", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNEnelGestalt::initGradShader()
{
	std::string shdr_Header = "#version 430\n";
//	shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n";

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 2 ) in vec2 texCoord;

	out vec2 tex_coord;

	void main()\n
	{\n
		tex_coord = texCoord;
		gl_Position = position;
	});

	vert = shdr_Header +vert;


	//----------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;

	uniform vec4 colBase;
	uniform vec4 colPeak;
	uniform float timeOffs;
	uniform float impFreq;
	uniform float impFreq2;

	in vec2 tex_coord;
	float pi = 3.1415926535897932384626433832795;

	void main() {
		vec2 normTexPos = tex_coord * 2.0 - vec2(1.0);
		float angle = atan(normTexPos.y, normTexPos.x) / pi / 2.0 + 0.5;
		angle = sin( angle * pi * 2.0 * impFreq + timeOffs )
				* sin( angle * pi * 2.0 * impFreq2 + sin( timeOffs * 0.1 ) * 0.5 + 0.5);
		angle = pow (max(angle, 0), 4.0);

		fragColor = mix( colBase, colPeak, angle);
	});

	frag = "// SNEnelGestalt lineShdr \n"+shdr_Header+frag;


	gradShader = shCol->addCheckShaderText("SNEnelGestalt_grad", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNEnelGestalt::initLineShader()
{
	std::string shdr_Header = "#version 430\n";
//	shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n";

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( std140, binding=1 ) buffer Cont { vec4 cont_pos[]; };

	uniform float progress;
	uniform float lineWidth;
	uniform float numInstances;
	uniform float scaleAmt;
	uniform float timeOffs;
	uniform float rotGradOffs;

	uniform int numLinePoints;
	uniform vec4 center;

	out block {
		vec2 texCoord;
		vec2 instfInd;
	} Out;

	vec2 RotateVec2D(vec2 v, float radians)\n
	{\n
		float ca = cos(radians);\n
	    float sa = sin(radians);\n
	    return vec2(ca * v.x - sa * v.y, sa * v.x + ca * v.y);\n
	}\n


	void main()\n
	{\n
		vec4 p = cont_pos[gl_VertexID];\n

//		float prog = 0.0;
		float prog = mod(float(gl_InstanceID) / numInstances + timeOffs, 1.0);
		Out.instfInd = vec2(prog, 0.0);

		prog *= scaleAmt;

		p -= center;\n // move to relative center
		p.x *= prog;\n	// scale
		p.y *= prog;\n
		p += center;\n

		gl_Position = p;

		vec2 rotP = RotateVec2D(vec2(p), float(gl_InstanceID) / numInstances * rotGradOffs);

		Out.texCoord = vec2((rotP.x - center.x) * 0.5 + 0.5,
							(rotP.y - center.y) * 0.5 + 0.5);\n
	});

	vert = shdr_Header +vert;


	//----------------------------------------------------

	std::string frag = STRINGIFY(

	in block {
		vec2 texCoord;
		vec2 instfInd;
	} In;

	layout(location=0) out vec4 fragColor;
	uniform vec4 colBase;
	uniform vec4 colPeak;
	uniform sampler2D gradTex;

	void main() {
		vec4 col = texture(gradTex, In.texCoord);
		float bright = pow( dot(col, col), 2.0) * 1.5;
		fragColor = col * bright;
		fragColor.a = In.instfInd.x > 0.05 ? (1.0 - In.instfInd.x) : In.instfInd.x / 0.05;
		//		fragColor = vec4(1.0);
	});

	frag = "// SNEnelGestalt lineShdr \n"+shdr_Header+frag;


	fluidLineShdr = shCol->addCheckShaderText("SNEnelGestalt_fluidLine", vert.c_str(), frag.c_str());
}

//------------------------------------------------------------

void SNEnelGestalt::initLineTriShader()
{
	std::string shdr_Header = "#version 430\n";
//	shdr_Header = "#version 430\n#extension GL_EXT_shader_io_blocks : enable\n";

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( std140, binding=1 ) buffer Cont { vec4 cont_pos[]; };

	uniform float progress;
	uniform float lineWidth;
	uniform int numLinePoints;
	uniform vec4 center;

	int quadIndices[6];

	out block {
		vec2 texCoord;
	} Out;

	vec2 RotateVec2D(vec2 v, float radians)\n
	{\n
		float ca = cos(radians);\n
	    float sa = sin(radians);\n
	    return vec2(ca * v.x - sa * v.y, sa * v.x + ca * v.y);\n
	}\n



	vec2 GetOffsetVec(int lineInd)\n
	{\n
		vec4 p[3];\n

		// get three points of the line and apply the scaling transformation
		// one point earlier, the actual point and the following point
		for (int i=-1;i<2;i++)\n
		{\n
			p[i] = cont_pos[ min( max( lineInd +i, 0), numLinePoints-1) ];\n

			p[i] -= center;\n // move to relative center
			p[i].x *= 1.0 + progress;\n	// scale
			p[i].y *= 1.0 + progress;\n
			p[i] += center;\n
		}\n

		vec2 p1p0 = vec2(p[0] - p[1]);\n // calc vector p1p0
		vec2 p1p2 = vec2(p[2] - p[1]);\n // calc vector p1p0

		// be sure that these vectors are not zero
		p1p0 = length(p1p0) < 0.001 ?  vec2(0.0, -1.0) : normalize(p1p0);\n
		p1p2 = length(p1p2) < 0.001 ?  vec2(0.0, -1.0) : normalize(p1p2);\n

		// get the vector to offset the line point to the triangle edges
		float angle = dot(p1p0, p1p2);\n

		// rotate p1p0 by half this angle to get the offset vec
		return normalize( RotateVec2D(p1p0, angle * 0.5) ) * lineWidth;\n
	}\n

	void main()\n
	{\n
		vec2 q[4];\n

		int lInd = gl_VertexID/6;\n
		int lIndPlusOne = min(lInd +1, numLinePoints-1);\n

		// get for the actual line point and the following point
		// the offset vectors to move the linepoint to double Triangle Quad Edge Points
		vec2 offVec0 = GetOffsetVec(lInd);\n
		vec2 offVec1 = GetOffsetVec(lIndPlusOne);\n

		// define the edge points of the quad [0], [1], [2], [3]
		q[0] = vec2(cont_pos[lInd]) + offVec0;\n
		q[1] = vec2(cont_pos[lInd]) - offVec0;\n
		q[2] = vec2(cont_pos[lIndPlusOne]) - offVec1;\n
		q[3] = vec2(cont_pos[lIndPlusOne]) + offVec1;\n

		Out.texCoord = vec2(0, 0);\n

		// the two tris of the quad are defined by the indices 0,3,1 and 1,3,2
		quadIndices[0] = 0; quadIndices[1] = 3; quadIndices[2] = 1;\n
		quadIndices[3] = 1; quadIndices[4] = 3; quadIndices[5] = 2;\n

		// determine at which point of the quad we are
		int quadInt = gl_VertexID % 6;\n

		gl_Position = vec4(q[ quadIndices[quadInt] ], 0.0, 1.0);
	});

	vert = shdr_Header +vert;


	//----------------------------------------------------

	std::string frag = STRINGIFY(

	in block {
		vec2 texCoord;
	} In;

	layout(location=0) out vec4 fragColor;

	void main() {
		fragColor = vec4(1.0);
	});

	frag = "// SNEnelGestalt lineShdr \n"+shdr_Header+frag;


	fluidLineShdr = shCol->addCheckShaderText("SNEnelGestalt_fluidLine", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelGestalt::initAccumShdr()
{
	std::string shdr_Header = "#version 410\n";

	std::string vert = STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			uniform mat4 m_pvm;\n
			out vec2 tex_coord;\n
			void main(){\n
				tex_coord = texCoord;\n
				gl_Position = m_pvm * position;\n
			});

	vert = "//SNEnelGestalt accum vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform sampler2D maskTex;\n
			uniform float alpha;\n
			uniform float burst;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				float mask = texture(maskTex, tex_coord).r;\n
				if (mask < 0.01){\n
					discard;\n
				} else {\n
					color = outCol * 0.5;\n
					color.a *= alpha;\n
					color += vec4(burst);\n
				}\n
			});

	frag = "// SNEnelGestalt accum frag\n"+shdr_Header+frag;

	accumShader = shCol->addCheckShaderText("SNEnelGestalt_accum", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelGestalt::initDiffShdr()
{
	std::string shdr_Header = "#version 410\n";

	std::string vert = STRINGIFY(
			layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec4 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			uniform mat4 m_pvm;\n
			out vec2 tex_coord;\n
			void main(){\n
				tex_coord = texCoord;\n
				gl_Position = m_pvm * position;\n
			});

	vert = "//SNEnelGestalt diff vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			uniform sampler2D refTex;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			vec4 outCol;
			vec4 refCol;
			void main(){\n
				outCol = texture(tex, tex_coord);\n
				refCol = texture(refTex, tex_coord);\n
				color = refCol - outCol;\n
				color.a = 1.0;\n
			});

	frag = "// SNEnelGestalt diff frag\n"+shdr_Header+frag;

	diffShader = shCol->addCheckShaderText("SNEnelGestalt_diff", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNEnelGestalt::onKey(int key, int scancode, int action, int mods)
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
			case GLFW_KEY_7 : actDrawMode = SENTENCES;
				printf("actDrawMode = SENTENCES \n");
				break;
			case GLFW_KEY_8 : actDrawMode = DRAW;
				printf("actDrawMode = DRAW \n");
				break;


			case GLFW_KEY_T : reqSnapShot = true;
				printf("snapshot \n");
				break;
			case GLFW_KEY_C : reqDeleteSnapBuf = true;
				printf("delete \n");
				break;
		}
	}
}

//---------------------------------------------------------------

void SNEnelGestalt::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = (ypos / scd->screenHeight);
}

//---------------------------------------------------------------

SNEnelGestalt::~SNEnelGestalt()
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
	delete gradFbo;
}
}
