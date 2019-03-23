//
//  KinectReproTools.cpp
//  tav_tracking
//
//  Created by Sven Hahne on 24/10/15.
//  Copyright © 2015 Sven Hahne. All rights reserved.
//
//  Using this file useDepthHisto should be 0
//
//  Tools for calibration of a Depth-/ColorSensor to a Projector
//  Features: - Depth-Thresholding for a depth image for background substraction
//              through a projected chessboard or specific window in the depth image
//            - Estimation of x, y and z-rotations of a depthsensor
//              through a projected chessboard or specific window in the depth image
//            - Transformation of DepthSensor Coordinates to a calibrated World Space
//            - Transformation of a calibrated World Space to a Beamer Space
//              beamer has to be defined by FovX, FovY and the angle of the beamer lens
//              center to the center of the projected image

#include "KinectReproTools.h"

#define STRINGIFY(A) #A

namespace tav
{

KinectReproTools::KinectReproTools(GWindowManager* _winMan, KinectInput* _kin,
		ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
		std::string _dataPath, int _kinDevId0, std::string _camCalibFile) :
		winMan(_winMan), kin(_kin), shCol(_shCol), scrWidth(_scrWidth), scrHeight(_scrHeight),
		kinDevId0(_kinDevId0), nrFrameBuf(2), texPtr(0), useCalib(true), dataPath(_dataPath)
{
	initBasic();

	/*
	 cv::FileStorage fs(_camCalibFile, cv::FileStorage::READ);
	 if ( fs.isOpened() )
	 {
	 fs["camera_matrix"] >> cameraMatrix;
	 fs["distortion_coefficients"] >> distCoeffs;
	 } else {
	 printf("couldn´t open file\n");
	 }
	 */
}

//---------------------------------------------------------------

KinectReproTools::KinectReproTools(GWindowManager* _winMan, KinectInput* _kin,
		ShaderCollector* _shCol, int _scrWidth, int _scrHeight,
		std::string _dataPath, int _kinDevId0) :
		winMan(_winMan), kin(_kin), shCol(_shCol), scrWidth(_scrWidth), scrHeight(
				_scrHeight), kinDevId0(_kinDevId0), nrFrameBuf(2), texPtr(0), dataPath(
				_dataPath)
{
	initBasic();
}

//---------------------------------------------------------------

void KinectReproTools::initBasic()
{
	kpCalib.boardSize = cv::Size(8, 4);
	// sollte vom device gelesen werden, wird in KinectInput ausgelesen
	//kpCalib.kinFovX = (61.66f / 360.f) * 2.f * M_PI;
	//kpCalib.kinFovY = (49.234f / 360.f) * 2.f * M_PI;
	kpCalib.kinFovX = kin->getDepthFovX();
	kpCalib.kinFovY = kin->getDepthFovY();
	kpCalib.kinFov = glm::vec2(kpCalib.kinFovX, kpCalib.kinFovY);

	// -------------- Textures -------------------

	kinTex = new TextureManager();
	kinDepthTex = new TextureManager();
	kinHistoDepth = new TextureManager();
	unwarpTex = new TextureManager*[nrFrameBuf];
	for (int i = 0; i < nrFrameBuf; i++)
		unwarpTex[i] = new TextureManager();

	// -------------- Median -------------------

	cbNormal = new Median<glm::vec3>(15.f);
	depthNormal = new Median<glm::vec3>(5.f);
	cbTrans = new Median<glm::vec3>(15.f);

	// -------------- shader -------------------

	texShader = shCol->getStdTex();
	colShader = shCol->getStdCol();
	setupUndistShdr();
	setupCropShdr();

	initTrigVao(); // muss vor setupGeoRotShdr aufgerufen werden
	setupGeoRotShdr();

	// -------------- Chessboard -------------------

	glChessBoardWidth = 0.8f;
	//glChessBoardWidth = 1.3f;
	chessBoard = new PropoImage(dataPath + "textures/chessboard.jpg", scrWidth,
			scrHeight, glChessBoardWidth);
	glChessBoardHeight = chessBoard->getImgHeight();

	// --------- VAO --------------------------

	quad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f, 0.f,
			0.f, 1.f);
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	rawQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
			0.f, 0.f, 1.f);

	whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f,
			1.f, 1.f, 1.f);

	cornerQuad = new Quad(
			-1.0f + (2.f - float(scrHeight) / float(scrWidth) * 2.f) * 0.5f,
			-1.0f, float(scrHeight) / float(scrWidth) * 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f), 0.f, 1.f, 0.f, 1.f);

	rotDepthMaessureSize = 8;
	rotDepthMaessure = new FBO(shCol, rotDepthMaessureSize,
			rotDepthMaessureSize, GL_RGB8, GL_TEXTURE_2D, false, 1, 1, 1,
			GL_REPEAT, false);
	rotDepthM = new uint8_t[rotDepthMaessureSize * rotDepthMaessureSize];

	// ------------------ init the calibWin Quad --------------------------

	calibWin = new VAO("position:3f,color:4f", GL_DYNAMIC_DRAW);
	calibWin->initData(5);
	calibWin->setStaticColor(0.f, 1.f, 0.f, 1.f);

	GLfloat* pPos = (GLfloat*) calibWin->getMapBuffer(POSITION);
	for (int i = 0; i < 5; i++)
	{
		pPos[i * 3] = kpCalib.calibWinCorners[i % 4].x;
		pPos[i * 3 + 1] = kpCalib.calibWinCorners[i % 4].y;
		pPos[i * 3 + 2] = 0.f;
	}
	calibWin->unMapBuffer();

	cornerQuadMat = glm::mat4(1.f);
	cornerQuadSize = 0.02f;

	stdTouchDepth = 100.f;

	// request another window for the controls
	//    printf("KinectReproTools requesting new window \n");
	//    winMan->winPushBack(640, 480, 60, false, true, 0, 0, true, false);

	outTexId = new GLint[2];
	outTexId[0] = -1;
	outTexId[1] = -1;
}

//---------------------------------------------------------------

void KinectReproTools::init()
{
	kpCalib.imgSize.width = static_cast<float>(kin->getColorWidth(kinDevId0));
	kpCalib.imgSize.height = static_cast<float>(kin->getColorHeight(kinDevId0));
	kpCalib.depthImgSize.width = static_cast<float>(kin->getDepthWidth(
			kinDevId0));
	kpCalib.depthImgSize.height = static_cast<float>(kin->getDepthHeight(
			kinDevId0));

	view = cv::Mat(kin->getColorHeight(kinDevId0),
			kin->getColorWidth(kinDevId0), CV_8UC3);
	rview = cv::Mat(kin->getColorHeight(kinDevId0),
			kin->getColorWidth(kinDevId0), CV_8UC3);
	thresh8 = cv::Mat(kin->getDepthHeight(kinDevId0),
			kin->getDepthWidth(kinDevId0), CV_8UC3);
	checkWarpDepth = cv::Mat(kin->getDepthHeight(kinDevId0),
			kin->getDepthWidth(kinDevId0), CV_8UC3);

	for (int i = 0; i < nrFrameBuf; i++)
		unwarpTex[i]->allocate(kin->getDepthWidth(kinDevId0),
				kin->getDepthHeight(kinDevId0),
				GL_RGB8, GL_RGB, GL_TEXTURE_2D);

	rotDepth = new FBO(shCol, kin->getDepthWidth(kinDevId0),
			kin->getDepthHeight(kinDevId0),
			GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	kinTex->allocate(kin->getColorWidth(kinDevId0),
			kin->getColorHeight(kinDevId0),
			GL_RGB8, GL_RGB, GL_TEXTURE_2D);

	kinHistoDepth->allocate(kin->getDepthWidth(kinDevId0),
			kin->getDepthHeight(kinDevId0),
			GL_R32F, GL_RED, GL_TEXTURE_2D);

	if (useCalib)
	{
		fastBlur = new FastBlur(shCol, kin->getDepthWidth(kinDevId0),
				kin->getDepthHeight(kinDevId0), GL_R32F);

		depthView = cv::Mat(kin->getDepthHeight(kinDevId0),
				kin->getDepthWidth(kinDevId0), CV_16UC1);
		depthRView = cv::Mat(kin->getDepthHeight(kinDevId0),
				kin->getDepthWidth(kinDevId0), CV_16UC1);

		imageSize = cv::Size(kin->getColorWidth(kinDevId0),
				kin->getColorHeight(kinDevId0));
		cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, cv::Mat(),
				cameraMatrix, imageSize, CV_16SC2, map1, map2);
		kinDepthTex->allocate(kin->getDepthWidth(kinDevId0),
				kin->getDepthHeight(kinDevId0),
				GL_R8, GL_RED, GL_TEXTURE_2D);
	}
	else
	{
		fastBlur = new FastBlur(shCol, kin->getDepthWidth(kinDevId0),
				kin->getDepthHeight(kinDevId0), GL_R32F);

		kinDepthTex->allocate(kin->getDepthWidth(kinDevId0),
				kin->getDepthHeight(kinDevId0),
				GL_R32F, GL_RED, GL_TEXTURE_2D);
	}

	inited = true;
}

//---------------------------------------------------------------

void KinectReproTools::initTrigVao()
{
	// heisst, aber nicht, dass das das Schnellste ist. besser von Hand tunen.
	//GLint maxGeoAmpPoints;
	//glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &maxGeoAmpPoints);
	maxGeoAmpPoints = 64; // muss klein genug sein, dass die Textur in ganzzahlige Vielfache davon geteilt werden koennen.
	std::cout << "maxGeoAmpPoints: " << maxGeoAmpPoints << std::endl;

	// get cell size
	float divisor = 2.f;
	while (divisor < (static_cast<float>(maxGeoAmpPoints) / divisor))
		divisor += 2.f;

	ptex.cellSize.x = divisor;
	ptex.cellSize.y =
			static_cast<unsigned int>(static_cast<float>(maxGeoAmpPoints)
					/ divisor);
	//std::cout << "ptex.cellSize : " << glm::to_string(ptex.cellSize) << std::endl;

	ptex.nrPartPerCell = ptex.cellSize.x * ptex.cellSize.y;
	//std::cout << "ptex.nrPartPerCell : " << ptex.nrPartPerCell << std::endl;

	// anzahl zellen in x und y aufteilen
	ptex.nrCells.x =
			static_cast<unsigned int>(static_cast<float>(kin->getDepthWidth(0))
					/ static_cast<float>(ptex.cellSize.x));
	ptex.nrCells.y =
			static_cast<unsigned int>(static_cast<float>(kin->getDepthHeight(0))
					/ static_cast<float>(ptex.cellSize.y));
	//std::cout << "ptex.nrCells : " << glm::to_string(ptex.nrCells) << std::endl;

	// berechne die anzahl benötigter zellen, nächst höheres vielfaches von 2
	ptex.totNrCells = ptex.nrCells.x * ptex.nrCells.y;
	//std::cout << "ptex.totNrCells : " << ptex.totNrCells << std::endl;

	// VAO mit den Trigger, die den aktuellen Schreibpositionen der Zellen entsprechen
	ptex.cellStep.x = 1.f / static_cast<float>(ptex.nrCells.x);
	ptex.cellStep.y = 1.f / static_cast<float>(ptex.nrCells.y);
	// std::cout << "ptex.cellStep : " << glm::to_string(ptex.cellStep) << std::endl;

	ptex.kinFovFact = glm::vec2(std::tan(kin->getDepthFovX() * 0.5f) * 2.0f,
			std::tan(kin->getDepthFovY() * 0.5f) * 2.0f);

	//---- trigVaoPoints ------------------------------------------------

	// emitPos beziehen sich auf textur koordinaten in integer deshalb ([0|tex_width], [0|tex_height])
	GLfloat initEmitPos[ptex.totNrCells * 4];
	for (int y = 0; y < ptex.nrCells.y; y++)
	{
		for (int x = 0; x < ptex.nrCells.x; x++)
		{
			initEmitPos[(y * ptex.nrCells.x + x) * 4] = static_cast<float>(x
					* ptex.cellSize.x);
			initEmitPos[(y * ptex.nrCells.x + x) * 4 + 1] = static_cast<float>(y
					* ptex.cellSize.y);
			//initEmitPos[(y * ptex.nrCells.x + x) * 4] = static_cast<float>(x * ptex.cellSize.x) / float(kin->getDepthWidth(0)) * 2.f -1.f;
			//initEmitPos[(y * ptex.nrCells.x + x) * 4 + 1] = static_cast<float>(y * ptex.cellSize.y) / float(kin->getDepthHeight(0)) * 2.f -1.f;

			initEmitPos[(y * ptex.nrCells.x + x) * 4 + 2] = 0.f;
			initEmitPos[(y * ptex.nrCells.x + x) * 4 + 3] = 1.f;

			//	std::cout << initEmitPos[(y * ptex.nrCells.x + x) * 4] << ", " << initEmitPos[(y * ptex.nrCells.x + x) * 4 +1] << std::endl;
		}
	}

	ptex.trigVao = new VAO("position:4f", GL_DYNAMIC_DRAW);
	ptex.trigVao->initData(ptex.totNrCells, initEmitPos);

	ptex.ppBuf = new PingPongFbo(shCol, kin->getDepthWidth(0),
			kin->getDepthHeight(0), GL_R32F,
			GL_TEXTURE_2D, false, 2, 1, GL_CLAMP_TO_EDGE);
	ptex.ppBuf->setMinFilter(GL_NEAREST);
	ptex.ppBuf->setMagFilter(GL_NEAREST);
	ptex.ppBuf->src->clear();
	ptex.ppBuf->dst->clear();

	/*
	ptex.fbo = new FBO(shCol, kin->getDepthWidth(0),
			kin->getDepthHeight(0), GL_R32F,
			GL_TEXTURE_2D, false, 2, 1, 1, GL_CLAMP_TO_EDGE, false);
	ptex.fbo->setMinFilter(GL_NEAREST);
	ptex.fbo->setMagFilter(GL_NEAREST);
	ptex.fbo->clear();
*/

	//glm::mat4 transMat = glm::mat4(1.f);
}

//---------------------------------------------------------------

void KinectReproTools::update()
{
	if (kin->isReady())
	{
		if (!inited)
			init();

		// proc color frame
		if (colFrameNr != kin->getColFrameNr(kinDevId0))
		{
			view.data = kin->getActColorImg(kinDevId0);
			procView = &view;

			if (useCalib)
			{
				cv::remap(view, rview, map1, map2, cv::INTER_LINEAR);
				procView = &rview;
			}

			kinTex->bind();
			glTexSubImage2D(
					GL_TEXTURE_2D,             // target
					0,                          // First mipmap level
					0,
					0,                       // x and y offset
					kin->getColorWidth(kinDevId0),
					kin->getColorHeight(kinDevId0),
					GL_RGB,
					GL_UNSIGNED_BYTE, procView->data);
		}

		// proc depth frame
		if (depthFrameNr != kin->getDepthFrameNr(kinDevId0))
		{
			// ----- upload depth image from kinect -----------

			kin->lockDepthMutex(kinDevId0);
			kinDepthTex->bind();

			// depth frame -> immer raw
			glTexSubImage2D(
					GL_TEXTURE_2D,             // target
					0,                          // First mipmap level
					0,
					0,                       // x and y offset
					kin->getDepthWidth(kinDevId0),
					kin->getDepthHeight(kinDevId0),
					GL_RED,
					GL_UNSIGNED_SHORT,
					kin->getDepthFrame(kinDevId0)->getData());


			if (actMode == CHECK_RAW_DEPTH || actMode == CALIB_ROT_WIN)
			{
				kinHistoDepth->bind();
				glTexSubImage2D(
						GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0,
						0,                       // x and y offset
						kin->getDepthWidth(kinDevId0),
						kin->getDepthHeight(kinDevId0),
						GL_RED,
						GL_UNSIGNED_SHORT, kin->getActDepthImg());
			}

			kin->unlockDepthMutex(kinDevId0);

			// ----- blur depth values -----------

			fastBlur->proc(kinDepthTex->getId());
			fastBlur->downloadData();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			depthFrameNr = kin->getDepthFrameNr(kinDevId0);

			switch (actMode)
			{
			case CHECK_RAW_DEPTH:
				break;
			case CALIB_ROT_WIN:
				procRotCalibWin();
				break;
			case CALIB_ROT_WARP:
				procRotWarp();
				break;
			case CHECK_ROT_WARP:
				rotateDepth();
				if (doUnwarp)
					unwarpDepth();
				break;
			case CHECK_ROT_WARP_LOADED:
				rotateDepth();
				if (doUnwarp)
					unwarpDepth();
				break;
			case USE_ROT_WARP:
				rotateDepth();
				if (doUnwarp)
					unwarpDepth();
				break;
			default:
				break;
			}
		}
	}
}

//---------------------------------------------------------------
// when the kinect doesn't get any depth information on a pixels (because the closest target
// is farer than 4,5m away), the result is undefined and noise produced
// to prevent noise we take the last 3 frames and declare a pixel only valid if it
// appeared during the last 3 frames


GLint* KinectReproTools::transformDepth(unsigned int devNr, bool useHisto, float* transMatPtr,
		float* screenTransMat, float pointSize, float pointWeight, float farThres, float nearThres,
		int tRotate)
{
	// needs a non-normalized depth texture in the format GL_32F
	ptex.ppBuf->src->bind();
	ptex.ppBuf->src->clear();

	//ptex.fbo->bind();
	//ptex.fbo->clear();

	geoRotShdr->begin();
	geoRotShdr->setUniform1i("depthTex0", 0);
	geoRotShdr->setUniform1i("depthTex1", 1);
	geoRotShdr->setUniform1i("depthTex2", 2);
	geoRotShdr->setUniform2i("cellSize", ptex.cellSize.x, ptex.cellSize.y);
	geoRotShdr->setUniform2i("nrCells", ptex.nrCells.x, ptex.nrCells.y);
	geoRotShdr->setUniform2f("kinFovFact", ptex.kinFovFact.x, ptex.kinFovFact.y);
	geoRotShdr->setUniform2f("texSize", float(kin->getDepthWidth(0)), float(kin->getDepthHeight(0)));
	geoRotShdr->setUniformMatrix4fv("transMat", transMatPtr);
	geoRotShdr->setUniformMatrix4fv("m_pvm", screenTransMat);
	geoRotShdr->setUniform1f("pointSize", pointSize);
	geoRotShdr->setUniform1f("pointWeight", pointWeight);
	geoRotShdr->setUniform1f("farThres", farThres);
	geoRotShdr->setUniform1f("nearThres", nearThres);
	geoRotShdr->setUniform1i("rotate", tRotate);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(useHisto, devNr, 0));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(useHisto, devNr, -1));

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(useHisto, devNr, -2));

	ptex.trigVao->draw(GL_POINTS);

	geoRotShdr->end();

	//ptex.fbo->unbind();

	ptex.ppBuf->src->unbind();
	ptex.ppBuf->swap();

	//-----------------------

	/*
	//------------------------------------

	ptex.ppBuf->src->bind();
	ptex.ppBuf->src->clear();

	texShader->begin();
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ptex.fbo->getColorImg(0));

	rawQuad->draw();

	texShader->end();

	ptex.ppBuf->src->unbind();
	ptex.ppBuf->swap();

	//------------------------------------

*/
	outTexId[0] = ptex.ppBuf->src->getColorImg(0);
	outTexId[1] = ptex.ppBuf->src->getColorImg(1);

	return outTexId;
//	return ptex.ppBuf->src->getColorImg();
}

//---------------------------------------------------------------

GLint KinectReproTools::getDepthTransTexId(int ind)
{
	return outTexId[ind];
}

//---------------------------------------------------------------

void KinectReproTools::rotateDepth()
{
	// kinect depth image comes in upside down (gl)
	// shader has to flip it otherwise undist doesn´t work
	glm::mat4 rot = glm::rotate(glm::mat4(1.f),
			-(kpCalib.rotations.x + kpCalib.rotXOffs), glm::vec3(1.f, 0.f, 0.f))
			* glm::rotate(glm::mat4(1.f), kpCalib.rotations.y,
					glm::vec3(0.f, 1.f, 0.f))
			* glm::rotate(glm::mat4(1.f), kpCalib.rotations.z,
					glm::vec3(0.f, 0.f, 1.f));

	rotDepth->bind();

	undistRotShdr->begin();
	undistRotShdr->setIdentMatrix4fv("m_pvm");
	undistRotShdr->setUniform1f("depthWidth", kpCalib.depthImgSize.width);
	undistRotShdr->setUniform1f("depthHeight", kpCalib.depthImgSize.height);
	undistRotShdr->setUniform1f("deeperThres", kpCalib.threshZDeep);
	undistRotShdr->setUniform1f("nearerThres", kpCalib.threshZNear);
	undistRotShdr->setUniform1f("cropRight", kpCalib.kinCropRight);
	undistRotShdr->setUniform1f("cropLeft", kpCalib.kinCropLeft);
	undistRotShdr->setUniform1f("cropUp", kpCalib.kinCropUp);
	undistRotShdr->setUniform1f("cropBottom", kpCalib.kinCropBottom);

	undistRotShdr->setUniform2fv("kinFov", &kpCalib.kinFov[0]);
	undistRotShdr->setUniformMatrix4fv("invRot", &rot[0][0]);
	undistRotShdr->setUniformMatrix4fv("de_dist", &kpCalib.camBeamerDist[0][0]);

	glActiveTexture(GL_TEXTURE0);
	//        glBindTexture(GL_TEXTURE_2D, fastBlur->getResult());
	glBindTexture(GL_TEXTURE_2D, kinDepthTex->getId());
	rawQuad->draw();

	rotDepth->unbind();
}

//---------------------------------------------------------------

void KinectReproTools::unwarpDepth()
{
	glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, thresh8.data);

	//hier persp. unwrap!!
	//cv::flip(thresh8, thresh8, 0); // flip around x-axis
	cv::warpPerspective(thresh8, checkWarpDepth, kpCalib.camBeamerPerspTrans,
			kpCalib.depthImgSize);
	if (kpCalib.hFlip)
		cv::flip(checkWarpDepth, checkWarpDepth, 0); // flip around x-axis

	texPtr = (texPtr + 1) % nrFrameBuf;
	glBindTexture(GL_TEXTURE_2D, unwarpTex[texPtr]->getId());
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // First mipmap level
			0, 0,                       // x and y offset
			kin->getDepthWidth(kinDevId0), kin->getDepthHeight(kinDevId0),
			GL_RGB,
			GL_UNSIGNED_BYTE, checkWarpDepth.data);
}

//---------------------------------------------------------------

// estimate rotations only from a define window in the depth image
// define a grid in this window with n x n points
// get their realworld coordinates
// do 4 run-throughs: from left top      -> right bottom
//                    from right top     -> left bottom
//                    from left bottom   -> right top
//                    from right bottom  -> left top
// run across each row, take the outmost point in vertical and horizontal
// direction, build two vectors from them and calculate the normal between them
// save each result in a vector
// clean the vector, eliminate results than vary more than x percent from the rest
// calculate a median result
void KinectReproTools::procRotCalibWin()
{
	unsigned int ind = 0, bright = 0;
	short nrPointsPerSide = 8;
	float diff;
	std::vector<glm::vec3> normals;
	std::map<float, glm::vec3> sortedNormals;
	glm::vec3 v1, v2, medVec;

	saveMutex.lock();

	realWorldCoord.clear();

	// define a grid in this window with n x n points
	// get their realworld coordinates
	float xpos, ypos;
	for (short y = 0; y < nrPointsPerSide; y++)
	{
		ypos = static_cast<float>(y) / static_cast<float>(nrPointsPerSide - 1);
		for (short x = 0; x < nrPointsPerSide; x++)
		{
			xpos = static_cast<float>(x)
					/ static_cast<float>(nrPointsPerSide - 1);
			realWorldCoord.push_back(
					glm::vec3(
							((kpCalib.calibWinCorners[3].x
									+ xpos * kpCalib.calibWinSize.x) * 0.5f
									+ 0.5f) * float(kpCalib.depthImgSize.width),
							((kpCalib.calibWinCorners[3].y * -1.f
									+ ypos * kpCalib.calibWinSize.y) * 0.5f
									+ 0.5f)
									* float(kpCalib.depthImgSize.height), 0.f));

			getKinRealWorldCoord(realWorldCoord.back());

			if ((int) rwCoord.size() < (nrPointsPerSide * nrPointsPerSide))
				rwCoord.push_back(new Median<glm::vec3>(5.f));

			rwCoord[ind]->update(realWorldCoord.back());
			ind++;
			//std::cout << glm::to_string(realWorldCoord.back()) << std::endl;
		}
	}

	// do 4 run-throughs
	// from left top      -> right bottom
	for (short y = 0; y < nrPointsPerSide - 1; y++)
	{
		for (short x = 0; x < nrPointsPerSide - 1; x++)
		{
			v1 = glm::normalize(
					rwCoord[y * nrPointsPerSide + (nrPointsPerSide - 1)]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			v2 = glm::normalize(
					rwCoord[(nrPointsPerSide - 1) * nrPointsPerSide + x]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			normals.push_back(glm::cross(v1, v2));
			//std::cout << glm::to_string(normals.back()) << std::endl;
		}
	}

	// right top     -> left bottom
	for (short y = 0; y < nrPointsPerSide - 1; y++)
	{
		for (short x = nrPointsPerSide - 1; x > 0; x--)
		{
			v1 = glm::normalize(
					rwCoord[y * nrPointsPerSide]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			v2 = glm::normalize(
					rwCoord[(nrPointsPerSide - 1) * nrPointsPerSide + x]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			normals.push_back(glm::cross(v1, v2) * -1.f);
			//std::cout << glm::to_string(normals.back()) << std::endl;
		}
	}

	// left bottom   -> right top
	for (short y = nrPointsPerSide - 1; y > 0; y--)
	{
		for (short x = 0; x < nrPointsPerSide - 1; x++)
		{
			v1 = glm::normalize(
					rwCoord[y * nrPointsPerSide + (nrPointsPerSide - 1)]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			v2 = glm::normalize(
					rwCoord[x]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			normals.push_back(glm::cross(v1, v2) * -1.f);
			//std::cout << glm::to_string(normals.back()) << std::endl;
		}
	}

	// from right bottom  -> left top
	for (short y = nrPointsPerSide - 1; y > 0; y--)
	{
		for (short x = nrPointsPerSide - 1; x > 0; x--)
		{
			v1 = glm::normalize(
					rwCoord[y * nrPointsPerSide]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			v2 = glm::normalize(
					rwCoord[x]->get()
							- rwCoord[y * nrPointsPerSide + x]->get());
			normals.push_back(glm::cross(v1, v2));
			//std::cout << glm::to_string(normals.back()) << std::endl;
		}
	}

	// calculate medium
	medVec = glm::vec3(0.f);
	for (std::vector<glm::vec3>::iterator it = normals.begin();
			it != normals.end(); it++)
		medVec += (*it);

	medVec /= static_cast<float>(normals.size());
	//std::cout << "medVec: " << glm::to_string(medVec) << std::endl;

	// run again through all normals and caluculate the difference to the medium
	for (std::vector<glm::vec3>::iterator it = normals.begin();
			it != normals.end(); it++)
	{
		diff = glm::length(medVec - (*it));
		sortedNormals[diff] = (*it);
	}

	// take only part of the results to eliminate any outliers
	normals.clear();
	unsigned int maxInd =
			static_cast<int>(static_cast<float>(sortedNormals.size()) * 0.6f);
	ind = 0;
	for (std::map<float, glm::vec3>::iterator it = sortedNormals.begin();
			it != sortedNormals.end(); it++)
	{
		if (ind < maxInd)
		{
			//std::cout << (*it).first << ": " << glm::to_string( (*it).second ) << std::endl;
			normals.push_back((*it).second);
			ind++;
		}
		else
		{
			break;
		}
	}

	// calculate again the medium
	medVec = glm::vec3(0.f);
	for (std::vector<glm::vec3>::iterator it = normals.begin();
			it != normals.end(); it++)
		medVec += (*it);

	medVec /= static_cast<float>(normals.size());
	depthNormal->update(medVec);
	//std::cout << "medVec clean: " << glm::to_string(depthNormal->get()) << std::endl;

	glm::quat rot = RotationBetweenVectors(glm::normalize(depthNormal->get()),
			glm::vec3(0.f, 0.f, -1.f));
	kpCalib.rotations = glm::eulerAngles(rot);
	kpCalib.rotations.z = 0.f; // there can´t be any information about this... anyway value is around 0.001...
	std::cout << "kp.rotations: " << glm::to_string(kpCalib.rotations)
			<< std::endl;

	kpCalib.chessRotXY = glm::mat4(rot);

	// get distance of kinect and calibration chessboard
	midPoint = (rwCoord[(nrPointsPerSide * nrPointsPerSide) - 1]->get()
			+ rwCoord[0]->get()) * 0.5f;
	// std:: cout << "midPoint: " << glm::to_string(midPoint) << std::endl;

	kpCalib.distKinObj = (glm::rotate(glm::mat4(1.f), kpCalib.rotations.x,
			glm::vec3(1.f, 0.f, 0.f)) * glm::vec4(midPoint, 1.f)).z;
	std::cout << "distance Kinect Calib Obj: " << kpCalib.distKinObj
			<< std::endl;

	// ----- check z-depth threshold ---------

	kpCalib.threshZDeep = kpCalib.distKinObj + 1.f;
	kpCalib.threshZNear = 0.f;

	unsigned int maxLoop = 20;
	ind = 0;
	bright = 255;

	while (bright != 0 && ind < maxLoop)
	{
		kpCalib.threshZDeep -= 1.f;

		// apply derotation
		rotateDepth();

		// draw the thresholded result in a very small fbo
		rotDepthMaessure->bind();

		cropShdr->begin();
		cropShdr->setIdentMatrix4fv("m_pvm");
		cropShdr->setUniform2f("center", kpCalib.calibWinCenter.x * 0.5f,
				kpCalib.calibWinCenter.y * -0.5f);
		cropShdr->setUniform2f("size", kpCalib.calibWinSize.x * 0.5f,
				kpCalib.calibWinSize.y * 0.5f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());

		rawQuad->draw();

		rotDepthMaessure->unbind();

		// download the result
		glBindTexture(GL_TEXTURE_2D, rotDepthMaessure->getColorImg());
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, rotDepthM);

		// get the total brightness
		bright = 0;
		for (short i = 0; i < rotDepthMaessureSize * rotDepthMaessureSize; i++)
			bright += rotDepthM[i];

		ind++;
	}

	kpCalib.threshZDeep -= 1.f; // manual safety

	if (kpCalib.threshZDeep < lastKinObjDist)
		lastKinObjDist = kpCalib.threshZDeep;
	else
		kpCalib.threshZDeep = lastKinObjDist;

	std::cout << "kpCalib.threshZDeep: " << kpCalib.threshZDeep << std::endl;

	// init active range for touch projection applications
	kpCalib.threshZNear = kpCalib.threshZDeep - stdTouchDepth;
	std::cout << "kpCalib.threshZNear: " << kpCalib.threshZNear << std::endl;

	kpCalib.kinCropRight = kpCalib.calibWinCorners[1].x;
	kpCalib.kinCropLeft = kpCalib.calibWinCorners[0].x;
	kpCalib.kinCropUp = kpCalib.calibWinCorners[3].y;
	kpCalib.kinCropBottom = kpCalib.calibWinCorners[0].y;

	saveMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::procRotWarp()
{
	// get chessboard
	foundChessb = cv::findChessboardCorners(*procView, kpCalib.boardSize,
			pointbuf, cv::CALIB_CB_ADAPTIVE_THRESH |
			//cv::CALIB_CB_FAST_CHECK |
					cv::CALIB_CB_NORMALIZE_IMAGE);
	if (foundChessb)
	{
		cv::drawChessboardCorners(*procView, kpCalib.boardSize,
				cv::Mat(pointbuf), true);

		realWorldCoord.clear();
		int ind = 0;
		for (std::vector<cv::Point2f>::iterator it = pointbuf.begin();
				it != pointbuf.end(); ++it)
		{
			realWorldCoord.push_back(glm::vec3((*it).x, (*it).y, 0.f));
			getKinRealWorldCoord(realWorldCoord.back());

			if ((int) rwCoord.size() < (int) pointbuf.size())
				rwCoord.push_back(new Median<glm::vec3>(5.f));

			//std:: cout << "realWorldCoord: " << glm::to_string(rwCoord[ind]->get()) << std::endl;

			rwCoord[ind]->update(realWorldCoord.back());
			ind++;
		}

		midPoint = getChessMidPoint(realWorldCoord, kpCalib);
		std::cout << "chessboard MidPoint: " << glm::to_string(midPoint)
				<< std::endl;

		/*
		 if(kp.distBeamerObjCenter != 0.f)
		 kp.distBeamerObjCenter = (kp.distBeamerObjCenter * 10.f + (midPoint.z - kpCalib.camBeamerRealOffs.z)) / 11.f;
		 else
		 kp.distBeamerObjCenter = midPoint.z - kpCalib.camBeamerRealOffs.z;

		 cout << "distBeamerObjCenter: " << kp.distBeamerObjCenter << endl;
		 cout << "BeamerObjReal: " << std::sqrt(std::pow(midPoint.y + kpCalib.camBeamerRealOffs.y, 2.f)
		 + std::pow(kp.distBeamerObjCenter, 2.f)) << endl;
		 */

		// brechne die normale der wand / chessboard zur Kinect Ebene
		kpCalib.chessRotXY = calcChessRotMed(rwCoord, kpCalib);
		kpCalib.camBeamerPerspTrans = getPerspTrans(pointbuf, glChessBoardWidth,
				glChessBoardHeight);
		kpCalib.camBeamerDist = cvMat33ToGlm(kpCalib.camBeamerPerspTrans);

		// get distance of kinect and calibration chessboard
		midPointWorld.z = (glm::rotate(glm::mat4(1.f), kpCalib.rotations.x,
				glm::vec3(1.f, 0.f, 0.f)) * glm::vec4(midPoint, 1.f)).z;
		midPointWorld.y = (glm::rotate(glm::mat4(1.f), kpCalib.rotations.y,
				glm::vec3(0.f, 1.f, 0.f)) * glm::vec4(midPoint, 1.f)).y;
		midPointWorld.x = (glm::rotate(glm::mat4(1.f), kpCalib.rotations.z,
				glm::vec3(0.f, 0.f, 1.f)) * glm::vec4(midPoint, 1.f)).x;
		std::cout << "chessboard MidPoint: " << glm::to_string(midPoint)
				<< std::endl;

		kpCalib.distKinObj = std::sqrt(
				midPointWorld.x * midPointWorld.x
						+ midPointWorld.y * midPointWorld.y
						+ midPointWorld.z * midPointWorld.z);
		std::cout << "distance Kinect Calib Obj: " << kpCalib.distKinObj
				<< std::endl;

		std::cout << std::endl;
	}
}

//---------------------------------------------------------

// rotations resulting from normals vary about 1 degree. this is caused by the kinect...
glm::mat4 KinectReproTools::calcChessRotMed(
		std::vector<Median<glm::vec3>*>& _realWorldCoord,
		KinectReproCalibData& kp)
{
	glm::mat4 outMat = glm::mat4(1.f);
	vector<glm::vec3> normals;

	// calculate rotation, by calculating normals for all corners
	// through the conversion in realworld coordinates the perspective
	// distortion is automatically corrected, so the result is corrected
	for (short y = 1; y < kp.boardSize.height; y++)
	{
		for (short x = 0; x < kp.boardSize.width - 1; x++)
		{
			glm::vec3 act = _realWorldCoord[y * kp.boardSize.width + x]->get();
			glm::vec3 upper =
					_realWorldCoord[(y - 1) * kp.boardSize.width + x]->get();
			glm::vec3 right =
					_realWorldCoord[y * kp.boardSize.width + x + 1]->get();

			glm::vec3 upVec = upper - act;
			glm::vec3 rightVec = right - act;

			normals.push_back(
					glm::normalize(
							glm::cross(glm::normalize(upVec),
									glm::normalize(rightVec))));
		}
	}

	// calculate medium normal
	glm::vec3 outNorm = glm::vec3(0.f, 0.f, 0.f);
	int nrValidNorm = 0;
	for (std::vector<glm::vec3>::iterator it = normals.begin();
			it != normals.end(); ++it)
		if (!std::isnan((*it).x) && !std::isnan((*it).y)
				&& !std::isnan((*it).z))
		{
			outNorm += (*it);
			nrValidNorm++;
		}

	if (nrValidNorm > 0)
	{
		outNorm /= float(nrValidNorm);
		outNorm = glm::normalize(outNorm);

		cbNormal->update(outNorm);
		kp.chessNormal = cbNormal->get();

		std::cout << "chessNormal: " << glm::to_string(kp.chessNormal)
				<< std::endl;

		// quaternion
		glm::quat rot = RotationBetweenVectors(kp.chessNormal,
				glm::vec3(0.f, 0.f, -1.f));
		outMat = glm::mat4(rot);
		kp.rotations = glm::eulerAngles(rot);
		std::cout << "kp.rotations: " << glm::to_string(kp.rotations)
				<< std::endl;
	}

	return outMat;
}

//---------------------------------------------------------

glm::vec3 KinectReproTools::getChessMidPoint(
		std::vector<glm::vec3>& _realWorldCoord, KinectReproCalibData& kp)
{
	glm::vec3 midPoint = glm::vec3(0.f);

	// calculate midpoints in the vertical axis
	// the outer most corners and iterate to the center
	vector<glm::vec3> hMidsEnd;

	for (short x = 0; x < kp.boardSize.width; x++)
	{
		vector<glm::vec3> hMids;
		for (short h = 0; h < kp.boardSize.height / 2; h++)
		{
			glm::vec3 midP =
					_realWorldCoord[x + (h * kp.boardSize.width)]
							+ _realWorldCoord[x
									+ (kp.boardSize.height - h - 1)
											* kp.boardSize.width];
			hMids.push_back(midP * 0.5f);
		}

		// get mediums
		glm::vec3 medMidPH = glm::vec3(0.f, 0.f, 0.f);
		for (short h = 0; h < kp.boardSize.height / 2; h++)
			medMidPH += hMids[h];

		hMidsEnd.push_back(
				medMidPH / static_cast<float>(kp.boardSize.height / 2));
	}

	vector<glm::vec3> vMids;
	int hMidsEndSizeHalf = static_cast<int>(hMidsEnd.size()) / 2;

	// iterate over all vMids and calc mediums for each pair
	for (int x = 0; x < hMidsEndSizeHalf; x++)
	{
		glm::vec3 midP = hMidsEnd[x]
				+ hMidsEnd[static_cast<int>(hMidsEnd.size()) - 1 - x];
		vMids.push_back(midP * 0.5f);
	}

	midPoint = glm::vec3(0.f, 0.f, 0.f);
	for (short i = 0; i < static_cast<short>(vMids.size()); i++)
		midPoint += vMids[i];

	if (static_cast<int>(vMids.size()) > 0)
		midPoint /= static_cast<float>(vMids.size());

	return midPoint;
}

//---------------------------------------------------------

cv::Mat KinectReproTools::getPerspTrans(std::vector<cv::Point2f>& pointbuf,
		float _chessWidth, float _chessHeight)
{
	// get perspective transformation in pixels
	// upper left corner is origin

	cv::Mat out;
	vector<cv::Point2f> inPoint;
	vector<cv::Point2f> dstPoint;

	// we don´t take the far out edges of the chessboard
	// but the next inner edges.
	// convert opengl normalized coordinates (-1|1) to pixels (0 - kinWidth|0 - kinHeight)
	float chessW = _chessWidth * static_cast<float>(kpCalib.boardSize.width - 1)
			/ static_cast<float>(kpCalib.boardSize.width + 1) * 0.5f
			* kpCalib.imgSize.width;
	float chessH = _chessHeight
			* static_cast<float>(kpCalib.boardSize.height - 1)
			/ static_cast<float>(kpCalib.boardSize.height + 1) * 0.5f
			* kpCalib.imgSize.height;

	float chessOffsX = (2.f - _chessWidth) * 0.25f * kpCalib.imgSize.width
			+ chessW / static_cast<float>(kpCalib.boardSize.width - 1);
	float chessOffsY = (2.f - _chessHeight) * 0.25f * kin->getColorHeight()
			+ chessH / static_cast<float>(kpCalib.boardSize.height - 1);

	// lower left (in point upper, weil kommt auf d. kopf rein)
	for (int y = 0; y < 2; y++)
		for (int x = 0; x < 2; x++)
			dstPoint.push_back(
					cv::Point2f(float(x) * chessW + chessOffsX,
							float(y) * chessH + chessOffsY));

	inPoint.push_back(pointbuf[0]);
	inPoint.push_back(pointbuf[kpCalib.boardSize.width - 1]);
	inPoint.push_back(
			pointbuf[(kpCalib.boardSize.height - 1) * kpCalib.boardSize.width]);
	inPoint.push_back(
			pointbuf[(kpCalib.boardSize.height - 1) * kpCalib.boardSize.width
					+ kpCalib.boardSize.width - 1]);

	out = cv::getPerspectiveTransform(inPoint, dstPoint);
	/*
	 glm::mat4 camBeamerDist = cvMat33ToGlm(out);

	 glm::vec2 checkPoint;
	 for (int i=0;i<4;i++)
	 {
	 std::cout << "raw: " << inPoint[i] << std::endl;

	 checkPoint = glm::vec2(inPoint[i].x, inPoint[i].y);
	 perspUnwarp(checkPoint, camBeamerDist);

	 std::cout << "warped: " << glm::to_string(checkPoint) << std::endl;
	 }

	 std::cout << std::endl;
	 */

	return out;
}

//---------------------------------------------------------

void KinectReproTools::getKinRealWorldCoord(glm::vec3& inCoord)
{
	//        const openni::DepthPixel* blurPix;
	double depthScale = 1.0;
	double depthVal = 0;
	float resX = static_cast<float>(kin->getDepthWidth());
	float resY = static_cast<float>(kin->getDepthHeight());

	// in case depth and color stream have different sizes
	inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth())
			* static_cast<float>(kin->getDepthWidth());
	inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight())
			* static_cast<float>(kin->getDepthHeight());

	//const openni::DepthPixel* blurPix = (const openni::DepthPixel*)kin->getDepthFrame(kinDevId0)->getData();
	const openni::DepthPixel* blurPix =
			(const openni::DepthPixel*) fastBlur->getDataR16();
	int rowSize = kin->getDepthFrame(kinDevId0)->getStrideInBytes()
			/ sizeof(openni::DepthPixel);

	//pDepthPix += rowSize * static_cast<int>(inCoord.y);
	//pDepthPix += static_cast<int>(inCoord.x);

	blurPix += rowSize * static_cast<int>(inCoord.y);
	blurPix += static_cast<int>(inCoord.x);

	depthVal = static_cast<double>(*blurPix);
	if (depthVal < 20.0)
		depthVal = lastBlurPix;
	lastBlurPix = depthVal;

	// asus xtion tends to measure lower depth with increasing distance
	// experimental correction
	double multDiff = depthVal - 950.0;
	double upperCorr = std::fmax(multDiff, 0) * 0.02;
	double scaledDepth = depthVal + (multDiff * 0.0102) + upperCorr;

	glm::vec2 fov = glm::vec2(kpCalib.kinFovX, kpCalib.kinFovY);

	double normalizedX = inCoord.x / resX - .5f;
	double normalizedY = .5f - inCoord.y / resY;

	double xzFactor = std::tan(fov.x * 0.5f) * 2.f; // stimmt noch nicht... wieso?
	double yzFactor = std::tan(fov.y * 0.5f) * 2.f;  // stimmt!!!

	inCoord.x = static_cast<float>(normalizedX * scaledDepth * xzFactor);
	inCoord.y = static_cast<float>(normalizedY * scaledDepth * yzFactor);
	inCoord.z = static_cast<float>(scaledDepth);
}

//---------------------------------------------------------------

void KinectReproTools::drawCalib()
{
	drawMutex.lock();

	glDisable(GL_DEPTH_TEST);

	colShader->begin();
	colShader->setIdentMatrix4fv("m_pvm");
	whiteQuad->draw();

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	chessBoard->draw();

	drawMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::drawCheckColor()
{
	drawMutex.lock();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinTex->getId());
	quad->draw();

	drawMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::drawCheckDepth()
{
	drawMutex.lock();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);

	if (actMode == CHECK_RAW_DEPTH)
	{
		glBindTexture(GL_TEXTURE_2D, kinHistoDepth->getId());
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, fastBlur->getResult());
	}

	quad->draw();

	drawMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::drawCalibWin()
{
	/*
	 if(contrWin.open)
	 {
	 drawMutex.lock();

	 // switch glfw contexts
	 win = glfwGetCurrentContext();
	 contrWin.makeCurrent();

	 glDisable(GL_DEPTH_TEST);
	 glEnable(GL_BLEND);

	 texShader->begin();
	 texShader->setIdentMatrix4fv("m_pvm");
	 texShader->setUniform1i("tex", 0);
	 kinHistoDepth->bind();
	 quad->draw();

	 colShader->begin();
	 colShader->setIdentMatrix4fv("m_pvm");
	 calibWin->draw(GL_LINE_STRIP);

	 for (int i=0;i<4;i++)
	 {
	 cornerQuadMat = glm::translate(glm::mat4(1.f), glm::vec3(kpCalib.calibWinCorners[i], 0.f))
	 * glm::scale(glm::mat4(1.f), glm::vec3(cornerQuadSize));
	 colShader->setUniformMatrix4fv("m_pvm", &cornerQuadMat[0][0]);
	 cornerQuad->draw();
	 }

	 contrWin.swap();
	 glfwPollEvents();

	 glfwMakeContextCurrent(win);

	 drawMutex.unlock();
	 }
	 */
}

//---------------------------------------------------------------

void KinectReproTools::drawFoundChess()
{
	drawMutex.lock();

	glDisable(GL_DEPTH_TEST);

	kinTex->bind();
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // First mipmap level
			0, 0,                       // x and y offset
			kin->getColorWidth(kinDevId0), kin->getColorHeight(kinDevId0),
			GL_RGB,
			GL_UNSIGNED_BYTE, procView->data);

	printf("draw found chess \n");
	texShader->begin();
	texShader->setIdentMatrix4fv("m_pvm");
	texShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	quad->draw();

	drawMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::drawCheckRotWarp()
{
	drawMutex.lock();

	if (inited)
	{
		printf("draw checkRotWarp \n");

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

		glActiveTexture(GL_TEXTURE0);
		if (doUnwarp)
			glBindTexture(GL_TEXTURE_2D, unwarpTex[texPtr]->getId());
		else
			glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());
		quad->draw();
	}

	drawMutex.unlock();
}

//---------------------------------------------------------------

void KinectReproTools::setMode(mode _mode)
{
	if (actMode != CALIB_ROT_WIN && _mode == CALIB_ROT_WIN)
	{
		/*
		 drawMutex.lock();

		 win = glfwGetCurrentContext();
		 lastKinObjDist = 10000.f;
		 contrWin.init(640, 480, 60, false, true, 0, 0, true, win);
		 glfwMakeContextCurrent(win);

		 drawMutex.unlock();
		 */

	}
	else
	{
		/*
		 if(contrWin.open)
		 {
		 drawMutex.lock();

		 win = glfwGetCurrentContext();
		 contrWin.makeCurrent();
		 contrWin.close();
		 glfwMakeContextCurrent(win);

		 drawMutex.unlock();
		 }
		 */
	}

	actMode = _mode;
}

//---------------------------------------------------------------

void KinectReproTools::setThreshZDeep(float _val)
{
	kpCalib.threshZDeep = _val;
}

//---------------------------------------------------------------

void KinectReproTools::setThreshZNear(float _val)
{
	kpCalib.threshZNear = _val;
}

//---------------------------------------------------------------

void KinectReproTools::setRotXOffs(float _val)
{
	kpCalib.rotXOffs = _val;
}

//---------------------------------------------------------

void KinectReproTools::setHFlip(bool _val)
{
	kpCalib.hFlip = _val;
}

//---------------------------------------------------------
// input must be normalized screen coordinates (x:0|1 y:0|1) origin left lower corner
void KinectReproTools::setCalibWinCorner(float _x, float _y)
{
	glm::vec2 cursorPos = glm::vec2(_x * 2.f - 1.f, (1.f - _y) * 2.f - 1.f);

	// find the nearest corner and update it´s position
	int ind = -1;
	float dist = 100000.f;
	float newDist;

	for (short i = 0; i < 4; i++)
	{
		newDist = std::fabs(
				glm::length(kpCalib.calibWinCorners[i] - cursorPos));
		if (newDist < dist)
		{
			dist = newDist;
			ind = i;
		}
	}

	if (ind != -1)
	{
		kpCalib.calibWinCorners[ind] = cursorPos;

		GLfloat* pPos = (GLfloat*) calibWin->getMapBuffer(POSITION);
		for (int i = 0; i < 5; i++)
		{
			pPos[i * 3] = kpCalib.calibWinCorners[i % 4].x;
			pPos[i * 3 + 1] = kpCalib.calibWinCorners[i % 4].y;
			pPos[i * 3 + 2] = 0.f;
		}
		calibWin->unMapBuffer();
	}
}

//---------------------------------------------------------

void KinectReproTools::setCalibDragInit(float _x, float _y)
{
	calibWinDragOrigin = glm::vec2(_x * 2.f - 1.f, (1.f - _y) * 2.f - 1.f);
	calibWinSizeInit = kpCalib.calibWinSize;
	calibWinCenterInit = kpCalib.calibWinCenter;

	// find the nearest corner and update it´s position
	int ind = -1;
	float dist = 100000.f;
	float newDist;

	for (short i = 0; i < 4; i++)
	{
		newDist = std::fabs(
				glm::length(kpCalib.calibWinCorners[i] - calibWinDragOrigin));
		if (newDist < dist)
		{
			dist = newDist;
			ind = i;
		}
	}

	calibWinDragOriginInd = ind;

	if (dist < (cornerQuadSize + 0.01f))
		calibWinScaling = true;
	else
		calibWinScaling = false;
}

//---------------------------------------------------------
// draging or scaling

void KinectReproTools::dragCalibWin(float _x, float _y)
{
	glm::vec2 cursorPos = glm::vec2(_x * 2.f - 1.f, (1.f - _y) * 2.f - 1.f);
	glm::vec2 newSize;

	if (!calibWinScaling)
	{
		kpCalib.calibWinCenter = calibWinCenterInit + cursorPos
				- calibWinDragOrigin;

	}
	else
	{
		kpCalib.calibWinCorners[calibWinDragOriginInd] = cursorPos;

		switch (calibWinDragOriginInd)
		{
		// left lower
		case 0:
			kpCalib.calibWinCorners[3].x = cursorPos.x;
			kpCalib.calibWinCorners[1].y = cursorPos.y;
			break;
			// right lower
		case 1:
			kpCalib.calibWinCorners[2].x = cursorPos.x;
			kpCalib.calibWinCorners[0].y = cursorPos.y;
			break;
			// right uppper
		case 2:
			kpCalib.calibWinCorners[1].x = cursorPos.x;
			kpCalib.calibWinCorners[3].y = cursorPos.y;
			break;
			// left upper
		case 3:
			kpCalib.calibWinCorners[0].x = cursorPos.x;
			kpCalib.calibWinCorners[2].y = cursorPos.y;
			break;
		}

		kpCalib.calibWinSize.x = std::fabs(
				kpCalib.calibWinCorners[1].x - kpCalib.calibWinCorners[0].x);
		kpCalib.calibWinSize.y = std::fabs(
				kpCalib.calibWinCorners[3].y - kpCalib.calibWinCorners[0].y);

		kpCalib.calibWinCenter.x = kpCalib.calibWinCorners[0].x
				+ kpCalib.calibWinSize.x * 0.5f;
		kpCalib.calibWinCenter.y = kpCalib.calibWinCorners[0].y
				+ kpCalib.calibWinSize.y * 0.5f;

		/*
		 // proportional scale
		 calibWinSize = calibWinSizeInit * glm::vec2(std::fabs(calibWinCenter.x - cursorPos.x)
		 / std::fabs(calibWinCenter.x - calibWinDragOrigin.x),
		 std::fabs(calibWinCenter.y - cursorPos.y)
		 / std::fabs(calibWinCenter.y - calibWinDragOrigin.y));
		 */
	}

	updateCalibWin();
}

//---------------------------------------------------------

void KinectReproTools::noUnwarp()
{
	doUnwarp = false;
}

//---------------------------------------------------------

GLuint KinectReproTools::getRotDepth()
{
	GLuint out = 0;
	if (inited)
		out = rotDepth->getColorImg();
	return out;
}

//---------------------------------------------------------

GLuint KinectReproTools::getUnwarpTex()
{
	return unwarpTex[texPtr]->getId();
}

//---------------------------------------------------------

GLuint KinectReproTools::getKinColTex()
{
	return kinTex->getId();
}

//---------------------------------------------------------

int KinectReproTools::getFrameNr()
{
	return depthFrameNr;
}

//---------------------------------------------------------

KinectReproCalibData* KinectReproTools::getCalibData()
{
	return &kpCalib;
}

//---------------------------------------------------------

glm::mat4 KinectReproTools::cvMatToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j = 0; j < 4; j++)
		for (short i = 0; i < 4; i++)
			out[i][j] = _mat.at<float>(j * 4 + i);

	return out;
}

//---------------------------------------------------------

glm::mat4 KinectReproTools::cvMat33ToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j = 0; j < 3; j++)
		for (short i = 0; i < 3; i++)
			out[i][j] = _mat.at<double>(j * 3 + i);

	return out;
}

//---------------------------------------------------------

void KinectReproTools::perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr)
{
	const double eps = FLT_EPSILON;

	float x = inPoint.x, y = inPoint.y;
	double w = x * perspMatr[0][2] + y * perspMatr[1][2] + perspMatr[2][2];

	if (std::fabs(w) > eps)
	{
		w = 1.f / w;
		inPoint.x = (float) ((x * perspMatr[0][0] + y * perspMatr[1][0]
				+ perspMatr[2][0]) * w);
		inPoint.y = (float) ((x * perspMatr[0][1] + y * perspMatr[1][1]
				+ perspMatr[2][1]) * w);
	}
	else
		inPoint.x = inPoint.y = 0.f;
}

//---------------------------------------------------------------

void KinectReproTools::loadCalib(std::string _filename)
{
	std::cout << "KinectReproTools::loadCalib: " << _filename << std::endl;

	cv::FileStorage fs(_filename, cv::FileStorage::READ);

	if (fs.isOpened())
	{
		if (!fs["invertMatr"].empty())
			fs["invertMatr"] >> kpCalib.invertMatr;
		if (!fs["image_width"].empty())
			fs["image_width"] >> kpCalib.imgSize.width;
		if (!fs["image_height"].empty())
			fs["image_height"] >> kpCalib.imgSize.height;
		if (!fs["depth_image_width"].empty())
			fs["depth_image_width"] >> kpCalib.depthImgSize.width;
		if (!fs["depth_image_height"].empty())
			fs["depth_image_height"] >> kpCalib.depthImgSize.height;

		if (!fs["board_width"].empty())
			fs["board_width"] >> kpCalib.boardSize.width;
		if (!fs["board_height"].empty())
			fs["board_height"] >> kpCalib.boardSize.height;

		if (!fs["nrSamples"].empty())
			fs["nrSamples"] >> kpCalib.nrSamples;
		if (!fs["beamerAspectRatio"].empty())
			fs["beamerAspectRatio"] >> kpCalib.beamerAspectRatio;
		if (!fs["beamerFovX"].empty())
			fs["beamerFovX"] >> kpCalib.beamerFovX;
		if (!fs["beamerFovY"].empty())
			fs["beamerFovY"] >> kpCalib.beamerFovY;
		if (!fs["beamerLookAngle"].empty())
			fs["beamerLookAngle"] >> kpCalib.beamerLookAngle;
		if (!fs["beamerLowEdgeAngle"].empty())
			fs["beamerLowEdgeAngle"] >> kpCalib.beamerLowEdgeAngle;
		if (!fs["beamerThrowRatio"].empty())
			fs["beamerThrowRatio"] >> kpCalib.beamerThrowRatio;
		if (!fs["beamerWallDist"].empty())
			fs["beamerWallDist"] >> kpCalib.beamerWallDist;
		if (!fs["distKinObj"].empty())
			fs["distKinObj"] >> kpCalib.distKinObj;
		if (!fs["kinAspectRatio"].empty())
			fs["kinAspectRatio"] >> kpCalib.kinAspectRatio;

		if (!fs["kinFovX"].empty())
			fs["kinFovX"] >> kpCalib.kinFovX;
		if (!fs["kinFovY"].empty())
			fs["kinFovY"] >> kpCalib.kinFovY;
		if (!fs["threshZDeep"].empty())
			fs["threshZDeep"] >> kpCalib.threshZDeep;
		if (!fs["threshZNear"].empty())
			fs["threshZNear"] >> kpCalib.threshZNear;
		if (!fs["kinCropLeft"].empty())
			fs["kinCropLeft"] >> kpCalib.kinCropLeft;
		if (!fs["kinCropRight"].empty())
			fs["kinCropRight"] >> kpCalib.kinCropRight;
		if (!fs["kinCropUp"].empty())
			fs["kinCropUp"] >> kpCalib.kinCropUp;
		if (!fs["kinCropBottom"].empty())
			fs["kinCropBottom"] >> kpCalib.kinCropBottom;

		cv::Mat glmVec2 = cv::Mat(2, 1, CV_32F);
		if (!fs["kinFov"].empty())
		{
			fs["kinFov"] >> glmVec2;
			kpCalib.kinFov = glm::vec2(glmVec2.at<float>(0),
					glmVec2.at<float>(1));
		}

		if (!fs["calibWinSize"].empty())
		{
			fs["calibWinSize"] >> glmVec2;
			kpCalib.calibWinSize = glm::vec2(glmVec2.at<float>(0),
					glmVec2.at<float>(1));
		}

		if (!fs["calibWinCenter"].empty())
		{
			fs["calibWinCenter"] >> glmVec2;
			kpCalib.calibWinCenter = glm::vec2(glmVec2.at<float>(0),
					glmVec2.at<float>(1));
		}

		if (!fs["camBeamerPerspTrans"].empty())
			fs["camBeamerPerspTrans"] >> kpCalib.camBeamerPerspTrans;
		kpCalib.camBeamerDist = cvMat33ToGlm(kpCalib.camBeamerPerspTrans);
		if (!fs["camBeamerInvPerspTrans"].empty())
			fs["camBeamerInvPerspTrans"] >> kpCalib.camBeamerInvPerspTrans;

		cv::Mat rotations, camBeamerRealOffs;
		if (!fs["camBeamerRealOffs"].empty())
		{
			fs["camBeamerRealOffs"] >> camBeamerRealOffs;
			kpCalib.camBeamerRealOffs = glm::vec3(
					camBeamerRealOffs.at<float>(0),
					camBeamerRealOffs.at<float>(1),
					camBeamerRealOffs.at<float>(2));
		}

		if (!fs["rotations"].empty())
		{
			fs["rotations"] >> rotations;
			kpCalib.rotations = glm::vec3(rotations.at<float>(0),
					rotations.at<float>(1), rotations.at<float>(2));
		}

		kpCalib.rotXOffs = 0.f;

		cv::Mat calibCorners = cv::Mat(4, 2, CV_32F);
		if (!fs["calibWinCorners"].empty())
		{
			fs["calibWinCorners"] >> calibCorners;
			for (short i = 0; i < 4; i++)
			{
				kpCalib.calibWinCorners[i].x = calibCorners.at<float>(i, 0);
				kpCalib.calibWinCorners[i].y = calibCorners.at<float>(i, 1);
			}
			updateCalibWin();
		}

		std::string beamMod;
		if (!fs["beamerModel"].empty())
			fs["beamerModel"] >> beamMod;

		if (std::strcmp(beamMod.c_str(), "ZOOM") == 0)
			kpCalib.beamModel = KinectReproCalibData::ZOOM;

		if (std::strcmp(beamMod.c_str(), "WIDE") == 0)
			kpCalib.beamModel = KinectReproCalibData::WIDE;

		if (std::strcmp(beamMod.c_str(), "ULTRA_WIDE") == 0)
			kpCalib.beamModel = KinectReproCalibData::ULTRA_WIDE;

	}
	else
	{
		printf("couldn´t open file\n");
	}
}

//---------------------------------------------------------

void KinectReproTools::saveCalib(std::string _filename)
{
	saveMutex.lock();

	printf("saving camera to beamer calib \n");

	cv::FileStorage fs(_filename, cv::FileStorage::WRITE);

	fs << "invertMatr" << kpCalib.invertMatr;
	fs << "image_width" << kpCalib.imgSize.width;
	fs << "image_height" << kpCalib.imgSize.height;
	fs << "depth_image_width" << kpCalib.depthImgSize.width;
	fs << "depth_image_height" << kpCalib.depthImgSize.height;
	fs << "board_width" << kpCalib.boardSize.width;
	fs << "board_height" << kpCalib.boardSize.height;
	fs << "nrSamples" << kpCalib.nrSamples;
	fs << "beamerAspectRatio" << kpCalib.beamerAspectRatio;
	fs << "beamerFovX" << kpCalib.beamerFovX;
	fs << "beamerFovY" << kpCalib.beamerFovY;

	fs << "beamerLookAngle" << kpCalib.beamerLookAngle;
	fs << "beamerLowEdgeAngle" << kpCalib.beamerLowEdgeAngle;
	fs << "beamerThrowRatio" << kpCalib.beamerThrowRatio;
	fs << "beamerWallDist" << kpCalib.beamerWallDist;
	fs << "distKinObj" << kpCalib.distKinObj;

	fs << "kinAspectRatio" << kpCalib.kinAspectRatio;
	fs << "kinFovX" << kpCalib.kinFovX;
	fs << "kinFovY" << kpCalib.kinFovY;

	fs << "threshZDeep" << kpCalib.threshZDeep;
	fs << "threshZNear" << kpCalib.threshZNear;
	fs << "kinCropLeft" << kpCalib.kinCropLeft;
	fs << "kinCropRight" << kpCalib.kinCropRight;
	fs << "kinCropUp" << kpCalib.kinCropUp;
	fs << "kinCropBottom" << kpCalib.kinCropBottom;

	fs << "camBeamerPerspTrans" << kpCalib.camBeamerPerspTrans;
	fs << "camBeamerInvPerspTrans" << kpCalib.camBeamerPerspTrans.inv();

	cv::Mat glmVec2 = cv::Mat(2, 1, CV_32F);
	glmVec2.at<float>(0) = kpCalib.kinFov.x;
	glmVec2.at<float>(1) = kpCalib.kinFov.y;
	fs << "kinFov" << glmVec2;

	glmVec2.at<float>(0) = kpCalib.calibWinSize.x;
	glmVec2.at<float>(1) = kpCalib.calibWinSize.y;
	fs << "calibWinSize" << glmVec2;

	glmVec2.at<float>(0) = kpCalib.calibWinCenter.x;
	glmVec2.at<float>(1) = kpCalib.calibWinCenter.y;
	fs << "calibWinCenter" << glmVec2;

	cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
	glmVec.at<float>(0) = kpCalib.chessNormal.x;
	glmVec.at<float>(1) = kpCalib.chessNormal.y;
	glmVec.at<float>(2) = kpCalib.chessNormal.z;
	fs << "camBeamerNormal" << glmVec;

	glmVec.at<float>(0) = kpCalib.rotations.x + kpCalib.rotXOffs;
	glmVec.at<float>(1) = kpCalib.rotations.y;
	glmVec.at<float>(2) = kpCalib.rotations.z;
	fs << "rotations" << glmVec;

	glmVec.at<float>(0) = kpCalib.camBeamerRealOffs.x;
	glmVec.at<float>(1) = kpCalib.camBeamerRealOffs.y;
	glmVec.at<float>(2) = kpCalib.camBeamerRealOffs.z;
	fs << "camBeamerRealOffs" << glmVec;

	cv::Mat calibCorners = cv::Mat(4, 2, CV_32F);
	for (short i = 0; i < 4; i++)
	{
		calibCorners.at<float>(i, 0) = kpCalib.calibWinCorners[i].x;
		calibCorners.at<float>(i, 1) = kpCalib.calibWinCorners[i].y;
	}
	fs << "calibWinCorners" << calibCorners;

	switch (kpCalib.beamModel)
	{
	case KinectReproCalibData::ZOOM:
		fs << "beamerModel" << "ZOOM";
		break;
	case KinectReproCalibData::WIDE:
		fs << "beamerModel" << "WIDE";
		break;
	case KinectReproCalibData::ULTRA_WIDE:
		fs << "beamerModel" << "ULTRA_WIDE";
		break;
	}

	saveMutex.unlock();
}

//---------------------------------------------------------

void KinectReproTools::updateCalibWin()
{
	kpCalib.calibWinCorners[0] = kpCalib.calibWinCenter
			+ glm::vec2(-0.5f, -0.5f) * kpCalib.calibWinSize; // left lower
	kpCalib.calibWinCorners[1] = kpCalib.calibWinCenter
			+ glm::vec2(0.5f, -0.5f) * kpCalib.calibWinSize; // right lower
	kpCalib.calibWinCorners[2] = kpCalib.calibWinCenter
			+ glm::vec2(0.5f, 0.5f) * kpCalib.calibWinSize; // right uppper
	kpCalib.calibWinCorners[3] = kpCalib.calibWinCenter
			+ glm::vec2(-0.5f, 0.5f) * kpCalib.calibWinSize; // left uppper

	GLfloat* pPos = (GLfloat*) calibWin->getMapBuffer(POSITION);
	for (short i = 0; i < 5; i++)
	{
		pPos[i * 3] = kpCalib.calibWinCorners[i % 4].x;
		pPos[i * 3 + 1] = kpCalib.calibWinCorners[i % 4].y;
		pPos[i * 3 + 2] = 0.f;
	}
	calibWin->unMapBuffer();
}

//---------------------------------------------------------

void KinectReproTools::setupCropShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(
		layout( location = 0 ) in vec4 position;
		layout( location = 2 ) in vec2 texCoord;
		uniform mat4 m_pvm;
		out vec2 tex_coord;

		void main() {
			tex_coord = texCoord;
			gl_Position = m_pvm * position;
		});

	vert = "// KinectReproTools crop shader vertex\n" + shdr_Header + vert;

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		uniform sampler2D tex;
		uniform vec2 center;
		uniform vec2 size;
		in vec2 tex_coord;

		void main(){
			color = texture(tex, tex_coord * size + vec2(1.0 - size.x, 1.0 - size.y) * 0.5 + center);
		});

	frag = "// KinectReproTools corp shader\n" + shdr_Header + frag;

	cropShdr = shCol->addCheckShaderText("kinectReproCrop", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------

void KinectReproTools::setupUndistShdr()
{
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert =
			STRINGIFY(
					layout( location = 0 ) in vec4 position; layout( location = 1 ) in vec4 normal; layout( location = 2 ) in vec2 texCoord; layout( location = 3 ) in vec4 color; uniform mat4 m_pvm; out vec2 tex_coord; out vec4 col;

					void main() { col = color; tex_coord = texCoord; gl_Position = m_pvm * position; });

	vert = "// KinectReproTools Undistortion + Rotation + Thresholding\n"
			+ shdr_Header + vert;

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;
		uniform sampler2D tex;
		uniform float deeperThres;
		uniform float nearerThres;
		uniform float depthWidth;
		uniform float depthHeight;
		uniform float cropRight;
		uniform float cropLeft;
		uniform float cropUp;
		uniform float cropBottom;
		uniform vec2 kinFov;
		uniform mat4 invRot;
		uniform mat4 de_dist;
		in vec2 tex_coord;
		in vec4 col;
		float xzFactor;
		float yzFactor;
		float multDiff;
		float upperCorr;
		float scaledDepth;
		float eps=1.19209290e-07;
		float deep;

		vec3 getKinRealWorldCoord(vec3 inCoord) {
			// asus xtion tends to measure lower depth with increasing distance
			// experimental correction
			multDiff = inCoord.z - 950.0;
			upperCorr = max(multDiff, 0) * 0.02;
			scaledDepth = inCoord.z + (multDiff * 0.0102) + upperCorr;

			xzFactor = tan(kinFov.x * 0.5) * 2.0;
			yzFactor = tan(kinFov.y * 0.5) * 2.0;

			return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
						inCoord.y * 0.5 * scaledDepth * yzFactor,
						scaledDepth);
		}

		// geht noch nicht...
		vec2 getUnwarpTexCoord(vec2 in_tex) {
			float x; float y; float w; vec2 outV;
			x = (in_tex.x) * depthWidth; y = (in_tex.y) * depthHeight;

			w = x * de_dist[0][2] + y * de_dist[1][2] + de_dist[2][2];

			if( abs(w) > eps ) {
				w = 1.0 / w; outV.x = (x * de_dist[0][0] + y * de_dist[1][0] + de_dist[2][0]) *w;
				outV.y = (x * de_dist[0][1] + y * de_dist[1][1] + de_dist[2][1]) *w;
			} else {
				outV.x = 0.0;
				outV.y = 0.0;
			}

			outV.x = (outV.x / depthWidth); outV.y = 1.0 - (outV.y / depthHeight);

			return outV;
		}

		void main(){

			vec4 depth = texture(tex, tex_coord);
			vec2 normTexCoord = vec2(tex_coord.x * 2.0 - 1.0, tex_coord.y * 2.0 - 1.0);

			vec3 rwPos = vec3(normTexCoord.x, normTexCoord.y, depth.r * 65535.0); rwPos = getKinRealWorldCoord(rwPos);

			vec4 rotPos = invRot * vec4(rwPos, 1.0);
			deep = rotPos.z > nearerThres ? (rotPos.z < deeperThres ? 1.0 : 0.0) : 0.0;

			deep = (normTexCoord.x < cropRight) ? deep : 0.0;
			deep = (normTexCoord.x > cropLeft) ? deep : 0.0;
			deep = (normTexCoord.y < cropUp) ? deep : 0.0;
			deep = (normTexCoord.y > cropBottom) ? deep : 0.0;

			color = vec4(deep, deep, deep, 1.0);
		});

	frag = "// KinectReproTools Undistortion + Rotation + Thresholding\n" + shdr_Header + frag;

	undistRotShdr = shCol->addCheckShaderText("undistRot", vert.c_str(), frag.c_str());
}

//---------------------------------------------------------

void KinectReproTools::setupGeoRotShdr()
{
	std::string shdr_Header = shCol->getShaderHeader();

	std::string vert = STRINGIFY(
		layout( location=0 ) in vec4 position;\n
		void main() {
			gl_Position = position;
		});
	vert = "// KinectReproTools Rotation per GeoShader\n" + shdr_Header + vert;

	//---------------------------------------------------------

	//std::cout << "init geo shader amp: " << maxGeoAmpPoints << std::endl;

	std::string geom = "layout (points) in;\n";
	geom += "layout (points, max_vertices = " + std::to_string(maxGeoAmpPoints)
			+ ") out;\n";

	geom += STRINGIFY(
					out float out_col;\n

					uniform sampler2D depthTex0;\n
					uniform sampler2D depthTex1;\n
					uniform sampler2D depthTex2;\n

					uniform ivec2 cellSize;\n
					uniform ivec2 nrCells;\n
					uniform vec2 kinFovFact;\n
					uniform vec2 texSize;\n

					uniform float pointSize;\n
					uniform mat4 transMat;\n
					uniform mat4 m_pvm;\n

					uniform int rotate;\n

					vec4 getKinRealWorldCoord(vec3 inCoord)\n
					{\n
						return vec4(inCoord.x * 0.5 * inCoord.z * kinFovFact.x,\n
								inCoord.y * 0.5 * inCoord.z * kinFovFact.y,\n
								inCoord.z, 1.0);\n
					}\n

					vec4 getScreenCoord(vec4 inCoord)\n
					{\n
						return vec4((2.0 * inCoord.x) / (inCoord.z * kinFovFact.x),\n
								(2.0 * inCoord.y) / (inCoord.z * kinFovFact.y),\n
								inCoord.z, 1.0);\n
					}\n

					void main()\n
					{\n
						vec4 realCoord;\n

						gl_PointSize = pointSize;\n

						for (int y=0;y<cellSize.y;y++)\n
						{\n
							for (int x=0;x<cellSize.x;x++)\n
							{\n
								ivec2 tex_coord = ivec2(int(gl_in[0].gl_Position.x) + x, int(gl_in[0].gl_Position.y) + y);
								float depthVal0 = texelFetch(depthTex0, tex_coord, 0).r;\n
								float depthVal1 = texelFetch(depthTex1, tex_coord, 0).r;\n
								float depthVal2 = texelFetch(depthTex2, tex_coord, 0).r;\n

								if (depthVal0 > 0.0 && depthVal1 > 0 && depthVal2 > 0)\n {\n
									// convert to real world coordinates
									realCoord = getKinRealWorldCoord( vec3(float(tex_coord.x) / texSize.x - 0.5,\n
											float(tex_coord.y) / texSize.y - 0.5,\n
											depthVal0) );\n
									// apply transformation
									realCoord = transMat * realCoord;\n
									// transform to screen coords
									realCoord = getScreenCoord(realCoord);\n
									// set z value as color
									out_col = realCoord.z;\n
									// write to screenPos
									gl_Position = m_pvm * vec4(realCoord.y * -2.0, realCoord.x * 2.0, 0.0, 1.0);\n

									/** (rotate == 1 ?  vec4(realCoord.y * -2.0, realCoord.x * 2.0, 0.0, 1.0)
														: vec4(realCoord.x * 2.0, realCoord.y * 2.0, 0.0, 1.0));\n
*/
									EmitVertex();\n
									EndPrimitive();\n
								}\n
							}\n
						}\n
					});

	geom = "// KinectReproTools Rotation per GeoShader\n" + shdr_Header + geom;

	//---------------------------------------------------------

	std::string frag = STRINGIFY(
		layout (location = 0) out vec4 color;\n
		layout (location = 1) out vec4 thres;\n
		uniform float pointWeight;\n
		uniform float nearThres;\n
		uniform float farThres;\n
		in float out_col;\n
		void main(){\n
			vec2 temp = gl_PointCoord - vec2(0.5);\n

			float f = pow(max(1.0 - min(dot(temp, temp), 1.0), 0.0), pointWeight);\n
			color = vec4(out_col, 0.0, 0.0, f);\n

			float depthCol = out_col > nearThres ? (out_col < farThres ? out_col : 0.0) : 0.0;
			//float depthCol = out_col;
			depthCol /= 4000.0;

			thres = vec4(depthCol, depthCol, depthCol, f);\n
		});

	frag = "// KinectReproTools Rotation per GeoShader\n" + shdr_Header + frag;

	geoRotShdr = shCol->addCheckShaderText("kinReproGeoRot", vert.c_str(),
			geom.c_str(), frag.c_str());
}

//---------------------------------------------------------

KinectReproTools::~KinectReproTools()
{
}
}
