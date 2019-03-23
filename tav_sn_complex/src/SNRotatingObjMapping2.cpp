//
//  SNRotatingObjMapping2.cpp
//  tav_scene
//
//  Created by Sven Hahne on 4/10/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SNRotatingObjMapping2.h"

#define STRINGIFY(A) #A

using namespace std;

namespace tav
{
SNRotatingObjMapping2::SNRotatingObjMapping2(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    				SceneNode(_scd, _sceneArgs), near(0.1f), far(150.f), objDist(-135.5f),
					modelRealHeight(9.5f), motorMaxSteps(800), ardNrStepLoop(8),
					actDrawMode(KinectReproTools::CHECK_RAW_DEPTH), useArduino(false), inited(false), calibLoaded(false),
					useKinect(true),
					kinFovY(47.8f),                     // gemessen  47,8 Handbuch sagt 45 Grad, onistream sagt 45,642
					kinFovX(61.66f),                    // gemessen 62.2, vielleicht von Kinect zu Kinect unterschiedlich
					// Handbuch sagt 58 Grad, onistream sagt 58,59
					realBoardWidth(24.3f),              // reale Breite des Schachbretts (alle kästchen)
					coordNorm(0.1f),
					maxModMotorInc(20),
					actText(0), actMesh(0),
					motorAbsPos16bitCrossed(0),
					dir(1.f),
					kinDeviceNr(0)
{
	kin = static_cast<KinectInput*>(scd->kin);
	winMan = static_cast<GWindowManager*>(scd->winMan);
	osc = static_cast<OSCData*>(scd->osc);
	shCol = static_cast<ShaderCollector*>(scd->shaderCollector);

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
	tex_paths.push_back((*scd->dataPath)+"textures/vegas/TexCombi_Scobi.jpg"); // 6

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

	//  Shaders

	std::cout << "load litsphere shader " << std::endl;
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

	shdrSet.push_back(shdr); // 0 ?
	shdrSet.push_back(shdr); // 1 fenster gummi
	shdrSet.push_back(shdr); // 2 karroserie
	shdrSet.push_back(mirror); // 3 metal
	shdrSet.push_back(shdr); // 4 räder
	shdrSet.push_back(glass); // 5 fenster
	shdrSet.push_back(shdr); // 6 lichter
	shdrSet.push_back(shdr); // 7 dach
	shdrSet.push_back(shdr); // 8 dach stoff


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
	kpCalib.beamerLookAngle = 0.1974f;
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
	scaleToRealDimMatr = glm::scale(glm::mat4(1.f), glm::vec3(23.9f, modelRealHeight, 10.f) * 0.5f);

	// move in +y so that the floor of the van is at y=0
	modelMatr = glm::rotate(glm::mat4(1.f), 0.f, glm::vec3(0.f, 1.f, 0.f));

	calibFilePath = (*scd->dataPath)+"calib_cam/rotObjMap2.yml";


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

SNRotatingObjMapping2::~SNRotatingObjMapping2()
{
	delete shdr;
}

//----------------------------------------------------

void SNRotatingObjMapping2::initDyn()
{
	if(useKinect)
	{
		view = cv::Mat(kin->getColorHeight(), kin->getColorWidth(), CV_8UC3);
		kinDepthTex->allocate(kin->getDepthWidth(), kin->getDepthHeight(),
				GL_R16, GL_RED, GL_TEXTURE_2D);
		fastBlur = new FastBlur(shCol, kin->getDepthWidth(),
				kin->getDepthHeight(), GL_R16);

		kin->setImageRegistration(true);
		kinRepro = new KinectReproTools(winMan, kin, shCol, scd->screenWidth,
				scd->screenHeight, *(scd->dataPath), kinDeviceNr);
		kinRepro->loadCalib(calibFilePath);
	}
	inited = true;
}

//----------------------------------------------------

void SNRotatingObjMapping2::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	/*
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
            case KinectReproTools::CHECK_RAW_DEPTH :
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
            case KinectReproTools::CALIB_ROT_WIN :
            {
                texShader->begin();
                texShader->setIdentMatrix4fv("m_pvm");
                texShader->setUniform1i("tex", 0);
                glActiveTexture(GL_TEXTURE0);
                whiteQuad->draw();

                break;
            }
            case KinectReproTools::CHECK_ROT :
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
                                                     glm::vec3(0.f, 0.f, -kpCalib.distBeamerObj * coordNorm + transZOffs))
	 * glm::rotate(kpCalib.chessRotXY, rotXOffs, glm::vec3(1.f, 0.f, 0.f));

                //baseTrans = beamerMatr * baseTrans;

                texShaderDist->begin();
                texShaderDist->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
                texShaderDist->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
                texShaderDist->setUniform1f("zObjPos", -kpCalib.distBeamerObj * coordNorm + transZOffs);

                texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAngle);
                texShaderDist->setUniform1i("tex", 0);
                texShaderDist->setUniform1f("alpha", 0.f);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());

                chessQA->draw();

                break;
            }
            case KinectReproTools::CHECK_ROT_WARP_LOADED :
            {
                glDisable(GL_DEPTH_TEST);
                texShader->begin();
                texShader->setIdentMatrix4fv("m_pvm");
                texShader->setUniform1i("tex", 0);
                glActiveTexture(GL_TEXTURE0);
                whiteQuad->draw();

                glm::mat4 baseTrans =
                                    //beamerMatr *
                                    glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, kpCalib.distBeamerObj))
	 * kpCalib.chessRotXY;

                texShaderDist->begin();
                texShaderDist->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
                texShaderDist->setUniformMatrix4fv("m_pvm", &baseTrans[0][0]);
                texShaderDist->setUniform1f("zObjPos", kpCalib.distBeamerObj);

                texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAngle);

                texShaderDist->setUniform1i("tex", 0);
                texShaderDist->setUniform1f("alpha", 0.f);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, chessBoard2->getTexId());

                chessQA->draw();

                break;
            }
            case KinectReproTools::USE_ROT_WARP :
            {
                float scfact = (osc->blurFdbk - 0.5f) * 2.f;

//                modelMatr = glm::rotate(glm::mat4(1.f),
//                                        (osc->rotYAxis - 0.5f) * float(M_PI) * 2.f,
//                                        glm::vec3(0.f, 1.f, 0.f));

                // blurFboAlpha und blurFdbk auf 0.5 (also 0)
                glm::mat4 baseTrans = glm::translate(glm::mat4(1.f), glm::vec3(scfact,
                                                                              (osc->blurFboAlpha - 0.5f) * 10.f,
                                                                              kpCalib.distBeamerObj))
	 * glm::rotate(kpCalib.chessRotXY, float(M_PI) * 0.5f, glm::vec3(1.f, 0.f, 0.f))
	 * modelMatr
	 * glm::scale(glm::mat4(1.f),
                                                 glm::vec3( 1.f + (osc->zoom -0.5f),
                                                            1.f + (osc->rotYAxis - 0.5f),
                                                            1.f + (osc->totalBrightness - 1.f)))
	 * scaleToRealDimMatr
	 * (*aImport->getNormCubeAndCenterMatr());

                glm::mat4 transMatr = beamerMatr * baseTrans;
                glm::mat3 normalMatr = glm::mat3( glm::transpose( glm::inverse( baseTrans ) ) );

                glm::mat4 viewMat = glm::mat4(1.f);
                glm::mat4 invViewMatr = glm::inverse( viewMat );

                // std shader setup
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


                for(int i=0;i<int(aImport->getMeshCount());i++)
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

                    shdr->setUniform1f("lookAngle", kpCalib.beamerLookAngle);
                    shdr->setUniform1f("zObjPos", kpCalib.distBeamerObj);

                    int blendText1 = int(blendTextStart);

                    glActiveTexture(GL_TEXTURE0);
                    shdr->setUniform1i("tex_diffuse0", 0);
                    glBindTexture(GL_TEXTURE_2D, switchTextures[blendText1].getId());

                    glActiveTexture(GL_TEXTURE1);
                    shdr->setUniform1i("tex_diffuse1", 1);
                    glBindTexture(GL_TEXTURE_2D, switchTextures[int(blendTextEnd)].getId());

                    shdr->setUniform1f("blend", blendText->getPercentage());

                    // luces
                    if(i == 7)
                    {
                        glActiveTexture(GL_TEXTURE0);
                        shdr->setUniform1i("tex_diffuse0", 0);
                        glBindTexture(GL_TEXTURE_2D, switchTextures[blendText1].getId());
                        shdr->setUniform1f("blend", 0.f);
                    }

                    shdr->setUniform1i("normalTex", 2);
                    normalTexture->bind(2);

                    shdr->setUniform1i("litsphereTexture", 3);
                    litsphereTexture->bind(3);

                    shdr->setUniform1i("cubeMap", 4);
                    cubeTex->bind(4);

                    shdr->setUniform1f("blendVec", 0.4f);
//                    shdr->setUniform1f("blendVec", osc->zoom);

                    // metal
                    if (i==3)
                    {
                        shdrSet[i]->begin();
                        shdrSet[i]->setUniformMatrix4fv("viewMatrix", &viewMat[0][0] ); // world to view transformation
                        shdrSet[i]->setUniformMatrix4fv("viewMatrixInverse", &invViewMatr[0][0] );
                        shdrSet[i]->setUniformMatrix4fv("modelMatrix", &baseTrans[0][0] ); // world to view transformation
                        shdrSet[i]->setUniformMatrix4fv("persp_matr", &beamerMatr[0][0]);
                        shdrSet[i]->setUniformMatrix4fv("m_pvm",  &baseTrans[0][0]);
                        shdrSet[i]->setUniformMatrix3fv("m_normal", &normalMatr[0][0]);
                        shdrSet[i]->setUniform1f("lookAngle", kpCalib.beamerLookAngle);
                        shdrSet[i]->setUniform1f("zObjPos", kpCalib.distBeamerObj);

                        shdrSet[i]->setUniform1i("cubeMap", 0);
                        cubeTex->bind(0);
                    }

                    // glass
                    //if (i==4)
                    if (i==5)
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
                        shdrSet[i]->setUniform1f("lookAngle", kpCalib.beamerLookAngle);
                        shdrSet[i]->setUniform1f("zObjPos", kpCalib.distBeamerObj);

                        shdrSet[i]->setUniform1i("EnvMap", 0);
                        cubeTex->bind(0);

                        shdrSet[i]->setUniform1i("RefractionMap", 1);
                        cubeTex->bind(1);
                    }


                    aImport->sendMeshMatToShdr(i, shdrSet[i]);
                    aImport->drawMeshNoShdr(i, GL_TRIANGLES);
                }

//                aImport->sendMeshMatToShdr(actMesh, shdr);
//                aImport->drawMeshNoShdr(actMesh, GL_TRIANGLES);



                glDisable(GL_BLEND);
	 */
	/*
                colShader->begin();
                colShader->setIdentMatrix4fv("m_pvm");
                yLine->draw(GL_LINES);
                xLine->draw(GL_LINES);
	 */
	/*
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

                cout <<  std::tan(kpCalib.beamerLookAngle) * (objDist - (-118.f)) << endl;

                texShaderDist->setUniform1f("lookAngle", kpCalib.beamerLookAngle);
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
	 */
}

//----------------------------------------------------

void SNRotatingObjMapping2::update(double time, double dt)
{
	/*
        actTime = time;
        blendText->update(time);

        if(!inited) initDyn();

        // --- CALIBRATE 

        if (inited && actDrawMode == KinectReproTools::CALIB_ROT_WIN && useKinect)
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

                    //chessRotMatr = getRotReal(kpCalib);
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

       // std::cout << "motorAbsPosOffs: " << motorAbsPosOffs << std::endl;

        // continue with the correct motorPosition between 0 and motorMaxSteps
        if (arduino)
            motorActPos = (arduino->getActPos() + motorAbsPosOffs) % motorMaxSteps;

//        std::cout << "motorActPos: " << motorActPos << std::endl;
//        std::cout << "motorActLastPos: " << motorActLastPos << std::endl;

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
           // std::cout << "newRotPos: " << newRotPos << std::endl;

            if(std::fabs(rotPos - newRotPos) > M_PI && rotPos < newRotPos)
            {
                //printf("values switch from TWO_M_PI to 0 \n");
                rotPos = ((rotPos + M_PI * 2.f) * medFact + newRotPos) / (medFact + 1.f);

            // values switch from 0 to TWO_M_PI
            } else if(std::fabs(rotPos - newRotPos) > M_PI && rotPos > newRotPos)
            {

                //printf("values switch from TWO_M_PI to 0 \n");
                rotPos = (((rotPos - M_PI * 2.f) * medFact) + newRotPos) / (medFact + 1.f);

            } else
            {
                rotPos = ( rotPos * medFact + newRotPos ) / (medFact + 1.f);
            }

            rotPos = std::fmod(rotPos, M_PI * 2.f);

//            std::cout << "out: " << rotPos << std::endl;
//            std::cout << std::endl;

            modelMatr = glm::rotate(glm::mat4(1.f), rotPos, glm::vec3(0.f, 1.f, 0.f));
            motorActLastPos = motorActPos;
        }
	 */
}

//----------------------------------------------------

std::string SNRotatingObjMapping2::execCmd(char* cmd)
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

void SNRotatingObjMapping2::addLitSphereShader()
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

	uniform vec3 lightPos;
	uniform float shininess;
	uniform vec4 specular;
	uniform vec4 diffuse;
	uniform vec4 ambient;
	uniform vec4 sceneColor;

	uniform float blend;
	uniform float blendVec;

	vec4 tex0;
	vec4 tex1;
	vec4 blendCol;
	vec4 envColor;

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

		color = ((Iamb + Idiff + Ispec + envColor * 0.5 * sceneColor) * shading ) * blendCol * 2.0;// + color;
		//    color = envColor;
		//    color *= vertex_in.color.a;
	});

	frag = "// mapping litsphere shader, frag\n"+shdr_Header+frag;

	std::cout << "shCol->addCheckShaderText" << std::endl;

	shdr = shCol->addCheckShaderText("texBlendShdr", vert.c_str(), frag.c_str());
}

//----------------------------------------------------

void SNRotatingObjMapping2::swapBlendTex(float* startInd, float* endInd)
{
	*startInd = *endInd;
	actText = *endInd;
}

//----------------------------------------------------

void SNRotatingObjMapping2::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if(mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_1 : actDrawMode = KinectReproTools::CHECK_RAW_DEPTH;
			printf("CHESSBOARD\n");
			break;
			case GLFW_KEY_2 : actDrawMode = KinectReproTools::CALIB_ROT_WIN;
			printf("CALIBRATE \n");
			break;
			case GLFW_KEY_3 : actDrawMode = KinectReproTools::CHECK_ROT_WARP;
			printf("check\n");
			calibLoaded = false;
			break;
			case GLFW_KEY_4 : actDrawMode = KinectReproTools::CHECK_ROT_WARP_LOADED;
			if(!calibLoaded) kinRepro->loadCalib(calibFilePath);
			printf("check loaded\n");
			break;
			case GLFW_KEY_5 : actDrawMode = KinectReproTools::USE_ROT_WARP;
			printf("draw\n");
			break;
			case GLFW_KEY_S :
				printf("save Settings\n");
				kinRepro->saveCalib(calibFilePath);
				break;
			}

			kinRepro->setMode(actDrawMode);
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
				break;
			case GLFW_KEY_LEFT :
				printf("left \n");
				blendTextStart = float(actText);
				blendTextEnd = (actText -1 + tex_paths.size()) % int(tex_paths.size());
				blendText->start(blendTextStart, blendTextEnd, 1.0, actTime, false);
				blendText->setEndFunc([this]() { return this->swapBlendTex(&blendTextStart, &blendTextEnd); });
				break;
			default:
				break;
			}
		}
	}
}

//----------------------------------------------------

void SNRotatingObjMapping2::onCursor(GLFWwindow* window, double xpos, double ypos)
{}

//----------------------------------------------------

void SNRotatingObjMapping2::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)ns;
	nanosleep(&tim, NULL);
}
}
