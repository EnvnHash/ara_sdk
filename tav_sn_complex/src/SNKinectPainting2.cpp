//
//  SNGLSLFluidTest.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//
//  Has to be calibrated by printed chessboard
//  distance defines the wallS"§"

#include "SNKinectPainting2.h"

using namespace glm;
using namespace std;

namespace tav
{
SNKinectPainting2::SNKinectPainting2(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs),useMouse(false), nrIntpSegs(3), doSplash(false), showKinectIn(true), maxUsers(4),
			zDrawThres(1.f), actMode(KinectReproTools::USE_ROT_WARP), kinDeviceNr(0), hFlip(false),
			actDrawMode(DIRECT_HAND)
{
	kin = static_cast<KinectInput*>(scd->kin);
	osc = static_cast<OSCData*>(scd->osc);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	shCol = static_cast<ShaderCollector*>(shCol);

	calibFileName = (*scd->dataPath)+"calib_cam/kinectPainting2.yml";

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


	// --- blob detection -

	cv::SimpleBlobDetector::Params pDefaultBLOB;

	// This is default parameters for SimpleBlobDetector
	pDefaultBLOB.thresholdStep = 10;
	pDefaultBLOB.minThreshold = 10;
	pDefaultBLOB.maxThreshold = 1000;
	pDefaultBLOB.minRepeatability = 2;
	pDefaultBLOB.minDistBetweenBlobs = 10;
	pDefaultBLOB.filterByColor = false;
	pDefaultBLOB.blobColor = 0;
	pDefaultBLOB.filterByArea = true;
	pDefaultBLOB.minArea = 25; // 35
	pDefaultBLOB.maxArea = 5000;

	pDefaultBLOB.filterByCircularity = false;
	pDefaultBLOB.minCircularity = 0.9f;
	pDefaultBLOB.maxCircularity = (float)1e37;

	pDefaultBLOB.filterByInertia = false;
	pDefaultBLOB.minInertiaRatio = 0.1f;
	pDefaultBLOB.maxInertiaRatio = (float)1e37;

	pDefaultBLOB.filterByConvexity = false;
	pDefaultBLOB.minConvexity = 0.95f;
	pDefaultBLOB.maxConvexity = (float)1e37;

	detector = cv::SimpleBlobDetector::create(pDefaultBLOB);


	// --- for nite skeleton debugging ---
	/*
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
	 */
	paintCol = glm::vec4(1.f, 0.f, 0.f, 1.f);

	// -- FBOs --

	drawFbo = new FBO(shCol, 1024, 1024);
	/*
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
        fastBlurPP = new PingPongFbo(_shCol, splashTexSize, splashTexSize, GL_RGBA8, GL_TEXTURE_2D);
	 */

	// -- Shaders --

	colShader = shCol->getStdCol();
	noiseShader = shCol->get("perlin");
	texShader = shCol->getStdTex();
	setupBlendShader();
	setupKinectDebugShader();

	/*
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
	 */
}

//----------------------------------------------------

SNKinectPainting2::~SNKinectPainting2()
{
	delete colShader;
	delete texShader;
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
	kinDebugShader->remove();
	blendShader->remove();
}

//----------------------------------------------------

void SNKinectPainting2::initKinDependent()
{
	kin->setImageRegistration(true);
	//kin->setMirror(false); // geht nicht bei depth stream, warum auch immer..

	kinRepro = new KinectReproTools(winMan, kin, shCol, scd->screenWidth,
			scd->screenHeight, *(scd->dataPath), kinDeviceNr);

	kinRepro->noUnwarp();
	kinRepro->loadCalib(calibFileName);
	kinRepro->setMode(actMode);
	kinRepro->setHFlip(hFlip);

	checkWarp = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC3);
}

//----------------------------------------------------

void SNKinectPainting2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (inited && kin->isReady())
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		switch(actMode)
		{
		case KinectReproTools::CALIB_ROT_WARP:
			kinRepro->drawFoundChess();
			break;

		case KinectReproTools::CHECK_ROT_WARP:
			kinRepro->drawCheckRotWarp();
			break;

		case KinectReproTools::USE_ROT_WARP:

			drawFbo->bind();
			drawFbo->clearAlpha(osc->feedback);

			//                   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			blendShader->begin();
			blendShader->setUniform1i("tex", 0);
			blendShader->setIdentMatrix4fv("m_pvm");
			blendShader->setUniform4fv("drawCol", &paintCol[0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kinRepro->getRotDepth());
			quad->draw();

			drawFbo->unbind();


			texShader->begin();
			texShader->setIdentMatrix4fv("m_pvm");
			texShader->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, drawFbo->getColorImg());
			debugQuad->draw();


			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			kinDebugShader->begin();
			kinDebugShader->setUniform1i("tex", 0);
			kinDebugShader->setUniform1i("tex2", 1);
			kinDebugShader->setUniformMatrix4fv("m_pvm", &debugMatr[0][0]);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kin->getColorTexId());

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, kinRepro->getRotDepth());
			debugQuad->draw();
			break;
		}
	}

	// cycle brush color
	paintCol.r = std::cos(time * 0.2 + M_PI) * 0.5f + 0.5f;
	paintCol.g = std::cos(time * 0.21) * 0.5f + 0.5f;
	paintCol.b = std::cos(time * 0.22) * 0.5f + 0.5f;

	/*
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
	 */
}

//----------------------------------------------------

void SNKinectPainting2::applyBlur()
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

void SNKinectPainting2::applyHeightMap(double time)
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

void SNKinectPainting2::blendBlurToAlpha()
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

void SNKinectPainting2::blendCanvasToAlpha()
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

void SNKinectPainting2::clearTextures()
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

void SNKinectPainting2::drawBrushStroke(float posX, float posY, float scale, float alphaMult,
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

void SNKinectPainting2::showKinectInput()
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
}

//----------------------------------------------------

void SNKinectPainting2::genBrush(float _x, float _y, vec2 _brushTexOffs, float _scale, float _patternAmt,
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

void SNKinectPainting2::genSplash(double time)
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

void SNKinectPainting2::genBrushTex()
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

void SNKinectPainting2::procBrushStrokes(double _time)
{
	//float _pressure = (zDrawThres - _handData->handPos.z - minZ) * (1.f / (zDrawThres -minZ));
	float _pressure = 1.f;
	_pressure = std::min(_pressure + 0.3f, 1.f);

	//        vec2 dir = vec2(_handData->handPos.x, _handData->handPos.y)
	//        - vec2(_handData->lastDrawPos.x, _handData->lastDrawPos.y);

	/*
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
	 */

}

//----------------------------------------------------

void SNKinectPainting2::procSplash(double time)
{
	/*
        genSplash(time);
        genBrush(_handData->handPos.x, _handData->handPos.y, _handData->brushTexOffs,
                 scaleBrushTex * 0.5f, 0.f, 1.f, fastBlurPP->getSrcTexId(),
                 fastBlurPP->getSrcTexId());

        drawBrushStroke(_handData->handPos.x, _handData->handPos.y, splashSize, 1.f,
                        pen->getColorImg(), penPressProfile->getColorImg());

        _handData->doSplash = false;
	 */
}

//----------------------------------------------------

void SNKinectPainting2::update(double time, double dt)
{
	if(!inited && kin->isReady())
	{
		initKinDependent();
		inited = true;
	} else
	{
		kinRepro->update();

		switch(actMode)
		{
		case KinectReproTools::CHECK_ROT_WARP :
			kinRepro->setThreshZNear(kinRepro->getCalibData()->distKinObj - osc->alpha * 500.f);
			kinRepro->setThreshZDeep(kinRepro->getCalibData()->distKinObj);

			// download rotDepthImg
			//                    glBindTexture(GL_TEXTURE_2D, kinRepro->getRotDepth());
			//                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, checkWarp.data);
			//
			//                    procBlobs(time, dt);

			break;
		case KinectReproTools::USE_ROT_WARP :
			kin->uploadColorImg(kinDeviceNr);
			break;
		}
	}
}

//----------------------------------------------------

void SNKinectPainting2::procBlobs(double time, double dt)
{
	glm::vec2 actPoint;

	// Detect blobs.
	std::vector<cv::KeyPoint> keypoints;
	detector->detect(checkWarp, keypoints);

	// draw keypoints
	cv::drawKeypoints(checkWarp, keypoints, im_with_keypoints,
			cv::Scalar(255,0,255),
			cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	// upload debug tex
	glBindTexture(GL_TEXTURE_2D, kinRepro->getRotDepth());
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // First mipmap level
			0, 0,                       // x and y offset
			kin->getDepthWidth(kinDeviceNr),
			kin->getDepthHeight(kinDeviceNr),
			GL_RGB,
			GL_UNSIGNED_BYTE,
			im_with_keypoints.data);
}

//----------------------------------------------------

void SNKinectPainting2::setupBlendShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	uniform mat4 m_pvm;\n

	out vec2 tex_coord;\n
	out vec4 col;\n

	void main(){\n
		col = color;\n
		tex_coord = texCoord;\n
		gl_Position = m_pvm * position;\n
	});


	vert = "// kinectPainting2 draw shader, vert\n"+shdr_Header+vert;

	std::string frag = STRINGIFY(uniform sampler2D tex;\n
	uniform vec4 drawCol;
	in vec2 tex_coord;\n
	in vec4 col;\n
	layout (location = 0) out vec4 color;\n

	void main(){\n
		vec4 texC = texture(tex, tex_coord);
	float alpha = texC.r + texC.g + texC.b;
	color = vec4(drawCol.rgb, texC.r * 0.5);\n
	});

	frag = "// kinectPainting2 draw shader, frag\n"+shdr_Header+frag;

	blendShader = shCol->addCheckShaderText("kinectPaint2Blend", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNKinectPainting2::setupKinectDebugShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;\n
	layout( location = 1 ) in vec4 normal;\n
	layout( location = 2 ) in vec2 texCoord;\n
	layout( location = 3 ) in vec4 color;\n
	uniform mat4 m_pvm;\n

	out vec2 tex_coord;\n
	out vec4 col;\n

	void main(){\n
		col = color;\n
		tex_coord = texCoord;\n
		gl_Position = m_pvm * position;\n
	});


	vert = "// kinectPainting2 kinect debug shader, vert\n"+shdr_Header+vert;

	std::string frag = STRINGIFY(uniform sampler2D tex;\n
	uniform sampler2D tex2;\n
	in vec2 tex_coord;\n
	in vec4 col;\n
	layout (location = 0) out vec4 color;\n

	void main(){\n
		vec4 tex1C = texture(tex, tex_coord);
	vec4 tex2C = texture(tex2, tex_coord);
	color = vec4(tex1C.rgb + vec3(0.0, 0.5 * tex2C.r, 0.0), 1.0);\n
	});

	frag = "// kinectPainting2 kinect debug shader, frag\n"+shdr_Header+frag;

	kinDebugShader = shCol->addCheckShaderText("kinectPaint2KinDebug", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNKinectPainting2::onKey(int key, int scancode, int action, int mods)
{
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
				actMode = KinectReproTools::CHECK_ROT_WARP;
				printf("SNTTexPartFluid::CHECK_ROT_WARP \n");
				break;
			case GLFW_KEY_3 :
				actMode = KinectReproTools::USE_ROT_WARP;
				printf("SNTTexPartFluid::USE_ROT_WARP \n");
				break;
			case GLFW_KEY_S : kinRepro->saveCalib(calibFileName);
			break;
			}

			kinRepro->setMode(actMode);
		} else
		{
			switch (key)
			{
			case GLFW_KEY_C :
				requestClear = true;
				break;
			}
		}
	}
}

//----------------------------------------------------

void SNKinectPainting2::onCursor(GLFWwindow* window, double xpos, double ypos)
{
	vec2 newPos = vec2(xpos, ypos);
}

//----------------------------------------------------

void SNKinectPainting2::onMouseButton(GLFWwindow* window, int button, int action, int mods)
{
	if (inited)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		{
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		{
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{
		}

		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
		{
		}
	}
}

}
