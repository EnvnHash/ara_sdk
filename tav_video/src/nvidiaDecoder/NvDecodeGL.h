#pragma once

#include "NvDecoder_def.h"
#include "headers/sceneData.h"

// CUDA Header includes
#include "dynlink_nvcuvid.h" // <nvcuvid.h>
#include "dynlink_cuda.h"    // <cuda.h>
#include "dynlink_cudaGL.h"  // <cudaGL.h>
#include "dynlink_builtin_types.h"

// CUDA utilities and system includes
#include "helper_functions.h"
#include "helper_cuda_drvapi.h"

// Includes
#include <stdlib.h>
#include <math.h>
#include <memory>
#include <cassert>

#include "FrameQueue.h"
#include "VideoSource.h"
#include "VideoParser.h"
#include "VideoDecoder.h"
#include "ImageGL.h"

// cudaDecodeGL related helper functions
#include "cudaProcessFrame.h"
#include "cudaModuleMgr.h"


namespace tav {

class NVDECODE_API NvDecodeGL {

public:
	NvDecodeGL(const char* file, sceneData* _scd, bool _loop);
	~NvDecodeGL();

	void start();
	void bufferLoop();
	void stop();

	long long SumSquareError(const unsigned char *src1, const unsigned char *src2, 
		unsigned int count);
	double PSNR(long long sse, long long count);
	void computeFPS();

	bool initGL(int *pbTCC);
	bool initGLTexture(unsigned int nWidth, unsigned int nHeight);
	bool loadVideoSource(const char *video_file,
						 unsigned int &width, unsigned int &height,
						 unsigned int &dispWidth, unsigned int &dispHeight);
	void initCudaVideo();

	void freeCudaResources(bool bDestroyContext);

	bool copyDecodedFrameToTexture(int *pbIsProgressive);
	void cudaPostProcessFrame(CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch,
	                     CUdeviceptr *ppTextureData,  size_t nTexturePitch,
	                     CUmodule cuModNV12toARGB,
	                     CUfunction fpCudaKernel, CUstream streamID);

	bool cleanup(bool bDestroyContext);
	bool initCudaResources(int *bTCC);

	void update();
	void streamEndDoLoop();

	void bind(GLuint texUnit);
	void switchFile(const char* file);
	void setLoop(bool _val) { bLoop = _val; }

	bool				initOk;
	bool                g_bRunning;

private :
	
	StopWatchInterface *frame_timer;
	StopWatchInterface *global_timer;
	
	int					g_DeviceID;

	bool				debug;

	bool                g_bDeviceLost;
	bool                g_bFrameRepeat;
	bool                g_bFrameStep;
	bool                g_bFirstFrame;
	bool                bLoop;
	bool                g_bUpdateCSC;
	bool                g_bUpdateAll;
	bool                g_bIsProgressive; // assume it is progressive, unless otherwise noted
	bool                g_bException;
	bool                g_bWaived;
	//int                 g_nBitDepth;

	bool				bDoBuffering;
	bool				requestLoop;
	bool				requestFileSwitch;

	long                g_nFrameStart;
	long                g_nFrameEnd;
	long long           g_nmse_luma;
	long long           g_nmse_luma_count;
	long long           g_nmse_chroma;
	long long           g_nmse_chroma_count;

	unsigned int		g_nWindowWidth;
	unsigned int		g_nWindowHeight;

	unsigned int		g_nClientAreaWidth;
	unsigned int		g_nClientAreaHeight;

	unsigned int		g_nVideoWidth;
	unsigned int		g_nVideoHeight;

	unsigned int		g_FrameCount;
	unsigned int		g_DecodeFrameCount;
	unsigned int		g_fpsCount;      // FPS count for averaging
	unsigned int		g_fpsLimit;     // FPS limit for sampling timer;

	unsigned int		actField;
	unsigned int		lastPboUpdate;
	unsigned int		nrFramesToBuffer;
	unsigned int		nrBufferedFrames;
	unsigned int		bufFrameWritePtr;
	unsigned int		bufFrameReadPtr;
	unsigned int 		actFramePtr;


	float				present_fps; 
	float				decoded_fps;
	float				total_time;
	double				monRefRate;

	double				startTime;

	long long			actFrameTimeStamp;	// long long

	cudaVideoCreateFlags g_eVideoCreateFlags;
	CUvideoctxlock       g_CtxLock;


	// These are CUDA function pointers to the CUDA kernels
	CUmoduleManager*	g_pCudaModule;

	CUmodule			cuModNV12toARGB;
	CUfunction			g_kernelNV12toARGB;
	CUfunction			g_kernelPassThru;

	CUcontext			g_oContext;
	CUdevice			g_oDevice;

	CUstream			g_ReadbackSID; 
	CUstream			g_KernelSID;

	eColorSpace			g_eColorSpace;
	float				g_nHue;

	// System Memory surface we want to readback to
	FrameQueue*			g_pFrameQueue;
	VideoSource*		g_pVideoSource;
	VideoParser*		g_pVideoParser;
	VideoDecoder*		g_pVideoDecoder;

	ImageGL**			g_pImageGL; // if we're using OpenGL

	CUVIDEOFORMAT		g_stFormat;
	CUVIDEOFORMAT		videoFormat;
	CUVIDEOFORMATEX		oFormatEx;

	std::string			sFileName;

	std::string			cu_kernel_path;
};

}
