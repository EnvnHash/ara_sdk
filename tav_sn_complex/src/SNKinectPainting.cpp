//
//  SNGLSLFluidTest.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNKinectPainting.h"

using namespace glm;
using namespace std;

namespace tav
{
SNKinectPainting::SNKinectPainting(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), useMouse(false), nrIntpSegs(3), doSplash(false),
			showKinectIn(true), maxUsers(4), zDrawThres(1.f)
{
	kin = static_cast<KinectInput*>(scd->kin);
	shCol = static_cast<ShaderCollector*>(shCol);

	nis = kin->getNis();
	nis->setUpdateImg(true);

	// artistical parameters, to play around
	brushSize = 0.1f;
	splashSize = 0.3f;
	brushHardness = 1.f;
	canvasFdbk = 0.9f;               // dissolve time (0 = disappear immediately, 1 = stay)
	paperAbsorbAmt = 0.8f;          // blur feedback, above one infinite,
	paperTextStrength = 0.6f;       // 0 = no structure, amorph absorbing, 1 = full structure
	gravitation = 1.f;
	smearing = 1.f;                 // smear colors, 0 = mix 100% with background color,
	// 1 = only foreground
	hMStrength = 1.f;               // 1 = full height, 0 = without heightmapping

	//--

	brushTexSize = 256;             // lower this to optimize performance
	noiseTexSize = 2048;
	splashTexSize = 512;

	// if a hand is closer to the kinect than this value, start drawing
	// aprox in m

	// closet distance
	minZ = 0.1f;
	// if the a hand moves faster than this value in the z space do a splash
	splashThres = 0.025f;
	// time to suppress drawing after a splash was released
	splashTimeThres = 1.5;

	// distance of a new and old input to decide if the hand moves or not
	moveThres = 0.01f;

	// how many subdivision between each new position input, should result in 1 pixel
	// distance between each subdivision, can be maybe more than 1 pixel -> better performance
	penDrawDist = 1.f / static_cast<float>(scd->screenWidth);
	penChangeSpeed = 1.f;
	scaleBrushTex = 8.f;


	// --- initializations

	inited = false;

	paintCol = vec4(1.f, 0.f, 0.f, 1.f);
	blendBlurAcc = 0.f;
	blendCanvAcc = 0.f;

	userHands = new handData*[maxUsers];

	// se suponpe que la gente con más de 2 manos no veniran...
	for (int i=0;i<maxUsers;i++)
		userHands[i] = new handData[2];

	// should cover the screen entirely if the pvm-matrix is set to identity
	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f);

	whiteQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			vec3(0.f, 0.f, 1.f),
			1.f, 1.f, 1.f, 1.f);

	debugQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 0.f,
			nullptr, 1, true);

	debugMatr = mat4(1.f);
	debugMatr = glm::translate(debugMatr, vec3(-0.75f, 0.75f, 0.f));
	debugMatr = glm::scale(debugMatr, vec3(0.25f, 0.25f, 1.f));
	//debugMatr = glm::rotate(debugMatr, static_cast<float>(M_PI), vec3(0.f, 0.f, 1.f));

	// --- fast blur kernels ---

	blurOffsV = new GLfloat[3];
	blurOffsV[0] = 0.f;
	blurOffsV[1] = 0.00135216346152f;
	blurOffsV[2] = 0.00315504807695f;

	blurOffsH = new GLfloat[3];
	blurOffsH[0] = 0.f;
	blurOffsH[1] = 0.00135216346152f;
	blurOffsH[2] = 0.00315504807695f;


	// --- kernel for smearing -

	coef = new float[9];
	coef[0] = 0.25f;    coef[1] = 0.5f;     coef[2] = 0.25f;
	coef[3] = 0.5f;     coef[4] = 0.f;      coef[5] = 0.5f;
	coef[6] = 0.25f;    coef[7] = 0.5f;     coef[8] = 0.25f;


	// --- for nite skeleton debugging ---

	colorCount = 3;

	skeletonColors = new float*[4];
	for (int i=0;i<4;i++) skeletonColors[i] = new float[3];
	skeletonColors[0][0] = 0.f; skeletonColors[0][1] = 1.f; skeletonColors[0][2] = 0.f;
	skeletonColors[1][0] = 0.f; skeletonColors[1][1] = 1.f; skeletonColors[1][2] = 0.f;
	skeletonColors[2][0] = 0.f; skeletonColors[2][1] = 0.f; skeletonColors[2][2] = 1.f;
	skeletonColors[3][0] = 1.f; skeletonColors[3][1] = 1.f; skeletonColors[3][2] = 1.f;

	pointsVao = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
	pointsVao->initData(50);

	skeletonVao = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
	skeletonVao->initData(60);

	paintCol = glm::vec4(1.f, 0.f, 0.f, 1.f);

	// -- FBOs --

	perlinNoise = new FBO(shCol, noiseTexSize, noiseTexSize);
	xBlendFboH = new FBO(shCol, noiseTexSize, noiseTexSize);
	xBlendFboV = new FBO(shCol, noiseTexSize, noiseTexSize);
	heightMap = new FBO(shCol, noiseTexSize, noiseTexSize);

	pen = new FBO(shCol, brushTexSize, brushTexSize);
	penPressProfile = new FBO(shCol, brushTexSize, brushTexSize);

	// canvas initially clear to white
	canvasColor = new PingPongFbo(shCol, scd->screenWidth, scd->screenHeight,
			GL_RGBA8, GL_TEXTURE_2D);
	canvasHeightMap = new FBO(shCol, scd->screenWidth, scd->screenHeight);
	canvasHeightMap->bind();
	canvasHeightMap->clearToColor(0.f, 0.f, 1.f, 1.f);
	canvasHeightMap->unbind();

	applyHMFbo = new FBO(shCol, scd->screenWidth, scd->screenHeight);

	blurFbo = new PingPongFbo(shCol, scd->screenWidth, scd->screenHeight,
			GL_RGBA8, GL_TEXTURE_2D);

	splash = new PingPongFbo(shCol, splashTexSize, splashTexSize, GL_RGBA8, GL_TEXTURE_2D);
	fastBlurPP = new PingPongFbo(shCol, splashTexSize, splashTexSize, GL_RGBA8, GL_TEXTURE_2D);

	// -- Shaders --

	colShader = shCol->getStdCol();
	noiseShader = shCol->get("perlin");
	texShader = shCol->getStdTex();

	applyHeightMapShader = new Shaders("shaders/applyHeightMap.vert", "shaders/applyHeightMap.frag", true);
	applyHeightMapShader->link();

	blurShader = new Shaders("shaders/absorbPaper.vert", "shaders/absorbPaper.frag", true);
	blurShader->link();

	drawBrushColor = new Shaders("shaders/drawBrushCol.vert", "shaders/drawBrushCol.frag", true);
	drawBrushColor->link();

	drawBrushCover = new Shaders("shaders/drawBrushCov.vert", "shaders/drawBrushCov.frag", true);
	drawBrushCover->link();

	fastBlurVShader = new Shaders("shaders/passthrough.vs", "shaders/linear_vert.fs", true);
	fastBlurVShader->link();

	fastBlurHShader = new Shaders("shaders/passthrough.vs", "shaders/linear_horiz.fs", true);
	fastBlurHShader->link();

	genBrushShader = new Shaders("shaders/drawBrushCov.vert", "shaders/texMultiply.frag", true);
	genBrushShader->link();

	genBrushHMShader = new Shaders("shaders/basic_tex.vert", "shaders/genBrushHeightM.frag", true);
	genBrushHMShader->link();

	heightMapShader = new Shaders("shaders/sobel_heightmap.vert", "shaders/sobel_heightmap.frag", true);
	heightMapShader->link();

	splashShader = new Shaders("shaders/basic_tex.vert", "shaders/genSplash.frag", true);
	splashShader->link();

	texToAlpha = new Shaders("shaders/basic_tex.vert", "shaders/basic_tex_alpha.frag", true);
	texToAlpha->link();

	xBlendShaderH = new Shaders("shaders/basic_tex.vert", "shaders/xBlendH.frag", true);
	xBlendShaderH->link();

	xBlendShaderV = new Shaders("shaders/basic_tex.vert", "shaders/xBlendV.frag", true);
	xBlendShaderV->link();

	// -- Textures --

	brushProfileTex = new TextureManager();
	brushProfileTex->loadTexture2D("/Users/useruser/tav_data/textures/penalpha1.jpg");

	hmProfileTex = new TextureManager();
	hmProfileTex->loadTexture2D("/Users/useruser/tav_data/textures/penalpha_soft.jpg");
}

//----------------------------------------------------

SNKinectPainting::~SNKinectPainting()
{
	delete colShader;
	// texShader->remove();
	delete texShader;
	//noiseShader->remove();
	delete noiseShader;
	delete heightMapShader;
	delete pen;
	applyHeightMapShader->remove();
	delete applyHeightMapShader;
	drawBrushCover->remove();
	delete drawBrushCover;
	genBrushShader->remove();
	delete genBrushShader;
	xBlendShaderH->remove();
	delete xBlendShaderH;
	xBlendShaderV->remove();
	delete xBlendShaderV;
	delete whiteQuad;
	texToAlpha->remove();
	delete texToAlpha;
	fastBlurHShader->remove();
	delete fastBlurHShader;
	fastBlurVShader->remove();
	delete fastBlurVShader;
}

//----------------------------------------------------

void SNKinectPainting::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	// canvasFdbk = osc->feedback;
	//        paperAbsorbAmt = osc->totalBrightness;
	//        pressure = osc->alpha;
	//        smearing = osc->feedback;

	if (_tfo) {
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	// generate the brush texture once
	if (!inited) {
		genBrushTex();
		inited = true;
	}

	if (requestClear) clearTextures();
	if (!useMouse) getNiteHands(time);

	// cycle brush color
	paintCol.r = std::cos(time * 0.1 + M_PI) * 0.5f + 0.5f;
	paintCol.g = std::cos(time * 0.11) * 0.5f + 0.5f;
	paintCol.b = std::cos(time * 0.12) * 0.5f + 0.5f;

	// --

	glDisable(GL_DEPTH_TEST);

	// preprocessing
	blendBlurToAlpha();
	blendCanvasToAlpha();
	applyHeightMap(time);
	applyBlur();

	// ---- begin draw to screen ---


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// make white background
	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");
	whiteQuad->draw();

	// draw result
	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	// draw blur
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, blurFbo->getSrcTexId());
	quad->draw();

	// draw heightmap
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, applyHMFbo->getColorImg());
	quad->draw();

	// process the input and generate a new brush and draw it
	// has to be at this position
	// process the input and generate a new brush and draw it
	if (useMouse)
	{
		if (debugMouse.doDraw) procBrushStrokes(actTime, &debugMouse);
		if (debugMouse.doSplash) procSplash(actTime, &debugMouse);

	} else
	{
		for (int i=0; i<maxUsers; i++)
		{
			for (int j=0; j<2; j++)
			{
				if (userHands[i][j].doSplash)
				{
					procSplash(actTime, &userHands[i][j]);

				} else if(userHands[i][j].doDraw)
				{
					procBrushStrokes(actTime, &userHands[i][j]);
				}
			}
		}
	}

	if (showKinectIn) showKinectInput();


	if (_tfo)
	{
		glEnable(GL_RASTERIZER_DISCARD);
		_shader->begin();       // extrem wichtig, sonst keine Bindepunkte für TFO!!!
		_tfo->begin(_tfo->getLastMode());
	}
}

//----------------------------------------------------

void SNKinectPainting::applyBlur()
{
	// overwrite colors, add alpha
	glBlendFuncSeparate (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	blurFbo->dst->bind();

	blurShader->begin();
	blurShader->setIdentMatrix4fv("m_pvm");
	blurShader->setUniform1i("inputTex", 0);
	blurShader->setUniform1i("fdbk", 1);
	blurShader->setUniform1i("paper", 2);
	blurShader->setUniform1f("paperStructAmt", paperTextStrength);
	blurShader->setUniform1f("absorbAmt", paperAbsorbAmt * 2.f);
	blurShader->setUniform1f("stepX", 1.f / static_cast<float>(scd->screenWidth));
	blurShader->setUniform1f("stepY", 1.f / static_cast<float>(scd->screenHeight));

	// calc new coeff with gravitation
	// if gravitation is one: row[y=0]:*2 row[y=1]:*1 row[y=2]:*0
	float* newCoeff = new float[9];
	float weight = 0.f;
	for (short y=0;y<3;y++)
	{
		for (short x=0;x<3;x++)
		{
			int ind = y*3 +x;
			float grav = std::pow(gravitation * static_cast<float>(2-y), 2.5) + (1.f - gravitation);
			grav *= (gravitation * static_cast<float>(x%2) + (1.f - gravitation));
			newCoeff[ind] = coef[ind] * grav;
			weight += newCoeff[ind];
		}
	}

	blurShader->setUniform1fv("coeff", &newCoeff[0], 9);
	blurShader->setUniform1f("weight", 1.f / weight);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, applyHMFbo->getColorImg() );

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blurFbo->getSrcTexId() );

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, perlinNoise->getColorImg() );

	quad->draw();

	blurFbo->dst->unbind();
	blurFbo->swap();
}

//----------------------------------------------------

void SNKinectPainting::applyHeightMap(double time)
{
	// - apply heightmap --

	applyHMFbo->bind();
	applyHMFbo->clear();

	// draw the result
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // ohne dass kein antialiasing

	applyHeightMapShader->begin();
	applyHeightMapShader->setIdentMatrix4fv("m_pvm");
	applyHeightMapShader->setUniform1i("cColor", 0);
	applyHeightMapShader->setUniform1i("cHeightMap", 1);

	glm::vec3 lightDir = glm::normalize( glm::vec3(0.6f, 0.8f, 1.0f) ); // from the object and not to the object

	applyHeightMapShader->setUniform3f("Ambient", 0.f, 0.f, 0.f); // zum testen
	applyHeightMapShader->setUniform3f("LightDirection", lightDir.x, lightDir.y, lightDir.z); // by scaling this, the intensity varies
	applyHeightMapShader->setUniform3f("HalfVector", lightDir.x, lightDir.y, lightDir.z);
	applyHeightMapShader->setUniform3f("LightColor", 1.0f, 1.0f, 1.0f);
	applyHeightMapShader->setUniform1f("Shininess", 20.f);
	applyHeightMapShader->setUniform1f("Strength", 0.8f);
	applyHeightMapShader->setUniform1f("hMStrength", hMStrength);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, canvasColor->getSrcTexId());

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, canvasHeightMap->getColorImg());

	quad->draw();

	applyHMFbo->unbind();
}

//----------------------------------------------------

void SNKinectPainting::blendBlurToAlpha()
{
	float actAlpha = 0.f;

	// tricky... the canvas texture has to be blended to alpha
	// so simply painting a quad with transparency doesn´t work
	// solution: a shader with subtracts a amount of the alpha value
	// since the texture is a RGBA8 alpha values range from 0 -255
	// and values under 1 (for slow blending) don´t work
	// therefore a accumulator value ist used.
	blendBlurAcc += (1.f - canvasFdbk) * 5.f;

	if ( blendBlurAcc > 1.f )
	{
		actAlpha = std::floor(blendBlurAcc) / 255.f;
		blendBlurAcc -= std::floor(blendBlurAcc);
	} else {
		actAlpha = 0.f;
	}


	for (int i=0;i<2;i++)
	{
		blurFbo->dst->bind();
		blurFbo->dst->clear();

		glBlendFunc(GL_ONE, GL_ZERO); // ohne dass kein antialiasing

		texToAlpha->begin();
		texToAlpha->setIdentMatrix4fv("m_pvm");
		texToAlpha->setUniform1f("alpha", actAlpha);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blurFbo->getSrcTexId());
		quad->draw();

		blurFbo->dst->unbind();
		blurFbo->swap();
	}
}

//----------------------------------------------------

void SNKinectPainting::blendCanvasToAlpha()
{
	// tricky... the canvas texture has to be blended to alpha
	// so simply painting a quad with transparency doesn´t work
	// solution: a shader with subtracts a amount of the alpha value
	// since the texture is a RGBA8 alpha values range from 0 -255
	// and values under 1 (for slow blending) don´t work
	// therefore a accumulator value ist used.

	// blend coverage
	blendCanvAcc += (1.f - canvasFdbk) * 5.0f;

	canvasColor->dst->bind();
	canvasColor->dst->clear();

	glBlendFunc(GL_ONE, GL_ZERO); // ohne dass kein antialiasing

	texToAlpha->begin();
	texToAlpha->setIdentMatrix4fv("m_pvm");

	if ( blendCanvAcc > 1.f )
	{
		texToAlpha->setUniform1f("alpha", std::floor(blendCanvAcc) / 255.f);
		blendCanvAcc -= std::floor(blendCanvAcc);
	} else {
		texToAlpha->setUniform1f("alpha", 0.f);
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, canvasColor->getSrcTexId());
	quad->draw();

	canvasColor->dst->unbind();
	canvasColor->swap();
}

//----------------------------------------------------

void SNKinectPainting::clearTextures()
{
	canvasColor->dst->bind();
	canvasColor->dst->clear();
	canvasColor->dst->unbind();
	canvasColor->src->bind();
	canvasColor->src->clear();
	canvasColor->src->unbind();

	canvasHeightMap->bind();
	canvasHeightMap->clearToColor(0.f, 0.f, 0.f, 1.f);
	canvasHeightMap->unbind();

	applyHMFbo->bind();
	applyHMFbo->clear();
	applyHMFbo->unbind();

	blurFbo->dst->bind();
	blurFbo->dst->clear();
	blurFbo->dst->unbind();
	blurFbo->src->bind();
	blurFbo->src->clear();
	blurFbo->src->unbind();

	requestClear = false;
}

//----------------------------------------------------

void SNKinectPainting::drawBrushStroke(float posX, float posY, float scale, float alphaMult,
		GLuint brushTexId, GLuint hmTexId)
{
	// draw pen strokes
	penMatr = mat4(1.f);
	penMatr = glm::translate(penMatr, vec3(posX, posY, 0.f));
	penMatr = glm::scale(penMatr, vec3(scale, scale, 1.f));

	toTexMatr = mat4(1.f);
	toTexMatr = glm::translate(toTexMatr, vec3(posX *0.5f +0.5f, posY *0.5f +0.5f, 0.f));
	toTexMatr = glm::translate(toTexMatr, vec3(scale * -0.5f, scale * -0.5f, 0.f));
	toTexMatr = glm::scale(toTexMatr, vec3(scale, scale, 1.f));

	// - draw the pen into the color fbo ----

	canvasColor->dst->bind();

	glDisable(GL_BLEND);

	// draw a new spot
	drawBrushColor->begin();
	drawBrushColor->setUniform1i("tex", 0);
	drawBrushColor->setUniform1i("oldCoverage", 1);
	drawBrushColor->setUniform1f("alphaMult", alphaMult);
	drawBrushColor->setUniformMatrix4fv("m_pvm", &penMatr[0][0]);
	drawBrushColor->setUniformMatrix4fv("toTexMatr", &toTexMatr[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brushTexId);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, canvasColor->getSrcTexId());

	quad->draw();

	canvasColor->dst->unbind();
	canvasColor->swap();

	// - draw the pen into the normal fbo ----

	glEnable (GL_BLEND);

	canvasHeightMap->bind();

	// separate functions for color and alpha
	glBlendFuncSeparate (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	texShader->begin();
	texShader->setUniform1i("tex", 0);   // muss gesetzt werden, sonst glsl fehler
	texShader->setUniformMatrix4fv("m_pvm", &penMatr[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hmTexId);
	quad->draw();

	canvasHeightMap->unbind();
}

//----------------------------------------------------

void SNKinectPainting::showKinectInput()
{
	kin->uploadColorImg();

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ohne dass kein antialiasing

	texShader->begin();
	texShader->setUniform1i("tex", 0);
	texShader->setUniformMatrix4fv("m_pvm", &debugMatr[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());

	debugQuad->draw();

	colShader->begin();
	colShader->setUniformMatrix4fv("m_pvm", &debugMatr[0][0]);

	// draw skeleton
	const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
	for (int i=0; i<users.getSize(); ++i)
	{
		const nite::UserData& user = users[i];

		if (user.isVisible())
		{
			drawPointPtr = 0;
			skelOffPtr = 0;

			glDisable(GL_DEPTH_TEST);

			if (users[i].getSkeleton().getState() == nite::SKELETON_TRACKED)
				DrawSkeletonFromData(nis->m_pUserTracker, user);

			glEnable(GL_DEPTH_TEST);
		}
	}
}

//----------------------------------------------------

void SNKinectPainting::getNiteHands(double time)
{
	vec3 newPos;

	if (kin->isNisInited() && depthFrameNr != kin->getNisFrameNr())
	{
		depthFrameNr = kin->getNisFrameNr(true);

		// update hand-data
		const nite::Array<nite::UserData>& users = nis->userTrackerFrame.getUsers();
		nite::JointType joints[2] = { nite::JOINT_LEFT_HAND, nite::JOINT_RIGHT_HAND };

		for (int i=0; i<maxUsers; i++)
		{
			if (i<users.getSize() && users[i].isVisible())
			{
				for (int j=0;j<2;j++)
				{
					// if the data is reliable
					if (users[i].getSkeleton().getState() == nite::SKELETON_TRACKED)
						newPos = nis->skelData->getJoint(i, joints[j]);

					// decide if draw, spot or nothing
					if ( newPos.z < zDrawThres
							&& (std::fabs(userHands[i][j].handPos.x - newPos.x) > moveThres
									|| std::fabs(userHands[i][j].handPos.y - newPos.y) > moveThres)
					)
					{
						userHands[i][j].lastHandPos = userHands[i][j].handPos;
						userHands[i][j].handPos = newPos;

						if ( std::fabs(userHands[i][j].lastHandPos.z - userHands[i][j].handPos.z) > splashThres )
						{
							userHands[i][j].doSplash = true;
							userHands[i][j].doDraw = false;
							userHands[i][j].lastSplashTime = time; // 1 second pause - no drawing after splash

						} else if (time - userHands[i][j].lastSplashTime > splashTimeThres)
						{
							if (!userHands[i][j].doDraw)
							{
								userHands[i][j].lastDrawPos.x = userHands[i][j].handPos.x -0.001f;
								userHands[i][j].lastDrawPos.y = userHands[i][j].handPos.y -0.001f;
								userHands[i][j].brushTexOffs = glm::vec2(getRandF(0.f, 1.f), getRandF(0.f, 1.f));
							}
							userHands[i][j].doDraw = true;
							userHands[i][j].doSplash = false;
						}
					} else
					{
						userHands[i][j].doSplash = false;
						userHands[i][j].doDraw = false;
					}
				}
			}
		}
	}
}

//----------------------------------------------------

void SNKinectPainting::genBrush(float _x, float _y, vec2 _brushTexOffs, float _scale, float _patternAmt,
		float _pressure, GLuint brushTexId, GLuint heightMProfTex)
{
	glm::vec2 brushTexOffset = _brushTexOffs + glm::vec2(_x +1.f, _y +1.f) * 0.25f;

	//  generate the actual brush color and normal map ----

	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_SUBTRACT);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ZERO);

	pen->bind();

	genBrushShader->begin();
	genBrushShader->setIdentMatrix4fv("m_pvm");
	genBrushShader->setUniformMatrix4fv("toTexMatr", &toTexMatr[0][0]);

	genBrushShader->setUniform1i("alphaTex", 0);
	genBrushShader->setUniform1i("patternTex", 1);
	genBrushShader->setUniform1i("oldCoverage", 2);

	genBrushShader->setUniform4fv("brushColor", &paintCol[0], 1);
	genBrushShader->setUniform1f("colMult", brushHardness);
	genBrushShader->setUniform1f("pressure", _pressure);
	genBrushShader->setUniform1f("patternAmt", _patternAmt);
	genBrushShader->setUniform2f("scale", _scale, _scale);
	genBrushShader->setUniform2f("offset", brushTexOffset.x, brushTexOffset.y);

	genBrushShader->setUniform1f("mixColors", smearing);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, brushTexId); // penalpha.jpg

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, xBlendFboV->getColorImg()); // perlin noise

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, canvasColor->getSrcTexId());

	quad->draw();

	pen->unbind();

	// ---- normal --

	glBlendEquation(GL_FUNC_ADD);

	penPressProfile->bind();
	penPressProfile->clearToColor(0.f, 0.f, 1.f, 0.f);

	glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_SRC_ALPHA, GL_ZERO);

	genBrushHMShader->begin();
	genBrushHMShader->setIdentMatrix4fv("m_pvm");
	genBrushHMShader->setUniform1i("patternTex", 0);
	genBrushHMShader->setUniform1i("blurTex", 1);
	genBrushHMShader->setUniform1f("colMult", 1.f);
	genBrushHMShader->setUniform2f("scale", _scale, _scale);
	genBrushHMShader->setUniform2f("offset", brushTexOffset.x, brushTexOffset.y);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, heightMap->getColorImg()); // penalpha.jpg

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, heightMProfTex); // penalpha_smooth.jpg

	quad->draw();

	penPressProfile->unbind();
}

//----------------------------------------------------

void SNKinectPainting::genSplash(double time)
{
	// threshold noise
	splash->dst->bind();

	splashShader->begin();
	splashShader->setIdentMatrix4fv("m_pvm");
	splashShader->setUniform1i("noiseTex", 0);
	splashShader->setUniform1f("offset", time);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, xBlendFboV->getColorImg()); // perlin noise
	quad->draw();

	splash->dst->unbind();
	splash->swap();


	fastBlurPP->clear(0.f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	// fast blur, 5 iterations
	for (int i=0;i<3;i++)
	{
		fastBlurPP->dst->bind();

		// linear vertical blur
		fastBlurVShader->begin();
		fastBlurVShader->setUniform1i("image", 0);
		fastBlurVShader->setUniform1i("old", 1);
		fastBlurVShader->setUniform1f("width", splashTexSize);
		fastBlurVShader->setUniform1f("height", splashTexSize);
		fastBlurVShader->setUniform1fv("offset", &blurOffsV[0], 3);
		fastBlurVShader->setUniform1f("oldFdbk", i==0 ? 0.f : 1.f);
		fastBlurVShader->setUniform1f("newFdbk", i==0 ? 1.f : 0.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, splash->getSrcTexId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fastBlurPP->getSrcTexId());

		quad->draw();

		fastBlurPP->dst->unbind();
		fastBlurPP->swap();


		// horizontal  blur
		fastBlurPP->dst->bind();

		fastBlurHShader->begin();
		fastBlurHShader->setUniform1i("image", 0);
		fastBlurHShader->setUniform1f("width", splashTexSize);
		fastBlurHShader->setUniform1f("height", splashTexSize);
		fastBlurHShader->setUniform1fv("offset", &blurOffsH[0], 3);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fastBlurPP->getSrcTexId());
		quad->draw();

		fastBlurPP->dst->unbind();
		fastBlurPP->swap();
	}
}

//----------------------------------------------------

void SNKinectPainting::genBrushTex()
{
	glDisable(GL_DEPTH_TEST);

	// ---- generate perlin noise pattern ---

	perlinNoise->bind();
	perlinNoise->clear();

	noiseShader->begin();
	noiseShader->setIdentMatrix4fv("m_pvm");
	noiseShader->setUniform1f("width", (float)noiseTexSize);
	noiseShader->setUniform1f("height", (float)noiseTexSize);
	noiseShader->setUniform2f("noiseScale", 1.f, 1.f);

	quad->draw();

	perlinNoise->unbind();

	// ---- blend texture perlin noise pattern to be used borderless ---
	// ---- blend horizontal ---

	xBlendFboH->bind();
	xBlendFboH->clear();

	xBlendShaderH->begin();
	xBlendShaderH->setIdentMatrix4fv("m_pvm");
	xBlendShaderH->setUniform1i("tex", 0);
	xBlendShaderH->setUniform1i("width", noiseTexSize);
	xBlendShaderH->setUniform1i("height", noiseTexSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, perlinNoise->getColorImg());

	quad->draw();

	xBlendFboH->unbind();

	// ---- blend vertical ---

	xBlendFboV->bind();
	xBlendFboV->clear();

	xBlendShaderV->begin();
	xBlendShaderV->setIdentMatrix4fv("m_pvm");
	xBlendShaderV->setUniform1i("tex", 0);
	xBlendShaderV->setUniform1i("width", noiseTexSize);
	xBlendShaderV->setUniform1i("height", noiseTexSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, xBlendFboH->getColorImg());

	quad->draw();

	xBlendFboV->unbind();

	// ---- generate normal map ---

	heightMap->bind();
	heightMapShader->begin();
	heightMapShader->setIdentMatrix4fv("m_pvm");
	heightMapShader->setUniform1i("tex", 0);
	heightMapShader->setUniform1f("stepX", static_cast<float>(1.f / noiseTexSize));
	heightMapShader->setUniform1f("stepY", static_cast<float>(1.f / noiseTexSize));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, xBlendFboV->getColorImg());
	quad->draw();

	heightMap->unbind();

}

//----------------------------------------------------

void SNKinectPainting::procBrushStrokes(double _time, handData* _handData)
{
	float _pressure = (zDrawThres - _handData->handPos.z - minZ) * (1.f / (zDrawThres -minZ));
	_pressure = std::min(_pressure + 0.3f, 1.f);

	vec2 dir = vec2(_handData->handPos.x, _handData->handPos.y)
        		- vec2(_handData->lastDrawPos.x, _handData->lastDrawPos.y);

	// if a new stroke begins
	if (_time - _handData->lastDrawTime > 0.4 && glm::length(dir) > 0.1)
		_handData->intp.clear();

	if (_handData->intp.size() < nrIntpSegs) {
		_handData->intp.push_back( vec2(_handData->handPos.x, _handData->handPos.y) );
	} else {
		_handData->intp.erase(0);
		_handData->intp.push_back( vec2(_handData->handPos.x, _handData->handPos.y) );
	}

	// calculate the brush points to draw
	if (_handData->intp.size() > 2)
	{
		float angle = std::atan2(dir.y, dir.x);
		int nrPoints = std::min(std::max(static_cast<int>(glm::length(dir) / penDrawDist), 2), 300);

		for (int i=0;i<nrPoints;i++)
		{
			float ind = static_cast<float>(i) / static_cast<float>(std::max(nrPoints-1, 1)); // 0-1
			ind /= static_cast<float>(nrIntpSegs -1);   // 0 - length of one segment
			vec2 intrpPos = _handData->intp.sampleAt(ind + static_cast<float>(nrIntpSegs -2)
					/ static_cast<float>(nrIntpSegs -1));

			genBrush(intrpPos.x, intrpPos.y, _handData->brushTexOffs, scaleBrushTex, 1.f,
					_pressure, brushProfileTex->getId(), hmProfileTex->getId());

			drawBrushStroke(intrpPos.x, intrpPos.y, brushSize, 0.1f, pen->getColorImg(),
					penPressProfile->getColorImg());
		}
	}

	_handData->lastDrawPos = _handData->handPos;
	_handData->lastDrawTime = _time;
}

//----------------------------------------------------

void SNKinectPainting::procSplash(double time, handData* _handData)
{
	genSplash(time);
	genBrush(_handData->handPos.x, _handData->handPos.y, _handData->brushTexOffs,
			scaleBrushTex * 0.5f, 0.f, 1.f, fastBlurPP->getSrcTexId(),
			fastBlurPP->getSrcTexId());

	drawBrushStroke(_handData->handPos.x, _handData->handPos.y, splashSize, 1.f,
			pen->getColorImg(), penPressProfile->getColorImg());

	_handData->doSplash = false;
}

//----------------------------------------------------

void SNKinectPainting::DrawLimbFromData(short userId, nite::JointType joint1, nite::JointType joint2, int color)
{
	float coordinates[4] = {-0.5f, -0.5f, -0.5f, -0.5f};

	vec3 p1 = nis->skelData->getJoint(userId, joint1);
	vec3 p2 = nis->skelData->getJoint(userId, joint2);

	// write lines into the vao
	GLfloat* pos = (GLfloat*) skeletonVao->getMapBuffer(POSITION);
	pos[skelOffPtr*3] = p1.x; pos[skelOffPtr*3+1] = p1.y; pos[skelOffPtr*3+2] = 0.f;
	pos[skelOffPtr*3+3] = p2.x; pos[skelOffPtr*3+4] = p2.y; pos[skelOffPtr*3+5] = 0.f;
	skeletonVao->unMapBuffer();

	GLfloat* colorPtr = (GLfloat*) skeletonVao->getMapBuffer(COLOR);
	for (int i=0;i<2;i++){
		colorPtr[(i+skelOffPtr)*4] = skeletonColors[color][0];
		colorPtr[(i+skelOffPtr)*4+1] = skeletonColors[color][1];
		colorPtr[(i+skelOffPtr)*4+2] = skeletonColors[color][2];
		colorPtr[(i+skelOffPtr)*4+3] = 1.f;
	}
	skeletonVao->unMapBuffer();

	// draw the lines
	skeletonVao->draw(GL_LINES, skelOffPtr, 2, nullptr);
	skelOffPtr += 2;


	// write points into vao
	GLfloat* pPos = (GLfloat*) pointsVao->getMapBuffer(POSITION);
	pPos[drawPointPtr*3] = p1.x; pPos[drawPointPtr*3+1] = p1.y; pPos[drawPointPtr*3+2] = 0.f;
	pPos[drawPointPtr*3+3] = p2.x; pPos[drawPointPtr*3+4] = p2.y; pPos[drawPointPtr*3+5] = 0.f;
	pointsVao->unMapBuffer();

	GLfloat* pColorPtr = (GLfloat*) pointsVao->getMapBuffer(COLOR);
	for (int i=0;i<2;i++){
		pColorPtr[(i+drawPointPtr)*4] = skeletonColors[color][0];
		pColorPtr[(i+drawPointPtr)*4+1] = skeletonColors[color][1];
		pColorPtr[(i+drawPointPtr)*4+2] = skeletonColors[color][2];
		pColorPtr[(i+drawPointPtr)*4+3] = 1.f;
	}
	pointsVao->unMapBuffer();

	glPointSize(10);
	pointsVao->draw(GL_POINTS, drawPointPtr, 2, nullptr);
	drawPointPtr += 2;
}

//----------------------------------------------------

void SNKinectPainting::DrawSkeletonFromData(nite::UserTracker* pUserTracker, const nite::UserData& userData)
{
	for (auto i=0;i<nis->skelData->getNrUsers();i++)
	{
		DrawLimbFromData(i, nite::JOINT_HEAD, nite::JOINT_NECK, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_LEFT_ELBOW, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_ELBOW, nite::JOINT_LEFT_HAND, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_RIGHT_SHOULDER, nite::JOINT_RIGHT_ELBOW, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_RIGHT_ELBOW, nite::JOINT_RIGHT_HAND, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_RIGHT_SHOULDER, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_SHOULDER, nite::JOINT_TORSO, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_RIGHT_SHOULDER, nite::JOINT_TORSO, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_TORSO, nite::JOINT_LEFT_HIP, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_TORSO, nite::JOINT_RIGHT_HIP, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_HIP, nite::JOINT_RIGHT_HIP, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_HIP, nite::JOINT_LEFT_KNEE, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_LEFT_KNEE, nite::JOINT_LEFT_FOOT, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_RIGHT_HIP, nite::JOINT_RIGHT_KNEE, i % colorCount);
		DrawLimbFromData(i, nite::JOINT_RIGHT_KNEE, nite::JOINT_RIGHT_FOOT, i % colorCount);
	}
}

//----------------------------------------------------

void SNKinectPainting::update(double time, double dt)
{}

//----------------------------------------------------

void SNKinectPainting::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_C :
			requestClear = true;
			break;
		}
	}
}

//----------------------------------------------------

void SNKinectPainting::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	vec2 newPos = vec2(xpos, ypos);

	// normalize coordinates (-1, 1)
	if (useMouse && inited
			&& (debugMouse.handPos.x != newPos.x || debugMouse.handPos.y != newPos.y))
	{
		debugMouse.handPos.x = (float)xpos / (float)scd->screenWidth *2.f - 1.f;
		debugMouse.handPos.y = (1.f - (float)ypos / (float)scd->screenHeight) * 2.f - 1.f;
		//std::cout << "handPos: " << glm::to_string(debugMouse.handPos) << std::endl;
	}
}

//----------------------------------------------------

void SNKinectPainting::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (inited)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
			debugMouse.doDraw = true;

			debugMouse.lastDrawPos.x = debugMouse.handPos.x -0.001f;
			debugMouse.lastDrawPos.y = debugMouse.handPos.y -0.001f;

			debugMouse.brushTexOffs = glm::vec2(getRandF(0.f, 1.f), getRandF(0.f, 1.f));
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
			debugMouse.doDraw = false;
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{
			debugMouse.doSplash = false;
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		{
			debugMouse.doSplash = true;
		}
	}
}

}
