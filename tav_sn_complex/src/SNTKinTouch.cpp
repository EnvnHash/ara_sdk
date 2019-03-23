//
// SNTKinTouch.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTKinTouch.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
SNTKinTouch::SNTKinTouch(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    								SceneNode(_scd, _sceneArgs),
									kinFovY(49.234f),                     // gemessen  47,8 Handbuch sagt 45 Grad, onistream sagt 45,642
									kinFovX(61.66f),
									getNrCheckSamples(50),              // anzahl von samples bis die werte als stabil angesehen werden
									actMode(USE),
									depthSafety(56.f),
									touchDepth(20.f),
									maxNrTrackObjs(5),
									kinDeviceNr(0),
									manMirror(false), // true for installation upside down on the ceiling
									widgetWinInd(0),
									screenType(LED)
{
	kin = static_cast<KinectInput*>(scd->kin);
	//osc = static_cast<OSCData*>(scd->osc);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	shCol = static_cast<ShaderCollector*>(scd->shaderCollector);

	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods);
	});

	//-------------------------------------------------------
	// add osc adjustment
	rotXOffs = 0.f; // sollte relativ gering sein
	addPar("rotXOffs", &rotXOffs);

	// add osc adjustment
	rotYOffs = 0.f; // sollte relativ gering sein
	addPar("rotYOffs", &rotYOffs);

	threshZDeep = 2000.f; // entspricht abstand zur Wand
	addPar("threshZDeep", &threshZDeep);

	touchDepth = 0.f;
	addPar("touchDepth", &touchDepth);

	cropLeft = -1.f; // irgendwas bischen groesser als -1
	addPar("cropLeft", &cropLeft);

	cropRight = 1.f; // irgendwas bischen kleiner als 1
	addPar("cropRight", &cropRight);

	cropUp = 1.f;
	addPar("cropUp", &cropUp);

	cropDown = -1.f;
	addPar("cropDown", &cropDown);

	//-------------------------------------------------------

	ktCalib.camFov.x = (kinFovX / 360.f) * 2.f * M_PI;
	ktCalib.camFov.y = (kinFovY / 360.f) * 2.f * M_PI;
	ktCalib.boardSize = cv::Size(8,4);
	ktCalib.depthSafety = depthSafety;
	ktCalib.touchDepth = touchDepth;

	//-------------------------------------------------------

	quad = scd->stdQuad;
	quad->rotate(M_PI, 0.f, 0.f, 1.f);
	quad->rotate(M_PI, 0.f, 1.f, 0.f);

	rawQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);

	whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 1.f, 1.f, 1.f);


	// chess muss die proportionen des realen chesss haben
	// proportionen sind in der grafik schon richtig, deshalb keine
	// aspect anpassung
	glChessBoardWidth = 0.7f;  // 1.3f!
	if ( screenType == BEAMER )
	{
		chessBoard = new PropoImage((*(scd->dataPath))+"textures/chessboard.jpg",
				scd->screenWidth, scd->screenHeight, glChessBoardWidth);
	} else if(screenType == LED )
	{
		chessBoard = new PropoImage((*(scd->dataPath))+"textures/chessboard.jpg",
				scd->screenWidth, scd->screenHeight, glChessBoardWidth);
	}

	glChessBoardHeight = chessBoard->getImgHeight();

	shdr = shCol->getStdTex();
	stdCol = shCol->getStdCol();
	depthRotShdr = initDepthRotShdr();
	blendShader = initTexBlendShdr();

	calibFile = (*(scd->dataPath))+("calib_cam/xtion_kinTouch.yml");

	if (actMode == USE)
	{
		cout << "load beamer to cam matrix" << endl;
		cv::FileStorage fs(calibFile, cv::FileStorage::READ);
		if ( fs.isOpened() ) loadCamToBeamerCalib(fs, ktCalib);
	}

	camWallDist = new Median<float>(5.f, getNrCheckSamples);
	rotations = new Median<glm::vec3>(5.f, getNrCheckSamples);

	cv::SimpleBlobDetector::Params pDefaultBLOB;

	// This is default parameters for SimpleBlobDetector
	pDefaultBLOB.thresholdStep = 10;
	pDefaultBLOB.minThreshold = 10;
	pDefaultBLOB.maxThreshold = 220;
	pDefaultBLOB.minRepeatability = 2;
	pDefaultBLOB.minDistBetweenBlobs = 10;
	pDefaultBLOB.filterByColor = false;
	pDefaultBLOB.blobColor = 0;
	pDefaultBLOB.filterByArea = true;
	pDefaultBLOB.minArea = 45; // 35
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

	/*
         trackObjs = new TrackObj*[maxNrTrackObjs];
         hasNewVal = new bool[maxNrTrackObjs];
         for (int i=0;i<maxNrTrackObjs;i++) {
         trackObjs[i] = new TrackObj(i);
         hasNewVal[i] = false;
         }


         ids = new vector<int>();
         pos = new vector<float>();
         speeds = new vector<float>();
         accels = new vector<float>();
	 */

	buildWidget();

#ifdef __APPLE__
	// get parameter for mouse simulation
	CGError res = CGDisplayNoErr;

	// query active displays
	dspCount = 0;
	res = CGGetActiveDisplayList(0, NULL, &dspCount);
	if (res || dspCount == 0) {
		return;
	}

	displays = (CGDirectDisplayID*)calloc((size_t)dspCount, sizeof(CGDirectDisplayID));
	res = CGGetActiveDisplayList(dspCount, displays, &dspCount);
	if (res || dspCount == 0) {
		return;
	}
#endif

	show = 0;
}

//----------------------------------------------------

void SNTKinTouch::initKinDependent()
{
	kin->setImageRegistration(true);
	//kin->setMirror(false); // geht nicht bei depth stream, warum auch immer..

	ktCalib.imgSize.width = static_cast<float>(kin->getColorWidth(kinDeviceNr));
	ktCalib.imgSize.height = static_cast<float>(kin->getColorHeight(kinDeviceNr));

	ktCalib.depthImgSize.width = static_cast<float>(kin->getDepthWidth(kinDeviceNr));
	ktCalib.depthImgSize.height = static_cast<float>(kin->getDepthHeight(kinDeviceNr));

	kinDepthW = static_cast<float>(kin->getDepthWidth(kinDeviceNr));
	kinDepthH = static_cast<float>(kin->getDepthHeight(kinDeviceNr));

	//        kpCalib.camAspectRatio = kpCalib.imgSize.width / kpCalib.imgSize.height;
	ktCalib.camAspectRatio = ktCalib.camFov.x / ktCalib.camFov.y;

	rotDepth = new FBO(shCol, kin->getDepthWidth(kinDeviceNr),
			kin->getDepthHeight(kinDeviceNr),
			GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	kinColor = new TextureManager();
	kinColor->allocate(kin->getColorWidth(kinDeviceNr), kin->getColorHeight(kinDeviceNr),
			GL_RGB8, GL_RGB, GL_TEXTURE_2D);

	kinDepth = new TextureManager();
	kinDepth->allocate(kin->getDepthWidth(kinDeviceNr), kin->getDepthHeight(kinDeviceNr),
			GL_R16F, GL_RED, GL_TEXTURE_2D);

	kinDepthDebug = new TextureManager();
	kinDepthDebug->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
			GL_R8, GL_RED, GL_TEXTURE_2D);

	view = cv::Mat(kin->getColorHeight(kinDeviceNr), kin->getColorWidth(kinDeviceNr), CV_8UC3);
	depth = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC1);

	//thresholded = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC1);
	thresh8 = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC3);
	checkWarp = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC3);
	checkWarpDepth = cv::Mat(kin->getDepthHeight(kinDeviceNr), kin->getDepthWidth(kinDeviceNr), CV_8UC1);

	debugTex = new TextureManager();
	debugTex->allocate(kin->getDepthWidth(kinDeviceNr), kin->getDepthHeight(kinDeviceNr),
			GL_RGB8, GL_RGB, GL_TEXTURE_2D);

	fastBlur = new FastBlur(shCol, kin->getDepthWidth(kinDeviceNr),
			kin->getDepthHeight(kinDeviceNr), GL_R16);
}

//----------------------------------------------------

Shaders* SNTKinTouch::initDepthRotShdr()
{
	//- Position Shader ---
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	uniform mat4 m_pvm;

	out vec4 pos;
	out vec2 tex_coord;
	out vec4 col;

	void main()
	{
		col = color;
		tex_coord = texCoord;
		pos = position;
		gl_Position = m_pvm * position;
	});
	vert = "// SNTKinectTouch depth rotation Shader vert\n"+shdr_Header+vert;



	std::string frag = STRINGIFY(uniform sampler2D tex;
	uniform float deeperThres;
	uniform float nearerThres;
	uniform float depthWidth;
	uniform float depthHeight;
	uniform float cropRight;
	uniform float cropLeft;
	uniform float cropUp;
	uniform float cropDown;
	uniform vec2 kinFov;
	uniform mat4 invRotX;

	in vec2 tex_coord;
	in vec4 pos;
	in vec4 col;

	layout (location = 0) out vec4 color;

	vec3 getKinRealWorldCoord(vec3 inCoord)
	{
		// asus xtion tends to measure lower depth with increasing distance
		// experimental correction
		float depthScale = 1.0 + pow(inCoord.z * 0.00033, 5.3);
		float scaledDepth = inCoord.z * depthScale;

		float xzFactor = tan(kinFov.x * 0.5) * 2.0;  // stimmt noch nicht... wieso?
		float yzFactor = tan(kinFov.y * 0.5) * 2.0;  // stimmt!!!

		return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
				inCoord.y * 0.5 * scaledDepth * yzFactor,
				scaledDepth);
	}

	void main()
	{
		vec4 depth = texture(tex, tex_coord);
		vec3 rwPos = vec3(pos.x, pos.y, depth.r * 65535.0);
		rwPos = getKinRealWorldCoord(rwPos);
		vec4 rotPos = invRotX * vec4(rwPos.x, rwPos.y, rwPos.z, 1.0);
		float deep = rotPos.z < deeperThres ? rotPos.z : 0.0;
		deep = deep > nearerThres ? 1.0 : 0.0;

		deep = (pos.x < cropRight) ? deep : 0.0;
		deep = (pos.x > cropLeft) ? deep : 0.0;
		deep = (pos.y < cropUp) ? deep : 0.0;
		deep = (pos.y > cropDown) ? deep : 0.0;

		color = vec4(deep, deep, deep, 1.0);
		//color = vec4(deep, 0.0, 0.0, 1.0);
		//color = vec4(rotPos.z / 1500.0, 0.0, 0.0, 1.0);
		//color = vec4(depth.r * 35535.0, 0.0, 0.0, 1.0);
	});
	frag = "// SNTKinectTouch depth rotation Shader frag\n" +shdr_Header +frag;


	return shCol->addCheckShaderText("kinDepthRot", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

Shaders* SNTKinTouch::initTexBlendShdr()
{
	//- Position Shader ---
	std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";


	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;
	uniform mat4 m_pvm;
	uniform mat4 de_dist;
	uniform int useDedist;

	out vec2 tex_coord;
	out vec4 col;

	vec4 pos;
	float x;
	float y;
	float w;
	float eps=1.19209290e-07;

	void main()
	{
		col = color;
		tex_coord = texCoord;
		pos = m_pvm * position;

		// apply perspective undistortion
		if (useDedist == 1)
		{
			// project onto the 2d plane, coordinates are still normalized (-1|1)
			pos /= pos.w;

			x = pos.x;
			y = pos.y;
			w = x * de_dist[0][2] + y * de_dist[1][2] + de_dist[2][2];

			if( abs(w) > eps )
			{
				w = 1./w;
				pos.x = (x * de_dist[0][0] + y * de_dist[1][0] + de_dist[2][0]) *w;
				pos.y = (x * de_dist[0][1] + y * de_dist[1][1] + de_dist[2][1]) *w;
			} else {
				pos.x = pos.y = 0.0;
			}

			//  pos = de_dist * pos;
			// z will end up positive, but has no real meaning anymore, so set it to 0
			pos.z = 0;
		}

		gl_Position = pos;
	});
	vert = "// SNTKinectTouch depth rotation Shader frag\n"+shdr_Header+vert;

	std::string frag = STRINGIFY(uniform sampler2D tex;
	uniform float alpha;

	in vec2 tex_coord;
	in vec4 col;
	layout (location = 0) out vec4 color;

	void main()
	{
		color = texture(tex, tex_coord);
		color.a = color.a - alpha;
	});
	frag = "// SNTKinectTouch texblend Shader vert\n" +shdr_Header +frag;


	return shCol->addCheckShaderText("texBlendShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNTKinTouch::buildWidget()
{
	// build widget
	widget = new Widget();
	widget->setPos(0.f, 0.f);
	widget->setSize(2.f, 2.f);
	widget->setBackColor(0.f, 0.f, 0.f, 1.f);
	widget->setBackTex((*(scd->dataPath))+"textures/blue-abstract-background.JPG");

	// add image slider
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/instore/Slide1.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/instore/Slide2.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/instore/Slide3.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/instore/Slide4.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/instore/Slide5.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/landscape0.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/landscape1.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/landscape2.jpg");
	imgSlidTexs.push_back((*(scd->dataPath))+"textures/landscape3.jpg");


	float propoSlides = 1769.f / 995.f;
	float screenAspect = float(scd->screenHeight) / float(scd->screenWidth);
	float destSize = 1.2f;
	float propoHeight = destSize / screenAspect / propoSlides;
	widget->add( new GUIImgSlider(glm::vec2(0.f, (destSize - propoHeight) * 0.5f),
			glm::vec2(destSize, propoHeight) ) );

	widget->getLastGuiObj()->setBackColor(0.f, 0.f, 0.f, 1.f);
	widget->getLastGuiObj()->setColor(0.f, 0.f, 0.f, 1.f);
	widget->getLastGuiObj()->setTextures(imgSlidTexs);
	widget->getLastGuiObj()->setAction(SWIPE_RIGHT, [this]() { return this->swipeImg(); });
	widget->getLastGuiObj()->setActionAnim(SWIPE_RIGHT, GUI_MOVE_RIGHT);
	widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_RIGHT, 1.0);
	widget->getLastGuiObj()->setAction(SWIPE_LEFT, [this]() { return this->swipeImg(); });
	widget->getLastGuiObj()->setActionAnim(SWIPE_LEFT, GUI_MOVE_LEFT);
	widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_LEFT, 1.0);

	widget->getLastGuiObj()->setAction(SWIPE_UP, [this]() { return this->swipeImg(); });
	widget->getLastGuiObj()->setActionAnim(SWIPE_UP, GUI_MOVE_UP);
	widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_UP, 1.0);
	widget->getLastGuiObj()->setAction(SWIPE_DOWN, [this]() { return this->swipeImg(); });
	widget->getLastGuiObj()->setActionAnim(SWIPE_DOWN, GUI_MOVE_DOWN);
	widget->getLastGuiObj()->setActionAnimDur(GUI_MOVE_DOWN, 1.0);

	winMan->addWidget(widgetWinInd, widget, shCol);
}

//----------------------------------------------------

void SNTKinTouch::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (kin->isReady())
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);


		switch(actMode)
		{
		// das rohe tiefenbild mit histogram
		case RAW_DEPTH:
			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);
			kin->uploadDepthImg(true, 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(true, 0));
			quad->draw();
			break;

			// das rohe farbbild
		case RAW_COLOR:
			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);
			kin->uploadColorImg(0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, kin->getColorTexId(0));
			quad->draw();
			break;

			// zeig das schachbrett zum kalibrieren
		case CALIBRATE:

			stdCol->begin();
			stdCol->setIdentMatrix4fv("m_pvm");
			whiteQuad->draw();

			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);
			chessBoard->draw();
			break;

			// zeig das rotierte tiefenbild mit threshold
		case CHECK_ROT:
			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());
			rawQuad->draw();

			shdr->end();
			break;

		case CHECK:
			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, debugTex->getId());
			quad->draw();

			shdr->end();
			break;

		case USE:

			shdr->begin();
			shdr->setIdentMatrix4fv("m_pvm");
			shdr->setUniform1i("tex", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, debugTex->getId());
			quad->draw();

			shdr->end();

			winMan->drawWidget(widgetWinInd);

			break;

		default:
			break;
		}
	}
}

//----------------------------------------------------

void SNTKinTouch::update(double time, double dt)
{
	if(!isInited)
	{
		initKinDependent();
		isInited = true;
	}

	// update from osc
	ktCalib.threshZDeep = threshZDeep;
	ktCalib.touchDepth = touchDepth;
	ktCalib.cropLeft = cropLeft;
	ktCalib.cropRight = cropRight;
	ktCalib.cropUp = cropUp;
	ktCalib.cropDown = cropDown;

	if (isInited)
	{
		if (kin->getDepthFrame(0)->isValid())
		{
			if (frameNr != kin->getDepthFrameNr(true, kinDeviceNr))
			{
				frameNr = kin->getDepthFrameNr(true, kinDeviceNr);

				// blur depth values
				kin->lockDepthMutex(kinDeviceNr);

				kinDepth->bind();
				glTexSubImage2D(GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0, 0,                       // x and y offset
						kin->getDepthWidth(kinDeviceNr),
						kin->getDepthHeight(kinDeviceNr),
						GL_RED,
						GL_UNSIGNED_SHORT,
						kin->getDepthFrame(kinDeviceNr)->getData());

				kin->unlockDepthMutex(kinDeviceNr);

				fastBlur->proc(kinDepth->getId());
				fastBlur->downloadData();
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			}

			switch(actMode)
			{
			case CALIBRATE :
				// color Frame Operations
				if (frameNr != kin->getColFrameNr(kinDeviceNr) && frameNr > 20)
				{
					frameNr = kin->getColFrameNr(kinDeviceNr);

					std::cout << " CALIBRATE " << std::endl;
					view.data = kin->getActColorImg(kinDeviceNr);
					if(manMirror) cv::flip(view, view, 1);

					bool found = cv::findChessboardCorners(view, ktCalib.boardSize, pointbuf,
							cv::CALIB_CB_ADAPTIVE_THRESH |
							//cv::CALIB_CB_FAST_CHECK |
							cv::CALIB_CB_NORMALIZE_IMAGE);
					if (found)
					{
						std::cout << " found " << std::endl;
						if (gotCamToBeamer <= getNrCheckSamples)
						{
							// get chessboard and calculate rotation against wall
							calcChessRotXYWall(pointbuf, ktCalib, ktCalib.boardSize);
							calcChessRotZ(pointbuf, ktCalib);

							ktCalib.camWallDist = calcWallDist(ktCalib);

							// get perspective transformation
							ktCalib.camBeamerPerspTrans = getPerspTrans(pointbuf, glChessBoardWidth, glChessBoardHeight);

							gotCamToBeamer++;

						} else
						{
							if(!saved)
							{
								// 4 iterations should be enough to minize the error
								//for (int i=0;i<4;i++) reCheckRot(ktCalib);
								saveCamToBeamerCalib(calibFile, ktCalib);
								saved = true;
							}
						}
					}
				}
				break;

			case CHECK_ROT:
				rotDepthTex();
				break;

			case CHECK:
				rotDepthTex();

				// download img, value are normalized floats 0-1
				glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, thresh8.data);

				//hier persp. unwrap!!
				cv::flip(thresh8, thresh8, 0);
				if(manMirror) cv::flip(thresh8, thresh8, 1);

				cv::warpPerspective(thresh8, checkWarpDepth, ktCalib.camBeamerPerspTrans, ktCalib.imgSize);

				procBlobs(time, dt);

				// upload debug tex
				glBindTexture(GL_TEXTURE_2D, debugTex->getId());
				glTexSubImage2D(GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0, 0,                       // x and y offset
						kin->getDepthWidth(kinDeviceNr),
						kin->getDepthHeight(kinDeviceNr),
						GL_RGB,
						GL_UNSIGNED_BYTE,
						im_with_keypoints.data);
				break;

			case USE:
				if (frameNr != kin->getDepthFrameNr(false, kinDeviceNr) && frameNr > 10)
				{
					frameNr = kin->getDepthFrameNr(false, kinDeviceNr);

					rotDepthTex();

					// download img, value are normalized floats 0-1
					glBindTexture(GL_TEXTURE_2D, rotDepth->getColorImg());
					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, thresh8.data);

					//hier persp. unwrap!!
					cv::flip(thresh8, thresh8, 0);
					if(manMirror) cv::flip(thresh8, thresh8, 1);
					cv::warpPerspective(thresh8, checkWarpDepth, ktCalib.camBeamerPerspTrans, ktCalib.imgSize);

					procBlobs(time, dt);

					// upload debug tex
					glBindTexture(GL_TEXTURE_2D, debugTex->getId());
					glTexSubImage2D(GL_TEXTURE_2D,             // target
							0,                          // First mipmap level
							0, 0,                       // x and y offset
							kin->getDepthWidth(kinDeviceNr),
							kin->getDepthHeight(kinDeviceNr),
							GL_RGB,
							GL_UNSIGNED_BYTE,
							im_with_keypoints.data);

					// interpret gestures
					procGestures(time);
				}

				break;
			default:
				break;
			}

			frameNr++;
		}
	}
}

//----------------------------------------------------

void SNTKinTouch::procGestures(double time)
{
	// loop through all found (active) trackpoints
	for(std::vector<TrackObj>::iterator it=trackObjs.begin(); it!=trackObjs.end(); ++it)
	{
		float posX = (*it).getPos()->x * float(winMan->getMonitorWidth(0));
		float posY = ((*it).getPos()->y) * float(winMan->getMonitorHeight(0));

		if( (*it).isActive() )
		{
			(*it).accumVel(true);

			if((*it).onTimeDelta(time) > 0.02f && !clicked)
			{
				winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, LEFT_CLICK);
				clicked = true;
			}

			/*

                 //cout << "vel " <<  glm::length((*it).getMedVel()) << endl;

                 //if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getAccumVel()) > 0.2f)

                 if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getMedVel()) > 0.07f && !(*it).getUse())
                 {
                 (*it).setUse(true);
                 float angle = std::atan2((*it).getMedVel().y, (*it).getMedVel().x);
                 //                    float angle = std::atan2((*it).getAccumVel().y, (*it).getAccumVel().x);

                 //                        cout << "angle: " << angle << endl;
                 //                        cout << "std::fabs(angle - M_PI): " << std::fabs(angle - M_PI) << endl;
                 //                        cout << "std::fabs(angle - M_PI_2): " << std::fabs(angle - M_PI_2) << endl;

                 //cout << "getAccumVel length: " << glm::length((*it).getAccumVel()) << endl;
                 //cout << "delta: " << (*it).onTimeDelta(time) << endl;


                 if (std::fabs(M_PI_2 + angle) < M_PI_4)
                 {
                 cout << "swipe up" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_UP);
                 } else if (std::fabs(angle - M_PI_2) < M_PI_4)
                 {
                 cout << "swipe down" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_DOWN);

                 } else if ((M_PI - std::fabs(angle)) < M_PI_4)
                 {
                 cout << "swipe left" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_LEFT);

                 } else if (std::fabs(angle) < M_PI_4)
                 {
                 cout << "swipe right" << endl;
                 taMods->rootWidget->setCmd(posX - 1280.f, posY, SWIPE_RIGHT);
                 }

                 cout << endl;
                 }
			 */
		} else
		{

			// if inactive
			if( (*it).wasActive() )
			{

				if ((*it).onTimeDelta(time) > 0.3f && glm::length((*it).getAccumVel()) > 0.1f)
				{
					//(*it).setUse(true);
					float angle = std::atan2((*it).getAccumVel().y, (*it).getAccumVel().x);

					//cout << "angle: " << glm::to_string((*it).getAccumVel()) << endl;
					// cout << "std::fabs(angle - M_PI): " << std::fabs(angle - M_PI) << endl;
					// cout << "std::fabs(angle - M_PI_2): " << std::fabs(angle - M_PI_2) << endl;

					//cout << "getAccumVel length: " << glm::length((*it).getAccumVel()) << endl;
					//cout << "delta: " << (*it).onTimeDelta(time) << endl;

					if (std::fabs(M_PI_2 + angle) < M_PI_4)
					{
						cout << "swipe up" << endl;
						winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, SWIPE_UP);

					} else if (std::fabs(angle - M_PI_2) < M_PI_4)
					{
						cout << "swipe down" << endl;
						winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, SWIPE_DOWN);

					} else if ((M_PI - std::fabs(angle)) < M_PI_4)
					{
						cout << "swipe left" << endl;
						winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, SWIPE_LEFT);

					} else if (std::fabs(angle) < M_PI_4)
					{
						cout << "swipe right" << endl;
						winMan->setWidgetCmd(widgetWinInd, posX - float(scd->screenWidth), posY, SWIPE_RIGHT);
					}

					cout << endl;
				}

				(*it).accumVel(false);
				(*it).setUse(false);
				clicked = false;
			}
		}
	}
}

//----------------------------------------------------

void SNTKinTouch::procBlobs(double time, double dt)
{
	glm::vec2 actPoint;

	// Detect blobs.
	std::vector<cv::KeyPoint> keypoints;
	//        detector->detect(thresh8, keypoints);
	detector->detect(checkWarpDepth, keypoints);

	// dirty test only with the first value
	if ( static_cast<int>(keypoints.size()) > 0 )
	{
		if ( static_cast<int>(trackObjs.size()) == 0 )
		{
			trackObjs.push_back( TrackObj() );
			trackObjs.back().setOn();

		} else
		{
			if(!trackObjs[0].isActive()) trackObjs[0].setOn();
			trackObjs[0].predict();

			// perspective undistortion
			actPoint = glm::vec2(keypoints[0].pt.x, keypoints[0].pt.y);
			//perspUnwarp(actPoint, perspDistMat);

			// normalize from 0 - 1
			if(!manMirror)
				trackObjs[0].update(actPoint.x / kinDepthW,
						actPoint.y / kinDepthH,
						dt);
			else
				trackObjs[0].update(actPoint.x / kinDepthW,
						1.0 - (actPoint.y / kinDepthH),
						dt);

		}
	} else
	{
		//cout << "keypoints.size(): " << keypoints.size() << endl;

		// to avoid false off-sets only take the off if it was send continously sent for a specific time
		for(std::vector<TrackObj>::iterator it=trackObjs.begin(); it!=trackObjs.end(); ++it)
			(*it).setOff();
	}


	/*
         // predict new results
         for (int i=0;i<maxNrTrackObjs;i++) trackObjs[i]->predict();

         kpCoords.clear(); allDistMap.clear();
         std::vector< std::map<float, int> > allDists;

         // loop through the found points
         for(int i=0;i<int(keypoints.size());i++)
         {
         float x = keypoints[i].pt.x / kin->getDepthWidth() -0.5f;
         float y = 0.5f - keypoints[i].pt.y / kin->getDepthHeight();

         kpCoords.push_back( glm::vec4(x, y, 0.f, 0.f) );

         dists.clear();

         // loop through all present trackPoints and calculate distances to the actual point
         for(int j=0;j<maxNrTrackObjs;j++)
         dists[ trackObjs[j]->getDist(x, y, 0.f) ] = j;

         allDists.push_back(dists);

         // save the lowest result in a map
         allDistMap[ dists.begin()->first ] = i;
         }


         // gehe alle resultate durch und wirf das zugeordnete TrackObj aus allen anderen Maps raus
         ids->clear(); pos->clear(); speeds->clear(); accels->clear();
         for (std::map<float,int>::iterator it=allDistMap.begin(); it!=allDistMap.end(); ++it)
         {
         int kpNr = (*it).second;
         int trackObjNr = allDists[kpNr].begin()->second;

         trackObjs[trackObjNr]->update( kpCoords[kpNr].x, kpCoords[kpNr].y, kpCoords[kpNr].z, dt );

         ids->push_back(trackObjNr);

         pos->push_back(kpCoords[kpNr].x);
         pos->push_back(kpCoords[kpNr].y);

         speeds->push_back( trackObjs[trackObjNr]->getVel(0) );
         speeds->push_back( trackObjs[trackObjNr]->getVel(1) );

         accels->push_back( trackObjs[trackObjNr]->getAccel(0) );
         accels->push_back( trackObjs[trackObjNr]->getAccel(1) );

         for (int i=0; i<allDists.size(); i++)
         {
         // look for the entry with the same trackobj nr in the specific dist map
         bool found = false;
         std::map<float,int>::iterator killThis;
         for (std::map<float,int>::iterator dIt=allDists[i].begin(); dIt!= allDists[i].end(); dIt++)
         {
         if ( (*dIt).second == trackObjNr )
         {
         found = true; killThis = dIt;
         }
         }
         if (found) allDists[i].erase( killThis );
         }
         }
	 */

	// Draw detected blobs as red circles.
	// DrawMatchesFlags::DRAW_RICH_KEYPOINTS flag ensures the size of the circle corresponds to the size of blob
	//        cv::drawKeypoints(thresh8, keypoints, im_with_keypoints,
	//                          cv::Scalar(255,0,255),
	//                          cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	cv::drawKeypoints(checkWarpDepth, keypoints, im_with_keypoints,
			cv::Scalar(255,0,255),
			cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

}

//----------------------------------------------------

void SNTKinTouch::perspUnwarp(glm::vec2& inPoint, glm::mat4& perspMatr)
{
	const double eps = FLT_EPSILON;

	float x = inPoint.x, y = inPoint.y;
	double w = x * perspMatr[0][2] + y * perspMatr[1][2] + perspMatr[2][2];

	if( std::fabs(w) > eps )
	{
		w = 1./w;
		inPoint.x = (float)((x * perspMatr[0][0] + y * perspMatr[1][0] + perspMatr[2][0]) *w);
		inPoint.y = (float)((x * perspMatr[0][1] + y * perspMatr[1][1] + perspMatr[2][1]) *w);
	}
	else
		inPoint.x = inPoint.y = (float)0;
}

//----------------------------------------------------

void SNTKinTouch::rotDepthTex()
{
	glm::mat4 rot = glm::rotate(glm::mat4(1.f), -(ktCalib.camBeamerRots.x + rotXOffs), glm::vec3(1.f, 0.f, 0.f))
	* glm::rotate(glm::mat4(1.f), -(ktCalib.camBeamerRots.y + rotYOffs), glm::vec3(0.f, 1.f, 0.f))
	* glm::rotate(glm::mat4(1.f), -ktCalib.camBeamerRots.z, glm::vec3(0.f, 0.f, 1.f));

	rotDepth->bind();

	depthRotShdr->begin();
	depthRotShdr->setIdentMatrix4fv("m_pvm");
	depthRotShdr->setUniform1f("depthWidth", ktCalib.depthImgSize.width);
	depthRotShdr->setUniform1f("depthHeight", ktCalib.depthImgSize.height);
	depthRotShdr->setUniform1f("deeperThres", ktCalib.threshZDeep );
	depthRotShdr->setUniform1f("nearerThres", ktCalib.threshZDeep - ktCalib.touchDepth );
	//	depthRotShdr->setUniform1f("nearerThres", ktCalib.camWallDist - ktCalib.touchDepth );
	depthRotShdr->setUniform1f("cropRight", ktCalib.cropRight );
	depthRotShdr->setUniform1f("cropLeft", ktCalib.cropLeft );
	depthRotShdr->setUniform1f("cropUp", ktCalib.cropUp );
	depthRotShdr->setUniform1f("cropDown", ktCalib.cropDown );

	depthRotShdr->setUniform2fv("kinFov", &ktCalib.camFov[0]);
	depthRotShdr->setUniformMatrix4fv("invRotX", &rot[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinDepth->getId());
	quad->draw();

	rotDepth->unbind();
}

//----------------------------------------------------

void SNTKinTouch::reCheckRot(ktCalibData& kp)
{
	std::vector<glm::vec3> realWorldCoord;

	glm::mat4 rot = glm::rotate(glm::mat4(1.f), kp.rotations.x, glm::vec3(1.f, 0.f, 0.f))
	* glm::rotate(glm::mat4(1.f), kp.rotations.y, glm::vec3(0.f, 1.f, 0.f))
	* glm::rotate(glm::mat4(1.f), kp.rotations.z, glm::vec3(0.f, 0.f, 1.f));

	for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
	{
		realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
		getKinRealWorldCoord(realWorldCoord.back(), kp);
		realWorldCoord.back() = glm::vec3( rot * glm::vec4(realWorldCoord.back(), 1.f) );
		// cout << glm::to_string( realWorldCoord.back() ) << endl;
	}

	// get 4 normals from the edges
	std::vector<glm::vec3> sides;
	glm::vec3 lowerLeft = realWorldCoord[kp.boardSize.width * (kp.boardSize.height-1)];
	glm::vec3 lowerRight = realWorldCoord[kp.boardSize.width * kp.boardSize.height -1];
	glm::vec3 upperRight = realWorldCoord[kp.boardSize.width -1];
	glm::vec3 upperLeft = realWorldCoord[0];


	// lower left to upper left
	sides.push_back( upperLeft - lowerLeft );
	// lower left to lower right
	sides.push_back( lowerRight - lowerLeft);

	// lower right to lower left
	sides.push_back( lowerLeft - lowerRight );
	// lower right to upper right
	sides.push_back( upperRight - lowerRight);

	// upper right to lower right
	sides.push_back( lowerRight - upperRight );
	// upper right to upper left
	sides.push_back( upperLeft - upperRight );

	// upper left to upper right
	sides.push_back( upperRight - upperLeft );
	// upper left to lower left
	sides.push_back( lowerLeft - upperLeft );


	for (std::vector<glm::vec3>::iterator it = sides.begin(); it != sides.end(); ++it)
		(*it) = glm::normalize((*it));


	// get normals and calculate medium
	glm::vec3 medNormal = glm::vec3(0.f);
	for (int i=0;i<static_cast<int>(sides.size()) / 2;i++)
	{
		medNormal += glm::cross( sides[i *2], sides[i *2 +1] );
		//cout << "norm[" << i << "]: " << glm::to_string(glm::cross( sides[i *2], sides[i *2 +1] )) << endl;
	}

	medNormal /= static_cast<float>(sides.size()) * 0.5f;
	cout << "final norm: " << glm::to_string(medNormal) << endl;

	glm::quat rotQ = RotationBetweenVectors(medNormal, glm::vec3(0.f, 0.f, -1.f));
	glm::vec3 rotAngles = glm::eulerAngles(rotQ);
	cout << "rotAngles: " << glm::to_string(rotAngles) << endl;

	// apply found correction
	kp.rotations += rotAngles;
}

//----------------------------------------------------

glm::mat4 SNTKinTouch::calcChessRotXYWall(std::vector<cv::Point2f> _pointbuf, ktCalibData& kp,
		cv::Size boardSize)
{
	float fIndJ, fIndI;
	vector<double> medVals;
	glm::mat4 outMat = glm::mat4(1.f);
	int nrTest = 40;
	float sampleWidth = 0.8f;

	// calculate x-Axis rotation take vertical test columns and get the rotation angle for each
	// get test column
	for(int j=0;j<nrTest;j++)
	{
		vector<glm::vec3> testCol;
		fIndJ = static_cast<float>(j) / static_cast<float>(nrTest);

		for(int i=0;i<nrTest;i++)
		{
			fIndI = static_cast<float>(i) / static_cast<float>(nrTest);
			testCol.push_back(glm::vec3((float)kin->getColorWidth() * (fIndJ * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
					(float)kin->getColorHeight() * (fIndI * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
					0.f));
			getKinRealWorldCoord(testCol.back(), kp);
		}

		// berechne die steigung für alle punkte
		double midVal = 0;
		for(int i=0;i<nrTest-1;i++)
			midVal += (M_PI * 0.5) - std::atan2(testCol[i].y - testCol[i+1].y, (testCol[i].z - testCol[i+1].z));
		midVal /= static_cast<double>(nrTest);
		medVals.push_back(midVal);
	}

	// get final x-rotation
	//rotations.x = 0.f;
	float rotX = 0.f;
	for(int j=0;j<nrTest;j++) rotX += medVals[j];
	rotX /= -static_cast<float>(nrTest);
	kp.rotations.x = rotX;
	cout << "rotation x-axis: " << rotX << endl;



	medVals.clear();
	// calculate y-Axis rotation take vertical test columns and get the rotation angle for each
	// get test column
	for(int j=0;j<nrTest;j++)
	{
		vector<glm::vec3> testRow;
		fIndJ = static_cast<float>(j) / static_cast<float>(nrTest);

		for(int i=0;i<nrTest;i++)
		{
			fIndI = static_cast<float>(i) / static_cast<float>(nrTest);
			testRow.push_back(glm::vec3((float)kin->getColorWidth() * (fIndI * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
					(float)kin->getColorHeight() * (fIndJ * sampleWidth + ((1.f - sampleWidth) * 0.5f)),
					0.f));
			getKinRealWorldCoord(testRow.back(), kp);
		}

		// berechne die steigung für alle punkte
		double midVal = 0;
		for(int i=0;i<nrTest-1;i++)
			midVal += std::atan2(testRow[i+1].z - testRow[i].z, testRow[i+1].x - testRow[i].x);
		midVal /= static_cast<double>(nrTest);

		medVals.push_back(midVal);
	}

	// get final x-rotation
	float rotY = 0.f;
	for(int j=0;j<nrTest;j++) rotY += medVals[j];
	rotY /= -static_cast<float>(nrTest);
	cout << "rotation y-axis: " << rotY << endl;
	kp.rotations.y = rotY;
	rotations->add(glm::vec3(rotX, rotY, 0.f));


	// quaternion
	glm::quat quatX = glm::angleAxis(kp.rotations.x, glm::vec3(1.f, 0.f, 0.f));
	glm::mat4 rotQuatX = glm::mat4(quatX);

	glm::quat quatY = glm::angleAxis(kp.rotations.y, glm::vec3(0.f, 1.f, 0.f));
	glm::mat4 rotQuatY = glm::mat4(quatY);

	outMat = rotQuatX * rotQuatY;

	//        if (kp.invertMatr)
	//            outMat = glm::inverse(outMat);

	return outMat;
}

//----------------------------------------------------

glm::mat4 SNTKinTouch::calcChessRotZ(std::vector<cv::Point2f> _pointbuf, ktCalibData& kp)
{
	std::vector<glm::vec3> realWorldCoord;
	vector< glm::vec3 > leftVecs;


	glm::mat4 rot = glm::rotate(glm::mat4(1.f), kp.rotations.x, glm::vec3(1.f, 0.f, 0.f))
	* glm::rotate(glm::mat4(1.f), kp.rotations.y, glm::vec3(0.f, 1.f, 0.f));

	for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
	{
		realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
		getKinRealWorldCoord(realWorldCoord.back(), kp);
		realWorldCoord.back() = glm::vec3( rot * glm::vec4(realWorldCoord.back(), 1.f) );
	}


	// calculate rotation
	// calculate vector pointing to +x for all corners
	for (short y=0;y<kp.boardSize.height;y++)
	{
		for (short x=0;x<kp.boardSize.width -1;x++)
		{
			glm::vec3 act = realWorldCoord[y * kp.boardSize.width + x];
			glm::vec3 right = realWorldCoord[y * kp.boardSize.width + x+1];
			leftVecs.push_back( glm::normalize ( right - act ) );
		}
	}

	// calculate medium leftVec
	glm::vec3 outLeftVec = glm::vec3(0.f, 0.f, 0.f);
	for (std::vector<glm::vec3>::iterator it = leftVecs.begin(); it != leftVecs.end(); ++it)
		if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
			outLeftVec += (*it);

	if (static_cast<int>(leftVecs.size()) > 0)
	{
		outLeftVec /= (float)leftVecs.size();
		outLeftVec.z = 0.f;
		kp.rotations.z = glm::angle(outLeftVec, glm::vec3(1.f, 0.f, 0.f));
		cout << "rotation z-axis: " << kp.rotations.z << endl;
	}

	glm::quat quatZ = glm::angleAxis(kp.rotations.z, glm::vec3(0.f, 0.f, 1.f));

	return glm::mat4(quatZ);
}

//----------------------------------------------------

float SNTKinTouch::calcWallDist(ktCalibData& kp)
{
	float dist = 0.f;

	glm::vec3 kinMid = glm::vec3(kp.imgSize.width * 0.5f, kp.imgSize.height * 0.5f, 0.f);
	getKinRealWorldCoord(kinMid, kp);

	glm::mat4 rot = glm::rotate(glm::mat4(1.f), -(kp.rotations.x), glm::vec3(1.f, 0.f, 0.f));

	kinMid = glm::vec3(rot * glm::vec4(kinMid, 1.f));

	camWallDist->add(kinMid.z);
	cout << "camWall: " << kinMid.z << endl;

	return kinMid.z;
}

//----------------------------------------------------

void SNTKinTouch::getKinRealWorldCoord(glm::vec3& inCoord, ktCalibData& kp)
{
	float depthScale = 1.0;
	float resX = static_cast<float>(kin->getDepthWidth());
	float resY = static_cast<float>(kin->getDepthHeight());

	// in case depth and color stream have different sizes
	inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth()) * static_cast<float>(kin->getDepthWidth());
	inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight()) * static_cast<float>(kin->getDepthHeight());

	//kin->lockDepthMutex();
	//const openni::DepthPixel* blurPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
	const openni::DepthPixel* blurPix = (const openni::DepthPixel*)fastBlur->getDataR16();

	int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
	//        pDepthPix += rowSize * static_cast<int>(inCoord.y);
	//        pDepthPix += static_cast<int>(inCoord.x);

	blurPix += rowSize * static_cast<int>(inCoord.y);
	blurPix += static_cast<int>(inCoord.x);

	//        kin->unlockDepthMutex();

	// asus xtion tends to measure lower depth with increasing distance
	// experimental correction
	//        depthScale = 1.0 + std::pow(static_cast<double>(*pDepthPix) * 0.00033, 5.3);
	depthScale = 1.0 + std::pow(static_cast<double>(*blurPix) * 0.00033, 5.3);

	// kinect v1 precision gets worse by distance
	// generally the distance will be a bit lower
	//        double scaledDepth = static_cast<double>(*pDepthPix) * depthScale;
	double scaledDepth = static_cast<double>(*blurPix) * depthScale;

	glm::vec2 fov = glm::vec2(kp.camFov.x, kp.camFov.y);

	double normalizedX = inCoord.x / resX - .5f;
	double normalizedY = .5f - inCoord.y / resY;

	double xzFactor = std::tan(fov.x * 0.5f) * 2.f;  // stimmt noch nicht.. wieso?
	double yzFactor = std::tan(fov.y * 0.5f) * 2.f;  // stimmt!!!

	inCoord.x = static_cast<float>(normalizedX * scaledDepth * xzFactor);
	inCoord.y = static_cast<float>(normalizedY * scaledDepth * yzFactor);
	inCoord.z = static_cast<float>(scaledDepth);
}

//----------------------------------------------------

cv::Mat SNTKinTouch::getPerspTrans(std::vector<cv::Point2f>& pointbuf, float _chessWidth,
		float _chessHeight)
{

	cv::Mat out;
	vector<cv::Point2f> inPoint;
	vector<cv::Point2f> dstPoint;

	// we don´t take the far out edges of the chessboard
	// but the next inner edges.
	float chessW = _chessWidth * static_cast<float>(ktCalib.boardSize.width -1)
        								/ static_cast<float>(ktCalib.boardSize.width +1)
										* 0.5f * ktCalib.imgSize.width;
	float chessH = _chessHeight * static_cast<float>(ktCalib.boardSize.height-1)
        								/ static_cast<float>(ktCalib.boardSize.height +1)
										* 0.5f * ktCalib.imgSize.height;

	float chessOffsX = (2.f - _chessWidth) * 0.25f * ktCalib.imgSize.width
			+ chessW / static_cast<float>(ktCalib.boardSize.width -1);
	float chessOffsY = (2.f - _chessHeight) * 0.25f * kin->getColorHeight()
        								+ chessH / static_cast<float>(ktCalib.boardSize.height -1);

	// lower left (in point upper, weil kommt auf d. kopf rein)
	for( int y = 0; y < 2; y++ )
		for( int x = 0; x < 2; x++ )
			dstPoint.push_back(cv::Point2f(float(x) * chessW + chessOffsX,
					float(y) * chessH + chessOffsY));

	inPoint.push_back(pointbuf[0]);
	inPoint.push_back(pointbuf[ktCalib.boardSize.width-1]);
	inPoint.push_back(pointbuf[(ktCalib.boardSize.height-1) * ktCalib.boardSize.width]);
	inPoint.push_back(pointbuf[(ktCalib.boardSize.height-1) * ktCalib.boardSize.width + ktCalib.boardSize.width -1]);

	out = cv::getPerspectiveTransform(inPoint, dstPoint);

	return out;
}

//----------------------------------------------------

glm::mat4 SNTKinTouch::cvMat33ToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j=0;j<3;j++)
		for (short i=0;i<3;i++)
			out[i][j] = _mat.at<double>(j*3 + i);

	return out;
}

//----------------------------------------------------

void SNTKinTouch::loadCamToBeamerCalib(cv::FileStorage& fs, ktCalibData& kp)
{
	fs["image_width"] >> kp.imgSize.width;
	fs["image_height"] >> kp.imgSize.height;
	fs["depth_image_width"] >> kp.depthImgSize.width;
	fs["depth_image_height"] >> kp.depthImgSize.height;

	fs["board_width"] >> kp.boardSize.width;
	fs["board_height"] >> kp.boardSize.height;

	fs["camAspectRatio"] >> kp.camAspectRatio;

	fs["camFovX"] >> kp.camFov.x;
	fs["camFovY"] >> kp.camFov.y;

	fs["camWallDist"] >> kp.camWallDist;
	fs["depthSafety"] >> kp.depthSafety;
	fs["touchDepth"] >> kp.touchDepth;
	fs["cropRight"] >> kp.cropRight;
	fs["cropLeft"] >> kp.cropLeft;
	fs["cropUp"] >> kp.cropUp;
	fs["cropDown"] >> kp.cropDown;


	fs["camBeamerPerspTrans"] >> kp.camBeamerPerspTrans;
	perspDistMat = cvMat33ToGlm(kp.camBeamerPerspTrans);
	//        kp.camBeamerPerspTrans = kp.camBeamerPerspTrans.inv();


	cv::Mat camBeamerRotations;
	fs["camBeamerRotations"] >> camBeamerRotations;
	kp.rotations = glm::vec3(camBeamerRotations.at<float>(0),
			camBeamerRotations.at<float>(1),
			camBeamerRotations.at<float>(2));

	gotCamToBeamer = getNrCheckSamples;
}

//----------------------------------------------------

void SNTKinTouch::saveCamToBeamerCalib(std::string _filename, ktCalibData& kp)
{
	printf("saving camera to beamer calib \n");

	cv::FileStorage fs( _filename, cv::FileStorage::WRITE );

	fs << "image_width" << kp.imgSize.width;
	fs << "image_height" << kp.imgSize.height;
	fs << "depth_image_width" << kp.depthImgSize.width;
	fs << "depth_image_height" << kp.depthImgSize.height;
	fs << "board_width" << kp.boardSize.width;
	fs << "board_height" << kp.boardSize.height;

	fs << "camAspectRatio" << kp.camAspectRatio;
	fs << "camFovX" << kp.camFov.x;
	fs << "camFovY" << kp.camFov.y;

	fs << "camWallDist" << kp.camWallDist;
	fs << "depthSafety" << kp.depthSafety;
	fs << "touchDepth" << kp.touchDepth;
	fs << "cropRight" << kp.cropRight;
	fs << "cropLeft" << kp.cropLeft;
	fs << "cropUp" << kp.cropUp;
	fs << "cropDown" << kp.cropDown;


	fs << "camBeamerPerspTrans" << kp.camBeamerPerspTrans;

	rotations->calcMed();
	kp.rotations = rotations->getMed();

	cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
	glmVec.at<float>(0) = kp.rotations.x + rotXOffs;
	glmVec.at<float>(1) = kp.rotations.y;
	glmVec.at<float>(2) = kp.rotations.z;
	fs << "camBeamerRotations" << glmVec;
}

//----------------------------------------------------

void SNTKinTouch::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)ns;
	nanosleep(&tim, NULL);
}

//----------------------------------------------------

void SNTKinTouch::swipeImg()
{}

//----------------------------------------------------

void SNTKinTouch::clickBut()
{}

//----------------------------------------------------

void SNTKinTouch::sendOsCmd()
{
	/*
         if((*it).onTimeDelta(time) < 0.25f)
         {
         cout << "click" << endl;
         taMods->rootWidget->setCmd(posX - 1280.f, posY, LEFT_CLICK);

         CGEventRef click_down = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown,
         CGPointMake(posX -1280.f, posY),
         kCGMouseButtonLeft);
         CGEventRef click_up = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp,
         CGPointMake(posX -1280.f, posY),
         kCGMouseButtonLeft);
         CGEventPost(kCGHIDEventTap, click_down);
         CGEventPost(kCGHIDEventTap, click_up);

         CFRelease(click_down);
         CFRelease(click_up);

         } else
	 */


	//CFStringRef key = CFSTR("SwipeActionRight");

	//CFArrayRef sequence = tl_CGEventSequenceCreateForAction(action);
	//CGEventPost(kCGHIDEventTap, (CGEventRef)event);

	/*
         NSDictionary* swipeInfo = [NSDictionary dictionaryWithObjectsAndKeys:
         [NSNumber numberWithInt:kTLInfoSubtypeSwipe],
         (id)kTLInfoKeyGestureSubtype,
         [NSNumber numberWithInt:pendingSwipe],
         (id)kTLInfoKeySwipeDirection, nil];

         CGEventRef e = tl_CGEventCreateFromGesture((CFDictionaryRef)swipeInfo,
         (CFArrayRef)eventTouches);
         CGEventPost(kCGHIDEventTap, e);
         CFRelease(e);
	 */
}

//----------------------------------------------------

void SNTKinTouch::onKey(int key, int scancode, int action, int mods)
{
	// trapez korrektur schnell und schmutzig
	if (action == GLFW_PRESS)
	{
		if(mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_1 : actMode = RAW_DEPTH; break;
			case GLFW_KEY_2 : actMode = RAW_COLOR; break;
			case GLFW_KEY_3 : actMode = CALIBRATE; gotCamToBeamer = 0; printf("CALIBRATE \n"); break;
			case GLFW_KEY_4 : actMode = CHECK_ROT; printf("CHECK_ROT \n"); break;
			case GLFW_KEY_5 : actMode = CHECK; printf("CHECK \n");  break;
			case GLFW_KEY_6 : actMode = USE; printf("USE \n");  break;
			case GLFW_KEY_S : saveCamToBeamerCalib(calibFile, ktCalib); printf("SAVE \n");  break;
			case GLFW_KEY_R : saveCamToBeamerCalib(calibFile, ktCalib); printf("SAVE \n");  break;
			}
		} else
		{
			switch (key)
			{
			case GLFW_KEY_UP : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_UP); break;
			case GLFW_KEY_LEFT : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_LEFT); break;
			case GLFW_KEY_RIGHT : winMan->setWidgetCmd(widgetWinInd, 320.f, 240.f, SWIPE_RIGHT); break;
			}
		}
	}
}

//----------------------------------------------------

SNTKinTouch::~SNTKinTouch()
{
	delete quad;
}

}
