/*
 * Copyright 1993-2017 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

/* This example demonstrates how to use the Video Decode Library with CUDA
 * bindings to interop between NVDECODE(using CUDA surfaces) and OpenGL (PBOs).
 * Post-Processing video (de-interlacing) is supported with this sample.
 */

/*
*	braucht die glvnd versionen der NVIDIA Treiber unter Linux
*	Wenn OpencV mit cuda support compiliert ist, fliegt diese Klasse bei der cuda initialisation raus...
*
*   die update Function muss jeden FrameUpdate aufgerufen werden
*   es wird per timestamp der frames entschieden, ob sie gezeigt werden oder nicht.
*   dazu wird eine idealisierte zeit, abhaengig von der Wiederholfrequenz des Bildschirmes, verwendet
*
*   es werden 20 frames gebuffert, dass geschieht innerhalb des frameworks - ein buffern von opengl texturen
*   ist eigentlich nicht notwendig
*
*
*	frame seeking ist nicht trivial, es muss das file an einer aproximativen stellen gelesen werden (groesse der
*	repraesentation eines frames in der datei abgeschaetzt werden) dann der naechste intra frame gefunden werden
*	(CUVIDPICPARAMS intra_frame_flag) und von da aus nach vorne oder nach hinten gesucht werden, bis der
*	entsprechende Frame gefunden ist. Andere Moeglichkeit: eine index datei erstellen in der die frame
*	speicherpositionen in der datei zugeordnet sind
*/


#include "NvDecodeGL.h"

#define ENABLE_DEBUG_OUT 0

namespace tav {

// tavDataPath needed for lookup of cuda kernels

NvDecodeGL::NvDecodeGL(const char* file, sceneData* _scd, bool _loop) :
	frame_timer(NULL), global_timer(NULL), g_DeviceID (0),
	g_bDeviceLost(false), g_bRunning(false),
	g_bFrameRepeat(true), // e.g if we have a 30fps movie and 60fps opengl loop, "false" would display the movie at 60
	g_bFrameStep(false),
	g_bFirstFrame(true), 
	g_bUpdateCSC(true),
	g_bUpdateAll(false),
	g_bIsProgressive(true),		// assume it is progressive, unless otherwise noted
	g_bException(false), 
	g_bWaived (false),
	bLoop(_loop),
	//g_nBitDepth(0),
	g_nFrameStart(-1), g_nFrameEnd(-1), g_nmse_luma(0), g_nmse_luma_count(0), g_nmse_chroma(0),
	g_nmse_chroma_count(0),
	g_eVideoCreateFlags(cudaVideoCreate_PreferCUVID), g_CtxLock(NULL), present_fps(0.f), 
	decoded_fps(0.f), total_time(0.0f), cuModNV12toARGB(0), g_kernelNV12toARGB(0), g_kernelPassThru(0),
	g_oContext(0), g_oDevice(0), g_KernelSID(0), g_eColorSpace(ITU601), g_nHue(0.0f),
	g_pFrameQueue(0), g_pVideoSource(0), g_pVideoParser(0), g_pVideoDecoder(0), g_pImageGL(0), // if we're using OpenGL
	g_nWindowWidth(0), g_nWindowHeight(0), g_nClientAreaWidth(0), g_nClientAreaHeight(0),
	g_nVideoWidth(0), g_nVideoHeight(0), g_FrameCount(0), g_DecodeFrameCount(0),
	g_fpsCount(0),      // FPS count for averaging
	g_fpsLimit(16),     // FPS limit for sampling timer;
	sFileName(std::string(file)),
	actField(0),
	monRefRate(60.f),
	actFrameTimeStamp(0),
	initOk(false),
	nrFramesToBuffer(3),
	nrBufferedFrames(0),
	bufFrameWritePtr(0),
	bufFrameReadPtr(0),
	actFramePtr(2),
	requestLoop(false),
	debug(false)
{
	cu_kernel_path = (*_scd->dataPath)+"cudaKernels/";
	monRefRate = 1.0 / 30.0;
//	monRefRate = 1.0 / (double) _scd->monRefRate;

    sdkCreateTimer(&frame_timer);
    sdkResetTimer(&frame_timer);

    sdkCreateTimer(&global_timer);
    sdkResetTimer(&global_timer);

    // Initialize the CUDA and NVDECODE
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void *CUDADRIVER;
#endif


    CUDADRIVER hHandleDriver = 0;
    CUresult res;

    res = cuInit   (0, __CUDA_API_VERSION, hHandleDriver);
    if (res != CUDA_SUCCESS)
    	printf("cuInit failed, code %d \n", (int)res);

    res = cuvidInit(0);
    if (res != CUDA_SUCCESS)
    	printf("cuvidInit failed, code %d \n", (int)res);

    // Find out the video size (uses NVDECODE calls)
    g_bIsProgressive = loadVideoSource(sFileName.c_str(),
                                       g_nVideoWidth, g_nVideoHeight,
                                       g_nWindowWidth, g_nWindowHeight);

    if(debug) printf("NvDecodeGL Video Source: %s loaded!! \n", sFileName.c_str());

    g_nVideoWidth   = PAD_ALIGN(g_nVideoWidth   , 0x3F);
    g_nVideoHeight  = PAD_ALIGN(g_nVideoHeight  , 0x0F);

    // Initialize CUDA and try to connect with an OpenGL context
    // Other video memory resources will be available
    int bTCC = 0;

    if (initCudaResources(&bTCC) == false)
    {
        g_bException = true;
        g_bWaived    = true;
		initOk	     = false;

		printf("NvDecodeGL Error initCudaResources \n");
	} else {
		initOk = true;
	}
}

//-------------------------------------------------------------------------------------------
	
void NvDecodeGL::start()
{
	if (initOk)
	{
		g_pVideoSource->start();
		g_bRunning = true;
		startTime = 0;

		if (debug){
			sdkStartTimer(&global_timer);
			sdkResetTimer(&global_timer);
		}
	}
}

//-------------------------------------------------------------------------------------------

void NvDecodeGL::stop()
{
	if (initOk && g_bRunning)
	{

		g_bRunning = false;

		// we only want to record this once
		if (debug){
			if (total_time == 0.0f)
				total_time = sdkGetTimerValue(&global_timer);

			sdkStopTimer(&global_timer);
		}

		g_pFrameQueue->endDecode();
		g_pVideoSource->stop();
	}
}

//-------------------------------------------------------------------------------------------

long long NvDecodeGL::SumSquareError(const unsigned char *src1, const unsigned char *src2, unsigned int count)
{
    long long sum = 0;
    for (unsigned int i = 0; i<count; i++)
    {
        int diff = src1[i] - src2[i];
        sum += diff*diff;
    }
    return sum;
}

//-------------------------------------------------------------------------------------------

double NvDecodeGL::PSNR(long long sse, long long count)
{
    return 10 * log10(255.0*255.0*(double)count / (double)sse);
}

//-------------------------------------------------------------------------------------------
/*
void NvDecodeGL::printStatistics()
{
    int   hh, mm, ss, msec;

    present_fps = 1.f / (total_time / (g_FrameCount * 1000.f));
    decoded_fps = 1.f / (total_time / (g_DecodeFrameCount * 1000.f));

    msec = ((int)total_time % 1000);
    ss   = (int)(total_time/1000) % 60;
    mm   = (int)(total_time/(1000*60)) % 60;
    hh   = (int)(total_time/(1000*60*60)) % 60;

    printf("\n[%s] statistics\n", sSDKname);
    printf("\t Video Length (hh:mm:ss.msec)   = %02d:%02d:%02d.%03d\n", hh, mm, ss, msec);

    printf("\t Frames Presented (inc repeats) = %d\n", g_FrameCount);
    printf("\t Average Present Rate     (fps) = %4.2f\n", present_fps);

    printf("\t Frames Decoded   (hardware)    = %d\n", g_DecodeFrameCount);
    printf("\t Average Rate of Decoding (fps) = %4.2f\n", decoded_fps);
}
*/
//-------------------------------------------------------------------------------------------

void NvDecodeGL::computeFPS()
{
    sdkStopTimer(&frame_timer);

    if (g_bRunning)
    {
        g_fpsCount++;

        if (!(g_pFrameQueue->isEndOfDecode() && g_pFrameQueue->isEmpty()))
        {
            g_FrameCount++;
        }
    }

    char sFPS[256];
    std::string sDecodeStatus;

    if (g_bDeviceLost)
    {
        sDecodeStatus = "DeviceLost!\0";
        sprintf(sFPS, "[%s] - [%s %d]",
                sDecodeStatus.c_str(),
                (g_bIsProgressive ? "Frame" : "Field"),
                g_DecodeFrameCount);

        sdkResetTimer(&frame_timer);
        g_fpsCount = 0;
        return;
    }

    if (g_pFrameQueue->isEndOfDecode() && g_pFrameQueue->isEmpty())
    {
        sDecodeStatus = "STOP (End of File)\0";

        // we only want to record this once
        if (total_time == 0.0f)
            total_time = sdkGetTimerValue(&global_timer);

        sdkStopTimer(&global_timer);
    }
    else
    {
        if (!g_bRunning)
        {
            sDecodeStatus = "PAUSE\0";
            sprintf(sFPS, "[%s] - [%s %d]",
                    sDecodeStatus.c_str(),
                    (g_bIsProgressive ? "Frame" : "Field"),
					g_DecodeFrameCount);
        }
        else
        {
            if (g_bFrameStep) 	sDecodeStatus = "STEP\0";
			else				sDecodeStatus = "PLAY\0";
        }

        if (g_fpsCount == g_fpsLimit)
        {
            float ifps = 1.f / (sdkGetAverageTimerValue(&frame_timer) / 1000.f);

            sprintf(sFPS, "[%s] - [%3.1f fps, %s %d]",
                    sDecodeStatus.c_str(), ifps,
                    (g_bIsProgressive ? "Frame" : "Field"), g_DecodeFrameCount);

            printf("[%s: %04d, %04.1f fps, time: %04.2f (ms) ]\n",
                   (g_bIsProgressive ? "Frame" : "Field"), g_FrameCount, ifps, 1000.f/ifps);

            sdkResetTimer(&frame_timer);
            g_fpsCount = 0;
        }
    }


    sdkStartTimer(&frame_timer);
}

//-------------------------------------------------------------------------------------------

bool NvDecodeGL::initCudaResources(int *bTCC)
{
	CUdevice cuda_device;

    // Device is specified at the command line, we need to check if this it TCC or not, and then call the
    // appropriate TCC/WDDM findCudaDevice in order to initialize the CUDA device

    initGL(bTCC);


	cuda_device = findCudaGLDeviceDRV(0, NULL);
    checkCudaErrors(cuDeviceGet(&g_oDevice, cuda_device));

    // get compute capabilities and the devicename
    int major, minor;
    size_t totalGlobalMem;
    char deviceName[256];
    checkCudaErrors(cuDeviceComputeCapability(&major, &minor, g_oDevice));
    checkCudaErrors(cuDeviceGetName(deviceName, 256, g_oDevice));
    if(debug) printf( "> Using GPU Device %d: %s has SM %d.%d compute capability\n", cuda_device, deviceName, major, minor );

    checkCudaErrors(cuDeviceTotalMem(&totalGlobalMem, g_oDevice));
    if(debug) printf("  Total amount of global memory:     %f MB\n", (float)totalGlobalMem/(1024*1024) );

    checkCudaErrors(cuGLCtxCreate(&g_oContext, CU_CTX_BLOCKING_SYNC, g_oDevice));

    if(debug) printf("NvDecodeGL::initCudaResources g_oDevice = %08lx\n", (unsigned long)g_oDevice);
    if(debug) printf("\n  exec_path %s \n", cu_kernel_path.c_str());

    try
    {
        // Initialize CUDA releated Driver API (32-bit or 64-bit), depending the platform running
        if (sizeof(void *) == 4)
            g_pCudaModule = new CUmoduleManager("NV12ToARGB_drvapi_Win32.ptx", cu_kernel_path.c_str(), 2, 2, 2);
        else
            g_pCudaModule = new CUmoduleManager("NV12ToARGB_drvapi_x64.ptx", cu_kernel_path.c_str(), 2, 2, 2);
    }
    catch (char const *p_file)
    {
        // If the CUmoduleManager constructor fails to load the PTX file, it will throw an exception
        printf("\n >> CUmoduleManager::Exception! %s not found!\n", p_file);
        printf(">> Please rebuild NV12ToARGB_drvapi.cu or re-install this sample.\n");
        return false;
    }

    g_pCudaModule->GetCudaFunction("NV12ToARGB_drvapi",   &g_kernelNV12toARGB);
    g_pCudaModule->GetCudaFunction("Passthru_drvapi",     &g_kernelPassThru);

    /////////////////Change///////////////////////////
    // Now we create the CUDA resources and the CUDA decoder context
    initCudaVideo();

    initGLTexture(g_pVideoDecoder->targetWidth(),
    			  g_pVideoDecoder->targetHeight());

    CUcontext cuCurrent = NULL;
    CUresult result = cuCtxPopCurrent(&cuCurrent);

    if (result != CUDA_SUCCESS)
    {
        printf("cuCtxPopCurrent: %d\n", result);
        assert(0);
    }

    /////////////////////////////////////////
    return ((g_pCudaModule && g_pVideoDecoder) ? true : false);
}

//-------------------------------------------------------------------------------------------

// Initialize OpenGL Resources
bool NvDecodeGL::initGL(int *pbTCC)
{
    int dev, device_count = 0;
    bool bSpecifyDevice=false;
    char device_name[256];

    // Check for a min spec of Compute 1.1 capability before running
    checkCudaErrors(cuDeviceGetCount(&device_count));

    // If deviceID == 0, and there is more than 1 device, let's find the first available graphics GPU
    if (!bSpecifyDevice && device_count > 0)
    {
    	for (int i=0; i < device_count; i++)
        {
            checkCudaErrors(cuDeviceGet(&dev, i));
            checkCudaErrors(cuDeviceGetName(device_name, 256, dev));

            int bSupported = checkCudaCapabilitiesDRV(1, 1, i);

            if (!bSupported)
            {
                printf("  -> GPU: \"%s\" does not meet the minimum spec of SM 1.1\n", device_name );
                printf("  -> A GPU with a minimum compute capability of SM 1.1 or higher is required.\n");
                return false;
            }

#if CUDA_VERSION >= 3020
            checkCudaErrors(cuDeviceGetAttribute(pbTCC ,  CU_DEVICE_ATTRIBUTE_TCC_DRIVER, dev));
            if(debug)  printf("  -> GPU %d: < %s > driver mode is: %s\n", dev, device_name, *pbTCC ? "TCC" : "WDDM" );

            if (*pbTCC)
                continue;
            else
                g_DeviceID = i; // we choose an available WDDM display device

#else

            // We don't know for sure if this is a TCC device or not, if it is Tesla we will not run
            if (!STRNCASECMP(device_name, "Tesla", 5))
            {
                printf("  \"%s\" does not support %s\n", device_name, sSDKname);
                *pbTCC = 1;
                return false;
            }
            else
            {
                *pbTCC = 0;
            }

#endif
            if(debug)  printf("\n");
        }
    }
    else
    {
        if ((g_DeviceID > (device_count-1)) || (g_DeviceID < 0))
        {
            printf( " >>> Invalid GPU Device ID=%d specified, only %d GPU device(s) are available.<<<\n", g_DeviceID, device_count );
            printf( " >>> Valid GPU ID (n) range is between [0,%d]...  Exiting... <<<\n", device_count-1 );
            return false;
        }

        // We are specifying a GPU device, check to see if it is TCC or not
        checkCudaErrors(cuDeviceGet(&dev, g_DeviceID));
        checkCudaErrors(cuDeviceGetName(device_name, 256, dev));

#if CUDA_VERSION >= 3020
        checkCudaErrors(cuDeviceGetAttribute(pbTCC ,  CU_DEVICE_ATTRIBUTE_TCC_DRIVER, dev));
        if(debug)  printf( "  -> GPU %d: < %s > driver mode is: %s\n", dev, device_name, *pbTCC ? "TCC" : "WDDM" );
#else

        // We don't know for sure if this is a TCC device or not, if it is Tesla we will not run
        if (!STRNCASECMP(device_name, "Tesla", 5))
        {
            printf("  \"%s\" does not support %s\n", device_name, sSDKname);
            *pbTCC = 1;
            return false;
        }
        else
        {
            *pbTCC = 0;
        }
#endif
    }

    return true;
}

//-------------------------------------------------------------------------------------------

// Initializes OpenGL Textures (allocation and initialization)
bool NvDecodeGL::initGLTexture(unsigned int nWidth, unsigned int nHeight)
{
	g_pImageGL = new ImageGL*[nrFramesToBuffer];
	for (unsigned int i=0; i<nrFramesToBuffer; i++)
	{
	    g_pImageGL[i] = new ImageGL(nWidth, nHeight,
	                             nWidth, nHeight,
	                             ImageGL::BGRA_PIXEL_FORMAT);
	    g_pImageGL[i]->clear(0x80);
	    g_pImageGL[i]->setCUDAcontext(g_oContext);
	    g_pImageGL[i]->setCUDAdevice(g_oDevice);
	}

    return true;
}

//-------------------------------------------------------------------------------------------

bool NvDecodeGL::loadVideoSource(const char *video_file,
                unsigned int &width    , unsigned int &height,
                unsigned int &dispWidth, unsigned int &dispHeight)
{
    std::auto_ptr<FrameQueue> apFrameQueue(new FrameQueue);
    std::auto_ptr<VideoSource> apVideoSource(new VideoSource(video_file, apFrameQueue.get(), bLoop));

    // retrieve the video source (width,height)
    apVideoSource->getSourceDimensions(width, height);
    apVideoSource->getSourceDimensions(dispWidth, dispHeight);
    apVideoSource->setEndCb(std::bind(&NvDecodeGL::streamEndDoLoop, this));

    memset(&g_stFormat, 0, sizeof(CUVIDEOFORMAT));
    if(debug)  std::cout << (g_stFormat = apVideoSource->format()) << std::endl;

    g_pFrameQueue  = apFrameQueue.release();
    g_pVideoSource = apVideoSource.release();

    if (g_pVideoSource->format().codec == cudaVideoCodec_JPEG)
        g_eVideoCreateFlags = cudaVideoCreate_PreferCUDA;

    bool IsProgressive = 0;
    g_pVideoSource->getProgressive(IsProgressive);
    std::cout << "NvDecodeGL::loadVideoSource IsProgressive: " << IsProgressive << std::endl;
    return IsProgressive;
}

//-------------------------------------------------------------------------------------------

void NvDecodeGL::initCudaVideo()
{
    // bind the context lock to the CUDA context
    CUresult result = cuvidCtxLockCreate(&g_CtxLock, g_oContext);
    CUVIDEOFORMATEX oFormatEx;
    memset(&oFormatEx, 0, sizeof(CUVIDEOFORMATEX));
    oFormatEx.format = g_stFormat;

    if (result != CUDA_SUCCESS)
    {
        printf("cuvidCtxLockCreate failed: %d\n", result);
        assert(0);
    }

    std::auto_ptr<VideoDecoder> apVideoDecoder(new VideoDecoder(g_pVideoSource->format(),
    		g_oContext, g_eVideoCreateFlags, g_CtxLock));

    std::auto_ptr<VideoParser> apVideoParser(new VideoParser(apVideoDecoder.get(),
    		g_pFrameQueue, &oFormatEx, &g_oContext));

    g_pVideoSource->setParser(*apVideoParser.get());

    g_pVideoParser  = apVideoParser.release();
    g_pVideoDecoder = apVideoDecoder.release();
}

//-------------------------------------------------------------------------------------------

void NvDecodeGL::freeCudaResources(bool bDestroyContext)
{
    if (g_pVideoParser) 	delete g_pVideoParser;
    if (g_pVideoDecoder)	delete g_pVideoDecoder;
    if (g_pVideoSource)		delete g_pVideoSource;
    if (g_pFrameQueue)		delete g_pFrameQueue;
	if (g_KernelSID)		checkCudaErrors(cuStreamDestroy(g_KernelSID));
    if (g_CtxLock)			checkCudaErrors(cuvidCtxLockDestroy(g_CtxLock));

    if (g_oContext && bDestroyContext)
    {
        checkCudaErrors(cuCtxDestroy(g_oContext));
        g_oContext = NULL;
    }     
}

//-------------------------------------------------------------------------------------------

// Run the Cuda part of the computation (if g_pFrameQueue is empty, then return false)
bool NvDecodeGL::copyDecodedFrameToTexture(int *pbIsProgressive)
{
    CUVIDPARSERDISPINFO oDisplayInfo;

    if (g_pFrameQueue->dequeue(&oDisplayInfo))
    {
        CCtxAutoLock lck(g_CtxLock);

        // Push the current CUDA context (only if we are using CUDA decoding path)
        cuCtxPushCurrent(g_oContext);

        CUdeviceptr  pDecodedFrame[3] = { 0, 0, 0 };
        CUdeviceptr  pInteropFrame[3] = { 0, 0, 0 };

        *pbIsProgressive = oDisplayInfo.progressive_frame;
        g_bIsProgressive = oDisplayInfo.progressive_frame ? true : false;

        CUVIDPROCPARAMS oVideoProcessingParameters;
        memset(&oVideoProcessingParameters, 0, sizeof(CUVIDPROCPARAMS));

        oVideoProcessingParameters.progressive_frame = oDisplayInfo.progressive_frame;        
        oVideoProcessingParameters.top_field_first = oDisplayInfo.top_field_first;
        oVideoProcessingParameters.unpaired_field = (oDisplayInfo.repeat_first_field < 0);


        int active_field = 0;
		unsigned int nDecodedPitch = 0;
		unsigned int nWidth = 0;
		unsigned int nHeight = 0;

		oVideoProcessingParameters.second_field = active_field;

		// map decoded video frame to CUDA surface
		if (g_pVideoDecoder->mapFrame(oDisplayInfo.picture_index, &pDecodedFrame[active_field],
				&nDecodedPitch, &oVideoProcessingParameters) != CUDA_SUCCESS)
		{
			// if we couldn't map the frame, so it can be re-used in decoder and return
			g_pFrameQueue->releaseFrame(&oDisplayInfo);

			// Detach from the Current thread
			checkCudaErrors(cuCtxPopCurrent(NULL));

			return false;
		}

		nWidth = g_pVideoDecoder->targetWidth(); // PAD_ALIGN(g_pVideoDecoder->targetWidth(), 0x3F);
		nHeight = g_pVideoDecoder->targetHeight(); // PAD_ALIGN(g_pVideoDecoder->targetHeight(), 0x0F);

		// map OpenGL PBO or CUDA memory
		size_t nTexturePitch = 0;

		// timestamp comes from videoparser which calls cuvidCreateVideoParser
		// values are set inside the cuvid library
#if ENABLE_DEBUG_OUT
		printf("%s = %02d, PicIndex = %02d, OutputPTS = %08d\n",
			   (oDisplayInfo.progressive_frame ? "Frame" : "Field"),
			   g_DecodeFrameCount, oDisplayInfo.picture_index, oDisplayInfo.timestamp);
#endif


		// map the texture surface
		g_pImageGL[bufFrameWritePtr]->setPts((double) oDisplayInfo.timestamp * 0.0001);
		g_pImageGL[bufFrameWritePtr]->map(&pInteropFrame[active_field], &nTexturePitch, active_field);
		nTexturePitch /= g_pVideoDecoder->targetHeight();


		// perform post processing on the CUDA surface (performs colors space conversion and post processing)
		// comment this out if we include the line of code seen above
		cudaPostProcessFrame(&pDecodedFrame[active_field], nDecodedPitch, &pInteropFrame[active_field],
							 nTexturePitch, g_pCudaModule->getModule(), g_kernelNV12toARGB, g_KernelSID);

		// unmap the texture surface
		g_pImageGL[bufFrameWritePtr]->unmap();
		g_pImageGL[bufFrameWritePtr]->unpackPbo(0);	// unpack pbo to texture
		bufFrameWritePtr = (bufFrameWritePtr +1) % nrFramesToBuffer;

		// unmap video frame
		// unmapFrame() synchronizes with the VideoDecode API (ensures the frame has finished decoding)
		g_pVideoDecoder->unmapFrame(pDecodedFrame[active_field]);
		g_DecodeFrameCount++;
		nrBufferedFrames++;

		// Detach from the Current thread
		checkCudaErrors(cuCtxPopCurrent(NULL));

		// release the frame, so it can be re-used in decoder
		g_pFrameQueue->releaseFrame(&oDisplayInfo);

    } else
    {
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------------------------

// This is the CUDA stage for Video Post Processing.  Last stage takes care of the NV12 to ARGB
void NvDecodeGL::cudaPostProcessFrame(CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch,
                     CUdeviceptr *ppTextureData,  size_t nTexturePitch,
                     CUmodule cuModNV12toARGB,
                     CUfunction fpCudaKernel, CUstream streamID)
{
    uint32 nWidth  = g_pVideoDecoder->targetWidth();
    uint32 nHeight = g_pVideoDecoder->targetHeight();

    // Upload the Color Space Conversion Matrices
    if (g_bUpdateCSC)
    {
        // CCIR 601/709
        float hueColorSpaceMat[9];
        setColorSpaceMatrix(g_eColorSpace,    hueColorSpaceMat, g_nHue);
        updateConstantMemory_drvapi(cuModNV12toARGB, hueColorSpaceMat);

        if (!g_bUpdateAll)
        {
            g_bUpdateCSC = false;
        }
    }

    // TODO: Stage for handling video post processing

    // Final Stage: NV12toARGB color space conversion
    cudaLaunchNV12toARGBDrv(*ppDecodedFrame, nDecodedPitch,
                                      *ppTextureData, nTexturePitch,
                                      nWidth, nHeight, fpCudaKernel, streamID);
}

//-------------------------------------------------------------------------------------------

// Release all previously initd objects
bool NvDecodeGL::cleanup(bool bDestroyContext)
{
    freeCudaResources(bDestroyContext);
    return true;
}

//-------------------------------------------------------------------------------------------

// update with an idealised value, 1:1 time leads to jittering
void NvDecodeGL::update()
{
	int bIsProgressive = 1;
	if (g_bRunning){

		// check if we need a new buffer, calculate the
		startTime += monRefRate;

		if (g_pFrameQueue)
		{
			// if there are less than needed frames buffer, decode and buffer!
			if (!g_bDeviceLost
					&& g_bRunning
					&& nrBufferedFrames < nrFramesToBuffer
					&& actFramePtr != bufFrameWritePtr)
				copyDecodedFrameToTexture(&bIsProgressive);

		} else return;

		if (g_pImageGL && nrBufferedFrames > 0)
		{
			double ptsSinceStart = startTime * 1000.0; // to ms

			// check if we need a new frame
			// (since there are rounding errors we are checking for the difference which is safer
			if ((ptsSinceStart - g_pImageGL[bufFrameReadPtr]->getPts()) > 0.1)
			{
				actFramePtr = bufFrameReadPtr;
				bufFrameReadPtr = (bufFrameReadPtr +1) % nrFramesToBuffer;
				nrBufferedFrames = std::max(--nrBufferedFrames, (unsigned int)0);

				// check if we the a new loop began
				if (g_pImageGL[(bufFrameReadPtr -1 + nrFramesToBuffer) % nrFramesToBuffer]->getPts()
					> g_pImageGL[bufFrameReadPtr]->getPts())
				{
					startTime = 0;
				}
			}
		}

		if (requestLoop || requestFileSwitch)
		{
			g_pVideoSource->stop();

			if(requestFileSwitch) g_pFrameQueue->releaseAllFrames();	// work in progress

			g_pVideoSource->ReloadVideo(sFileName.c_str(), g_pFrameQueue, g_pVideoParser, bLoop);
			g_pVideoSource->setEndCb(std::bind(&NvDecodeGL::streamEndDoLoop, this));
			g_pVideoSource->start();

			requestLoop = false;
			requestFileSwitch = false;
		}
	}
}

//-------------------------------------------------------------------------------------------

// callback set in VideoSource, called on CUVID_PKT_ENDOFSTREAM
// (from cuvidThread, so just set a flag and do the work in the main thread)

void NvDecodeGL::streamEndDoLoop()
{
	if (bLoop)
		requestLoop = true;
}

//-------------------------------------------------------------------------------------------

void NvDecodeGL::bind(GLuint texUnit)
{
	if (g_pImageGL){
		glActiveTexture(GL_TEXTURE0 + texUnit);
		glBindTexture(GL_TEXTURE_2D, g_pImageGL[actFramePtr]->getTexID());
	}
}

//-------------------------------------------------------------------------------------------

// can be called async, so just set a switch
void NvDecodeGL::switchFile(const char* file)
{
	sFileName = std::string(file);
	requestFileSwitch = true;
}

//-------------------------------------------------------------------------------------------

NvDecodeGL::~NvDecodeGL()
{
    cleanup(g_bWaived ? false : true);

    if (g_pImageGL){

        delete [] g_pImageGL;
        g_pImageGL = NULL;
    }
}

}
