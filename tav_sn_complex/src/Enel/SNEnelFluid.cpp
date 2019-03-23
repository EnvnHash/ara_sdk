//
//  SNEnelFluid.cpp
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

#include "SNEnelFluid.h"

using namespace std;
using namespace cv;

namespace tav
{
SNEnelFluid::SNEnelFluid(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
SceneNode(_scd, _sceneArgs), inited(false), psInited(false), nrParticle(800000),
flWidth(512), flHeight(512), emitPartPerUpdate(8000), thinDownSampleFact(8),
actDrawMode(DRAW),  // RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, OPT_FLOW, OPT_FLOW_BLUR, FLUID_VEL, DRAW
fblurSize(512),
noiseTexSize(512),
depthThresh(3100.f),
fastBlurAlpha(0.7647f),
contThresh(163.f),
optFlowBlurAlpha(0.76f),
fluidColorSpeed(0.1f),
fluidColTexForce(0.29f),
fluidDissip(0.8f),
fluidVelTexThres(0.8f),
fluidVelTexRadius(0.2f),
fluidVelTexForce(1.17f),
fluidSmoke(0.457f),
fluidSmokeWeight(0.03f),
fluidVelDissip(0.924f),
fluidColAmp(1.f),
partColorSpeed(1.35f),
partFdbk(0.63f),
partFriction(0.56f),
partBright(0.25f),
partVeloBright(0.16f),
veloBlend(0.05f),
sentFluidDist(0.0008f),
typoSpeed(0.014f),
typoMoveAmt(0.27f),
activityMax(5000.f),
sentSwitchInt(10.f),
sentSwitchTime(2.f),
colorStayTime(20.f),
backColFadeTime(1.f),
backBright(0.5f),
interactRate(1.f),
typoSize(0.8f),
typoYOffs(-0.54f),
oscTextSetNr(0),
drawParticles(false)
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
	addPar("fluidColAmp", &fluidColAmp); // 0.0f, 1.f, 0.001f, 0.93f, OSCData::LIN);

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
	addPar("sentFluidDist", &sentFluidDist);
	addPar("sentSwitchTime", &sentSwitchTime);
	addPar("sentSwitchInt", &sentSwitchInt);

	addPar("typoSize", &typoSize);
	addPar("typoYOffs", &typoYOffs);
	addPar("typoSpeed", &typoSpeed);
	addPar("typoMoveAmt", &typoMoveAmt);
	addPar("activityMax", &activityMax);

	addPar("backBright", &backBright);
	addPar("colorStayTime", &colorStayTime);
	addPar("interactRate", &interactRate);
	addPar("oscTextSetNr", &oscTextSetNr );


    // ------- FBOs --------

	particleFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight, GL_RGBA8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	particleFbo->clear();

	typoFbo = new FBO(shCol,
			static_cast<unsigned int>( float(scd->screenWidth) ),
			static_cast<unsigned int>( float(scd->screenHeight) )
			);
	accumFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight);
	diffFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, GL_R8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

	typoMaskFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);
	diffFbo = new FBO(shCol, _scd->screenWidth, _scd->screenHeight, GL_R8,
			GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);
	// ------- Historgram -------------------

	histo = new GLSLHistogram(shCol, _scd->screenWidth, _scd->screenHeight,
			GL_R8, 2, 128, true, false, 1.f);


// muevate kommt als typo nach links neben das logo
	//muevate = new TextureManager();
	//muevate->loadTexture2D((*_scd->dataPath)+"/textures/muevate.png");

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

	fadeToBackCol = 1;

	// ------------

	// forceScale nur zum Mouse test
	forceScale = glm::vec2(static_cast<float>(flWidth) * 0.125f,
						   static_cast<float>(flHeight) * 0.125f);


	// --- Geo Primitives ---
	rawQuad = _scd->stdQuad;
	rotateQuad = _scd->stdHFlipQuad;
	colQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 1.f, 1.f, 1.f);
	quadAr = new QuadArray(20, 20);

	//-----------------------------------------------
	// --- Shaders ---

	colShader = shCol->getStdCol();
	texShader = shCol->getStdTex();
	texAlphaShader = shCol->getStdTexAlpha();
	stdMultiTex = shCol->getStdTexAlpha(true);

	initShaders();
	initSentShdr();
	initDiffShdr();

	activity = new Median<float>(85.f);

	//--------------------------------------------------

	if ( _sceneArgs->find("font0") != _sceneArgs->end() )
	{
		textSetPtr = 0;
		sentPtr = 0;

		// init sentences
		// set 0 batchelet
		//text_sets.push_back( std::vector< std::string >() );
		//text_sets.back().push_back( "\"El futuro de los niños es siempre hoy. Mañana será tarde.\"\nGabriela Mistral" );
		//text_sets.back().push_back( "\"El ñino que no juega no es niño, pero el hombre no juega perdió para siempre al niño que vivia en él y que le hará mucha falta.\"\nPablo Neruda" );

		// set 1
		text_sets.push_back( std::vector< std::string >() );
		text_sets.back().push_back( "\"El futuro de los niños es siempre hoy. Mañana será tarde.\"\nGabriela Mistral" );
		text_sets.back().push_back( "\"El ñino que no juega no es niño, pero el hombre no juega \nperdió para siempre al niño que vivia en él y que le hará mucha falta.\"\nPablo Neruda" );text_sets.back().push_back("\"Codesto solo oggi possiamo dirti, ciò che non siamo, ciò che non vogliamo\"\n\n\"Sólo esto podemos hoy decirte: lo que no somos, lo que no queremos.\"\n\nEugenio Montale");
		text_sets.back().push_back("\"Mi illumino d'immenso.\"\n\n\"Me ilumino de inmenso.\"\n\nGiuseppe Ungaretti" );
		text_sets.back().push_back( "\"El futuro de los niños es siempre hoy. Mañana será tarde.\"\nGabriela Mistral" );
		text_sets.back().push_back("\"Si sta come d'autunno sugli alberi le foglie.\"\n\n\"Aquí estamos, como en otoño en los árboles las hojas\"\n\nGiuseppe Ungaretti" );
		text_sets.back().push_back( "\"El ñino que no juega no es niño, pero el hombre no juega \nperdió para siempre al niño que vivia en él y que le hará mucha falta.\"\nPablo Neruda" );text_sets.back().push_back("\"... Ed è subito sera\"\n\n\"…y de pronto anochece.\"\n\nSalvatore Quasimodo" );
		text_sets.back().push_back("\"L'amor che move il sole e l'altre stelle.\"\n\n\"El amor que mueve el sol y las demás estrellas\"\n\nDante Alighieri");
		text_sets.back().push_back( "\"El futuro de los niños es siempre hoy. Mañana será tarde.\"\nGabriela Mistral" );
		text_sets.back().push_back("\"Che fai tu, luna, in ciel? Dimmi, che fai, silenziosa luna?\"\n\n\"¿Qué haces tú, luna, en el cielo? Dime, ¿qué haces, silenciosa luna?\"\n\nGiacomo Leopardi");
		text_sets.back().push_back("\"Chi ha il coraggio di ridere è padrone del mondo.\"\n\n\"Aquel que tiene el coraje de reír es el amo del mundo.\"\n\nGiacomo Leopardi");

		// set 2
		text_sets.push_back( std::vector< std::string >() );
		text_sets.back().push_back( "\"La poesia aggiunge vita alla vita.\"\n\n\"La poesía añade vida a la vida.\"\n\nMario Luzi");
		text_sets.back().push_back( "\"Un popolo senza satira e senza senso dell'ironia è un popolo morto.\"\n\n\"Un pueblo sin sátira y sentido de la ironía es un pueblo muerto.\"\n\nDario Fo");
		text_sets.back().push_back( "\"Le parole hanno un senso!\"\n\n\"Las palabras tienen un  sentido!\"\n\nNanni Moretti");
		text_sets.back().push_back( "\"E io pago… e io pago!\"\n\n\"Y yo pago ... y yo pago!\"\n\nTotó (Antonio de Curtis)");
		text_sets.back().push_back( "\"Non so leggere, ma intuisco\"\n\n\"No sé leer, pero intuyo.\"\n\nTotó (Antonio de Curtis)");
		text_sets.back().push_back( "\"Il bene di un libro sta nell’essere letto\"\n\n\"Lo bueno de un libro está en que sea leído.\"\n\nUmberto Eco");
		text_sets.back().push_back( "\"Per essere poeti, bisogna avere molto tempo\"\n\n\"Para ser poeta, se necesita tener mucho tiempo\"\n\nPier Paolo Pasolini");

		// set 3
		text_sets.push_back( std::vector< std::string >() );
		text_sets.back().push_back( "\"Non ho più notizie di me da tanto tempo\"\n\n\"No tengo noticias de mí hace mucho tiempo\"\n\nAlda Merini");
		text_sets.back().push_back( "\"La superficialità mi inquieta ma il profondo mi uccide.\"\n\n\"La superficialidad me inquieta, pero la profundidad me mata.\"\n\nAlda Merini");
		text_sets.back().push_back( "\"Illumino spesso gli altri ma io rimango sempre al buio\"\n\n\"Ilumino a menudo a los demás pero  yo me quedo siempre en  la oscuridad\"\n\nAlda Merini");
		text_sets.back().push_back( "\"Se comprendere è impossibile, conoscere è necessario.\"\n\n\"Si comprender es imposible, conocer es necesario.\"\n\nPrimo Levi");
		text_sets.back().push_back( "\"Meglio aggiungere vita ai giorni, che non giorni alla vita.\"\n\n\"Es mejor agregar vida a los días, no días a la vida.\"\n\nRita Levi Montalcini");
		text_sets.back().push_back( "\"Se alzi un muro, pensa a cosa lasci fuori.\"\n\n\"Si levantas un muro, piensa en lo que estás dejando fuera.\"\n\nItalo Calvino");
		text_sets.back().push_back( "\"Se vostro figlio vuole fare lo scrittore o il poeta sconsigliatelo fortemente\"\n\n\"Si vuestro hijo quiere ser poeta o escritor, desaconséjenlo fuertemente\"\n\nGrazia Deledda");
		text_sets.back().push_back( "\"Quando io morirò, tu portami il caffè, e vedrai che io resuscito come Lazzaro\"\n\n\"Cuando muera, tráeme un café y verás que resucito como Lázaro\"\n\nEduardo De Filippo");

		text_block = new NVTextBlock*[2];
		typoTex = new GLint[2];
		for (unsigned int i=0; i<2; i++)
		{
			text_block[i] = new NVTextBlock(shCol, fonts[ static_cast<int>(_sceneArgs->at("font0")) ] );
			typoTex[i] = 0;
		}

	} else {
		std::cout << "SNTestNVFonts font0 definition  not found in setup xml" << std::endl;
	}
}

//---------------------------------------------------------------

void SNEnelFluid::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (!textInited)
	{
		for (unsigned int i=0;i<2;i++)
		{
			text_block[i]->setAlign(CENTER_X);
			text_block[i]->setString(text_sets[textSetPtr][i], cp);
			text_block[i]->setTextColor(enel_colors[2].x, enel_colors[2].y, enel_colors[2].z, 1.f);
			text_block[i]->setTextSize(20.f);
			text_block[i]->setLineHeight(22.f);
			text_block[i]->setSize(2.f, 1.f);

			typoTex[i] = text_block[i]->drawToTex(cp);
		}

		textInited = true;
	}


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
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());
				rawQuad->draw();

				break;

			case DRAW: // 8
			{
				//-----------------------------------
				// draw particles into fbo for feedback

				if (drawParticles){

					particleFbo->bind();
					particleFbo->clear();
					//particleFbo->clearAlpha(partFdbk);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					ps->draw( cp->mvp );
					particleFbo->unbind();
				}

				//------------------------------
				// render the fluid and optionally the particles
				// for later drawing and for use as a mask

				typoMaskFbo->bind();
				typoMaskFbo->clear();

				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				fluidSim->draw();

				if (drawParticles)
				{
					texShader->begin();
					texShader->setIdentMatrix4fv("m_pvm");
					texShader->setUniform1i("tex", 0);
					glBindTexture(GL_TEXTURE_2D, particleFbo->getColorImg());
					rawQuad->draw();
				}

				typoMaskFbo->unbind();

				//------------------------------

/*
				// change color after a defined time
				if ( (time - lastChangeColTime) > colorStayTime ) {

					backColPtr = ++backColPtr % nrEnelColors;
					lastChangeColTime = time;

				} else if(time - lastChangeColTime < backColFadeTime){

					float fadePos = (time - lastChangeColTime) / backColFadeTime;

					fadeFromBackCol = backColPtr;
					fadeToBackCol = (fadeFromBackCol + 1) % nrEnelColors;

					backCol = glm::mix(enel_colors[fadeFromBackCol], enel_colors[fadeToBackCol], fadePos);

				} else {

					backCol = enel_colors[fadeToBackCol];
				}
*/

				backCol = enel_colors[5];		// rosa

				glClearColor(backCol.r * backBright, backCol.g * backBright, backCol.b * backBright, 1.0);
//				glClearColor(backCol.r * backBright, backCol.g * backBright, backCol.b * backBright, 1.0);
				glClear(GL_COLOR_BUFFER_BIT);

				//------------------------------
				// draw threshols fbo -> white silhouettes
				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, fblur2nd->getResult());
				rawQuad->draw();


				glBlendFunc(GL_SRC_ALPHA,  GL_ONE_MINUS_SRC_ALPHA);
				//	glBlendFunc(GL_SRC_ALPHA,  GL_ONE);

				texShader->begin();
				texShader->setIdentMatrix4fv("m_pvm");
				texShader->setUniform1i("tex", 0);
				glBindTexture(GL_TEXTURE_2D, typoMaskFbo->getColorImg());

				rawQuad->draw();

				//--------------------------------------------
				// draw accum fbo

				renderTypoVelTex(time, cp);

			}
				break;

			default:
				break;
		}
	}
}

//---------------------------------------------------------------

void SNEnelFluid::procActivity()
{
	// get the range of filling
	histo->proc(diffFbo->getColorImg());
	float sum = histo->getEnergySum(0.1f);


	//activity
	typoProgress = std::fmax( std::fmin(sum, activityMax) / activityMax, 0.f);

	activity->update(typoProgress);
}

//---------------------------------------------------------------

void SNEnelFluid::renderTypo(double time, camPar* cp)
{
	typoPosMat = glm::translate(
			glm::vec3(
					glm::perlin( glm::vec2(time * typoSpeed) ) * typoMoveAmt,
					glm::perlin( glm::vec2(time * typoSpeed + 0.1f) ) * typoMoveAmt,
					0.f )
	);


	if (oscTextSetNr != lastOscTextSetNr)
	{
		textSetPtr = static_cast<unsigned int>(oscTextSetNr);
		sentPtr = sentPtr % (int) text_sets[textSetPtr].size();

		text_block[0]->setString(text_sets[textSetPtr][sentPtr], cp);
		text_block[1]->setString(text_sets[textSetPtr][(sentPtr +1) % (int) text_sets[textSetPtr].size()], cp);

		for (unsigned int i=0; i<2; i++)
			typoTex[i] = (GLint) text_block[i]->drawToTex(cp);

		lastOscTextSetNr = oscTextSetNr;
	}


	// blending
	float bAmt = 0.f;
	if (time - lastSentSwitchTime > sentSwitchInt)
	{
		sentSwitch = true;
		lastSentSwitchTime = time;
	}

	if(sentSwitch)
	{
		bAmt = (time - lastSentSwitchTime) / sentSwitchTime;

		if (bAmt >= 1.f)
		{
			sentPtr = ++sentPtr % (int) text_sets[textSetPtr].size();
			bAmt = 0.f;
			sentSwitch = false;

			text_block[0]->setString(text_sets[textSetPtr][sentPtr], cp);
			text_block[1]->setString(text_sets[textSetPtr][(sentPtr +1) % (int) text_sets[textSetPtr].size()], cp);

			for (unsigned int i=0; i<2; i++)
			{
				typoTex[i] = (GLint) text_block[i]->drawToTex(cp);
			}
		}
	}

	typoFbo->bind();
	typoFbo->clear();

	stdMultiTex->begin();
	stdMultiTex->setUniformMatrix4fv("m_pvm", &typoPosMat[0][0]);
	stdMultiTex->setUniform1i("tex", 0);
	stdMultiTex->setUniform1i("nrSamples", text_block[0]->getNrSamples());
	stdMultiTex->setUniform2fv("scr_size", &cp->actFboSize[0]);

	glActiveTexture(GL_TEXTURE0);

	stdMultiTex->setUniform1f("alpha", 1.f - bAmt);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, typoTex[0]);
	rawQuad->draw();


	stdMultiTex->setUniform1f("alpha", bAmt);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, typoTex[1]);
	rawQuad->draw();

	typoFbo->unbind();
}

//---------------------------------------------------------------

void SNEnelFluid::renderTypoVelTex(double time, camPar* cp)
{
	float scaleAmt = typoSize + 0.2f + activity->get() * 0.8f;

	glm::vec3 pos = glm::vec3(
			glm::perlin( glm::vec2(time * typoSpeed) ) * typoMoveAmt,
			glm::perlin( glm::vec2(time * typoSpeed + 0.1f) ) * typoMoveAmt,
			0.f );

	pos.y += typoYOffs;

	scaleAmt = glm::mix(typoSize + 1.f, scaleAmt, interactRate);

	typoPosMat = glm::translate(  glm::mix( glm::vec3(0.f, typoYOffs, 0.f), pos, interactRate) )
		* glm::scale( glm::vec3(scaleAmt, scaleAmt, 1.f) );

	glm::mat4 pv = cp->projection_matrix_mat4 * cp->view_matrix_mat4;



	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	sentenceShader->begin();
	sentenceShader->setUniformMatrix4fv("m_model", &typoPosMat[0][0]);
	sentenceShader->setUniformMatrix4fv("m_pv", &pv[0][0]);

	sentenceShader->setUniform1i("tex", 0);
	sentenceShader->setUniform1i("fluidMaskTex", 1);
	sentenceShader->setUniform1i("fluidVelTex", 2);

	//sentenceShader->setUniform1f("fluidDist", sentFluidDist * (1.f - typoProgress));
	//	sentenceShader->setUniform1f("alpha", std::pow(typoProgress, 2.f));
	sentenceShader->setUniform1f("fluidDist", sentFluidDist);
	sentenceShader->setUniform1f("alpha", 1.f);

	sentenceShader->setUniform1f("interactRate", interactRate);

	sentenceShader->setUniform4fv("typoCol", &enel_colors[3][0]);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, typoFbo->getColorImg());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, typoMaskFbo->getColorImg());

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fluidSim->getVelocityTex());

	quadAr->draw();
}

//---------------------------------------------------------------

void SNEnelFluid::update(double time, double dt)
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

				depthThresShdr->begin();
				depthThresShdr->setIdentMatrix4fv("m_pvm");
				depthThresShdr->setUniform1i("kinDepthTex", 0);
				depthThresShdr->setUniform1f("depthThres", depthThresh);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false));

				rawQuad->draw();
				threshFbo->unbind();

				// ------ apply blur on silhoutte --------
				fblur->setAlpha(fastBlurAlpha);
				fblur->proc(threshFbo->getColorImg());

				fblur2nd->setAlpha(fastBlurAlpha);
				fblur2nd->proc(fblur->getResult());

				//-------------------------------------------------------

				// -- calculate optical flow, the result will be used to add velocity to the fluid --
				optFlow->update(fblur2nd->getResult(), fblur2nd->getLastResult());

				optFlowBlur->setAlpha(optFlowBlurAlpha);
				optFlowBlur->proc(optFlow->getResTexId());

				//---------------------------------------
				// measure progress

				diffFbo->bind();
				diffFbo->clear();

				glBlendFunc(GL_ONE, GL_ZERO);

				diffShader->begin();
				diffShader->setIdentMatrix4fv("m_pvm");
				diffShader->setUniform1i("tex", 0);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, optFlowBlur->getLastResult());

				rawQuad->draw();

				diffFbo->unbind();


				//--- Emit Particles --
				// emit Color
				if (drawParticles)
				{
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
			}

			// measure activity
			procActivity();

			//--- Update Fluid, muss jeden Frame passieren, sonst flimmer effekte ----
			// set Fluid Color
			float timeMult = std::fmod( time * fluidColorSpeed, float(nrEnelColors) );
			unsigned int lowCol = std::floor( timeMult );
			unsigned int highCol = (lowCol + 1) % nrEnelColors;
			float bAmt = std::fmod( timeMult, 1.f );

			bAmt = 1.f - (std::cos(bAmt * float(M_PI) * 0.5f) * 0.5 + 0.5f);
			eCol = glm::mix( enel_colors[lowCol], enel_colors[highCol], bAmt);


			fluidSim->setVelTexThresh(fluidVelTexThres);
			fluidSim->setSmokeBuoyancy(fluidSmoke);
			fluidSim->setVelTexRadius(fluidVelTexRadius);
			fluidSim->dissipation = std::pow(fluidDissip, 0.2) * 0.1f + 0.9f;
			fluidSim->velocityDissipation = std::pow(fluidVelDissip, 0.2) * 0.1f + 0.9f;

			fluidSim->addVelocity(optFlowBlur->getResult(), fluidVelTexForce);
			fluidSim->addColor(optFlowBlur->getResult(), glm::vec3(eCol) * fluidColAmp, fluidColTexForce, true);

			if( interactRate < 0.3f )
			{

				glm::vec2 m = glm::vec2(
						(glm::perlin( glm::vec2(time * 0.1f) ) * 0.5f + 0.5f) * flWidth,
						(glm::perlin( glm::vec2(time * 0.1f + 0.1f) ) * 0.5f + 0.5f) * flHeight
				);

				m += glm::vec2(1.f, 1.f);

				glm::vec2 d = (m - oldM) * 10.f;
				oldM = m;
				glm::vec2 c = glm::normalize(forceScale - m);

				float tm = static_cast<float>(std::sin(time));
				fluidSim->addTemporalVelocity(m, d, fluidVelTexRadius * 10.f, 0.3f, 0.4f);
				fluidSim->addTemporalForce(m,                           // pos
						d,                           // vel
						glm::vec4(std::fabs(c.x * tm),
								std::fabs(c.y * tm),
								std::fabs(0.5f * tm),
								1.f),
								3.0f);
			}

			fluidSim->update();

			if (drawParticles)
				ps->update(time, fluidSim->getVelocityTex());
		}
	}
}

//---------------------------------------------------------------

void SNEnelFluid::initShaders()
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

	stdVert = "// SNEnelFluid depth threshold vertex shader\n" +shdr_Header +stdVert;
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

	frag = "// SNEnelFluid depth threshold fragment shader\n"+shdr_Header+frag;

	depthThresShdr = shCol->addCheckShaderText("SNEnelFluid_thres", stdVert.c_str(), frag.c_str());

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

	frag = "// SNEnelFluid subtract frag shader\n"+shdr_Header+frag;

	subtrShadr = shCol->addCheckShaderText("SNEnelFluid_subtr", stdVert.c_str(), frag.c_str());

	//------------------------------------------------------------

	frag = STRINGIFY(uniform sampler2D tex;
					 in vec2 tex_coord;
					 layout (location = 0) out vec4 color;
					 void main()
					 {
						 color = vec4(texture(tex, tex_coord).r > 0.7 ? 1.0 : 0.0);
					 });

	frag = "// SNEnelFluid second thresh frag shader\n"+shdr_Header+frag;

	blurThres = shCol->addCheckShaderText("SNEnelFluid_thres2", stdVert.c_str(), frag.c_str());

}

//------------------------------------------------------------

void SNEnelFluid::initSentShdr()
{
	std::string shdr_Header = "#version 410\n";

	std::string vert = STRINGIFY(
	layout( location = 0 ) in vec4 position;
	layout( location = 2 ) in vec2 texCoord;
	uniform mat4 m_model;
	uniform mat4 m_pv;

	uniform float fluidDist;
	uniform float interactRate;\n
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
		mod_pos.xy += mix(vec2(0.0), vel.xy * fluidDist, interactRate);

		gl_Position = m_pv * mod_pos;
	});

	vert = shdr_Header +vert;


	//----------------------------------------------------

	std::string frag = STRINGIFY(
	layout(location=0) out vec4 fragColor;

	uniform sampler2D tex;\n
	uniform sampler2D fluidMaskTex;
	uniform float alpha;\n
	uniform float interactRate;\n
	uniform vec4 typoCol;\n
	in vec2 tex_coord;\n
	in vec2 mod_tex_coord;\n

	void main(){\n

		vec4 fluidCol = texture(fluidMaskTex, mod_tex_coord);
		float fluidBright = min( (fluidCol.r + fluidCol.g + fluidCol.b), 1.0);

		vec4 fontCol = texture(tex, tex_coord);
		float fontBright = min( (fontCol.r + fontCol.g + fontCol.b), 1.0);

		fragColor = fontCol * mix(1.0, fluidBright, interactRate); 	// the typo
		fragColor.a *= fontBright;
	});


	frag = "// SNEnelFluid sentence Shdr \n"+shdr_Header+frag;

	sentenceShader = shCol->addCheckShaderText("SNEnelFluid_sent", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNEnelFluid::initDiffShdr()
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

	vert = "//SNEnelFluid diff vert\n"+shdr_Header +vert;

	//----------------------------------------------------------

	std::string frag = STRINGIFY(
			uniform sampler2D tex;\n
			in vec2 tex_coord;\n
			layout (location = 0) out vec4 color;\n
			void main(){\n
				color = vec4(texture(tex, tex_coord).r);\n
			});

	frag = "// SNEnelFluid diff frag\n"+shdr_Header+frag;

	diffShader = shCol->addCheckShaderText("SNEnelFluid_diff", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------------

void SNEnelFluid::onKey(int key, int scancode, int action, int mods)
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
		}
	}
}

//---------------------------------------------------------------

void SNEnelFluid::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	mouseX = xpos / scd->screenWidth;
	mouseY = (ypos / scd->screenHeight);
}

//---------------------------------------------------------------

SNEnelFluid::~SNEnelFluid()
{
	delete userMapConv;
	delete userMapRGBA;
	delete ps;
	delete fluidSim;
	delete partEmitCol;
	delete rawQuad;
	delete rotateQuad;
	delete edgePP;
	delete optFlow;
}
}
