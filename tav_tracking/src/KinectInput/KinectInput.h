//
//  KinectInput.h
//  KinectInput
//
//  Created by Sven Hahne on 13.04.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//
//  weitere Einstellungen gehen in /usr/etc/primesense/GlobalDefaults.ini

#pragma once

#include <iostream>
#include <map>
#include <glm/glm.hpp>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include "KinectColorStreamListener.h"
#include "KinectDepthStreamListener.h"
#include "KinectIrStreamListener.h"
#include <PS1080.h>
#include <GLFW/glfw3.h>

#define MAX_STRINGS 20

// Undeprecate CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#if (ONI_PLATFORM == ONI_PLATFORM_WIN32)
#include <conio.h>
#include <direct.h>
#elif (ONI_PLATFORM == ONI_PLATFORM_LINUX_X86 || ONI_PLATFORM == ONI_PLATFORM_LINUX_ARM || ONI_PLATFORM == ONI_PLATFORM_MACOSX)
#define _getch() getchar()
#endif

namespace tav
{

enum ki_StreamType
{
	KI_IR = 0, KI_COLOR = 1, KI_DEPTH = 2, KI_NR_STREAMS = 3
};

typedef struct
{
	int nValuesCount;
	unsigned int pValues[MAX_STRINGS];
	const char* pValueToName[MAX_STRINGS];
} DeviceParameter;

typedef struct
{
	openni::VideoMode mode;
	int nrChans = 0;
	bool texInited = false;
	bool texInitedNoHisto = false;
	GLuint nrTexs=4;
	GLint texPtr = -1;
	GLuint* texId = 0;
	GLuint* texIdNoHisto = 0;
	GLenum intFormat = 0;
	GLenum extFormat = 0;
	GLenum pixType = GL_UNSIGNED_BYTE;
} ki_StreamPar;

typedef struct
{
	bool activate = true;
	bool useFile = false;
	bool useDepth = true;
	bool useColor = true;
	bool useIr = false;
	bool useNiteSkel = false;
	bool useNiteHand = false;
	bool autoExp = true;
	bool autoWB = true;
	bool emitter = true;
	bool closeRange = false;
	bool mirror = false;
	bool registration = true;

	unsigned int depthFps = 30;
	unsigned int depthW = 640;
	unsigned int depthH = 480;
	openni::PixelFormat depthFmt = openni::PIXEL_FORMAT_DEPTH_1_MM;
	uint32_t depthGain = 30;
	unsigned int colorFps = 30;
	unsigned int colorW = 640;
	unsigned int colorH = 480;
	openni::PixelFormat colorFmt = openni::PIXEL_FORMAT_RGB888;
	unsigned int irFps = 30;
	unsigned int irW = 640;
	unsigned int irH = 480;
	openni::PixelFormat irFmt = openni::PIXEL_FORMAT_GRAY16;

	float irAmp;
	float depthFovX;
	float depthFovY;
} kinPar;

class KinectInput
{
public:
	KinectInput(std::string path, kinPar* _kPar, kinectMapping* _kMap);
	KinectInput(bool _getDepth, bool _getColor, kinectMapping* _kMap);
	KinectInput(kinPar* _kPar, kinectMapping* _kMap);
	~KinectInput();
	int init();
	void getGlFormat(ki_StreamPar* strPar);
	void allocate(ki_StreamType _type, int deviceNr, void* img, GLuint* texId);
	void uploadImg(ki_StreamType _type, int deviceNr = 0, bool useHisto = false);
	bool uploadColorImg(int deviceNr = 0);
	bool uploadDepthImg(bool histo, int deviceNr = 0);
	bool uploadIrImg(int deviceNr = 0);
	bool uploadShadowImg(int deviceNr = 0);

	void setImageAutoExposure(bool onOff, int deviceNr = 0);
	void setImageAutoWhiteBalance(bool onOff, int deviceNr = 0);
	void setImageExposure(int delta, int deviceNr = 0);
	void setImageRegistration(bool val, int deviceNr = 0);
	void setImageGain(int delta, int deviceNr = 0);
	void setDepthGain(int delta, int deviceNr = 0);
	void setCloseRange(bool val, int deviceNr = 0);
	void setMirror(bool val, int deviceNr = 0);
	void setDepthMirror(bool val, int deviceNr = 0);
	void setColorMirror(bool val, int deviceNr = 0);
	void setIRMirror(bool val, int deviceNr = 0);
	void setDepthVMirror(bool val, int deviceNr = 0);
	void setColorVMirror(bool val, int deviceNr = 0);

	openni::Status setPlaybackSpeed(float speed, int deviceNr = 0);
	void setUpdateNis(bool _val, int deviceNr = 0);
	void setUpdateShadow(bool _val, int deviceNr = 0);
	void setEmitter(bool _val, int deviceNr = 0);

	int getNrDevices();
	float getPlaybackSpeed(int deviceNr = 0);
	int getNrChans(openni::VideoMode* mode, int deviceNr = 0);

	int getColorWidth(int deviceNr = 0);
	int getColorHeight(int deviceNr = 0);
	int getDepthWidth(int deviceNr = 0);
	int getDepthHeight(int deviceNr = 0);
	int getIrWidth(int deviceNr = 0);
	int getIrHeight(int deviceNr = 0);

	float getDepthFovY();
	float getDepthFovX();
	int getDepthMaxDepth(int deviceNr = 0);

	uint8_t* getColorImg(int deviceNr = 0, int offset = 0);
	uint8_t* getActColorImg(int deviceNr = 0);
	uint8_t* getActColorGrImg(int deviceNr = 0);
	float* getActDepthImg(int deviceNr = 0);
	float* getActDepthImgNoHisto(int deviceNr = 0);
	uint8_t* getActDepthImg8(int deviceNr = 0);
	uint8_t* getActIrImg(int deviceNr = 0);
	uint8_t* getShadowImg(int deviceNr = 0, int frameNr = -1);

	GLuint getTexId(ki_StreamType _type, int deviceNr, bool histo, int bufOffs =0);
	GLuint getDepthTexId(bool histo, int deviceNr = 0, int bufOffs =0);
	GLuint getColorTexId(int deviceNr = 0);
	GLuint getIrTexId(int deviceNr = 0);
	GLuint getShdwTexId(int deviceNr = 0);

	int getColFrameNr(int deviceNr = 0);
	int getDepthFrameNr(bool histo, int deviceNr = 0);
	int getIrFrameNr(int deviceNr = 0);
	int getShadowFrameNr(int deviceNr = 0);
	int getNisFrameNr(int deviceNr = 0);
	int getColUplFrameNr(int deviceNr = 0);
	int getDepthUplFrameNr(bool histo, int deviceNr = 0);
	int getIrUplFrameNr(int deviceNr = 0);
	int getShadowUplFrameNr(int deviceNr = 0);
	openni::Device* getDevice(int deviceNr = 0);
	openni::VideoFrameRef* getDepthFrame(int deviceNr = 0);
	openni::VideoStream* getDepthStream(int deviceNr = 0);
	int getMaxUsers();

	glm::vec3* getShadowUserCenter(int userNr, int deviceNr = 0);

	void rotIr90(int deviceNr);
	void rotColor90(int deviceNr);
	void colUseGray(int deviceNr, bool _val);
	bool isDeviceReady(int deviceNr = 0);
	bool isReady();
	bool isNisInited(int deviceNr = 0);

	void lockDepthMutex(int deviceNr = 0);
	void unlockDepthMutex(int deviceNr = 0);

	void onKey(GLFWwindow* window, int key, int scancode, int action, int mods);

	NISkeleton* getNis(int deviceNr = 0);
	bool useNis = false;

	void pause(bool _val);
	void shutDown();

	void myNanoSleep(uint32_t ns);
	std::vector<std::string>& split(const std::string &s, char delim,
			std::vector<std::string> &elems);
	std::vector<std::string> split(const std::string &s, char delim);

	openni::Device* device;

private:
	openni::Status rc;
	openni::Array<openni::DeviceInfo> deviceList;

	openni::VideoStream** streams;
	ki_StreamPar** streamPars;

	openni::PlaybackControl** pPlaybackControl;
	KinectStreamListener*** frameListeners;

	NIKinectShadow* kinectShadow = 0;

	bool emitterState;
	bool playRecorded;
	bool bIsReady;
	bool generateMips;
	bool debug = false;

	bool* getStream;
	bool* devicePresent;
	std::vector<bool> useKinectv2;
	int mimapLevels;
	int nrDevices;

	NISkeleton** nis = 0;
	bool shdwTexInited = false;
	int shdwUplframeNr;
	int nrShadowBufs;
	int shadow_ptr = -1;
	GLuint shadow_tex;

	int* lastUploadColFrame;
	int* lastUploadDepthFrame;
	int* lastUploadDepthFrameNoHisto;
	int* lastUploadIrFrame;

	const int maxUsers = 6;

	kinectMapping* kMap;
	HistoPar* histoPar;
	kinPar* kPar;

	std::string recPath;

	std::map<std::string, unsigned short> uriMap;
};

}
