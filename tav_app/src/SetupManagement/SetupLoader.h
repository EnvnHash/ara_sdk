/*
 *  SetupLoader.h
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 30.08.12.
 *  Copyright 2012 Sven Hahne. All rights reserved.
 *
 */

#pragma once

#include <string>
#include <iostream>
#include <map>
#include <stdlib.h>
#include <mutex>

#include <libxml++/libxml++.h>
#include <boost/algorithm/string.hpp>

#ifdef HAVE_OPENNI2
#include <KinectInput/KinectReproTools.h>
#endif
#ifdef HAVE_FREENECT2
#include <Freenect2/Freenect2In.h>
#endif

#ifdef HAVE_GSTREAMER
#include <gst/gst.h>
#endif

#include <CameraFact.h>
#include <Communication/OSC/OSCHandler.h>
#include <GLUtils/BoundingBoxer.h>
#include <GLUtils/GWindowManager.h>
#include <GLUtils/Typo/FreeTypeFont.h>
#include "SceneNodeFact.h"
#include "Sequencer/Sequencer.h"
#include <math_utils.h>

#ifdef WITH_AUDIO
#include <PAudio.h>
#include <SoundFilePlayer.h>
#endif

#ifdef HAVE_OPENCV
#include <VideoTextureCv.h>
#ifdef WITH_TRACKING
#include <VideoTextureCvActRange.h>
#endif
#endif

namespace tav
{
class SetupLoader
{
public:
	SetupLoader(char* filename, char* dataPath, bool _debug);
	~SetupLoader();

	void initBoundingBoxer();
	void initFreenect(xmlpp::Node* root);
	void initGlfw(xmlpp::Node* root);
	void initGlew();
	void initStdQuadAndShader();
	void initRoomDim(xmlpp::Node* root);
	void initMarks(xmlpp::Node* root);
	void initFboView(xmlpp::Node* root);
	void initMasks(xmlpp::Node* root);
	void initKinect(xmlpp::Node* root);
	void initOpenGlCbs();
	void initOsc(xmlpp::Node* root);
	void initPaudio(xmlpp::Node* root);
	void initSoundFilePlayer(xmlpp::Node* root);
	void initTextures(xmlpp::Node* root);
	void initVideoTextures(xmlpp::Node* root);
	void initBackTex(xmlpp::Node* root);
	void initBackVideo(xmlpp::Node* root);
	void initColors(xmlpp::Node* root);
	void initFonts(xmlpp::Node* root);
	void initCamSetup(xmlpp::Node* root, sceneData* camScd);
	void initSceneName(xmlpp::Node* root);
	void initSceneBlending(xmlpp::Node* root);
	void initSequencer(xmlpp::Node* root);

	void fillSceneDataObj(sceneData* scd, sceneData* camScd);

	void loadScenesFlat(xmlpp::Node* root, sceneData* camScd);
	void loadScenesHierachic(xmlpp::Node* root, sceneData* camScd);
	void iterateNode(xmlpp::Node* node, SceneNode* scRoot, sceneData* camScd);

	float getSingleXmlFloat(const xmlpp::Element* el, std::string name);

	void close();
	int getNrSceneNodes();
	OSCData* getOscData();
	sceneData* getSceneData(int ind);
	std::vector<CameraSet*>* getCamera();

	GWindowManager* getWinMan();
	unsigned int getNrGlfwWin();
	bool getPrintFps();
	bool useKin();
	bool to_bool(std::string const& s);
	void myNanoSleep(uint32_t ns);

	sceneStructMode scnStructMode;

	BoundingBoxer* boundBoxer;
	std::vector<CameraSet*> cam;
	glm::vec4* colors;
#ifdef HAVE_FREENECT2
	Freenect2In** fnc = 0;
#endif
	FT_Library* ft_library;
	FreeTypeFont** ft_fonts;
#ifdef HAVE_OPENNI2
	KinectInput* kin;
	KinectReproTools* kinRepro;
#endif
	Quad** maskQuads;
	Quad* stdQuad;
	Quad* stdHFlipQuad;
	GLFWmonitor** monitors;
	int nrMonitors;
	SceneNode* sceneTree;
	std::map<std::string, SceneNode*> sceneMap;
#ifdef WITH_AUDIO
	AudioTexture* audioTex;
	PAudio* pa = 0;
	SoundFilePlayer** soundFilePlayer=0;
#endif
	std::vector<SceneNode*> sceneNodes;
	ShaderCollector* shaderCollector;
	GLuint* textures;
	TextureManager** texObjs;
#ifdef HAVE_OPENCV
	VideoTextureCv** videoTextures;
#ifdef WITH_TRACKING
	VideoTextureCvActRange** videoTextsRange;
#endif
#endif
	TextureManager* backTex;
	std::string backVidPath;
	Sequencer* sequencer;
	std::vector< std::function<void()> >* openGlCbs;

	int nrSceneNodes;
	int nrWins;
	int nrMaskQuads;

	int scrWidth;
	int scrHeight;

private:
	void readFile(char* filename);

	int sampleRate;
	int maxNrChans;
	int scrXOffset;
	int scrYOffset;
	int scrNrSamples;
	int monitor;
	int monRefreshRate = 60;
	int fullScreen = 0;
	int printFps = 0;
	int swapInterval = 1;
	int nrTextures;
	unsigned int nrFonts;
	int frameSize;
	int nrFnc;
	int nrVTexts;
	int nrSoundFileLayers;

	float mscaleX = 1.0;
	float mscaleY = 1.0;
	float mscaleZ = 1.0;
	float mtransX = 1.0;
	float mtransY = 1.0;
	float mtransZ = 1.0;

	bool paudioIsReady = false;
	bool debug;

	GWindowManager& winMan;

	OSCHandler* osc_handler;
	std::string oscPortNr;
	std::string dpStr;

	glm::vec3 roomDim;

	sceneData sceneD;
	std::vector<std::map<std::string, float> > sceneArgs;// alles was szenen bezogen ist

	std::mutex paMutex;

	xmlpp::Element* nodeElement;
	xmlpp::Element* genSetEl;
	xmlpp::Node::NodeList light;
	xmlpp::Node::NodeList dFx;
	xmlpp::NodeSet player;
	xmlpp::NodeSet lights;
	xmlpp::NodeSet tex;

	std::string texAr;
	std::string setupName;

	std::vector<std::string> strs;
	std::vector<std::string> texPaths;
	std::vector<std::string> videoTexPaths;
	std::vector<fboView*>* fboViews;
	std::map<std::string, glm::vec3> mark;

#ifdef HAVE_OPENNI2
	kinectMapping kinMap;
	kinPar kPar;
#endif
};
}
