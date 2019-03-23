//
// SNGam_Libro.h
//  tav_gl4
//
//  Created by Sven Hahne on 19.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <chrono>
#include <iostream>
#include <thread>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <GLUtils/Assimp/AssimpImport.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/FBO.h>
#include <GLUtils/TextureManager.h>
#include <GeoPrimitives/QuadArray.h>
#include <Shaders/ShaderCollector.h>
#include <SceneNode.h>
#include <AnimVal.h>
//#include <PAudio.h>
//#include <SoundFilePlayer.h>

//#define SNGAMLIBRO_USE_FFMPEG

#include <lo/lo.h>


#ifndef SNGAMLIBRO_USE_FFMPEG
#include <VideoTextureCv.h>
#else
#include <FFMpegDecode.h>
#endif


namespace tav
{

class SNGam_Libro : public SceneNode
{
public:

    enum drawMode { IDLE_BLACK, PAGE_BUILDS_UP, PAGE_LOOPING, TURN_PAGE_PLUS, TURN_PAGE_MINUS };
    enum sndLayer { SND_LAYER_TURN, SND_LAYER_INTRO, SND_LAYER_LOOP };
    enum transReq { TR_TURN_PAGE_CHANGE_INTRO_AND_LOOP, TR_FADEIN_INTRO, TR_START_LOOP, TR_NONE };

    typedef struct {
    	std::string 	loop_audio;
    	std::string 	intro_audio;
    	std::string 	loop;
    	std::string 	intro;
    	double 			dur_intro;
    } pageElement;

	SNGam_Libro(sceneData* _scd, std::map<std::string, float>* _sceneArgs);
    ~SNGam_Libro();

    void initCurveData();
    void initPageData();
    void initShader();
    void initYuvShader();
    void draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo = 0);
    void cleanUp();
    void update(double time, double dt);
    void onKey(int key, int scancode, int action, int mods);
    void onCursor(double xpos, double ypos);
    void onMouseButton(int button, int action, int mods);

    void startPages();
    void fadeInFromIdle();
    void pageMinus();
    void pagePlus();
    void turnPage(int _destPage, bool forw);

    void tr_start_intro();
    void fadeInFromTurnPage(double delay);
    void tr_fade_in_intro();
    void tr_start_loop();

    void myNanoSleep(uint32_t ns);

    void loadCalib();
    void saveCalib();

private:
	AssimpImport*       					aImport;
	GWindowManager*							winMan;
	Quad*									rawquad;
	//PAudio*									pa;
	//SoundFilePlayer**						sndPlayer;

	QuadArray*								quadArray;
	SceneNode*								modelRoot=0;
	SceneNode*								modelNode=0;
	Shaders*								drawShdr=0;
	Shaders*								stdTex=0;
    ShaderCollector*						shCol;
    TextureManager*							gui_tex;
    TextureManager*							test_map_tex;

    //AnimVal<float>*							mouseX;

#ifdef SNGAMLIBRO_USE_FFMPEG
    FFMpegDecode*							intro_page_vt;
	FFMpegDecode*							loop_page_vt;
	FFMpegDecode*							turn_page_plus_vt;
	FFMpegDecode*							turn_page_minus_vt;
#else
	VideoTextureCv*							intro_page_vt;
    VideoTextureCv*							loop_page_vt;
	VideoTextureCv*							turn_page_plus_vt;
	VideoTextureCv*							turn_page_minus_vt;
	VideoTextureCv** 						vts;
#endif

	lo_address 								scAddr;

    Median<float>*							mouseX;

	AnimVal<float>**						opacity;
    std::string								calibFileName;

    bool									blockInteraction;
    bool									requestFadeIn;
    bool									decodeYuv420OnGpu;

    unsigned int 							nrCurveSymPoints;
    unsigned int							nrSoundFileLayers;
    unsigned int							nrLevels;
    unsigned int							nrPages;
    unsigned int							requestPushButton=-1;

    int										dstWidth;
	int										dstHeight;
	int										nrDecodeThreads;

    int										actPage;
    int										destPage;

    float 									nrQuadsX;
    float 									nrQuadsY;

    float 									aspectRatio;
    float									far;
    float									fov;
    float									near;
    float									tx;
    float									ty;
    float									tz;
    float									rx;
    float									ry;
    float									rz;
    float									sx;
    float									sy;
    float									sz;
    float									zoomRatio;
    float									yDistAmt;
    float									borderSizeX;
    float									borderSizeY;

    double									actTime;
    double									fadeInTime;
    double									pageBuildUpTime;
    double									pageBlendTurnFadeInTime;
    double									pageFadeTime;
    double									turnPageTime;

    double									lastChange=0.0;

    glm::vec2 								mousePos;
    glm::vec2* 								curveSymetric;

    glm::vec2**								buttPos;

    glm::mat4								projMat;
    glm::mat4								viewMat;

    drawMode								actDrawMode;

    std::thread*							fadeInThread;
    pageElement*							pages;

    std::string								basePath;
    std::string								audioBasePath;
    std::string								turn_page_audio;


    transReq								request;

  //  std::mutex								changeIntroMtx;
  //  std::mutex								changeLoopMtx;

};

}
