/*
 * SNTNiteMultMorph.h
 *
 *  Created on: 13.03.2016
 *      Copyright by Sven Hahne
 */

#ifndef TRACKING_SNTNITEMULTMORPH_H_
#define TRACKING_SNTNITEMULTMORPH_H_

#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <chrono>
#include <vector>

#include "SNFreenect2Motion.h"

#include <NiTE/NISkeleton.h>
#include <KinectInput/KinectInput.h>

#include <GeoPrimitives/QuadArray.h>
#include <GLUtils/Noise3DTexGen.h>
#include <AnimVal.h>
#include <SceneNode.h>

namespace tav
{

class SNTNiteMultMorph : public SceneNode
{
public:
	typedef struct {
		bool 				active = false;
		bool 				isDupl = false;
		bool				isFading = false;
		unsigned short		duplCamId;
		unsigned short		duplUserId;
		unsigned short		assignedCamId;
		unsigned short		assignedUserId;
		unsigned short		lastAssignedCamId;
		unsigned short		lastAssignedUserId;

		glm::vec4 			pos = glm::vec4(0.f, 0.f, 0.f, 1.f);
		glm::vec4 			lastPos = glm::vec4(0.f, 0.f, 0.f, 1.f);
		glm::vec4 			vel = glm::vec4(0.f, 0.f, 0.f, 0.f);
		unsigned short 		id;
		std::chrono::time_point<std::chrono::high_resolution_clock,
		                                std::chrono::nanoseconds> startTime;
		std::chrono::time_point<std::chrono::high_resolution_clock,
				                                std::chrono::nanoseconds> lastUpdt;
        double				onTime;
        CvKalmanFilter* 	kFilt=0;
        glm::mat4			transMat;
        glm::mat4			transMat2;
        AnimVal<float>*		fadeOut=0;
        glm::vec3			noiseOffs;

	} userData;

	typedef struct {
		bool 				active = false;
		glm::vec4 			pos = glm::vec4(0.f, 0.f, 0.f, 1.f);
		glm::vec4 			lastPos = glm::vec4(0.f, 0.f, 0.f, 1.f);
		glm::vec4 			vel = glm::vec4(0.f, 0.f, 0.f, 0.f);
		short*			 	id;
		std::chrono::time_point<std::chrono::high_resolution_clock,
		                                std::chrono::nanoseconds> startTime;
		std::chrono::time_point<std::chrono::high_resolution_clock,
				                                std::chrono::nanoseconds> lastUpdt;
        double				onTime=0;
        CvKalmanFilter* 	kFilt=0;
        glm::mat4			transMat;
        glm::mat4			transMat2;
        AnimVal<float>*		fadeOut;
        glm::vec3			noiseOffs;
	} singleData;

	SNTNiteMultMorph(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	~SNTNiteMultMorph();


    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void update(double time, double dt);
    void applyMapping(glm::vec4* pos, unsigned short camNr);
    //void applyMapping(userData* _user, unsigned short camNr);
    void initShdr(camPar* cp);



    void cleanUp();

    void onKey(int key, int scancode, int action, int mods){}

private:
    KinectInput* 					kin;
    NISkeleton**     				nis;
    SNFreenect2Motion::mapConfig*	mapConf;
    TextureManager*     			objTex;
    TextureManager*     			objTexInv;
    TextureManager*     			objTexSombra;
    TextureManager*     			objTexInvSombra;
    Shaders*           				explShdr;
    QuadArray*          			quadAr;
    OSCData* 						osc;
    Noise3DTexGen*     				noiseTex;
    ShaderCollector*				shCol;

    glm::vec3						normRoomDim;

    unsigned short					nrDevices;
    unsigned short					maxNrUsers;
    int*							nisFrameNr;

    bool							inited = false;
    float 							posSameThres;
    float							velSameThres;
    float              				objSize;
    float							roomScaleF;

    double							timeToDeact;
    double 							timeToDeactSingle;
    double							onTimeToAdd;
    double							fadeInTime;
    double							fadeOutTime;

    userData**						userDat;
    std::vector<singleData*>		singleUserDat;
    AnimVal<float>*					testFade;
};

}

#endif /* TRACKING_SNTNITEMULTMORPH_H_ */
