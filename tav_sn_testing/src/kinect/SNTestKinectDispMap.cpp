//
// SNTestKinectDispMap.cpp
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved.
//

#include "SNTestKinectDispMap.h"

namespace tav
{

SNTestKinectDispMap::SNTestKinectDispMap(sceneData* _scd, std::map<std::string, float>* _sceneArgs) :
    		SceneNode(_scd, _sceneArgs), isInit(false), useIr(false), updtInt(2.0),
			lastime(0)
{
	kin = static_cast<KinectInput*>(scd->kin);
	stm = new StereoMatchingGPU(kin, shCol);

	shdr = shCol->getStdTex();
	show = 2;
}

//----------------------------------------------------

void SNTestKinectDispMap::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
	if ( time - lastime > updtInt )
	{
		stm->update();
		lastime = time;
	}

	// stm->renderRectified();
	//        stm->renderCost();
	stm->renderWinSize();
	//        stm->testR32();


	/*
        minDispar = osc->totalBrightness * 200 - 100;
        numberOfDisparities = std::max((int)(osc->alpha * 10.f) * 16, 16);
        p1 = osc->feedback * 800;
        p2 = osc->blurOffs * 2400;
        disp12MaxDiff = (int)(osc->blurFboAlpha * 20);
        preFilterCap =(int)(osc->blurFdbk * 20);
        blockSize = (int)(osc->rotYAxis * 20);
        speckleRange = (int)(osc->zoom * 10);
        speckleWindowSize = (int)(osc->speed * 200);
        uniquenessRatio= (int)(osc->backColor * 20);

        if (!isInit && kin->isReady())
        {
            nrDevices = kin->getNrDevices();
            nrQuads = nrDevices+1;

            frameNr = new int[nrDevices];
            quad = new Quad*[nrQuads];
            img = new cv::Mat[nrDevices];
            //gray = new cv::UMat[nrDevices];
            rimg = new cv::Mat[nrDevices];

//            img[0] = cv::imread("/Users/useruser/Desktop/stereo_calib/01_l.jpg");
//            img[1] = cv::imread("/Users/useruser/Desktop/stereo_calib/01_r.jpg");
//            img[0] = cv::imread("/Users/useruser/tav_data/textures/stereo_1.jpg");
//            img[1] = cv::imread("/Users/useruser/tav_data/textures/stereo_2.jpg");
            //imageSize = img[0].size();

            imageSize = cv::Size(kin->getColorHeight(), kin->getColorWidth());
//            imageSize = cv::Size(kin->getIrHeight(), kin->getIrWidth());

            imgDisparity16S = cv::Mat( imageSize, CV_16S );
            imgDisparity8U = cv::Mat( imageSize, CV_8UC1 );

            cv::stereoRectify(cameraMatrix[0], distCoeffs[0],
                              cameraMatrix[1], distCoeffs[1],
                              imageSize, R, T, R1, R2, P1, P2, Q,
                              cv::CALIB_ZERO_DISPARITY, 1, imageSize,
                              &validRoi[0], &validRoi[1]);

            cv::initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize,
                                        CV_16SC2, rmap[0][0], rmap[0][1]);
            cv::initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize,
                                        CV_16SC2, rmap[1][0], rmap[1][1]);

            dispTex = new TextureManager();
            dispTex->allocate(imageSize.width, imageSize.height, GL_R8, GL_RED,
                              GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

            texImg = new TextureManager*[2];

            for (int i=0;i<nrDevices;i++)
            {
                //kin->rotIr90(i);
                kin->rotColor90(i);
                kin->colUseGray(i, true);
//                kin->setImageAutoExposure(i, false);
//                kin->setImageAutoWhiteBalance(i, false);

                img[i] = cv::Mat(imageSize, CV_8UC1);
                rimg[i] = cv::Mat(imageSize, CV_8UC1);

                texImg[i] = new TextureManager();
                texImg[i]->allocate(imageSize.width, imageSize.height, GL_R8, GL_RED,
                                    GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
            }

            for (int i=0;i<nrQuads;i++)
            {
                quad[i] = new Quad(-1.f + (2.f / (float)(nrQuads)) * (float)i, -1.f,
                                   2.f / (float)(nrQuads), 2.f,
                                   glm::vec3(0.f, 0.f, 1.f),
                                   0.f, 0.f, 0.f, 1.f);
                quad[i]->rotate(M_PI, 0.f, 0.f, 1.f);
                quad[i]->rotate(M_PI, 0.f, 1.f, 0.f);
            }

            isInit = true;

        } else
        {
            shdr->begin();
            shdr->setIdentMatrix4fv("m_pvm");
            shdr->setUniform1i("tex", 0);

            glActiveTexture(GL_TEXTURE0);

            for (int i=0;i<nrDevices;i++)
            {
                if (useIr)
                {
                    printf("irframeNr[i]: %d kin->getIrFrameNr(i): %d\n", frameNr[i], kin->getIrFrameNr(i));
                    if (frameNr[i] != kin->getIrFrameNr(i))
                    {
                        std::cout << "minDispar:  " << minDispar << std::endl;
                        std::cout << "numberOfDisparities: " << numberOfDisparities << std::endl;
                        std::cout << "p1: " << p1 << std::endl;
                        std::cout << "p2: " << p2 << std::endl;
                        std::cout << "disp12MaxDiff: " << disp12MaxDiff << std::endl;
                        std::cout << "preFilterCap: " << preFilterCap << std::endl;
                        std::cout << "blockSize: " << blockSize << std::endl;
                        std::cout << "speckleRange: " << speckleRange << std::endl;
                        std::cout << "speckleWindowSize: " << speckleWindowSize << std::endl;
                        std::cout << "uniquenessRatio: " << uniquenessRatio << std::endl;


                        img[i].data = kin->getActIrImg(i);

                        //                    kin->uploadIrImg(i);
                        frameNr[i] = kin->getIrFrameNr(i);

                        cv::remap(img[i], rimg[i], rmap[i][0], rmap[i][1], cv::INTER_LINEAR);

                        glBindTexture(GL_TEXTURE_2D, texImg[i]->getId());
                        glTexSubImage2D(GL_TEXTURE_2D,             // target
                                        0,                          // First mipmap level
                                        0, 0,                       // x and y offset
                                        imageSize.width,
                                        imageSize.height,
                                        GL_RED,
                                        GL_UNSIGNED_BYTE,
                                        rimg[i].data);
                    }
                } else
                {
                    if (frameNr[i] != kin->getColFrameNr(i))
                    {
                        std::cout << "minDispar:  " << minDispar << std::endl;
                        std::cout << "numberOfDisparities: " << numberOfDisparities << std::endl;
                        std::cout << "p1: " << p1 << std::endl;
                        std::cout << "p2: " << p2 << std::endl;
                        std::cout << "disp12MaxDiff: " << disp12MaxDiff << std::endl;
                        std::cout << "preFilterCap: " << preFilterCap << std::endl;
                        std::cout << "blockSize: " << blockSize << std::endl;
                        std::cout << "speckleRange: " << speckleRange << std::endl;
                        std::cout << "speckleWindowSize: " << speckleWindowSize << std::endl;
                        std::cout << "uniquenessRatio: " << uniquenessRatio << std::endl;

                        img[i].data = kin->getActColorGrImg(i);

                        //                    kin->uploadIrImg(i);
                        frameNr[i] = kin->getColFrameNr(i);

                        cv::remap(img[i], rimg[i], rmap[i][0], rmap[i][1], cv::INTER_LINEAR);

                        glBindTexture(GL_TEXTURE_2D, texImg[i]->getId());
                        glTexSubImage2D(GL_TEXTURE_2D,             // target
                                        0,                          // First mipmap level
                                        0, 0,                       // x and y offset
                                        imageSize.width,
                                        imageSize.height,
                                        GL_RED,
                                        GL_UNSIGNED_BYTE,
                                        rimg[i].data);
                    }
                }
            }

            // tune sgbm
            sgbm->setBlockSize(blockSize);
            sgbm->setP1(p1);
            sgbm->setP2(p2);
            sgbm->setMinDisparity(minDispar);
            sgbm->setNumDisparities(numberOfDisparities);
            sgbm->setUniquenessRatio(uniquenessRatio);
            sgbm->setSpeckleWindowSize(speckleWindowSize);
            sgbm->setSpeckleRange(speckleRange);
            sgbm->setDisp12MaxDiff(disp12MaxDiff);
            sgbm->setMode(cv::StereoSGBM::MODE_SGBM);

            // calculate disparity map
//            sgbm->compute(img[0], img[1], imgDisparity16S);
            sgbm->compute(rimg[0], rimg[1], imgDisparity16S);

            //-- Check its extreme values
            cv::normalize(imgDisparity16S, imgDisparity8U, 0, 255, CV_MINMAX, CV_8U);


            glBindTexture(GL_TEXTURE_2D, dispTex->getId());
            glTexSubImage2D(GL_TEXTURE_2D,             // target
                            0,                          // First mipmap level
                            0, 0,                       // x and y offset
                            imageSize.width,
                            imageSize.height,
                            GL_RED,
                            GL_UNSIGNED_BYTE,
                            imgDisparity8U.data);
            quad[2]->draw();

            for (int i=0;i<nrDevices;i++)
            {
                glBindTexture(GL_TEXTURE_2D, texImg[i]->getId());
                quad[i]->draw();
            }

            shdr->end();
        }
	 */
}

//----------------------------------------------------

void SNTestKinectDispMap::update(double time, double dt)
{
}

//----------------------------------------------------

void SNTestKinectDispMap::onKey(int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_4 :
			minDispar += 1;
			std::cout << minDispar << std::endl;
			break;
		case GLFW_KEY_R :
			minDispar -= 1;
			std::cout << minDispar << std::endl;
			break;
		}
	}
}

//----------------------------------------------------

void SNTestKinectDispMap::pathToResources()
{
#ifdef __APPLE__
CFBundleRef mainBundle = CFBundleGetMainBundle();
CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
char path[PATH_MAX];
if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) printf("error setting path to resources \n");
CFRelease(resourcesURL);
chdir(path);
#endif
}

//----------------------------------------------------

SNTestKinectDispMap::~SNTestKinectDispMap()
{
	delete quad;
}

}
