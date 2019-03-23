//
//  SNRotatingObjMapping.cpp
//  tav_scene
//
//  Created by Sven Hahne on 4/10/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNRotatingObjMapping.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
SNRotatingObjMapping::SNRotatingObjMapping(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), near(0.1f), far(150.f), objDist(-135.5f),
			modelRealHeight(9.5f), motorMaxSteps(800), ardNrStepLoop(8),
			actDrawMode(DRAW), useArduino(false), inited(false), calibLoaded(false),
			useKinect(false),
			kinFovY(47.8f),                     // gemessen  47,8 Handbuch sagt 45 Grad, onistream sagt 45,642
			kinFovX(61.66f),                    // gemessen 62.2, vielleicht von Kinect zu Kinect unterschiedlich
			// Handbuch sagt 58 Grad, onistream sagt 58,59
			realBoardWidth(24.3f),              // reale Breite des Schachbretts (alle kästchen)
			coordNorm(0.1f),
			maxModMotorInc(20),
			actText(0),
			actMesh(0),
			motorAbsPos16bitCrossed(0),
			dir(1.f),
			switchTexInt(8.0),
			checkCross(false),
			rotPos(0.f)
{
	kin = static_cast<KinectInput*>(scd->kin);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	osc = static_cast<OSCData*>(scd->osc);
	pa = (PAudio*) scd->pa;
	shCol = static_cast<ShaderCollector*>(shCol);

	// add onKeyFunction to Window
	winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
		return this->onKey(key, scancode, action, mods); });

	busNode = new SceneNode();
	busNode->setName("bus");
	busNode->setActive(true);

	AssimpImport* aImport = new AssimpImport(scd, true);
	aImport->load(((*scd->dataPath)+"models/Vw_bus_pancho/CombiCruz.obj").c_str(), busNode, [this](){});



	//  Textures

	kinDepthTex = new TextureManager();
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_surf.jpg"); // 0
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Metal.jpg"); // 1
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Graffiti.jpg"); // 2
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Lumina.jpg"); // 3
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Hippie.jpg"); // 4
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Rock.jpg"); // 5
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Scobi.jpg");

	switchTextures = new TextureManager[int(tex_paths.size())];
	for(int i=0;i<int(tex_paths.size());i++)
		switchTextures[i].loadTexture2D(tex_paths[i]);

	cubeTex = new TextureManager();
	cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skyboxsunLumina.png").c_str() );

	litsphereTexture = new TextureManager();
	litsphereTexture->loadTexture2D( ((*scd->dataPath)+"textures/vegas/silver_sphere.jpg").c_str() );

	normalTexture = new TextureManager();
	normalTexture->loadTexture2D( ((*scd->dataPath)+"textures/vegas/Unknown-25.jpeg").c_str() );

	lucesTex = new TextureManager();
	lucesTex->loadTexture2D( (*scd->dataPath)+"textures/vegas/Luces.jpg" ); // 7

	audioBuf = new TextureBuffer(pa->getFramesPerBuffer(), 1);

	//  Shaders

	//shdr = shCol->getStdDirLight();
	addLitSphereShader();

	texShaderDist = shCol->addCheckShader("texShdrDist", "shaders/mapping2.vert",
			"shaders/mapping2.frag");
	mirror = shCol->addCheckShader("envMap", "shaders/mapping_envMap.vert",
			"shaders/mapping_envMap.frag");
	glass = shCol->addCheckShader("glass", "shaders/mapping_glass.vert",
			"shaders/mapping_glass.frag");

	texShader = shCol->getStdTex();
	colShader = shCol->getStdCol();

	shdrSet.push_back(shdr); // 0 kreuz vert
	shdrSet.push_back(shdr); // 1 kreuz hori
	shdrSet.push_back(shdr); // 2
	shdrSet.push_back(mirror); // 3 radkappe
	shdrSet.push_back(shdr); // 4 ?
	shdrSet.push_back(mirror); // 5 radkappe
	shdrSet.push_back(shdr); // 6 karosserie
	shdrSet.push_back(glass); // 7 fenster
	shdrSet.push_back(shdr); // 8 gummi fenster
	shdrSet.push_back(shdr); // 9 lichter
	shdrSet.push_back(mirror); // 10 metal
	shdrSet.push_back(shdr); // 11 ?


	//  Udp ----

	udp = new UDPServer(5001);


	//  arduino ----

	// start arduino communication
	if (useArduino)
	{
#ifdef __APPLE__
		string cmd = "ls /dev/tty.usbmodem*";
#else
		string cmd = "ls /dev/ttyACM*";
#endif

		string ardPort = execCmd( (char*) cmd.c_str() );
		ardPort = ardPort.substr(0, ardPort.size() -1);
		printf("Arduino port: %s \n", ardPort.c_str());

		arduino = new Arduino(ardPort, 57600, Arduino::PAR_NONE,
				Arduino::STOP_1, Arduino::FLOW_OFF);

		myNanoSleep(1000000);
		if(arduino) arduino->send('x');
		myNanoSleep(1000000);
		if(arduino) arduino->send('x');
	}

	actNrStepLoop = ardNrStepLoop;
	motorMaxAbsPos = int(std::pow(2.f, 16.f));


	//  Calibration ----

	// gemessen
	//        kpCalib.beamerFovX = 1.384f;
	//        kpCalib.beamerFovY = 0.56246440383659f * 2.f;
	kpCalib.beamerFovY = 0.395f;
	kpCalib.beamerLookAtAngle = 0.1974f;
	kpCalib.boardSize = cv::Size(8,4);

	kpCalib.kinFovX = (kinFovX / 360.f) * 2.f * M_PI;
	kpCalib.kinFovY = (kinFovY * 1.03f / 360.f) * 2.f * M_PI;

	kpCalib.camBeamerRealOffs = glm::vec3(0.f, 67.f, 10.f);


	//        modelMatr = glm::mat4(1.f);
	beamerMatr = glm::perspectiveFov(kpCalib.beamerFovY,
			(float)scd->screenWidth,
			(float)scd->screenHeight,
			near, far);

	// assimpimporter normalisiert höhe zu 2
	scaleToRealDimMatr = glm::scale(glm::mat4(1.f), glm::vec3(1.f));
	//        scaleToRealDimMatr = glm::scale(glm::mat4(1.f), glm::vec3(23.9f, modelRealHeight, 10.f) * 0.5f);

	// move in +y so that the floor of the van is at y=0
	modelMatr = glm::rotate(glm::mat4(1.f), 0.f, glm::vec3(0.f, 1.f, 0.f));

	calibFilePath = (*scd->dataPath)+"calib_cam/rotObjMap.yml";
	loadCalib(calibFilePath, kpCalib);


	//  GeoObjects ----

	xLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
	GLfloat* xLinePoints = (GLfloat*) xLine->getMapBuffer(POSITION);
	xLinePoints[0] = -1.f; xLinePoints[1] = 0.f; xLinePoints[2] = 0.f;
	xLinePoints[3] = 1.f; xLinePoints[4] = 0.f; xLinePoints[5] = 0.f;
	xLine->unMapBuffer();

	yLine = new Line(2, 1.f, 0.f, 0.f, 1.f);
	GLfloat* yLinePoints = (GLfloat*) yLine->getMapBuffer(POSITION);
	yLinePoints[0] = 0.f; yLinePoints[1] = 1.f; yLinePoints[2] = 0.f;
	yLinePoints[3] = 0.f; yLinePoints[4] = -1.f; yLinePoints[5] = 0.f;
	yLine->unMapBuffer();

	chessBoard = new PropoImage((*scd->dataPath)+"textures/chessboard_512.jpg",
			(int)((float)scd->screenWidth / 1.8f),
			scd->screenWidth, 2.f);

	// 512 x 512 textur, nicht quadratische texture, skalieren nicht richtig...
	// proportion texture ist 1.8
	chessBoard2 = new PropoImage((*scd->dataPath)+"textures/chessboard_512.jpg",
			(int)((float)scd->screenWidth / 1.8f),
			scd->screenWidth, realBoardWidth);

	chessQA = new QuadArray(20, 20, 0.f, 0.f,
			realBoardWidth,
			chessBoard2->getImgHeight(),
			glm::vec3(0.f, 0.f, 1.f));


	whiteQuad = new Quad(-1.0f, -1.0f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			1.f, 1.f, 1.f, 1.f);

	cbNormal = new Median<glm::vec3>(18.f);

	blendTextStart = 0.f;
	blendTextEnd = 1.f;
	blendText = new AnimVal<float>(tav::RAMP_LIN_UP, nullptr);
}

//----------------------------------------------------

SNRotatingObjMapping::~SNRotatingObjMapping()
{
	delete shdr;
}

//----------------------------------------------------

void SNRotatingObjMapping::initDyn()
{
	if(useKinect)
	{
		view = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
		kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
				GL_R16, GL_RED, GL_TEXTURE_2D);

		fastBlur = new FastBlur(shCol, kin->getDepthWidth(),
				kin->getDepthHeight(), GL_R16);
	}
	inited = true;
}

//----------------------------------------------------

void SNRotatingObjMapping::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (_tfo)
	{
		_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		_tfo->end();
		glDisable(GL_RASTERIZER_DISCARD);
	}

	glm::vec4 mCol = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec3 lightPos = glm::vec3(1.f, 3.f, 2.f) * 8.f;
	glm::vec4 ambient = glm::vec4(0.2, 0.2, 0.2, 1.0);
	glm::vec4 sceneColor = glm::vec4(0.7, 0.7, 0.7, 0.7);
	glm::vec4 diffuse = glm::vec4(0.8, 0.8, 0.8, 1.0);
	glm::vec4 specular = glm::vec4(0.8, 0.8, 0.8f, 1.0);

	float shininess = 10.0;

	switch(actDrawMode)
	{
	case CHESSBOARD :
	{
		glDisable(GL_DEPTH_TEST);
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		whiteQuad->draw();
		chessBoard->draw();

		colShader->begin();
		colShader->setIdentMatrix4fv("m_pvm");
		yLine->draw(GL_LINES);
		xLine->draw(GL_LINES);

		break;
	}
	case CALIBRATE :
	{
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		whiteQuad->draw();

		break;
	}
	case CHECK :
	{
		rotXOffs = float(M_PI) * (osc->feedback - 0.5f) * 0.8f;
		transZOffs = (osc->alpha - 0.5f) * 80.f;

		glDisable(GL_DEPTH_TEST);
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		whiteQuad->draw();

		glm::mat4 baseTrans = glm::translate(glm::mat4(1.f),
				glm::vec3(0.f, 0.f, -kpCalib.distBeamerObjCenter * coordNorm + transZOffs))
		* glm::rotate(kpCalib.rotXY, rotXOffs, glm::vec3(1.f, 0.f, 0.f));

		//baseTrans = beamerMatr * baseTrans;

		texShaderDist->begin();
		texShaderDist->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
		texShaderDist->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
		texShaderDist->setUniform1f("zObjPos", -kpCalib.distBeamerObjCenter * coordNorm + transZOffs);

		texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
		texShaderDist->setUniform1i("tex", 0);
		texShaderDist->setUniform1f("alpha", 0.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());

		chessQA->draw();

		break;
	}
	case CHECK_LOADED :
	{
		glDisable(GL_DEPTH_TEST);
		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		whiteQuad->draw();

		glm::mat4 baseTrans =
				//beamerMatr *
				glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, kpCalib.distBeamerObjCenter))
		* kpCalib.rotXY;

		texShaderDist->begin();
		texShaderDist->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
		texShaderDist->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
		texShaderDist->setUniform1f("zObjPos", kpCalib.distBeamerObjCenter);

		texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);

		texShaderDist->setUniform1i("tex", 0);
		texShaderDist->setUniform1f("alpha", 0.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());

		chessQA->draw();

		break;
	}
	case DRAW :
	{
		float scfact = (osc->blurFdbk - 0.5f) * 2.f;

		//                modelMatr = glm::rotate(glm::mat4(1.f),
		//                                        0.f,
		//                                        (osc->rotYAxis - 0.5f) * float(M_PI) * 2.f,
		//                                        glm::vec3(0.f, 1.f, 0.f));

		// blurFboAlpha und blurFdbk auf 0.5 (also 0)
		glm::mat4 baseTrans = glm::translate(glm::mat4(1.f), glm::vec3(scfact,
				(osc->blurFboAlpha - 0.5f) * 10.f,
				kpCalib.distBeamerObjCenter))
		* glm::rotate(kpCalib.rotXY, float(M_PI) * 0.5f, glm::vec3(1.f, 0.f, 0.f))
		* modelMatr
		* glm::scale(glm::mat4(1.f),
				glm::vec3( 1.f + (osc->zoom -0.5f),
						1.f + (osc->rotYAxis - 0.5f),
						1.f + (osc->totalBrightness - 1.f)))
		* scaleToRealDimMatr;
		//                                    * (*aImport->getNormCubeAndCenterMatr());

		glm::mat4 transMatr = beamerMatr * baseTrans;
		glm::mat3 normalMatr = glm::mat3( glm::transpose( glm::inverse( baseTrans ) ) );

		glm::mat4 viewMat = glm::mat4(1.f);
		glm::mat4 invViewMatr = glm::inverse( viewMat );

		// std shader setup
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		/*
		if(!checkCross)
		{

			for(int i=2;i<int(busNode->getChildren());i++)
			{
				shdr->begin();
				shdr->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
				shdr->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
				shdr->setUniformMatrix3fv("m_normal", &normalMatr[0][0] );

				shdr->setUniform3fv("lightPos", &lightPos[0]);
				shdr->setUniform1f("shininess", shininess);
				shdr->setUniform4fv("specular", &specular[0]);
				shdr->setUniform4fv("diffuse", &diffuse[0]);
				shdr->setUniform4fv("ambient", &sceneColor[0]);

				shdr->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
				shdr->setUniform1f("zObjPos", kpCalib.distBeamerObjCenter);
				shdr->setUniform1f("numAudioSamples", float(pa->waveData.frames_per_buffer));

				glActiveTexture(GL_TEXTURE0);
				shdr->setUniform1i("tex_diffuse0", 0);
				glBindTexture(GL_TEXTURE_2D, switchTextures[int(blendTextStart)].getId());

				glActiveTexture(GL_TEXTURE1);
				shdr->setUniform1i("tex_diffuse1", 1);
				glBindTexture(GL_TEXTURE_2D, switchTextures[int(blendTextEnd)].getId());

				shdr->setUniform1f("blend", blendText->getPercentage());
				shdr->setUniform1i("normalTex", 2);
				normalTexture->bind(2);

				shdr->setUniform1i("litsphereTexture", 3);
				litsphereTexture->bind(3);

				shdr->setUniform1i("cubeMap", 4);
				cubeTex->bind(4);

				glActiveTexture(GL_TEXTURE5);
				shdr->setUniform1i("audio_tex", 5);
				audioBuf->bindTex();

				shdr->setUniform1f("blendVec", 0.4f);
				//                    shdr->setUniform1f("blendVec", osc->zoom);

				if(i==6)
					shdr->setUniform1i("drawOszi", 1);
				else
					shdr->setUniform1i("drawOszi", 0);

				// metal
				if (i==3 || i==5 || i==10)
				{
					shdrSet[i]->begin();
					shdrSet[i]->setUniformMatrix4fv("viewMatrix", &viewMat[0][0] ); // world to view transformation
					shdrSet[i]->setUniformMatrix4fv("viewMatrixInverse", &invViewMatr[0][0] );
					shdrSet[i]->setUniformMatrix4fv("modelMatrix", &baseTrans[0][0] ); // world to view transformation
					shdrSet[i]->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
					shdrSet[i]->setUniformMatrix4fv("m_pvm",  &baseTrans[0][0]);
					shdrSet[i]->setUniformMatrix3fv("m_normal", &normalMatr[0][0]);
					shdrSet[i]->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
					shdrSet[i]->setUniform1f("zObjPos", kpCalib.distBeamerObjCenter);

					shdrSet[i]->setUniform1i("cubeMap", 0);
					cubeTex->bind(0);
				}

				// glass
				if (i==7)
				{
					//glm::vec3 lightPos = glm::vec3(-1.0, 1.0, 4.0);
					glm::vec3 baseCol = glm::vec3(1.0, 1.0, 1.0);

					shdrSet[i]->begin();
					shdrSet[i]->setUniform3fv("LightPos", &lightPos[0]);
					shdrSet[i]->setUniform3fv("BaseColor", &baseCol[0]);
					shdrSet[i]->setUniform1f("Depth", 0.1f);
					shdrSet[i]->setUniform1f("MixRatio", 1.f);
					shdrSet[i]->setUniform1f("alpha", 0.9f);

					shdrSet[i]->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
					shdrSet[i]->setUniformMatrix4fv("m_vm",  &baseTrans[0][0]);
					shdrSet[i]->setUniformMatrix3fv("m_normal", &normalMatr[0][0]);
					shdrSet[i]->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
					shdrSet[i]->setUniform1f("zObjPos", kpCalib.distBeamerObjCenter);

					shdrSet[i]->setUniform1i("EnvMap", 0);
					cubeTex->bind(0);

					shdrSet[i]->setUniform1i("RefractionMap", 1);
					cubeTex->bind(1);
				}

				// luces
				if (i==9)
				{
					glActiveTexture(GL_TEXTURE0);
					shdr->setUniform1i("tex_diffuse0", 0);
					lucesTex->bind();

					shdr->setUniform1f("blend", 0.f);
				}

				aImport->sendMeshMatToShdr(i, shdrSet[i]);
				aImport->drawMeshNoShdr(i, GL_TRIANGLES);
			}
		} else
		{
			for(int i=0;i<2;i++)
			{
				shdr->begin();
				shdr->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
				shdr->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
				shdr->setUniformMatrix3fv("m_normal", &normalMatr[0][0] );

				shdr->setUniform3fv("lightPos", &lightPos[0]);
				shdr->setUniform1f("shininess", shininess);
				shdr->setUniform4fv("specular", &specular[0]);
				shdr->setUniform4fv("diffuse", &diffuse[0]);
				shdr->setUniform4fv("ambient", &sceneColor[0]);

				shdr->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
				shdr->setUniform1f("zObjPos", kpCalib.distBeamerObjCenter);
				shdr->setUniform1f("numAudioSamples", float(pa->waveData.frames_per_buffer));

				aImport->sendMeshMatToShdr(i, shdrSet[i]);
				aImport->drawMeshNoShdr(i, GL_TRIANGLES);
			}
		}

		//                aImport->sendMeshMatToShdr(actMesh, shdr);
		//                aImport->drawMeshNoShdr(actMesh, GL_TRIANGLES);
*/
		glDisable(GL_BLEND);

		break;
	}
	case TESTDIST:
	{
		float objDist = -77.f;
		glm::mat4 baseTrans = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, objDist));

		texShaderDist->begin();
		texShaderDist->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
		texShaderDist->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
		texShaderDist->setUniform1f("zObjPos", -118.f);

		cout <<  std::tan(kpCalib.beamerLookAtAngle) * (objDist - (-118.f)) << endl;

		texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAtAngle);
		//texShaderDist->setUniform1f("lookAngle", osc->alpha * 0.5f);

		texShaderDist->setUniform1i("tex", 0);
		texShaderDist->setUniform1f("alpha", 0.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());

		chessQA->draw();

		break;
	}
	default:
		break;
	}
}

//----------------------------------------------------

void SNRotatingObjMapping::update(double time, double dt)
{
	actTime = time;
	blendText->update(time);

	if(!inited) initDyn();

	// --- CALIBRATE

	if (inited && actDrawMode == CALIBRATE && useKinect)
	{
		// color Frame Operations
		if (frameNr != kin->getColFrameNr())
		{
			frameNr = kin->getColFrameNr();

			// take undistorted kin Color Image, since for this the Kinect internal
			view.data = kin->getActColorImg();

			// blur depth values
			kin->lockDepthMutex();
			kinDepthTex->bind();
			glTexSubImage2D(GL_TEXTURE_2D,             // target
					0,                          // First mipmap level
					0, 0,                       // x and y offset
					kin->getDepthWidth(),
					kin->getDepthHeight(),
					GL_RED,
					GL_UNSIGNED_SHORT,
					kin->getDepthFrame()->getData());
			kin->unlockDepthMutex();

			fastBlur->proc(kinDepthTex->getId());
			fastBlur->downloadData();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


			// get chessboard
			bool found = cv::findChessboardCorners(view, kpCalib.boardSize, pointbuf,
					cv::CALIB_CB_ADAPTIVE_THRESH |
					//cv::CALIB_CB_FAST_CHECK |
					cv::CALIB_CB_NORMALIZE_IMAGE);
			if (found)
			{
				cout << "fond chessb" << endl;
				chessRotMatr = getRotReal(kpCalib);
				cout << endl;
			}
		}
	}

	//

	// handle input to send to arduino
	if(udp->gotNew)
	{
		printf("got new upd command:\n");
		std::cout << udp->getCmdStr()->c_str() << std::endl;

		string cmdS = *udp->getCmdStr();
		char* cmd3Char = new char[3];
		std::strncpy(cmd3Char, cmdS.c_str(), 3);

		if(std::strcmp(udp->getCmdStr()->c_str(), "stop") == 0)
			if (arduino) arduino->send('s');

		if(std::strcmp(udp->getCmdStr()->c_str(), "forw") == 0)
			if (arduino) arduino->send('f');

		if(std::strcmp(udp->getCmdStr()->c_str(), "revr") == 0)
			if (arduino) arduino->send('r');

		if(std::strcmp(udp->getCmdStr()->c_str(), "minu") == 0)
			if (arduino) arduino->send('m');

		if(std::strcmp(udp->getCmdStr()->c_str(), "plus") == 0)
			if (arduino) arduino->send('p');

		if(std::strcmp(cmd3Char, "tex") == 0)
		{
			int texNr = std::atoi( &(cmdS.c_str()[3]) );
			int useTex = 0;
			switch(texNr)
			{
			case 1: useTex = 0; break;  // surf
			case 2: useTex = 6; break;  // scooby doo
			case 3: useTex = 1; break;  // metal
			case 4: useTex = 3; break;  // lumina
			case 5: useTex = 2; break;  // graffiti
			case 6: useTex = 4; break;  // hippie
			case 7: useTex = 5; break;  // rock
			}

			blendTextStart = float(actText);
			blendTextEnd = useTex % int(tex_paths.size());
			blendText->start(blendTextStart, blendTextEnd, 1.0, actTime, false);
			blendText->setEndFunc([this]() { return this->swapBlendTex(&blendTextStart, &blendTextEnd); });
			blendText->reset();
		}

		udp->gotNew = false;
	}

	// check feedback from arduino
	//if (arduino && arduino->getActSpeed() != motorActSpeed)
	if (arduino)
		motorActSpeed = arduino->getActSpeed();

	// --- get absolute motor position in steps and correct the 16 bit offset ---
	if (arduino)
		motorAbsPos = arduino->getActPos();

	// wenn motor über die 65535 schwelle springt
	if( std::abs(motorAbsPos - motorLastAbsPos) > (motorMaxAbsPos /2) )
	{
		// printf("schwelle: motorAbsPos: %d motorLastAbsPos: %d std::abs(motorAbsPos - motorLastAbsPos): %d\n", motorAbsPos, motorLastAbsPos, std::abs(motorAbsPos - motorLastAbsPos));

		// motor 16 bit schwelle in positiver richtung überschritten
		if (motorAbsPos < motorLastAbsPos) {
			motorAbsPos16bitCrossed++;
		} else {
			motorAbsPos16bitCrossed--;
		}
	}

	motorAbsPosOffs = (motorMaxAbsPos * ((motorMaxSteps + motorAbsPos16bitCrossed) % motorMaxSteps)) % motorMaxSteps;
	motorLastAbsPos = motorAbsPos;

	// continue with the correct motorPosition between 0 and motorMaxSteps
	if (arduino)
		motorActPos = (arduino->getActPos() + motorAbsPosOffs) % motorMaxSteps;

	if (arduino)
	{
		float medFact = 3.f;
		float fMotorPos = float(motorActPos) / float(motorMaxSteps);
		float fMotorVel = (motorActSpeed / float(motorMaxSteps)) * 0.01f;

		if (motorActPos != motorActLastPos)
		{
			if (std::abs(motorActPos - motorActLastPos) < (motorMaxSteps / 2))
			{
				if (motorActPos > motorActLastPos) dir = 1.f; else dir = -1.f;
			} else
			{
				if (motorActPos < motorActLastPos) dir = 1.f; else dir = -1.f;
			}
		}

		float newRotPos = (1.f - std::fmod(fMotorPos + fMotorVel * osc->speed * dir, 1.0))
                                		* float(M_PI) * 2.f;

		if(std::fabs(rotPos - newRotPos) > M_PI && rotPos < newRotPos)
		{
			rotPos = ((rotPos + M_PI * 2.f) * medFact + newRotPos) / (medFact + 1.f);

			// values switch from 0 to TWO_M_PI
		} else if(std::fabs(rotPos - newRotPos) > M_PI && rotPos > newRotPos)
		{
			rotPos = (((rotPos - M_PI * 2.f) * medFact) + newRotPos) / (medFact + 1.f);
		} else
		{
			rotPos = ( rotPos * medFact + newRotPos ) / (medFact + 1.f);
		}

		rotPos = std::fmod(rotPos, M_PI * 2.f);

		modelMatr = glm::rotate(glm::mat4(1.f), rotPos, glm::vec3(0.f, 1.f, 0.f));
		motorActLastPos = motorActPos;
	}

	//   update audio, upload to texBuf

	if (lastAudioFrameNr != pa->getFrameNr())
	{
		GLfloat* ptr = (GLfloat*) audioBuf->getMapBuffer();

		for (auto i=0;i<pa->getFramesPerBuffer();i++)
		{
			//ptr[i] = pa->getPllAtPos(0, static_cast<float>(i) / static_cast<float>(pa->waveData.frames_per_buffer -1));
			ptr[i] = pa->getSampDataAtPos(0, static_cast<float>(i) / static_cast<float>(pa->getFramesPerBuffer() -1));
		}

		//ptr[i] = pa->getPllAtPos(0, static_cast<float>(i) / static_cast<float>(pa->waveData.frames_per_buffer -1));

		audioBuf->unMapBuffer();

		lastAudioFrameNr = pa->getFrameNr();
	}

	/*
        // auto switch textures
        if(time - lastSwitchTime > switchTexInt)
        {
            blendTextStart = float(actText);
            blendTextEnd = (actText +1) % int(tex_paths.size());
            blendText->start(blendTextStart, blendTextEnd, 1.0, actTime);
            blendText->setEndFunc([this]() { return this->swapBlendTex(&blendTextStart, &blendTextEnd); });
            blendText->reset();
            lastSwitchTime = time;
        }
	 */
}

//----------------------------------------------------

std::string SNRotatingObjMapping::execCmd(char* cmd)
{
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return "ERROR";
	char buffer[128];
	std::string result = "";
	while(!feof(pipe)) {
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	return result;
}

//----------------------------------------------------

glm::mat4 SNRotatingObjMapping::getRotReal(kpCalibData& kp)
{
	glm::mat4 out;
	glm::vec3 midPoint;
	std::vector<glm::vec3> realWorldCoord;

	int ind = 0;
	for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
	{
		realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
		getKinRealWorldCoord(realWorldCoord.back());

		if ((int)rwCoord.size() < (int)pointbuf.size())
			rwCoord.push_back(new Median<glm::vec3>(10.f));
		rwCoord[ind]->update(realWorldCoord.back());

		ind++;
	}

	midPoint = getChessMidPoint(realWorldCoord, kp);
	cout << glm::to_string(midPoint) << endl;

	if(kp.distBeamerObjCenter != 0.f)
		kp.distBeamerObjCenter = (kp.distBeamerObjCenter * 10.f + (midPoint.z - kpCalib.camBeamerRealOffs.z)) / 11.f;
	else
		kp.distBeamerObjCenter = midPoint.z - kpCalib.camBeamerRealOffs.z;

	cout << "distBeamerObjCenter: " << kp.distBeamerObjCenter << endl;

	cout << "BeamerObjReal: " << std::sqrt(std::pow(midPoint.y + kpCalib.camBeamerRealOffs.y, 2.f)
	+ std::pow(kp.distBeamerObjCenter, 2.f)) << endl;

	// brechne die normale der wand / chessboard zur Kinect Ebene
	kp.rotXY = calcChessRotMed(rwCoord, kp);

	return out;
}

//----------------------------------------------------

// rotations resulting from normals vary about 1 degree. this is caused by the kinect...
glm::mat4 SNRotatingObjMapping::calcChessRotMed(std::vector< Median<glm::vec3>* >& _realWorldCoord, kpCalibData& kp)
{
	glm::mat4 outMat = glm::mat4(1.f);
	vector< glm::vec3 > normals;

	// calculate rotation, by calculating normals for all corners
	// through the conversion in realworld coordinates the perspective
	// distortion is automatically corrected, so the result is corrected
	for (short y=1;y<kp.boardSize.height;y++)
	{
		for (short x=0;x<kp.boardSize.width -1;x++)
		{
			glm::vec3 act = _realWorldCoord[y * kp.boardSize.width + x]->get();
			glm::vec3 upper = _realWorldCoord[(y-1) * kp.boardSize.width + x]->get();
			glm::vec3 right = _realWorldCoord[y * kp.boardSize.width + x+1]->get();

			glm::vec3 upVec = upper - act;
			glm::vec3 rightVec = right - act;

			normals.push_back( glm::normalize ( glm::cross( glm::normalize(upVec), glm::normalize(rightVec) ) ) );
		}
	}

	// calculate medium normal
	glm::vec3 outNorm = glm::vec3(0.f, 0.f, 0.f);
	for (std::vector<glm::vec3>::iterator it = normals.begin(); it != normals.end(); ++it)
		if (!std::isnan((*it).x) && !std::isnan((*it).y) && !std::isnan((*it).z))
			outNorm += (*it);

	if ((int) normals.size() > 0)
	{
		outNorm /= (float)normals.size();
		outNorm = glm::normalize(outNorm);

		cbNormal->update(outNorm);
		kp.chessNormal = cbNormal->get();

		// only x-rot
		kp.chessNormal.z = -1.f;
		kp.chessNormal.x = 0.f;
		cout << "chessNormal: " << glm::to_string(kp.chessNormal) << endl;

		// quaternion
		glm::quat rot = RotationBetweenVectors(kp.chessNormal, glm::vec3(0.f, 0.f, -1.f));
		outMat = glm::mat4(rot);
		kp.rotations = glm::eulerAngles(rot);
		cout << "kp.rotations: " << glm::to_string(kp.rotations) << endl;
	}

	return outMat;
}

//----------------------------------------------------

void SNRotatingObjMapping::reCheckRot(kpCalibData& kp)
{
	std::vector<glm::vec3> realWorldCoord;

	glm::mat4 rot = glm::rotate(glm::mat4(1.f), kp.rotations.x, glm::vec3(1.f, 0.f, 0.f))
	* glm::rotate(glm::mat4(1.f), kp.rotations.y, glm::vec3(0.f, 1.f, 0.f))
	* glm::rotate(glm::mat4(1.f), kp.rotations.z, glm::vec3(0.f, 0.f, 1.f));

	for (std::vector<cv::Point2f>::iterator it = pointbuf.begin(); it != pointbuf.end(); ++it)
	{
		realWorldCoord.push_back( glm::vec3((*it).x, (*it).y, 0.f) );
		getKinRealWorldCoord(realWorldCoord.back());
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
	//cout << "final norm: " << glm::to_string(medNormal) << endl;

	glm::quat rotQ = RotationBetweenVectors(medNormal, glm::vec3(0.f, 0.f, -1.f));
	glm::vec3 rotAngles = glm::eulerAngles(rotQ);
	//cout << "rotAngles: " << glm::to_string(rotAngles) << endl;

	// apply found correction
	kp.rotations += rotAngles;
}

//----------------------------------------------------

glm::vec3 SNRotatingObjMapping::getChessMidPoint(std::vector<glm::vec3>& _realWorldCoord, kpCalibData& kp)
{
	glm::vec3 midPoint = glm::vec3(0.f);

	// calculate midpoints in the vertical axis
	// the outer most corners and iterate to the center
	vector<glm::vec3> hMidsEnd;

	for (short x=0;x<kp.boardSize.width;x++)
	{
		vector<glm::vec3> hMids;
		for (short h=0;h<kp.boardSize.height/2;h++)
		{
			glm::vec3 midP = _realWorldCoord[x + (h * kp.boardSize.width)]
											 + _realWorldCoord[x + (kp.boardSize.height -h -1) * kp.boardSize.width];
			hMids.push_back(midP * 0.5f);
		}

		// get mediums
		glm::vec3 medMidPH = glm::vec3(0.f, 0.f, 0.f);
		for (short h=0;h<kp.boardSize.height/2;h++)
			medMidPH += hMids[h];

		hMidsEnd.push_back(medMidPH / static_cast<float>(kp.boardSize.height/2));
	}

	vector<glm::vec3> vMids;
	int hMidsEndSizeHalf = static_cast<int>(hMidsEnd.size())/2;

	// iterate over all vMids and calc mediums for each pair
	for (int x=0;x<hMidsEndSizeHalf;x++)
	{
		glm::vec3 midP = hMidsEnd[x] + hMidsEnd[static_cast<int>(hMidsEnd.size()) -1 -x];
		vMids.push_back(midP * 0.5f);
	}

	midPoint = glm::vec3(0.f, 0.f, 0.f);
	for (short i=0;i<static_cast<short>(vMids.size());i++)
		midPoint += vMids[i];

	if(static_cast<int>(vMids.size()) > 0)
		midPoint /= static_cast<float>(vMids.size());

	return midPoint;
}

//----------------------------------------------------

void SNRotatingObjMapping::getKinRealWorldCoord(glm::vec3& inCoord)
{
	double depthScale = 1.0;
	float resX = static_cast<float>(kin->getDepthWidth());
	float resY = static_cast<float>(kin->getDepthHeight());

	// in case depth and color stream have different sizes
	inCoord.x = inCoord.x / static_cast<float>(kin->getColorWidth()) * static_cast<float>(kin->getDepthWidth());
	inCoord.y = inCoord.y / static_cast<float>(kin->getColorHeight()) * static_cast<float>(kin->getDepthHeight());

	//kin->lockDepthMutex();
	//const openni::DepthPixel* pDepthPix = (const openni::DepthPixel*)kin->getDepthFrame()->getData();
	const openni::DepthPixel* blurPix = (const openni::DepthPixel*)fastBlur->getDataR16();

	int rowSize = kin->getDepthFrame()->getStrideInBytes() / sizeof(openni::DepthPixel);
	//pDepthPix += rowSize * static_cast<int>(inCoord.y);
	//pDepthPix += static_cast<int>(inCoord.x);

	blurPix += rowSize * static_cast<int>(inCoord.y);
	blurPix += static_cast<int>(inCoord.x);

	//kin->unlockDepthMutex();

	// asus xtion tends to measure lower depth with increasing distance
	// experimental correction
	//        depthScale = 1.0 + std::pow(static_cast<double>(*blurPix) * 0.00032, 5.3);

	double multDiff = static_cast<double>(*blurPix) - 950.0;
	double upperCorr = std::fmax(multDiff, 0) * 0.02;
	double scaledDepth = static_cast<double>(*blurPix) + (multDiff * 0.0102) + upperCorr;


	glm::vec2 fov = glm::vec2(kpCalib.kinFovX, kpCalib.kinFovY);

	double normalizedX = inCoord.x / resX - .5f;
	double normalizedY = .5f - inCoord.y / resY;

	double xzFactor = std::tan(fov.x * 0.5f) * 2.f;  // stimmt noch nicht... wieso?
			double yzFactor = std::tan(fov.y * 0.5f) * 2.f;  // stimmt!!!

			inCoord.x = static_cast<float>(normalizedX * scaledDepth * xzFactor);
			inCoord.y = static_cast<float>(normalizedY * scaledDepth * yzFactor);
			inCoord.z = static_cast<float>(scaledDepth);
}

//----------------------------------------------------

void SNRotatingObjMapping::loadCalib(std::string _filename, kpCalibData& kp)
{
	cv::Mat loadMat;

	printf("loading calibration \n");

	cv::FileStorage fs(_filename, cv::FileStorage::READ);

	if ( fs.isOpened() )
	{
		fs["rotXY"] >> loadMat;
		kpCalib.rotXY = (cvMatToGlm(loadMat));

		fs["rotZ"] >> loadMat;
		kpCalib.rotZ = (cvMatToGlm(loadMat));

		fs["trans"] >> loadMat;
		kpCalib.trans = (cvMatToGlm(loadMat));

		fs["beamerFovY"] >> kp.beamerFovY;
		fs["kinFovX"] >> kp.kinFovX;
		fs["kinFovY"] >> kp.kinFovY;
		fs["distBeamerObjCenter"] >> kp.distBeamerObjCenter;
		fs["beamerLookAtAngle"] >> kp.beamerLookAtAngle;

		cv::Mat loadVec;
		fs["camBeamerRealOffs"] >> loadVec;
		kp.camBeamerRealOffs = glm::vec3(loadVec.at<float>(0),
				loadVec.at<float>(1),
				loadVec.at<float>(2));

		fs["chessNormal"] >> loadVec;
		kp.chessNormal = glm::vec3(loadVec.at<float>(0),
				loadVec.at<float>(1),
				loadVec.at<float>(2));

		fs["rotations"] >> loadVec;
		kp.rotations = glm::vec3(loadVec.at<float>(0),
				loadVec.at<float>(1),
				loadVec.at<float>(2));

		calibLoaded = true;
	} else
	{
		printf("couldn´t open file\n");
	}
}

//----------------------------------------------------

void SNRotatingObjMapping::saveCalib(std::string _filename, kpCalibData& kp)
{
	printf("saving camera to beamer calib \n");

	cv::FileStorage fs( _filename, cv::FileStorage::WRITE );

	cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
	glmVec.at<float>(0) = kp.camBeamerRealOffs.x;
	glmVec.at<float>(1) = kp.camBeamerRealOffs.y;
	glmVec.at<float>(2) = kp.camBeamerRealOffs.z;
	fs << "camBeamerRealOffs" << glmVec;

	glmVec.at<float>(0) = kp.chessNormal.x;
	glmVec.at<float>(1) = kp.chessNormal.y;
	glmVec.at<float>(2) = kp.chessNormal.z;
	fs << "chessNormal" << glmVec;

	glmVec.at<float>(0) = kp.rotations.x;
	glmVec.at<float>(1) = kp.rotations.y;
	glmVec.at<float>(2) = kp.rotations.z;
	fs << "rotations" << glmVec;

	glm::mat4 rotTemp = glm::rotate(kpCalib.rotXY, rotXOffs, glm::vec3(1.f, 0.f, 0.f));
	fs << "rotXY" << glmToCvMat(rotTemp);
	fs << "rotZ" << glmToCvMat(kp.rotZ);
	fs << "trans" << glmToCvMat(kp.trans);

	fs << "beamerFovY" << kp.beamerFovY;
	fs << "kinFovX" << kp.kinFovX;
	fs << "kinFovY" << kp.kinFovY;
	fs << "distBeamerObjCenter" << -kp.distBeamerObjCenter * coordNorm + transZOffs;
	fs << "beamerLookAtAngle" << kp.beamerLookAtAngle;
}

//----------------------------------------------------

glm::mat4 SNRotatingObjMapping::cvMatToGlm(cv::Mat& _mat)
{
	glm::mat4 out = glm::mat4(1.f);
	for (short j=0;j<4;j++)
		for (short i=0;i<4;i++)
			out[i][j] = _mat.at<float>(j*4 + i);

	return out;
}

//----------------------------------------------------

cv::Mat SNRotatingObjMapping::glmToCvMat(glm::mat4& mat)
{
	cv::Mat out = cv::Mat(4, 4, CV_32F);
	for (short j=0;j<4;j++)
		for (short i=0;i<4;i++)
			out.at<float>(j*4 + i) = mat[i][j];

	return out;
}

//----------------------------------------------------

void SNRotatingObjMapping::swapBlendTex(float* startInd, float* endInd)
{
	*startInd = *endInd;
	actText = *endInd;
}

//----------------------------------------------------

void SNRotatingObjMapping::addLitSphereShader()
{
	std::string shdr_Header = "#version 410\n#pragma optimize(on)\n";

	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec3 normal;
	layout( location = 2 ) in vec2 texCoord;
	layout( location = 3 ) in vec4 color;

	uniform mat4 m_pvm;
	uniform mat4 persp_matr;
	uniform mat3 m_normal;
	uniform float lookAngle;
	uniform float zObjPos;

	out VS_FS_VERTEX
	{
		vec4 pos;
		vec3 normal;
		vec4 color;
		vec2 tex_coord;
	} vertex_out;

	vec4 pos;

	void main ()
	{
		vertex_out.color = color;
		// transform the normal, without perspective, and normalize it
		vertex_out.normal = normalize(m_normal * normal);
		//Normal = mat3(transpose(inverse(model))) * normal;
		vertex_out.tex_coord = texCoord;

		pos = m_pvm * position;
		pos.y += tan(lookAngle) * (pos.z - zObjPos);

		vertex_out.pos = position;

		gl_Position = persp_matr * pos;
	});

	vert = "// mapping litsphere, vert\n"+shdr_Header+vert;

	std::string frag = STRINGIFY(layout (location = 0) out vec4 color;
	in VS_FS_VERTEX
	{
		vec4 pos;
		vec3 normal;
		vec4 color;
		vec2 tex_coord;
	} vertex_in;

	uniform sampler2D tex_diffuse0;
	uniform sampler2D tex_diffuse1;
	uniform sampler2D litsphereTexture;
	uniform sampler2D normalTex;
	uniform samplerCube cubeMap;
	uniform samplerBuffer audio_tex;

	uniform vec3 lightPos;
	uniform float shininess;
	uniform vec4 specular;
	uniform vec4 diffuse;
	uniform vec4 ambient;
	uniform vec4 sceneColor;

	uniform float numAudioSamples;
	uniform float blend;
	uniform float blendVec;
	uniform int drawOszi;

	vec4 tex0;
	vec4 tex1;
	vec4 blendCol;
	vec4 envColor;
	vec4 audioVal;

	void main()
	{
		tex0 = texture(tex_diffuse0, vertex_in.tex_coord);
		tex1 = texture(tex_diffuse1, vertex_in.tex_coord);
		blendCol = tex0 * (1.0 - blend) + tex1 * blend;

		//vec4 normal = texture( normalTex, vertex_in.tex_coord );

		//vec3 eyeNormal = normal.xyz;
		vec3 eyeNormal = vertex_in.normal.xyz;
		vec3 L = normalize(lightPos - vertex_in.pos.xyz);
		vec3 E = normalize(-vertex_in.pos.xyz); // we are in Eye Coordinates, so EyePos is (0,0,0)
		vec3 R = normalize(-reflect(L, eyeNormal));

		//calculate Ambient Term:
		vec4 Iamb = ambient;

		//calculate Diffuse Term:
		//    vec4 Idiff = blendCol * max(dot(eyeNormal, L), 0.0);
		vec4 Idiff = diffuse * max(dot(eyeNormal, L), 0.0);
		Idiff = clamp(Idiff, 0.0, 1.0);

		// calculate Specular Term:
		vec4 Ispec = specular * pow(max(dot(R, E), 0.0),0.3 * shininess);
		Ispec = clamp(Ispec, 0.0, 1.0);

		vec4 shading = texture(litsphereTexture, (-vertex_in.normal.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));
		vec4 shading2 = texture(litsphereTexture, (lightPos.xy + vec2(1.0, 1.0)) * vec2(0.5, 0.5));

		shading = shading * blendVec + shading2 * (1.0 - blendVec);

		envColor = texture(cubeMap, R);

		// get audio_tex render oscilloscope
		audioVal = vec4(0.0);

		if(drawOszi == 1)
		{
			audioVal = texelFetch(audio_tex,
					int((vertex_in.tex_coord.y - 0.33) * 0.48 * numAudioSamples));
			audioVal.r = audioVal.r * 0.5 + 0.5;
			audioVal.r = abs( ((vertex_in.tex_coord.x - 0.016) / 0.171) - audioVal.r) < 0.02 ? 1.0 : 0.0
					+ abs( ((vertex_in.tex_coord.x - 0.646) / 0.171) - audioVal.r) < 0.02 ? 1.0 : 0.0;
			audioVal.r = vertex_in.tex_coord.y > 0.338 ? audioVal.r : 0.0;
			//                                        audioVal.r = vertex_in.tex_coord.y > 0.03 ? audioVal.r : 0.0;

			audioVal = vec4(audioVal.r);
		}

		color = ((Iamb + Idiff + Ispec + envColor * 0.5 * sceneColor) * shading ) * blendCol * 2.0
				+ audioVal;// + color;
		//    color = envColor;
		//    color *= vertex_in.color.a;
	});

	frag = "// mapping litsphere shader, frag\n"+shdr_Header+frag;

	shdr = shCol->addCheckShaderText("texBlendShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNRotatingObjMapping::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if(mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_1 : actDrawMode = CHESSBOARD;
			printf("CHESSBOARD\n");
			break;
			case GLFW_KEY_2 : actDrawMode = CALIBRATE;
			printf("CALIBRATE \n");
			break;
			case GLFW_KEY_3 : actDrawMode = CHECK;
			printf("check\n");
			calibLoaded = false;
			break;
			case GLFW_KEY_4 : actDrawMode = CHECK_LOADED;
			if(!calibLoaded) loadCalib(calibFilePath, kpCalib);
			printf("check loaded\n");
			break;
			case GLFW_KEY_5 : actDrawMode = DRAW;
			printf("draw\n");
			break;
			case GLFW_KEY_6 : checkCross = !checkCross;
			printf("checkCross: %d\n", checkCross);
			break;
			case GLFW_KEY_S :
				printf("save Settings\n");
				saveCalib(calibFilePath, kpCalib);
				break;
			}
		} else
		{
			switch (key)
			{
			case GLFW_KEY_1 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.f);
				break;
			case GLFW_KEY_2 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.125f);
				break;
			case GLFW_KEY_3 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.25f);
				break;
			case GLFW_KEY_4 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.375f);
				break;
			case GLFW_KEY_5 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.5f);
				break;
			case GLFW_KEY_6 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.625f);
				break;
			case GLFW_KEY_7 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.75f);
				break;
			case GLFW_KEY_8 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 0.875f);
				break;
			case GLFW_KEY_9 :
				if(arduino) arduino->send('v', float(motorMaxSteps) * 1.f);
				break;
			case GLFW_KEY_P :
				if(arduino) arduino->send('p');
				break;
			case GLFW_KEY_M :
				if(arduino) arduino->send('m');
				break;
			case GLFW_KEY_S :
				if(arduino) arduino->send('s');
				break;
			case GLFW_KEY_F :
				if(arduino) arduino->send('f');
				printf("forward \n");
				break;
			case GLFW_KEY_R :
				if(arduino) arduino->send('r');
				break;
			case GLFW_KEY_X :
				if(arduino) arduino->send('x');
				modelMatr = glm::rotate(glm::mat4(1.f), 0.f, glm::vec3(0.f, 1.f, 0.f));
				break;
			case GLFW_KEY_UP :
				actMesh++;
				printf("actMesh: %d \n", actMesh);
				break;
			case GLFW_KEY_DOWN :
				actMesh--;
				printf("actMesh: %d \n", actMesh);
				break;
			case GLFW_KEY_RIGHT :
				printf("right \n");
				blendTextStart = float(actText);
				blendTextEnd = (actText +1) % int(tex_paths.size());
				blendText->start(blendTextStart, blendTextEnd, 1.0, actTime, false);
				blendText->setEndFunc([this]() { return this->swapBlendTex(&blendTextStart, &blendTextEnd); });
				blendText->reset();
				break;
			case GLFW_KEY_LEFT :
				printf("left \n");
				blendTextStart = float(actText);
				blendTextEnd = (actText -1 + tex_paths.size()) % int(tex_paths.size());
				blendText->start(blendTextStart, blendTextEnd, 1.0, actTime, false);
				blendText->setEndFunc([this]() { return this->swapBlendTex(&blendTextStart, &blendTextEnd); });
				blendText->reset();

				break;
			default:
				break;
			}
		}
	}
}

//----------------------------------------------------

void SNRotatingObjMapping::onCursor(GLFWwindow* window, double xpos, double ypos)
{}

//----------------------------------------------------

void SNRotatingObjMapping::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)ns;
	nanosleep(&tim, NULL);
}
}
