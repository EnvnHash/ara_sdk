/*
 * SNFreenect2Motion.cpp
 *
 *  Created on: 12.01.2016
 *      Copyright by Sven Hahne
 *
 *	multiple kinects, should all run in the same resolution
 *	output texture should be square
 *
 *  Raumdimensionen muessen in der setup.xml datei stehen als roomDim
 *  zum Kalibrien Roty erstmal auf Mittelstellung (0.5)
 * */

#include "SNFreenect2Motion.h"
#define STRINGIFY(A) #A

namespace tav
{
SNFreenect2Motion::SNFreenect2Motion(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    								SceneNode(_scd, _sceneArgs), destTexSize(512),
									nrDevices(0), useV2(false), maxNrPointAmp(50),
									actMode(PROC_ONLY)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
    kin = static_cast<KinectInput*>(scd->kin);
    osc = static_cast<OSCData*>(scd->osc);
    winMan = static_cast<GWindowManager*>(scd->winMan);
	fnc = static_cast<Freenect2In**>(scd->fnc);
	nrFnc = scd->nrFnc;

	calibFilePath = (*_scd->dataPath)+"/calib_cam/Freenect2Motion_Samsung.yml";

	// enter kinect v2 serials here for fixed mapping
	serialMap = new std::string[2];
	serialMap[0] = "008475450547";
	serialMap[1] = "500921442542";

	mapConf.xtionAssignMap = new unsigned short[2];
	mapConf.xtionAssignMap[0] = 0;
	mapConf.xtionAssignMap[1] = 1;

	// damit, bei nis dasselbe rauskommt, besser direkt von OpenNi lesen
//	mapConf.kinFov.x = (61.66f / 360.f) * 2.f * M_PI;
//	mapConf.kinFov.y = (49.234f / 360.f) * 2.f * M_PI;
	mapConf.kinFov.x = kin->getDepthFovX();
	mapConf.kinFov.y = kin->getDepthFovY();

	//std::cout << glm::to_string(mapConf.kinFov) << std::endl;


	quad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on

	quad->rotate(M_PI, 0.f, 0.f, 1.f);

	rotQuad = new Quad(-1.f, -1.f, 2.f, 2.f,
			glm::vec3(0.f, 0.f, 1.f),
			0.f, 0.f, 0.f, 1.f);    // color will be replace when rendering with blending on
	//rotQuad->rotate(M_PI, 0.f, 0.f, 1.f);

//	ppFbo = new PingPongFbo(shCol, destTexSize, destTexSize,
//			GL_RGBA16F, GL_TEXTURE_2D, false, 2);

	/*
	// fastblur fuer kinect map resultat
	mapBlur = new FastBlurMem(0.08f, 0.3f, _shCol,
			kin->getDepthWidth(0), kin->getDepthHeight(0),
			GL_RGBA16F);

	mapBlurFbo = new FBO(shCol, destTexSize, destTexSize,
			GL_RGBA16F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT);
			*/

	emitTex = new TextureManager();
	emitTex->loadTexture2D((*scd->dataPath) + "textures/test_emit.png");
	emitTex->setFiltering(GL_NEAREST, GL_NEAREST);


	// check Nr of Kinects
	if(useV2 && fnc)
	{
		nrDevices = nrFnc;
		printf("got %d Freenect Devices \n", nrDevices);

		/*
		blur = new FastBlurMem*[nrDevices];
		optFlow = new GLSLOpticalFlowDepth*[nrDevices];

		for (int i=0; i<nrDevices; i++)
		{
			blur[i] = new FastBlurMem(0.02f, 0.02f, _shCol,
					fnc[i]->getDepthWidth(), fnc[i]->getDepthHeight(),
					fnc[i]->getDepthFormat());

			optFlow[i] = new GLSLOpticalFlowDepth(_shCol, fnc[i]->getDepthWidth(),
					fnc[i]->getDepthHeight());
		}
		*/
	} else
	{
		// check Nr of Freenects
		nrDevices = kin->getNrDevices();
		printf("got %d Kinect v1 Devices \n", nrDevices);


//		blur = new FastBlurMem*[nrDevices];
//		optFlow = new GLSLOpticalFlowDepth*[nrDevices];
		nis = new NISkeleton*[nrDevices];
		nisTexId = new GLuint[nrDevices];
		nisTexInited = new bool[nrDevices];
		nisFrameNr = new int[nrDevices];
		//	reproTools = new KinectReproTools*[nrDevices];

		// mirror vertical

		//kin->setMirror(false, 0);

		for (int i=0; i<nrDevices; i++)
		{
			kin->setDepthVMirror(true, i);
			kin->setColorVMirror(true, i);

			nis[i] = kin->getNis(i);
			nis[i]->setUpdateImg(true);

//			blur[i] = new FastBlurMem(0.08f, 0.35f, _shCol,
//					kin->getDepthWidth(i), kin->getDepthHeight(i),
//					GL_RGBA16F, false);
//			//   blur[i]->setBright(1.3f);
//
//			optFlow[i] = new GLSLOpticalFlowDepth(_shCol, kin->getDepthWidth(i),
//					kin->getDepthHeight(i));
			nisTexInited[i] = false;
			nisFrameNr[i] = -1;
		}
	}

	lastFrame = new int[nrDevices];
	lastColorFrame = new int[nrDevices];
	mapConf.scale = new glm::vec3[nrDevices];
	mapConf.rotAngleX = new float[nrDevices];
	mapConf.rotAngleY = new float[nrDevices];
	mapConf.trans = new glm::vec3[nrDevices];
	texNrAr = new GLint[nrDevices];
	maskTexNrAr = new GLint[nrDevices];
	velTexNrAr = new GLint[nrDevices];
	rotMats = new glm::mat4[nrDevices];

	for (int i=0; i<nrDevices; i++)
	{
		lastFrame[i] = -1;
		lastColorFrame[i] = -1;
		mapConf.scale[i] = glm::vec3(0.f);
		mapConf.rotAngleX[i] = 0.f;
		mapConf.rotAngleY[i] = 0.f;
		mapConf.trans[i] = glm::vec3(1.f);
	}


	stdTexShader = shCol->getStdTex();

	initShaders();
	initFlipAxisShdr();


	prevMats = new glm::mat4[nrDevices*2];
	for (int i=0; i<nrDevices; i++)
	{
		// depth image oben
		prevMats[i] = glm::translate(glm::mat4(1.f),
				glm::vec3(((static_cast<float>(i +1) / static_cast<float>(nrDevices))
						- (1.f / (static_cast<float>(nrDevices)) *2.f)) * 2.f -1.f,
						0.5f, 0.f));
		// color image unten
		prevMats[i + nrDevices] = glm::translate(glm::mat4(1.f),
				glm::vec3(((static_cast<float>(i +1) / static_cast<float>(nrDevices))
						- (1.f / (static_cast<float>(nrDevices) ) *2.f)) * 2.f -1.f,
						-0.5f, 0.f));
	}

	for (int i=0; i<nrDevices*2; i++)
		prevMats[i] = glm::scale(prevMats[i], glm::vec3(1.f / static_cast<float>(nrDevices), 0.5f, 1.f ) );

	mapOffs = new glm::vec3[nrDevices];



	// init a grid for iterating through the depth textures and drawing them as point clouds

	float divisor = 2.f;
	while (divisor < (static_cast<float>(maxNrPointAmp) / static_cast<float>(nrDevices) / divisor)) divisor += 2.f;

	emitTrigCellSize.x = divisor;
	emitTrigCellSize.y = static_cast<unsigned int>(static_cast<float>(maxNrPointAmp) / static_cast<float>(nrDevices) / divisor);
	emitTrigNrPartPerCell =  emitTrigCellSize.x * emitTrigCellSize.y;

	if(!useV2)
	{
		emitTrigGridSize.x = kin->getDepthWidth(0) / emitTrigCellSize.x;
		emitTrigGridSize.y = kin->getDepthWidth(0) / emitTrigCellSize.y;
	}

	GLfloat initEmitPos[emitTrigGridSize.x * emitTrigGridSize.y *4];
	unsigned int posOffset=0;

	// init in Pixeln, weil spaeter texelFetch mit integern
	for (int y=0;y<emitTrigGridSize.y;y++)
	{
		for(int x=0;x<emitTrigGridSize.x;x++)
		{
			initEmitPos[posOffset *4   ] = static_cast<float>(x * emitTrigCellSize.x);
			initEmitPos[posOffset *4 +1] = static_cast<float>(y * emitTrigCellSize.y);
			initEmitPos[posOffset *4 +2] = 0.f;
			initEmitPos[posOffset *4 +3] = 1.f;

			// std::cout << "initEmitPos[" << posOffset << "] : " << initEmitPos[posOffset *4] << ", " << initEmitPos[posOffset *4 +1] << std::endl;
			posOffset++;

		}
	}

	emitTrig = new VAO("position:4f", GL_STATIC_DRAW);
	emitTrig->initData(emitTrigGridSize.x * emitTrigGridSize.y, initEmitPos);

	loadSetting();

	// mapping x Kinects -> one world
	// uncomment for manual override
/*
	mapConf.scale[ mapConf.xtionAssignMap[0] ] = glm::vec3(1.f, 1.f, 1.f);
	mapConf.scale[ mapConf.xtionAssignMap[1] ] = glm::vec3(1.f, 1.f, -1.f);

	mapConf.trans[ mapConf.xtionAssignMap[0] ].z = -3450.f;
	//	mapConf.trans[ mapConf.xtionAssignMap[0] ].z = tunnelLength * -0.5f;
	mapConf.trans[ mapConf.xtionAssignMap[0] ].y = tunnelHeight;
	mapConf.trans[ mapConf.xtionAssignMap[0] ].x = 0.f;

	mapConf.trans[ mapConf.xtionAssignMap[1] ].z = 2800.f;
	//	mapConf.trans[ mapConf.xtionAssignMap[1] ].z = tunnelLength * -0.5f;
	mapConf.trans[ mapConf.xtionAssignMap[1] ].y = tunnelHeight;
	mapConf.trans[ mapConf.xtionAssignMap[1] ].x = 0.f;
*/
}



void SNFreenect2Motion::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if (!initRoomDimen)
	{
		mapConf.roomDim = cp->roomDimen;
		initRoomDimen = true;
	}

	if(actMode != PROC_ONLY)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		// show them
		//displayInputs();
		//displayDebug();
		displayFlipRes();
	}
}



void SNFreenect2Motion::displayInputs()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	renderGrayShader->begin();
	renderGrayShader->setUniform1i("act", 0);
	glActiveTexture(GL_TEXTURE0);

	short ind = 0;
	if (useV2)
	{
		for (short i=0;i<nrDevices;i++)
		{
			renderGrayShader->setUniformMatrix4fv("m_pvm", &prevMats[i][0][0]);
			renderGrayShader->setUniform1f("maxDist", 4000.f);
			renderGrayShader->setUniform1f("formatConv", 1.f / 4500.f);
			glBindTexture(GL_TEXTURE_2D, fnc[i]->getDepthTexId() );

			quad->draw();
			ind++;
		}
	} else
	{
        if (nrDevices == 1)
        {
            renderGrayShader->begin();
            renderGrayShader->setUniform1i("act", 0);
            renderGrayShader->setIdentMatrix4fv("m_pvm");
            renderGrayShader->setUniform1f("maxDist", 4500.f);
            glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, 0) );
            quad->draw();
            
/*
            stdTexShader->begin();
            stdTexShader->setUniform1i("act", 0);
            stdTexShader->setIdentMatrix4fv("m_pvm");
//            stdTexShader->setUniformMatrix4fv("m_pvm", &prevMats[i+2][0][0]);
            stdTexShader->setUniform1f("maxDist", 4500.f);
            glBindTexture(GL_TEXTURE_2D, kin->getColorTexId(0) );
            quad->draw();
 */
            
        } else
        {
            for (short i=0;i<nrDevices;i++)
            {
                renderGrayShader->begin();
                renderGrayShader->setUniform1i("act", 0);
                renderGrayShader->setUniformMatrix4fv("m_pvm", &prevMats[i][0][0]);
                renderGrayShader->setUniform1f("maxDist", 4500.f);
                glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, i) );
                
                quad->draw();
                
                stdTexShader->begin();
                stdTexShader->setUniform1i("act", 0);
                stdTexShader->setUniformMatrix4fv("m_pvm", &prevMats[i+nrDevices][0][0]);
                stdTexShader->setUniform1f("maxDist", 4500.f);
                glBindTexture(GL_TEXTURE_2D, kin->getColorTexId(i) );
                
                quad->draw();
            }
        }
	}
}



void SNFreenect2Motion::displayDebug()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// draw the flip result
	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	stdTexShader->setUniform1i("tex", 0);

    if (nrDevices == 1)
    {
        if (nisTexId[0] < 256)
        {
            stdTexShader->setIdentMatrix4fv("m_pvm");
            glActiveTexture(GL_TEXTURE0);
            
            //glBindTexture(GL_TEXTURE_2D, blur[0]->getResult());
            //glBindTexture(GL_TEXTURE_2D, optFlow[0]->getDiffTexId());
            glBindTexture(GL_TEXTURE_2D, nisTexId[0]);
            //glBindTexture(GL_TEXTURE_2D, optFlow[0]->getResTexId());
            
            rotQuad->draw();
        }
        
    } else {
        for (short i=0;i<nrDevices;i++)
        {
            if (nisTexId[i] < 256)
            {
                stdTexShader->setUniformMatrix4fv("m_pvm", &prevMats[i][0][0]);
                glActiveTexture(GL_TEXTURE0);
                
                //glBindTexture(GL_TEXTURE_2D, blur[i]->getResult());
                //glBindTexture(GL_TEXTURE_2D, optFlow[i]->getDiffTexId());
                glBindTexture(GL_TEXTURE_2D, nisTexId[i]);
                //glBindTexture(GL_TEXTURE_2D, optFlow[i]->getResTexId());
                
                rotQuad->draw();
            }
        }
    }
}



void SNFreenect2Motion::displayFlipRes()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	procFlipAxis();

	/*
	// draw the flip result
	stdTexShader->begin();
	stdTexShader->setIdentMatrix4fv("m_pvm");
	stdTexShader->setUniform1i("tex", 0);

	glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, mapBlur->getResult());
	glBindTexture(GL_TEXTURE_2D, ppFbo->getSrcTexId(0));
	quad->draw();
	 */
}



void SNFreenect2Motion::update(double time, double dt)
{
	// upload all depth Images to Opengl
	GLuint actDepthTexId = 0;

	for (short i=0;i<nrDevices;i++)
	{
		//blur[i]->setThresh(osc->blurOffs * 0.5f); // bringt nix
		//optFlow[i]->setDiffAmp(4.f);

		if (useV2)
		{
			fnc[i]->uploadDepth();

			if (fnc[i]->getFrameNr() != lastFrame[i] && fnc[i]->getFrameNr() > 12)
			{
/*				blur[i]->proc(fnc[i]->getDepthTexId());
 *
				optFlow[i]->setCurTexId(blur[i]->getResult());
				optFlow[i]->setLastTexId(blur[i]->getLastResult());
				optFlow[i]->update();
*/
				lastFrame[i] = fnc[i]->getFrameNr();
			}

		} else
		{
			//if (actMode == CALIB) reproTools[i]->update();

			if (kin->isReady() && kin->isDeviceReady(i) && kin->getDepthFrameNr(false, i) > 12)
			{
				kin->uploadColorImg(i);

				if (kin->uploadDepthImg(false, i))
				{
/*
					blur[i]->proc(kin->getDepthTexId(false, i));

					optFlow[i]->setCurTexId(blur[i]->getResult());
					optFlow[i]->setLastTexId(blur[i]->getLastResult());
					optFlow[i]->update();
					*/
				}

				if (nisFrameNr[i] != nis[i]->getFrameNr())
				{
					nisFrameNr[i] = nis[i]->getFrameNr();
					uploadNisImg(i);
					const nite::Array<nite::UserData>& users = nis[i]->getUserTrackerFrame()->getUsers();
					//  std::cout << "cam " << i << " nrUsers: " << users.getSize() << std::endl;
				}
			}
		}
	}
}



void SNFreenect2Motion::procFlipAxis()
{
	if(actMode == CALIB)
	{
        if (nrDevices > 0)
        {
            mapConf.scale[ mapConf.xtionAssignMap[0] ] = glm::vec3(1.f, -1.f, 1.f);
            mapConf.rotAngleX[ mapConf.xtionAssignMap[0] ] = osc->alpha * M_PI;
            mapConf.rotAngleY[ mapConf.xtionAssignMap[0] ] = (osc->blurOffs - 0.5f) * M_PI;
        }
        
        if (nrDevices > 1)
        {
            mapConf.scale[ mapConf.xtionAssignMap[1] ] = glm::vec3(1.f, 1.f, -1.f);
            mapConf.rotAngleX[ mapConf.xtionAssignMap[1] ] = osc->feedback * M_PI;
            mapConf.rotAngleY[ mapConf.xtionAssignMap[1] ] = (osc->blurFboAlpha - 0.5f) * M_PI;
        }

        mapConf.cropX.x = mapConf.roomDim->x * -1.f;
        mapConf.cropX.y = mapConf.roomDim->x * 1.f;
        
        mapConf.cropY.x = -1.f * mapConf.roomDim->y;
        mapConf.cropY.y = mapConf.roomDim->y * 1.f;

        mapConf.cropZ.x = mapConf.roomDim->z * -1.f;
        mapConf.cropZ.y = mapConf.roomDim->z * 1.f;
        
      //  printf("roomDIm: %f %f %f \n",mapConf.roomDim->x, mapConf.roomDim->y, mapConf.roomDim->z );

        /*
		switch ( cropSelector )
		{
		case 0:
			mapConf.cropX.x = osc->zoom * mapConf.roomDim->x * -0.5f;
			mapConf.cropX.y = osc->speed * mapConf.roomDim->x * 0.5f;
			break;
		case 1:
			mapConf.cropY.x = (osc->zoom - 0.5f) * 2.f * (mapConf.roomDim->y * -0.5f);
			mapConf.cropY.y = osc->speed * mapConf.roomDim->y * 0.5f;
			break;
		case 2:
			mapConf.cropZ.x = osc->zoom * mapConf.roomDim->z * -1.f;
			mapConf.cropZ.y = osc->speed * mapConf.roomDim->z * 1.f;
			break;
		}
         */
	}


	for (short i=0;i<nrDevices;i++)
	{
		texNrAr[i] = mapConf.xtionAssignMap[i];
		maskTexNrAr[i] = mapConf.xtionAssignMap[i] +nrDevices;
		velTexNrAr[i] = mapConf.xtionAssignMap[i] +(nrDevices*2);

		rotMats[i] = glm::translate(glm::mat4(1.f), mapConf.trans[ mapConf.xtionAssignMap[i] ] );
		rotMats[i] = glm::scale(rotMats[i], mapConf.scale[ mapConf.xtionAssignMap[i] ]);
		rotMats[i] = glm::rotate(rotMats[i],
				mapConf.rotAngleY[ mapConf.xtionAssignMap[i] ],
				glm::vec3(0.f, 1.f, 0.f));
		rotMats[i] = glm::rotate(rotMats[i],
				mapConf.rotAngleX[ mapConf.xtionAssignMap[i] ],
				glm::vec3(1.f, 0.f, 0.f));
	}

	glm::mat4 rotYMat = glm::rotate(glm::mat4(1.f), float(M_PI) * osc->rotYAxis, glm::vec3(0.f, 1.f, 0.f));


	// use the flipaxis shader
	glEnable(GL_BLEND);

	//	ppFbo->src->bind();
	//	ppFbo->src->clear();
	//ppFbo->src->clearToAlpha(0.f);

	flipAxisShdr->begin();
	flipAxisShdr->setIdentMatrix4fv("m_pvm");
	flipAxisShdr->setUniformMatrix4fv("rotMats", &rotMats[0][0][0], nrDevices);
	flipAxisShdr->setUniformMatrix4fv("rotY", &rotYMat[0][0]);

	flipAxisShdr->setUniform1i("nrDevices", nrDevices);
	flipAxisShdr->setUniform2f("inTex_Size", float(kin->getDepthWidth()), float(kin->getDepthHeight()));
	flipAxisShdr->setUniform2i("cellSize", emitTrigCellSize.x, emitTrigCellSize.y);

	flipAxisShdr->setUniform1f("maxDepth", mapConf.roomDim->x);
	flipAxisShdr->setUniform1f("powDepth", 0.f);
	flipAxisShdr->setUniform1iv("depthTex", texNrAr, nrDevices);
	flipAxisShdr->setUniform1iv("maskTex", maskTexNrAr, nrDevices);
	flipAxisShdr->setUniform1iv("velTex", velTexNrAr, nrDevices);
	flipAxisShdr->setUniform2fv("cropX", &mapConf.cropX[0]);
	flipAxisShdr->setUniform2fv("cropY", &mapConf.cropY[0]);
	flipAxisShdr->setUniform2fv("cropZ", &mapConf.cropZ[0]);
	flipAxisShdr->setUniform2f("kinFov", mapConf.kinFov.x, mapConf.kinFov.y);
	flipAxisShdr->setUniform1f("realNormScale", 2.f / mapConf.roomDim->x);
	//flipAxisShdr->setUniform1f("dSc", 0.46f);


	if (useV2) {
		flipAxisShdr->setUniform2f("inTex_Size", float(fnc[0]->getDepthWidth()),
				float(fnc[0]->getDepthHeight()));
	} else {
		flipAxisShdr->setUniform2f("inTex_Size", float(kin->getDepthWidth()),
				float(kin->getDepthHeight()));
	}


	for (int i=0; i<nrDevices; i++)
	{
		glActiveTexture(GL_TEXTURE0 +i);
        
		if (useV2)
			glBindTexture(GL_TEXTURE_2D, fnc[0]->getDepthTexId());
		else
			glBindTexture(GL_TEXTURE_2D, kin->getDepthTexId(false, mapConf.xtionAssignMap[i] ));

		//		glActiveTexture(GL_TEXTURE0 +i +nrDevices);
		//		glBindTexture(GL_TEXTURE_2D, nisTexId[ mapConf.xtionAssignMap[i] ]);
		//		//          	glBindTexture(GL_TEXTURE_2D, optFlow[ mapConf.xtionAssignMap[i] ]->getDiffTexId());

		//		glActiveTexture(GL_TEXTURE0 +i +(nrDevices*2));
		//		glBindTexture(GL_TEXTURE_2D, optFlow[ mapConf.xtionAssignMap[i] ]->getResTexId());
	}

	emitTrig->draw(GL_POINTS);


	//	ppFbo->src->unbind();
	//	ppFbo->swap();

	// printf("flip axis has now img on %d \n", ppFbo->getSrcTexId(0));


	//mapBlur->proc(ppFbo->getSrcTexId(0));
	//mapBlur->setFdbk(osc->feedback);
	//mapBlur->setAlpha(osc->alpha);
}


void SNFreenect2Motion::uploadNisImg(int device)
{
	if (!nisTexInited[device])
	{
		glGenTextures(1, &nisTexId[device]);
		glBindTexture(GL_TEXTURE_2D, nisTexId[device]);

		// define inmutable storage
		glTexStorage2D(GL_TEXTURE_2D,
				1,                 // nr of mipmap levels
				GL_RGBA8,
				kin->getDepthWidth(device),
				kin->getDepthHeight(device));

		glTexSubImage2D(GL_TEXTURE_2D,              // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				kin->getDepthWidth(device),
				kin->getDepthHeight(device),                 // width and height
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				nis[device]->getResImg());

		glBindTexture(GL_TEXTURE_2D, 0);

		nisTexInited[device] = true;
	}


	glBindTexture(GL_TEXTURE_2D, nisTexId[device]);
	glTexSubImage2D(GL_TEXTURE_2D,             // target
			0,                          // First mipmap level
			0, 0,                       // x and y offset
			kin->getDepthWidth(device),
			kin->getDepthHeight(device),              // width and height
			GL_BGRA,
			GL_UNSIGNED_BYTE,
			//kin->getActColorImg(device));
			nis[device]->getResImg());
}



void SNFreenect2Motion::initShaders()
{
	std::string shdr_Header = "#version 410\n\n";

	std::string vert =
			STRINGIFY(layout( location = 0 ) in vec4 position;\n
			layout( location = 1 ) in vec3 normal;\n
			layout( location = 2 ) in vec2 texCoord;\n
			layout( location = 3 ) in vec4 color;\n
			uniform mat4 m_pvm;\n
			out vec2 TexCoord;\n
			void main(void) {\n
				TexCoord = texCoord;\n
				gl_Position = m_pvm * position;\n
			});

	vert = shdr_Header + vert;

	std::string grayfrag =
			STRINGIFY(
					uniform sampler2D act;\n
					uniform float maxDist;\n
					uniform float formatConv;\n
					vec4 actColor;\n
					in vec2 TexCoord;\n
					layout(location = 0) out vec4 Color;\n

					float grayVal;

					void main(void) {\n
						actColor = texture(act, TexCoord);\n
						grayVal = actColor.r / maxDist;
						Color = vec4(grayVal, grayVal, grayVal, 1.0);\n
					});

	grayfrag = shdr_Header + grayfrag;

	std::string frag =
			STRINGIFY(uniform sampler2D Data;\n
			in vec2 TexCoord;\n
			layout(location = 0) out vec4 color;\n

			void main(void) {\n
				color = texture(Data, TexCoord);\n
			});

	frag = shdr_Header + frag;

	renderShader = shCol->addCheckShaderText("Freenect2In", vert.c_str(),
			frag.c_str());

	renderGrayShader = shCol->addCheckShaderText("Freenect2InGray",
			vert.c_str(), grayfrag.c_str());
}



void SNFreenect2Motion::initFlipAxisShdr()
{
	std::string shdr_Header = "// SNFreenect2Motion \n #version 410\n";

	std::string vert =
			STRINGIFY(layout( location = 0 ) in vec4 position;\n
			void main(void) {\n
				gl_Position = position;\n
			});
	vert = shdr_Header + vert;


	std::string geom =
			STRINGIFY(
					out GS_FS_VERTEX {
						vec4 color;
						//vec4 vel_color;
					} vertex_out;

	uniform mat4 m_pvm;\n
	uniform mat4 rotY;\n
	uniform vec2 inTex_Size;\n

	uniform int nrDevices;\n
	uniform ivec2 cellSize;\n
	uniform float maxDepth;\n
	uniform float powDepth;\n
	uniform float realNormScale;\n
	//uniform float dSc;\n
	uniform vec2 kinFov;\n

	uniform vec2 cropX;\n
	uniform vec2 cropY;\n
	uniform vec2 cropZ;\n

	ivec2 readOffset;\n
	vec4 depth;\n
	vec4 tempPos;\n
	vec3 rwPos;\n
	vec4 rotPos;\n
	float mask;\n
	float scaledDepth;\n
	vec4 vel;\n
	int drawSwitch;

	vec3 getKinRealWorldCoord(vec3 inCoord)
	{
		// asus xtion tends to measure lower depth with increasing distance
		// experimental correction
		//float depthScale = 1.0 + pow(inCoord.z * 0.00033, powDepth);
		scaledDepth = inCoord.z;
/*
        // asus xtion tends to measure lower depth with increasing distance
        // experimental correction
		float multDiff = inCoord.z - 950.0;
		float upperCorr = max(multDiff, 0) * 0.02;
		scaledDepth = inCoord.z + (multDiff * 0.0102) + upperCorr;
		*/

		float xzFactor = tan(kinFov.x * 0.5) * 2.0;
		float yzFactor = tan(kinFov.y * 0.5) * 2.0;

		return vec3(inCoord.x * 0.5 * scaledDepth * xzFactor,
				inCoord.y * 0.5 * scaledDepth * yzFactor,
				scaledDepth);
	}

	void main()
	{
		vertex_out.color = vec4(1.0, 1.0, 1.0, 1.0);

		for (int y=0;y<cellSize.y;y++)
		{
			for (int x=0;x<cellSize.x;x++)
			{
				for (int i=0;i<nrDevices;i++)
				{
					readOffset = ivec2(gl_in[0].gl_Position.xy) + ivec2(x, y);

					depth = texelFetch(depthTex[i], readOffset, 0);

					//					mask = texture(maskTex[i], readOffset).r + texture(maskTex[i], readOffset).g + texture(maskTex[i], readOffset).b;
					//					vel = texture(velTex[i], readOffset);

					vec3 rwPos = vec3(
							gl_in[0].gl_Position.x / inTex_Size.x * 2.0 -1.0,
							(1.0 - (gl_in[0].gl_Position.y / inTex_Size.y)) * 2.0 -1.0,
							depth.r);

					rwPos = getKinRealWorldCoord(rwPos);
					rwPos = (rotMats[i] * vec4(rwPos.xyz, 1.0)).xyz;

					drawSwitch = int(rwPos.x > cropX.x) * int(rwPos.x < cropX.y)
							* int(rwPos.y > cropY.x) * int(rwPos.y < cropY.y)
							* int(rwPos.z > cropZ.x) * int(rwPos.z < cropZ.y);
					if(drawSwitch > 0)
					{
						// flip x and Z axis
						//rwPos = vec3(rwPos.z, rwPos.y, rwPos.x);

						// rotate around y axis - for debugging
						rwPos = (rotY * vec4(rwPos, 1.0)).xyz;

						rwPos /= maxDepth;

						vertex_out.color = vec4(float(i), 1.0, 1.0, 1.0);
						gl_Position = m_pvm * vec4(rwPos, 1.0);

						EmitVertex();
						EndPrimitive();
					}
				}
			}
		}
	});

	geom = shdr_Header + "layout (points) in;\n	layout (points, max_vertices = 50) out;\n"
	+ "uniform sampler2D depthTex["+std::to_string(nrDevices)+"];\n"
	+ "uniform sampler2D maskTex["+std::to_string(nrDevices)+"];\n"
	+ "uniform sampler2D velTex["+std::to_string(nrDevices)+"];\n"
	+ "uniform mat4 rotMats["+std::to_string(nrDevices)+"];\n"+ geom;


	std::string frag =
			STRINGIFY(uniform sampler2D Data;\n
			in GS_FS_VERTEX
			{
				vec4 color;
				//vec4 vel_color;
			} vertex_out;

			layout(location = 0) out vec4 Color;\n
			//layout(location = 1) out vec4 Velocity;\n

			void main(void) {\n
				Color = vertex_out.color;\n
				//Velocity = vertex_out.vel_color;\n
			});

	frag = shdr_Header + frag;

	flipAxisShdr = shCol->addCheckShaderText("KinectFlipAxis", vert.c_str(),
			geom.c_str(), frag.c_str());
}



GLuint SNFreenect2Motion::getVelTex(int device)
{
	//return ppFbo->getSrcTexId(1);
	//return optFlow[device]->getResTexId();
	return 0;
}



GLuint SNFreenect2Motion::getEmitTex(int device)
{

	//return mapBlur->getResult();
	//return emitTex->getId();
//	return ppFbo->getSrcTexId(0);
	//return optFlow[device]->getDiffTexId();
	return 0;
}



int	SNFreenect2Motion::getEmitTexHeight()
{
	return destTexSize;
	//return emitTex->getWidth();
}



int	SNFreenect2Motion::getEmitTexWidth()
{
	//return emitTex->getHeight();
	return destTexSize;
}



SNFreenect2Motion::mapConfig* SNFreenect2Motion::getMapConfig()
{
	return &mapConf;
}



void SNFreenect2Motion::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if(mods == GLFW_MOD_SHIFT)
		{
			switch (key)
			{
			case GLFW_KEY_A :
				printf("CALIB \n");
				actMode = CALIB;
				break;
			case GLFW_KEY_D :
				printf("DRAW \n");
				actMode = DRAW;
				break;
			case GLFW_KEY_W :
				printf("WRITE \n");
				saveSetting();
				loadSetting();
				break;
			}

		} else
		{
			switch (key)
			{
			case GLFW_KEY_1 : camSelect = 0; printf("camSelect: %d \n",camSelect); break;
			case GLFW_KEY_2 : camSelect = 1; printf("camSelect: %d \n",camSelect); break;
			case GLFW_KEY_LEFT : mapConf.trans[camSelect].x += 10.f; break;
			case GLFW_KEY_RIGHT : mapConf.trans[camSelect].x -= 10.f; break;
			case GLFW_KEY_UP : mapConf.trans[camSelect].y += 10.f; break;
			case GLFW_KEY_DOWN : mapConf.trans[camSelect].y -= 10.f; break;
			case GLFW_KEY_O : mapConf.trans[camSelect].z += 10.f; break;
			case GLFW_KEY_L : mapConf.trans[camSelect].z -= 10.f; break;
			case GLFW_KEY_X: cropSelector = 0; printf("cropX selected \n"); break;
			case GLFW_KEY_Y: cropSelector = 1; printf("cropY selected \n"); break;
			case GLFW_KEY_Z: cropSelector = 2; printf("cropZ selected \n"); break;
			}
		}
	}
}



void SNFreenect2Motion::saveSetting()
{
    cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
    cv::Mat glmVec2 = cv::Mat(2, 1, CV_32F);

    cv::FileStorage fs( calibFilePath, cv::FileStorage::WRITE );

	mapConf.scale[ mapConf.xtionAssignMap[0] ] = glm::vec3(1.f, 1.f, 1.f);
	mapConf.scale[ mapConf.xtionAssignMap[1] ] = glm::vec3(1.f, 1.f, -1.f);

    fs << "nrDevices" << int(nrDevices);

    glmVec.at<float>(0) = mapConf.roomDim->x;
    glmVec.at<float>(1) = mapConf.roomDim->y;
    glmVec.at<float>(2) = mapConf.roomDim->z;
    fs << "roomDim" << glmVec;

    glmVec2.at<float>(0) = mapConf.cropX.x;
    glmVec2.at<float>(1) = mapConf.cropX.y;
    fs << "cropX" << glmVec2;

    glmVec2.at<float>(0) = mapConf.cropY.x;
    glmVec2.at<float>(1) = mapConf.cropY.y;
    fs << "cropY" << glmVec2;

    glmVec2.at<float>(0) = mapConf.cropZ.x;
    glmVec2.at<float>(1) = mapConf.cropZ.y;
    fs << "cropZ" << glmVec2;

	for (short i=0;i<nrDevices;i++)
	{
	    glmVec.at<float>(0) = mapConf.scale[i].x;
	    glmVec.at<float>(1) = mapConf.scale[i].y;
	    glmVec.at<float>(2) = mapConf.scale[i].z;
	    fs << "scale_"+std::to_string(i) << glmVec;

	    glmVec.at<float>(0) = mapConf.trans[i].x;
	    glmVec.at<float>(1) = mapConf.trans[i].y;
	    glmVec.at<float>(2) = mapConf.trans[i].z;
	    fs << "trans_"+std::to_string(i) << glmVec;

	    fs << "rotAngleX_"+std::to_string(i) << mapConf.rotAngleX[i];
	    fs << "rotAngleY_"+std::to_string(i) << mapConf.rotAngleY[i];

	    fs << "assignMap_"+std::to_string(i) << int( mapConf.xtionAssignMap[i] );
	}
}



void SNFreenect2Motion::loadSetting()
{
	int thisNrDev=0;
    cv::Mat glmVec = cv::Mat(3, 1, CV_32F);
    cv::Mat glmVec2 = cv::Mat(2, 1, CV_32F);

    cv::FileStorage fs( calibFilePath, cv::FileStorage::READ );

    if(!fs["nrDevices"].empty())
    {
    	fs["nrDevices"] >> thisNrDev;
    	if (thisNrDev != nrDevices)
    		printf("SNFreenect2Motion::loadSetting Warning!!! nrDevices differs!!!\n");
    }

	printf("SNFreenect2Motion::loadSetting %d cameras\n", nrDevices);

	if( !fs["cropX"].empty() )
	{
    	fs["cropX"] >> glmVec2;
		mapConf.cropX.x = glmVec2.at<float>(0);
		mapConf.cropX.y = glmVec2.at<float>(1);
	}

	if( !fs["cropY"].empty() )
	{
    	fs["cropY"] >> glmVec2;
		mapConf.cropY.x = glmVec2.at<float>(0);
		mapConf.cropY.y = glmVec2.at<float>(1);
	}

	if( !fs["cropZ"].empty() )
	{
    	fs["cropZ"] >> glmVec2;
		mapConf.cropZ.x = glmVec2.at<float>(0);
		mapConf.cropZ.y = glmVec2.at<float>(1);
	}

	for (short i=0;i<thisNrDev;i++)
	{
		if( !fs["scale_"+std::to_string(i)].empty() )
		{
			fs["scale_"+std::to_string(i)] >> glmVec;
			mapConf.scale[i] = glm::vec3(glmVec.at<float>(0),
					glmVec.at<float>(1),
					glmVec.at<float>(2));
		}

		if(!fs["trans_"+std::to_string(i)].empty())
		{
			fs["trans_"+std::to_string(i)] >> glmVec;
			mapConf.trans[i] = glm::vec3(glmVec.at<float>(0),
					glmVec.at<float>(1),
					glmVec.at<float>(2));
		}

		if(!fs["rotAngleX_"+std::to_string(i)].empty())
		{
			fs["rotAngleX_"+std::to_string(i)] >> mapConf.rotAngleX[i];
		}

		if(!fs["rotAngleY_"+std::to_string(i)].empty())
		{
			fs["rotAngleY_"+std::to_string(i)] >> mapConf.rotAngleY[i];
		}

		if(!fs["assignMap_"+std::to_string(i)].empty())
		{
			fs["assignMap_"+std::to_string(i)] >> mapConf.xtionAssignMap[i];
		}
    }
}



SNFreenect2Motion::~SNFreenect2Motion()
{
	delete quad;
}
}
