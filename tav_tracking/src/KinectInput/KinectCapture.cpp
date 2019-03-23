#include "KinectCapture.h"

namespace tav
{

KinectCapture::KinectCapture()
{
	captureInit();
}

//---------------------------------------------------------------

KinectCapture::~KinectCapture()
{
}

//---------------------------------------------------------------

void KinectCapture::captureInit()
{
	// Depth Formats
	int nIndex = 0;

	g_DepthCapturing.pValues[nIndex] = STREAM_CAPTURE_LOSSLESS;
	g_DepthCapturing.pValueToName[nIndex] = "Lossless";
	nIndex++;

	g_DepthCapturing.pValues[nIndex] = STREAM_DONT_CAPTURE;
	g_DepthCapturing.pValueToName[nIndex] = "Don't Capture";
	nIndex++;

	g_DepthCapturing.nValuesCount = nIndex;

	// Color Formats
	nIndex = 0;

	g_ColorCapturing.pValues[nIndex] = STREAM_CAPTURE_LOSSLESS;
	g_ColorCapturing.pValueToName[nIndex] = "Lossless";
	nIndex++;

	g_ColorCapturing.pValues[nIndex] = STREAM_CAPTURE_LOSSY;
	g_ColorCapturing.pValueToName[nIndex] = "Lossy";
	nIndex++;

	g_ColorCapturing.pValues[nIndex] = STREAM_DONT_CAPTURE;
	g_ColorCapturing.pValueToName[nIndex] = "Don't Capture";
	nIndex++;

	g_ColorCapturing.nValuesCount = nIndex;

	// IR Formats
	nIndex = 0;

	g_IRCapturing.pValues[nIndex] = STREAM_CAPTURE_LOSSLESS;
	g_IRCapturing.pValueToName[nIndex] = "Lossless";
	nIndex++;

	g_IRCapturing.pValues[nIndex] = STREAM_DONT_CAPTURE;
	g_IRCapturing.pValueToName[nIndex] = "Don't Capture";
	nIndex++;

	g_IRCapturing.nValuesCount = nIndex;

	// Init
	g_Capture.csFileName[0] = 0;
	g_Capture.State = NOT_CAPTURING;
	g_Capture.nCapturedFrameUniqueID = 0;
	g_Capture.csDisplayMessage[0] = '\0';
	g_Capture.bSkipFirstFrame = false;
	/*
	 g_Capture.streams[CAPTURE_DEPTH_STREAM].captureType = STREAM_CAPTURE_LOSSLESS;
	 g_Capture.streams[CAPTURE_DEPTH_STREAM].name = "Depth";
	 g_Capture.streams[CAPTURE_DEPTH_STREAM].getFrameFunc = KinectInput::getDepthFrame;
	 g_Capture.streams[CAPTURE_DEPTH_STREAM].getStream = getDepthStream;
	 g_Capture.streams[CAPTURE_DEPTH_STREAM].isStreamOn = isDepthOn;
	 g_Capture.streams[CAPTURE_COLOR_STREAM].captureType = STREAM_CAPTURE_LOSSY;
	 g_Capture.streams[CAPTURE_COLOR_STREAM].name = "Color";
	 g_Capture.streams[CAPTURE_COLOR_STREAM].getFrameFunc = getColorFrame;
	 g_Capture.streams[CAPTURE_COLOR_STREAM].getStream = getColorStream;
	 g_Capture.streams[CAPTURE_COLOR_STREAM].isStreamOn = isColorOn;
	 g_Capture.streams[CAPTURE_IR_STREAM].captureType = STREAM_CAPTURE_LOSSLESS;
	 g_Capture.streams[CAPTURE_IR_STREAM].name = "IR";
	 g_Capture.streams[CAPTURE_IR_STREAM].getFrameFunc = getIRFrame;
	 g_Capture.streams[CAPTURE_IR_STREAM].getStream = getIRStream;
	 g_Capture.streams[CAPTURE_IR_STREAM].isStreamOn = isIROn;
	 */
}

//---------------------------------------------------------------

bool KinectCapture::isCapturing()
{
	return (g_Capture.State != NOT_CAPTURING);
}

//---------------------------------------------------------------

void KinectCapture::captureBrowse(int)
{
#if (ONI_PLATFORM == ONI_PLATFORM_WIN32)
	OPENFILENAME ofn =
	{	0};
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFilter = TEXT("Oni Files (*.oni)\0*.oni\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = g_Capture.csFileName;
	ofn.nMaxFile = sizeof(g_Capture.csFileName);
	ofn.lpstrTitle = TEXT("Capture to...");
	ofn.lpstrDefExt = TEXT("oni");
	ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR;
	BOOL gotFileName = GetSaveFileName(&ofn);

	if (gotFileName)
	{
		if (g_Capture.csFileName[0] != 0)
		{
			if (strstr(g_Capture.csFileName, ".oni") == NULL)
			{
				strcat(g_Capture.csFileName, ".oni");
			}
		}
	}
#else
	// Set capture file to defaults.
	strcpy(g_Capture.csFileName, "./Captured.oni");
#endif // ONI_PLATFORM_WIN32

	// as we waited for user input, it's probably better to discard first frame (especially if an accumulating
	// stream is on, like audio).
	g_Capture.bSkipFirstFrame = true;
}

//---------------------------------------------------------------

void KinectCapture::captureStart(int nDelay)
{
	captureBrowse(0);

	// On some platforms a user can cancel capturing. Whenever he cancels
	// capturing, the gs_filePath[0] remains empty.
	if ('\0' == g_Capture.csFileName[0])
	{
		return;
	}

	openni::Status rc = g_Capture.recorder.create(g_Capture.csFileName);
	if (rc != openni::STATUS_OK)
	{
		printf("Failed to create recorder!");
		return;
	}

	unsigned long long int nNow;
	xnOSGetTimeStamp(&nNow);
	nNow /= 1000;

	g_Capture.nStartOn = (uint32_t) nNow + nDelay;
	g_Capture.State = SHOULD_CAPTURE;
}

//---------------------------------------------------------------

void KinectCapture::captureRestart(int)
{
	captureStop(0);
	captureStart(0);
}

//---------------------------------------------------------------

void KinectCapture::captureStop(int)
{
	if (g_Capture.recorder.isValid())
	{
		g_Capture.recorder.destroy();
		g_Capture.State = NOT_CAPTURING;
	}
}

//---------------------------------------------------------------

#define START_CAPTURE_CHECK_RC(rc, what)												\
if (nRetVal != XN_STATUS_OK)														\
{																					\
displayError("Failed to %s: %s\n", what, openni::OpenNI::getExtendedError());	\
g_Capture.recorder.destroy();													\
g_Capture.State = NOT_CAPTURING;												\
return;																			\
}

//---------------------------------------------------------------

void KinectCapture::captureRun()
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (g_Capture.State != SHOULD_CAPTURE)
	{
		return;
	}

	XnUInt64 nNow;
	xnOSGetTimeStamp(&nNow);
	nNow /= 1000;

	// check if time has arrived
	if ((XnInt64) nNow >= g_Capture.nStartOn)
	{
		// check if we need to discard first frame
		if (g_Capture.bSkipFirstFrame)
		{
			g_Capture.bSkipFirstFrame = false;
		}
		else
		{
			// start recording
			for (int i = 0; i < CAPTURE_STREAM_COUNT; ++i)
			{
				g_Capture.streams[i].bRecording = false;

				if (g_Capture.streams[i].isStreamOn()
						&& g_Capture.streams[i].captureType
								!= STREAM_DONT_CAPTURE)
				{
					nRetVal = g_Capture.recorder.attach(
							g_Capture.streams[i].getStream(),
							g_Capture.streams[i].captureType
									== STREAM_CAPTURE_LOSSY);
					START_CAPTURE_CHECK_RC(nRetVal, "add stream");
					g_Capture.streams[i].bRecording = TRUE;
					g_Capture.streams[i].startFrame =
							g_Capture.streams[i].getFrameFunc().getFrameIndex();
				}
			}

			nRetVal = g_Capture.recorder.start();
			START_CAPTURE_CHECK_RC(nRetVal, "start recording");
			g_Capture.State = CAPTURING;
		}
	}
}

//---------------------------------------------------------------

void KinectCapture::captureSetDepthFormat(int format)
{
	g_Capture.streams[CAPTURE_DEPTH_STREAM].captureType =
			(StreamCaptureType) format;
}

//---------------------------------------------------------------

void KinectCapture::captureSetColorFormat(int format)
{
	g_Capture.streams[CAPTURE_COLOR_STREAM].captureType =
			(StreamCaptureType) format;
}

//---------------------------------------------------------------

void KinectCapture::captureSetIRFormat(int format)
{
	g_Capture.streams[CAPTURE_IR_STREAM].captureType =
			(StreamCaptureType) format;
}

//---------------------------------------------------------------

void KinectCapture::getCaptureMessage(char* pMessage)
{
	switch (g_Capture.State)
	{
	case SHOULD_CAPTURE:
	{
		XnUInt64 nNow;
		xnOSGetTimeStamp(&nNow);
		nNow /= 1000;
		sprintf(pMessage, "Capturing will start in %u seconds...",
				g_Capture.nStartOn - (XnUInt32) nNow);
	}
		break;
	case CAPTURING:
	{
		int nChars =
				sprintf(pMessage,
						"* Recording! Press any key or use menu to stop *\nRecorded Frames: ");
		for (int i = 0; i < CAPTURE_STREAM_COUNT; ++i)
		{
			if (g_Capture.streams[i].bRecording)
			{
				nChars += sprintf(pMessage + nChars, "%s-%d ",
						g_Capture.streams[i].name,
						g_Capture.streams[i].getFrameFunc().getFrameIndex()
								- g_Capture.streams[i].startFrame);
			}
		}
	}
		break;
	default:
		pMessage[0] = 0;
	}
}

//---------------------------------------------------------------

void KinectCapture::getColorFileName(int num, char* csName)
{
	sprintf(csName, "%s/Color_%d.raw", CAPTURED_FRAMES_DIR_NAME, num);
}

//---------------------------------------------------------------

void KinectCapture::getDepthFileName(int num, char* csName)
{
	sprintf(csName, "%s/Depth_%d.raw", CAPTURED_FRAMES_DIR_NAME, num);
}

//---------------------------------------------------------------

void KinectCapture::getIRFileName(int num, char* csName)
{
	sprintf(csName, "%s/IR_%d.raw", CAPTURED_FRAMES_DIR_NAME, num);
}

//---------------------------------------------------------------

int KinectCapture::findUniqueFileName()
{
	xnOSCreateDirectory(CAPTURED_FRAMES_DIR_NAME);

	int num = g_Capture.nCapturedFrameUniqueID;

	XnBool bExist = FALSE;
	XnStatus nRetVal = XN_STATUS_OK;
	XnChar csColorFileName[XN_FILE_MAX_PATH];
	XnChar csDepthFileName[XN_FILE_MAX_PATH];
	XnChar csIRFileName[XN_FILE_MAX_PATH];

	for (;;)
	{
		// check color
		getColorFileName(num, csColorFileName);

		nRetVal = xnOSDoesFileExist(csColorFileName, &bExist);
		if (nRetVal != XN_STATUS_OK)
			break;

		if (!bExist)
		{
			// check depth
			getDepthFileName(num, csDepthFileName);

			nRetVal = xnOSDoesFileExist(csDepthFileName, &bExist);
			if (nRetVal != XN_STATUS_OK || !bExist)
				break;
		}

		if (!bExist)
		{
			// check IR
			getIRFileName(num, csIRFileName);

			nRetVal = xnOSDoesFileExist(csIRFileName, &bExist);
			if (nRetVal != XN_STATUS_OK || !bExist)
				break;
		}

		++num;
	}

	return num;
}

//---------------------------------------------------------------

void KinectCapture::captureSingleFrame(int)
{
	int num = findUniqueFileName();

	XnChar csColorFileName[XN_FILE_MAX_PATH];
	XnChar csDepthFileName[XN_FILE_MAX_PATH];
	XnChar csIRFileName[XN_FILE_MAX_PATH];
	getColorFileName(num, csColorFileName);
	getDepthFileName(num, csDepthFileName);
	getIRFileName(num, csIRFileName);
	/*
	 openni::VideoFrameRef& colorFrame = getColorFrame();
	 if (colorFrame.isValid())
	 {
	 xnOSSaveFile(csColorFileName, colorFrame.getData(), colorFrame.getDataSize());
	 }
	 
	 openni::VideoFrameRef& depthFrame = getDepthFrame();
	 if (depthFrame.isValid())
	 {
	 xnOSSaveFile(csDepthFileName, depthFrame.getData(), depthFrame.getDataSize());
	 }
	 
	 openni::VideoFrameRef& irFrame = getIRFrame();
	 if (irFrame.isValid())
	 {
	 xnOSSaveFile(csIRFileName, irFrame.getData(), irFrame.getDataSize());
	 }
	 
	 g_Capture.nCapturedFrameUniqueID = num + 1;
	 
	 displayMessage("Frames saved with ID %d", num);
	 */
}

//---------------------------------------------------------------

const char* KinectCapture::getCaptureTypeName(StreamCaptureType type)
{
	switch (type)
	{
	case STREAM_CAPTURE_LOSSLESS:
		return "Lossless";
	case STREAM_CAPTURE_LOSSY:
		return "Lossy";
	case STREAM_DONT_CAPTURE:
		return "Don't Capture";
	default:
		XN_ASSERT(FALSE);
		return "";
	}
}

//---------------------------------------------------------------

void KinectCapture::displayError(const char* csFormat, ...)
{
	/*
	 g_DrawConfig.messageType = ERROR_MESSAGE;
	 g_DrawConfig.bShowMessage = true;
	 va_list args;
	 va_start(args, csFormat);
	 XnUInt32 nCount;
	 xnOSStrFormatV(g_csUserMessage, sizeof(g_csUserMessage), &nCount, csFormat, args);
	 va_end(args);
	 */
}

//---------------------------------------------------------------

const char* KinectCapture::captureGetDepthFormatName()
{
	return getCaptureTypeName(
			g_Capture.streams[CAPTURE_DEPTH_STREAM].captureType);
}

//---------------------------------------------------------------

const char* KinectCapture::captureGetColorFormatName()
{
	return getCaptureTypeName(
			g_Capture.streams[CAPTURE_COLOR_STREAM].captureType);
}

//---------------------------------------------------------------

const char* KinectCapture::captureGetIRFormatName()
{
	return getCaptureTypeName(g_Capture.streams[CAPTURE_IR_STREAM].captureType);
}
}
