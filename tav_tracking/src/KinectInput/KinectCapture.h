/*****************************************************************************
 *                                                                            *
 *  OpenNI 2.x Alpha                                                          *
 *  Copyright (C) 2012 PrimeSense Ltd.                                        *
 *                                                                            *
 *  This file is part of OpenNI.                                              *
 *                                                                            *
 *  Licensed under the Apache License, Version 2.0 (the "License");           *
 *  you may not use this file except in compliance with the License.          *
 *  You may obtain a copy of the License at                                   *
 *                                                                            *
 *      http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                            *
 *  Unless required by applicable law or agreed to in writing, software       *
 *  distributed under the License is distributed on an "AS IS" BASIS,         *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 *  See the License for the specific language governing permissions and       *
 *  limitations under the License.                                            *
 *                                                                            *
 *****************************************************************************/

#ifndef __tav_gl4__KinectCaputure__
#define __tav_gl4__KinectCaputure__

#include <iostream>
#include <XnLib.h>

#include "KinectInput/KinectInput.h"

#define CAPTURED_FRAMES_DIR_NAME "CapturedFrames"

namespace tav
{

class KinectCapture
{
public:

	typedef enum
	{
		NOT_CAPTURING, SHOULD_CAPTURE, CAPTURING,
	} CapturingState;

	typedef enum
	{
		CAPTURE_DEPTH_STREAM,
		CAPTURE_COLOR_STREAM,
		CAPTURE_IR_STREAM,
		CAPTURE_STREAM_COUNT
	} CaptureSourceType;

	typedef enum
	{
		STREAM_CAPTURE_LOSSLESS = false,
		STREAM_CAPTURE_LOSSY = true,
		STREAM_DONT_CAPTURE,
	} StreamCaptureType;

	typedef struct StreamCapturingData
	{
		StreamCaptureType captureType;
		const char* name;
		bool bRecording;
		openni::VideoFrameRef& (*getFrameFunc)();
		openni::VideoStream& (*getStream)();
		bool (*isStreamOn)();
		int startFrame;
	} StreamCapturingData;

	typedef struct CapturingData
	{
		StreamCapturingData streams[CAPTURE_STREAM_COUNT];
		openni::Recorder recorder;
		char csFileName[256];
		int nStartOn; // time to start, in seconds
		bool bSkipFirstFrame;
		CapturingState State;
		int nCapturedFrameUniqueID;
		char csDisplayMessage[500];
	} CapturingData;

	KinectCapture();
	~KinectCapture();

	void captureInit();
	void captureBrowse(int);
	void captureStart(int nDelay);
	void captureRestart(int);
	void captureStop(int);
	bool isCapturing();

	void captureSetDepthFormat(int format);
	void captureSetColorFormat(int format);
	void captureSetIRFormat(int format);
	const char* captureGetDepthFormatName();
	const char* captureGetColorFormatName();
	const char* captureGetIRFormatName();

	void captureRun();
	void captureSingleFrame(int);

	void displayError(const char* csFormat, ...);

	void getCaptureMessage(char* pMessage);
	void getColorFileName(int num, char* csName);
	void getDepthFileName(int num, char* csName);
	void getIRFileName(int num, char* csName);
	int findUniqueFileName();

	const char* getCaptureTypeName(StreamCaptureType type);

private:
	CapturingData g_Capture;

	DeviceParameter g_DepthCapturing;
	DeviceParameter g_ColorCapturing;
	DeviceParameter g_IRCapturing;
};

}

#endif /* defined(__tav_gl4__KinectCaputure__) */
