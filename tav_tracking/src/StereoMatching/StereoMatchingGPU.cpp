//
//  StereoMatchingGPU.cpp
//  tav_tracking
//
//  Created by Sven Hahne on 7/7/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "StereoMatchingGPU.h"

using namespace std;

namespace tav
{
StereoMatchingGPU::StereoMatchingGPU(KinectInput* _kin, ShaderCollector* _shCol) :
		kin(_kin), shCol(_shCol), dMin(0), dMax(30), defaultBorderCost(0.999), lambdaAD(
				10.f), lambdaCensus(30.f), aggregatingIterations(4), colorThreshold1(
				20), colorThreshold2(6), maxLength1(34), maxLength2(17), colorDifference(
				15), pi1(0.1), pi2(0.3), dispTolerance(0), votingThreshold(20), votingRatioThreshold(
				0.4), maxSearchDepth(20), cannyThreshold1(20), cannyThreshold2(
				60), cannyKernelSize(3), blurKernelSize(3), bInit(false), bValid(
				true), useTestFile(true), bAggrInit(false)
{
	censusWin = cv::Size(9, 7);
	texShader = shCol->getStdTex();

	if (useTestFile)
	{
		testImgFile = new cv::Mat[2];
		testImgFile[0] = cv::imread(
				"/Users/useruser/tav_data/textures/01left_picture.jpg");
		testImgFile[1] = cv::imread(
				"/Users/useruser/tav_data/textures/01right_picture.jpg");
	}

	// --- load camera calibration -------------------------------------------

	cameraMatrix[0] = cv::Mat::eye(3, 3, CV_64F);
	cameraMatrix[1] = cv::Mat::eye(3, 3, CV_64F);

	cv::FileStorage fs("kinect_stereo_intr.yml", cv::FileStorage::READ);
	if (!fs.isOpened())
		printf("couldn´t open kinect_stereo_intr.yml\n");

	fs["M1"] >> cameraMatrix[0];
	fs["M2"] >> cameraMatrix[1];
	fs["D1"] >> distCoeffs[0];
	fs["D2"] >> distCoeffs[1];
	fs.release();

	cv::FileStorage fe("kinect_stereo_extr.yml", cv::FileStorage::READ);
	if (!fe.isOpened())
		printf("couldn´t open kinect_stereo_extr.yml\n");

	fe["R"] >> R;
	fe["T"] >> T;
	fe["R1"] >> R1;
	fe["R2"] >> R2;
	fe["P1"] >> P1;
	fe["P2"] >> P2;
	fe["Q"] >> Q;
	fe.release();

	// --- check parameters -------------------------------------------

	if (!useTestFile)
		bValid &= kin->getNrDevices() == 2;
	bValid &= dMin < dMax;
	bValid &= censusWin.height > 2 && censusWin.width > 2
			&& censusWin.height % 2 != 0 && censusWin.width % 2 != 0;
	bValid &= defaultBorderCost >= 0 && defaultBorderCost < 1 && lambdaAD >= 0
			&& lambdaCensus >= 0 && aggregatingIterations > 0;
	bValid &= colorThreshold1 > colorThreshold2 && colorThreshold2 > 0;
	bValid &= maxLength1 > maxLength2 && maxLength2 > 0;
	bValid &= colorDifference > 0 && pi1 > 0 && pi2 > 0;
	bValid &= votingThreshold > 0 && votingRatioThreshold > 0
			&& maxSearchDepth > 0;
	bValid &= blurKernelSize > 2 && blurKernelSize % 2 != 0;
	bValid &= cannyThreshold1 < cannyThreshold2;
	bValid &= cannyKernelSize > 2 && cannyKernelSize % 2 != 0;

	if (!bValid)
	{
		printf("configuration error!!! \n");
	}
	else
	{
		/*
		 dispRef = new DisparityRefinement(dispTolerance, dMin, dMax, votingThreshold,
		 votingRatioThreshold, maxSearchDepth, blurKernelSize,
		 cannyThreshold1, cannyThreshold2, cannyKernelSize);
		 */
	}
}

void StereoMatchingGPU::init()
{
	if (!useTestFile)
	{
		nrDevices = kin->getNrDevices();
		imageSize = cv::Size(kin->getColorHeight(), kin->getColorWidth());
		halfImageSize = cv::Size(kin->getColorHeight() / 2,
				kin->getColorWidth() / 2);
	}
	else
	{
		imageSize = cv::Size(testImgFile[0].cols, testImgFile[0].rows);
		halfImageSize = cv::Size(testImgFile[0].cols, testImgFile[0].rows);
		//halfImageSize = cv::Size(testImgFile[0].cols / 2, testImgFile[0].rows / 2);
		nrDevices = 2;
	}

	img = new cv::Mat[nrDevices];
	//gray = new cv::UMat[nrDevices];
	rimg = new cv::Mat[nrDevices];
	frameNr = new unsigned int[nrDevices];
	debugQuads = new Quad*[nrDevices];

	for (unsigned int i = 0; i < nrDevices; i++)
	{
		if (!useTestFile)
		{
			kin->rotColor90(i);
			// kin->colUseGray(i, true);
			kin->setImageAutoExposure(false, i);
			kin->setImageAutoWhiteBalance(false, i);
		}

		img[i] = cv::Mat(imageSize, CV_8UC3);
		rimg[i] = cv::Mat(imageSize, CV_8UC3);
		frameNr[i] = -1;

		debugQuads[i] = new Quad(-1.f + (2.f / (float) (nrDevices)) * (float) i,
				-1.f, 2.f / (float) (nrDevices), 2.f, glm::vec3(0.f, 0.f, 1.f),
				0.f, 0.f, 0.f, 1.f);
		debugQuads[i]->rotate(M_PI, 0.f, 0.f, 1.f);
		debugQuads[i]->rotate(M_PI, 0.f, 1.f, 0.f);

		fullQuad = new Quad(-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 0.f,
				0.f, 0.f, 0.f);
	}

	// --- init textures parameters -------------------------------------------

	imgTex = new TextureManager*[nrDevices];
	for (unsigned short i = 0; i < nrDevices; i++)
	{
		imgTex[i] = new TextureManager();
		imgTex[i]->allocate(imageSize.width, imageSize.height, GL_RGB8, GL_BGR,
		GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
	}

	// --- init camera calibration -------------------------------------------

	cv::stereoRectify(cameraMatrix[0], distCoeffs[0], cameraMatrix[1],
			distCoeffs[1], imageSize, R, T, R1, R2, P1, P2, Q,
			cv::CALIB_ZERO_DISPARITY, 1, imageSize, &validRoi[0], &validRoi[1]);

	cv::initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1,
			imageSize, CV_16SC2, rmap[0][0], rmap[0][1]);
	cv::initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2,
			imageSize, CV_16SC2, rmap[1][0], rmap[1][1]);

	// --- init shaders -------------------------------------------------------

	adShader = new Shaders("shaders/adcensus.vert", "shaders/adcensus.frag",
			true);
	adShader->link();

	adAggrNormShader = new Shaders("shaders/adAggrNorm.vert",
			"shaders/adAggrNorm.frag", true);
	adAggrNormShader->link();

	adAggrShader = new Shaders("shaders/adAggregation.vert",
			"shaders/adAggregation.frag", true);
	adAggrShader->link();

	// 8 attachment
	nrAdFboAttachments = 8;
	nrCostFbos = static_cast<int>(std::ceil(
			static_cast<float>(std::abs(dMax - dMin))
					/ static_cast<float>(nrAdFboAttachments)));

	windowSizePP = new PingPongFbo(shCol, halfImageSize.width,
			halfImageSize.height, GL_R32F, GL_TEXTURE_2D);
	cout << "windowSizePP texIds: " << windowSizePP->getSrcTexId() << ", "
			<< windowSizePP->getDstTexId() << endl;

	aggrTempFBO = new FBO(shCol, halfImageSize.width, halfImageSize.height,
			GL_R32F, GL_TEXTURE_2D, false, 2, 1, 1, GL_REPEAT, false);
	aggrNormTempFBO = new FBO(shCol, halfImageSize.width, halfImageSize.height,
			GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_REPEAT, false);

	costMaps = new PingPongFbo*[nrCostFbos];

	for (int i = 0; i < nrCostFbos; i++)
	{
		costMaps[i] = new PingPongFbo(shCol, halfImageSize.width,
				halfImageSize.height, GL_R32F, GL_TEXTURE_2D, false,
				nrAdFboAttachments);
	}

	// -------------------------------------------------------------------------

	aggregation = new Aggregation(imgTex, &halfImageSize, shCol,
			colorThreshold1, colorThreshold2, maxLength1, maxLength2);
}

void StereoMatchingGPU::update()
{
	if (!bInit && bValid)
	{
		init();

		if (useTestFile)
		{
			for (unsigned int i = 0; i < nrDevices; i++)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, imgTex[i]->getId());
				glTexSubImage2D(GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0, 0,                       // x and y offset
						imageSize.width, imageSize.height,
						GL_BGR,
						GL_UNSIGNED_BYTE, testImgFile[i].data);
			}
		}

		bInit = true;
	}

	if (!useTestFile && bInit && bValid && kin->isReady())
	{
		glActiveTexture(GL_TEXTURE0);

		// get new images from kinect
		for (unsigned int i = 0; i < nrDevices; i++)
		{
			if (frameNr[i] != (unsigned int) kin->getColFrameNr(i))
			{
				img[i].data = kin->getActColorImg(i);
				frameNr[i] = kin->getColFrameNr(i);

				cv::remap(img[i], rimg[i], rmap[i][0], rmap[i][1],
						cv::INTER_LINEAR);

				glBindTexture(GL_TEXTURE_2D, imgTex[i]->getId());
				glTexSubImage2D(GL_TEXTURE_2D,             // target
						0,                          // First mipmap level
						0, 0,                       // x and y offset
						imageSize.width, imageSize.height,
						GL_RGB,
						GL_UNSIGNED_BYTE, rimg[i].data);
			}
		}
	}

	compute();
}

void StereoMatchingGPU::compute()
{
	printf(" \n");
	printf("cost init \n");

	costInitialization();
	printf("cost aggregation \n");
	costAggregation();

	//testR32();

	/*
	 scanlineOptimization();
	 outlierElimination();
	 regionVoting();
	 properInterpolation();
	 discontinuityAdjustment();
	 subpixelEnhancement();
	 dispComputed = true;
	 */
}

void StereoMatchingGPU::testR32()
{
	if (bInit)
	{
		int directionH = 1, directionW = 0;

		for (int direction = 0; direction < 2; direction++)
		{
			windowSizePP->dst->bind();

			adAggrShader->begin();
			adAggrShader->setIdentMatrix4fv("m_pvm");
			//adAggrShader->setUniform1i("costMapIn", 0);
			adAggrShader->setUniform1i("winSizeIn", 0);

			adAggrShader->setUniform1i("leftLimits", 2);
			adAggrShader->setUniform1i("rightLimits", 3);
			adAggrShader->setUniform1i("upLimits", 4);
			adAggrShader->setUniform1i("downLimits", 5);

			adAggrShader->setUniform1i("initWhite", direction);
			adAggrShader->setUniform1i("directionH", directionH);
			adAggrShader->setUniform1i("directionW", directionW);
			adAggrShader->setUniform1f("width",
					static_cast<float>(imageSize.width));
			adAggrShader->setUniform1f("height",
					static_cast<float>(imageSize.height));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, windowSizePP->getSrcTexId());

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D,
					aggregation->leftLimits[0]->getColorImg());
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D,
					aggregation->rightLimits[0]->getColorImg());
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D,
					aggregation->upLimits[0]->getColorImg());
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D,
					aggregation->downLimits[0]->getColorImg());

			fullQuad->draw();

			windowSizePP->dst->unbind();
			windowSizePP->swap();

		}

		shCol->getStdTex()->begin();
		shCol->getStdTex()->setIdentMatrix4fv("m_pvm");
		shCol->getStdTex()->setUniform1i("tex", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, windowSizePP->getSrcTexId());
//            glBindTexture(GL_TEXTURE_2D, aggregation->leftLimits[0]->getColorImg());

		fullQuad->draw();
	}
}

void StereoMatchingGPU::costInitialization()
{
	// render to 8 destination in one runthrough
	for (int fboNr = 0; fboNr < nrCostFbos; fboNr++)
	{
		costMaps[fboNr]->dst->bind();

		adShader->begin();
		adShader->setIdentMatrix4fv("m_pvm");
		adShader->setUniform1i("leftImg", 0);
		adShader->setUniform1i("rightImg", 1);
		adShader->setUniform1f("width_step", 1.f / halfImageSize.width);
		adShader->setUniform1f("height_step", 1.f / halfImageSize.height);
		adShader->setUniform1i("nrAttachments", nrAdFboAttachments);
		adShader->setUniform1f("lambdaAD", lambdaAD);
		adShader->setUniform1f("lambdaCensus", lambdaCensus);
		adShader->setUniform1i("censWHalf", censusWin.width / 2);
		adShader->setUniform1i("censHHalf", censusWin.height / 2);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, imgTex[0]->getId());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, imgTex[1]->getId());

		adShader->setUniform1f("dispOffs",
				static_cast<float>(fboNr * nrAdFboAttachments));
		fullQuad->draw();

		costMaps[fboNr]->dst->unbind();
		costMaps[fboNr]->swap();
	}
}

void StereoMatchingGPU::costAggregation()
{
	if (!bAggrInit)
	{
		aggregation->computeLimits();
		bAggrInit = true;
	}

	for (int fboNr = 0; fboNr < 1; fboNr++)
	{
		for (int attNr = 0; attNr < 1; attNr++)
		{
			bool horizontalFirst = true;

			for (unsigned int i = 0; i < aggregatingIterations; i++)
			{
				int directionH = 1, directionW = 0;

				if (horizontalFirst)
					std::swap(directionH, directionW);

				for (int direction = 0; direction < 2; direction++)
				{
					aggrTempFBO->assignTex(0,
							costMaps[fboNr]->getDstTexId(attNr));
					aggrTempFBO->assignTex(1, windowSizePP->getDstTexId());
					aggrTempFBO->bind();

					adAggrShader->begin();
					adAggrShader->setIdentMatrix4fv("m_pvm");
					adAggrShader->setUniform1i("costMapIn", 0);
					adAggrShader->setUniform1i("winSizeIn", 1);

					adAggrShader->setUniform1i("leftLimits", 2);
					adAggrShader->setUniform1i("rightLimits", 3);
					adAggrShader->setUniform1i("upLimits", 4);
					adAggrShader->setUniform1i("downLimits", 5);

					adAggrShader->setUniform1i("initWhite", direction);
					adAggrShader->setUniform1i("aggrIt", i);
					adAggrShader->setUniform1f("costFactor", 1.f / 65000.f);
					adAggrShader->setUniform1i("directionH", directionH);
					adAggrShader->setUniform1i("directionW", directionW);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D,
							costMaps[fboNr]->getSrcTexId(attNr));
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, windowSizePP->getSrcTexId());

					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D,
							aggregation->leftLimits[0]->getColorImg());
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D,
							aggregation->rightLimits[0]->getColorImg());
					glActiveTexture(GL_TEXTURE4);
					glBindTexture(GL_TEXTURE_2D,
							aggregation->upLimits[0]->getColorImg());
					glActiveTexture(GL_TEXTURE5);
					glBindTexture(GL_TEXTURE_2D,
							aggregation->downLimits[0]->getColorImg());

					fullQuad->draw();

					aggrTempFBO->unbind();

					windowSizePP->swap();
					costMaps[fboNr]->swap();

					std::swap(directionH, directionW);

					string path = "winSize_tav/tav_costMaps_itNr_"
							+ std::to_string(i) + "_dir_"
							+ std::to_string(direction) + ".png";
					aggregation->saveToFloatDisparity(
							costMaps[fboNr]->getSrcTexId(0), imageSize, path,
							true);
				}

//                    string path = "winSize_tav/tav_winSize_it_"+std::to_string(i)+".png";
//                    aggregation->saveToFloatDisparity(costMaps[fboNr]->getSrcTexId(0), imageSize, path, true);

				aggrNormTempFBO->assignTex(0,
						costMaps[fboNr]->getDstTexId(attNr));
				aggrNormTempFBO->bind();

				adAggrNormShader->begin();
				adAggrNormShader->setIdentMatrix4fv("m_pvm");
				adAggrNormShader->setUniform1i("costMap", 0);
				adAggrNormShader->setUniform1i("winSize", 1);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D,
						costMaps[fboNr]->getSrcTexId(attNr));

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, windowSizePP->getSrcTexId());

				fullQuad->draw();

				aggrNormTempFBO->unbind();

				costMaps[fboNr]->swap();

				horizontalFirst = !horizontalFirst;
			}
		}
	}
}

void StereoMatchingGPU::renderRectified()
{
	if (bInit)
	{
		glActiveTexture(GL_TEXTURE0);

		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

		for (unsigned int i = 0; i < nrDevices; i++)
		{
			glBindTexture(GL_TEXTURE_2D, imgTex[i]->getId());
			debugQuads[i]->draw();
		}
	}
}

void StereoMatchingGPU::renderCost()
{
	if (bInit)
	{
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);

		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

		glBindTexture(GL_TEXTURE_2D, costMaps[0]->getSrcTexId(0));
		debugQuads[0]->draw();
	}
}

void StereoMatchingGPU::renderWinSize()
{
	if (bInit)
	{
		glDisable(GL_DEPTH_TEST);
		glActiveTexture(GL_TEXTURE0);

		texShader->begin();
		texShader->setIdentMatrix4fv("m_pvm");
		texShader->setUniform1i("tex", 0);

//            glBindTexture(GL_TEXTURE_2D, aggregation->leftLimits[0]->getColorImg());
		glBindTexture(GL_TEXTURE_2D, windowSizePP->getSrcTexId());

		debugQuads[0]->draw();
	}
}

StereoMatchingGPU::~StereoMatchingGPU()
{
	adShader->remove();
	delete adShader;
	adAggrShader->remove();
	delete adAggrShader;
	delete[] costMaps;
	delete aggregation;
}
}
