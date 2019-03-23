//
//  main.cpp
//  Tav_StdApp
//
//  Created by Sven Hahne on 30/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include <vector>
#include <iostream>
#include <pwd.h>

#include <headers/gl_header.h>
#include "SetupManagement/SetupLoader.h"
#include "SceneNodeBlender.h"

#ifdef WITH_VIDEO
#ifdef WITH_AUDIO
#include "MediaRecorder.h"
MediaRecorder* recorder;
#endif
#endif

using namespace tav;

SetupLoader* sl;
SceneNodeBlender* blender;

bool snapShotMode = true;
bool debug = false;
bool recordVideo = true;

#ifdef __linux__
void DebugCallbackFunction(GLenum source, GLenum type, GLuint id,
		GLenum severity, GLsizei length, const GLchar* message,
		const void* userParam)
{
	printf("Debug Message: SOURCE(0x%04X)," "TYPE(0x%04X)," "ID(0x%08X),"
			"SEVERITY(0x%04X), \"%s\"\n", source, type, id, severity, message);
}
#endif

//---------------------------------------------------------------

void init()
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
	{
		(*cIt)->addLightProto("NoLight", sl->scnStructMode);
		(*cIt)->addLightProto("DirLight", sl->scnStructMode);
		(*cIt)->addLightProto("DirLightNoTex", sl->scnStructMode);
		(*cIt)->addLightProto("LitSphere", sl->scnStructMode);
		(*cIt)->addLightProto("PerlinAudioSphere", sl->scnStructMode);
		(*cIt)->addLightProto("PerlinTexCoord", sl->scnStructMode);
	}

	if (debug)
	{
		getGlError();
	}

	if (sl->scnStructMode == BLEND || sl->scnStructMode == FLAT)
	{
		blender = new SceneNodeBlender(sl);
		blender->setSnapShotMode(snapShotMode);

		if (debug)
		{
			getGlError();
			std::cout << "--- SceneNodeBlender init finished" << std::endl;
		}
	}

	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClearDepth(1.0f);

	glEnable(GL_BLEND);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_SCISSOR_TEST);          // if using glScissor
	glEnable(GL_LINE_SMOOTH);// bei der implementation von nvidia gibt es nur LineWidth 0 -1 ...
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glFrontFace(GL_CCW); // counter clockwise definition means front, as default war GL_CCW ging aber nicht mit std quad
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // ohne dass kein antialiasing

	glEnable(GL_DEPTH_BOUNDS_TEST_EXT); // is needed for drawing cubes
	glEnable(GL_CULL_FACE);             // is needed for drawing cubes
	glEnable(GL_STENCIL_TEST);          // is needed for drawing cubes

	//glEnable(GL_PRIMITIVE_RESTART);     // is needed for drawing cubes
	//glPrimitiveRestartIndex(0xFFFF);

	getGlError();

#ifdef __linux__
	if (debug)
	{
		glEnable(GL_DEBUG_OUTPUT);
		//glDebugMessageCallback(DebugCallbackFunction, 0);
	}
	else
	{
		glDisable(GL_DEBUG_OUTPUT);
	}
#endif

#ifdef WITH_VIDEO
#ifdef WITH_AUDIO
	if (recordVideo)
	{
		recorder = new MediaRecorder(
				"/home/sven/tav_data/recordings/hyperlapse.mov", sl->pa,
				sl->getSceneData(0)->screenWidth,
				sl->getSceneData(0)->screenHeight,
				sl->getSceneData(0)->monRefRate, "nvenc_h264");
		recorder->setOutputSize(1280, 768);
		recorder->setOutputFramerate(sl->getSceneData(0)->monRefRate);
		recorder->setVideoBitRate(recorder->getOutputWidth() * recorder->getOutputHeight()
						* sl->getSceneData(0)->monRefRate * 3 / 20);
		recorder->setAudioBitRate(128000);
	}
#endif
#endif
	if (debug)
	{
		std::cout << "--- end init" << std::endl;
		getGlError();
	}
}

//---------------------------------------------------------------

inline void preDisp(std::vector<CameraSet*>::iterator cIt, double time,
		double dt, unsigned int ctxNr)
{
	if (ctxNr == 0)
		theOscData.getNewVals(dt);    // Glättung der OSC Daten

#ifdef WITH_AUDIO
	if (sl->pa)
		sl->pa->setPllLoP(theOscData.audioSmooth);
#endif

	// process opengl callbacks from other threads and delete them afterwards
	std::vector< std::function<void()> >* openGlCbs = static_cast< std::vector< std::function<void()> >* >( (*cIt)->scd->openGlCbs );
	for (std::vector< std::function<void()> >::iterator cbIt = openGlCbs->begin(); cbIt != openGlCbs->end(); ++cbIt)
		(*cbIt)();
	openGlCbs->clear();


	(*cIt)->clearScreen();
	(*cIt)->preDisp(time, dt);
}

//---------------------------------------------------------------

inline void postDisp(std::vector<CameraSet*>::iterator cIt, double time,
		double dt, unsigned int ctxNr)
{
	(*cIt)->renderFbos(time, dt, ctxNr);

#ifdef WITH_VIDEO
#ifdef WITH_AUDIO
	if (recordVideo)
		recorder->downloadFrame();
#endif
#endif

	if (debug)
		getGlError();
}

//---------------------------------------------------------------
// one draw function for all contexts
void display(double time, double dt, unsigned int ctxNr)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
	{
		preDisp(cIt, time, dt, ctxNr);
		(*cIt)->render(blender->getSceneNode0(), time, dt, ctxNr);
		postDisp(cIt, time, dt, ctxNr);
	}
}

//---------------------------------------------------------------
// one draw function for all contexts
void displayBlend(double time, double dt, unsigned int ctxNr)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
	{
		preDisp(cIt, time, dt, ctxNr);
		if (ctxNr == 0)
			blender->blend(time, dt);
		(*cIt)->render(blender->getMorphSceneNode(), time, dt, ctxNr);
		postDisp(cIt, time, dt, ctxNr);
	}
}

//---------------------------------------------------------------
// one draw function for all contexts
void displayNode(double time, double dt, unsigned int ctxNr)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin(); cIt != sl->cam.end(); ++cIt)
	{
		preDisp(cIt, time, dt, ctxNr);
		(*cIt)->renderTree(sl->sceneTree, time, dt, ctxNr);
		postDisp(cIt, time, dt, ctxNr);
	}
}

//---------------------------------------------------------------

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
	{
		(*cIt)->onKey(key, scancode, action, mods);

		// irgendwie muss noch definiert werden, welche Camera oder Fbo der Recorder aufnimmt...
		if (cIt == sl->cam.begin())
		{
#ifdef HAVE_OPENNI2
			if (sl->useKin())
				sl->kin->onKey(window, key, scancode, action, mods);
			//if (sl->sceneBlending) blender->getMorphSceneNode()->onKey(key, scancode, action, mods);
			//if(freenect2Motion) freenect2Motion->onKey(key, scancode, action, mods);
#endif

			if (action == GLFW_PRESS && mods != GLFW_MOD_SHIFT)
			{
				switch (key)
				{
				case GLFW_KEY_S:
#ifdef WITH_VIDEO
#ifdef WITH_AUDIO
					if (recordVideo)
					{
						if (!recorder->isRecording())
							recorder->record();
						else
							recorder->stop();
					}
					break;
#endif
#endif
				case GLFW_KEY_ESCAPE:
					sl->close();
					break;
				}
			}
		}
	}
}

//---------------------------------------------------------------

void mouseButCb(GLFWwindow* window, int button, int action, int mods)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
		(*cIt)->mouseBut(window, button, action, mods);
}

//---------------------------------------------------------------

void mouseCursorCb(GLFWwindow* window, double xpos, double ypos)
{
	for (std::vector<CameraSet*>::iterator cIt = sl->cam.begin();
			cIt != sl->cam.end(); ++cIt)
		(*cIt)->mouseCursor(window, xpos, ypos);
}

//---------------------------------------------------------------

int main(int argc, char* argv[])
{
	// setup loader startet alles
	// ffmpeg Videotexturen brauchen einiges an threads (ca 7 pro video)
	// der data path ist für alle texturen und bilder der "/" am Ende ist wichtig!!!
	// der einfachheit halber muss der data path im User Verzeichnis sein
	struct passwd *pw = getpwuid(getuid());
	std::string homedir = std::string(pw->pw_dir);
	std::string dataPath("/tav_data/");
	dataPath = homedir + dataPath;

	try
	{
		if (argc < 2){
			std::cerr << "setup.xml argument missing!" << std::endl;
		} else {
			sl = new SetupLoader(argv[1], (char*) dataPath.c_str(), debug);
		}
	} catch (std::exception& e)
	{
		std::cerr << "--- failed to init SetupLoader" << e.what();
	}

	if (debug)
	{
		getGlError();
		std::cout << "--- start init" << std::endl;
	}

	init();

	if (debug)
	{
		getGlError();
		std::cout << "--- finished init" << std::endl;
	}

	sl->getWinMan()->setGlobalMouseButtonCallback(mouseButCb);
	sl->getWinMan()->setGlobalMouseCursorCallback(mouseCursorCb);
	sl->getWinMan()->setGlobalKeyCallback(onKey);

	if (debug)
	{
		getGlError();
		std::cout << "--- start main loop" << std::endl;
	}

	switch (sl->scnStructMode)
	{
	case FLAT:
		sl->getWinMan()->runMainLoop(display);
		break;
	case BLEND:
		sl->getWinMan()->runMainLoop(displayBlend);
		break;
	case NODE:
		sl->getWinMan()->runMainLoop(displayNode);
		break;
	}

	delete sl;

	return 0;
}
