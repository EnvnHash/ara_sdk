/*
 * SNBodyWrite.h
 *
 *  Created on: 18.10.2017
 *      Author: sven
 */

#ifndef SRC_ENEL_SNBODYWRITE_H_
#define SRC_ENEL_SNBODYWRITE_H_

#pragma once

#include <string>

#include <GLUtils/GLSL/FastBlurMem.h>
#include <GLUtils/GLSL/GLSLHistogram.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/PingPongFbo.h>
#include <KinectInput/KinectInput.h>
#include <KinectInput/KinectReproTools.h>
#include <GLUtils/Typo/FreetypeTex.h>
#include <SceneNode.h>

namespace tav
{

class SNBodyWrite : public SceneNode
{
public:
    enum drawMode { RAW_DEPTH, DEPTH_THRESH, DEPTH_BLUR, DIFF, DRAW };

	SNBodyWrite(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
	virtual ~SNBodyWrite();

    void initDepthThresh();
    void initDepthThresh2();
    void initDiffShdr();
    void initSilShdr();
    void initSnapShotShdr();

    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = nullptr);
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods);

private:
	GLSLHistogram*					histo;
    GWindowManager*                 winMan;

    FBO*                            diffFbo;
    FBO*                            recFbo;
    FBO*                            threshFbo;
    FBO*							thresh2Fbo;

    FBO**							snapShotFbos;

    std::vector< std::vector< FreetypeTex* > >	ft;

    FastBlurMem*                    fblur;
    FastBlurMem*                    fblur2nd;

    KinectInput*                    kin;
    KinectReproTools*				kinRepro;

    Quad*							rawQuad;
    Quad*							rotateQuad;

    Shaders*						depthThres;
    Shaders*						depthThres2;
    Shaders*						diffShader;
    Shaders*						texShader;
    Shaders*						texAlphaShader;
    Shaders*						silShdr;
    Shaders*						snapShotShdr;

    ShaderCollector*				shCol;

    drawMode                        actDrawMode;

    bool							inited;
    bool							gotActivity=false;
    bool							permitRecord = false;
    bool							startRec = false;
    bool							tookSnapShot = false;
    bool							manSnap = false;

    unsigned int 					nrEnelColors;
    unsigned int 					enelColPtr;
    unsigned int 					lastDepthFrame=0;
    unsigned int 					maxNrSnapShotFbos;
    unsigned int 					snapShotPtr=0;
    unsigned int 					nrActiveSnapshots=0;

    unsigned int 					nrWords;
    unsigned int 					wordPtr;
    unsigned int 					charPtr;

    glm::vec4*						enel_colors;

	float							depthThresh;
	float							depth2Thresh;
	float							fastBlurAlpha;
	float							noActivityThresh;
	float							recordThresh;
	float							secondThres;
	float							snapPicWidth;
	float							yPos;

	float							trig;
	float							clear;

	double							startRecTime;
	double							tookSnapShotTime;
	double							resetSnapShotTime;

	GLint							transTexId=0;

	Median<float>*					energySum;
};

} /* namespace tav */

#endif /* SRC_ENEL_SNBODYWRITE_H_ */
