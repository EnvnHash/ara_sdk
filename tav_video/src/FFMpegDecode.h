/*
 * FFMpegDecode.h
 *
 *  Created on: 29.01.2018
 *      Author: sven
 */

#ifndef FFMPEGDECODE_H_
#define FFMPEGDECODE_H_

#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "StopWatch.h"

#include <GLUtils/TextureManager.h>
#include <Shaders/ShaderCollector.h>

#include <headers/gl_header.h>
#include <GLFW/glfw3.h>


#ifndef __GNUC__
#define access _access
#define F_OK    0       /* Test for existence.  */
#endif

/*
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif

#define snprintf _snprintf
*/



#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
}

#ifdef  __linux__
#ifdef  __cplusplus

#ifndef AV_MAKE_ERROR_STRING_IMPL
#define AV_MAKE_ERROR_STRING_IMPL

static const std::string av_make_error_string(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return (std::string)errbuf;
}

#undef av_err2str
#define av_err2str(errnum) av_make_error_string(errnum).c_str()

#endif

#endif // __cplusplus
#endif // __linux__

using namespace tav;


class FFMpegDecode
{
public:

	typedef struct {
		unsigned int	texPerFrame;
		TextureManager*	tex;
		double 			pts;
	} glFrame;

	FFMpegDecode();
	~FFMpegDecode();

	int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type);

 
	void						setTexIDs(GLuint* ids);
	int							OpenFile(ShaderCollector* _shCol, std::string _filePath, int _useNrThreads,
										 int _destWidth, int _destHeight, bool _useHwAccel, bool _decodeYuv420OnGpu);
	void						start();
	void 						stop();

	void 						initShader(AVPixelFormat _srcPixFmt);
	Shaders* 					getShader() { return shader; }
	void 						shaderBegin();
	void 						shaderEnd();

	void 						alloc_gl_res(AVPixelFormat _srcPixFmt);
	AVFrame* 					alloc_picture(enum AVPixelFormat pix_fmt, int width, int height,
												uint8_t* buf);

	void 						singleThreadDecodeLoop();
	int 						decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext);

	// 2 thread implementation of sending/receiving decoded frames, .... not much faster than the "while" variant
	void 						sendFrameLoop();
	int 						receiveFrameLoop();

	void 						loadFrameToTexture(double time);
	GLenum						texture_pixel_format(AVPixelFormat srcFmt);
	void 						logging(const char *fmt, ...);
	void 						lockMutex();
	void 						unLockMutex();
	void 						log_callback(void *ptr, int level, const char *fmt, va_list vargs);

	double  					get_duration_sec();
	double 						get_fps();
	int64_t 					get_total_frames();
	double 						r2d(AVRational r);

	void 						seek_frame(int64_t _frame_number, double time);
	void 						seek(double sec, double time);

	void						resetToStart(double time);

	GLuint						getTex() { if (textures) return textures[0].getId(); else return 0; }
	bool						isReady() { return ready; }

	void 						setEndCbFunc(std::function<void()> cbFunc) { endCb = cbFunc; }

	static FFMpegDecode* 		this_dec;

	int 						srcWidth;
	int 						srcHeight;
	int 						destWidth;
	int 						destHeight;


	enum AVPixelFormat			srcPixFmt;
	enum AVPixelFormat			destPixFmt;

private:
	AVCodec*					pCodec;
	AVCodecContext*				pCodecContext;
	AVCodecParameters*			pCodecParameters;
	AVFormatContext*			pFormatContext;

	AVFrame* 					frame = NULL;
	AVFrame**					pFrame;
	AVFrame**					bgraFrame;
	AVPacket*					pPacket;
//	struct SwsContext*			img_convert_ctx=0;
	struct SwsContext*			img_convert_ctx;

	enum AVHWDeviceType 		type;
	AVBufferRef*				hw_device_ctx = NULL;
	AVCodecContext*				decoder_ctx = NULL;

	ShaderCollector* 			shCol;
	Shaders*					shader;
	TextureManager*				textures;
	GLuint*						pbos;

	std::string					filePath;
	std::thread					decodeThread;
	std::thread*				sendThread;
	std::thread*				receiveThread;
	std::mutex					mutex;
	std::mutex					pCtxMutex;
	std::mutex					ctxMutex;
	std::condition_variable		ctxCond;

	std::mutex					pp_mutex;
	std::condition_variable 	decodeCond;

	std::mutex					endThread_mutex;
	std::condition_variable 	endThreadCond;

	std::function<void()>		endCb;

	bool						gl_res_inited;
	bool						useHwAccel;
	bool						decodeYuv420OnGpu;
	bool						showLogging;
	bool						intBufFull;
	bool						ready;
	bool						firstFramePresented;
	bool						loop;
	bool*						fBlocked;
	bool						run;
	bool						decodeThreadEnded;
	bool						usePbos;
	bool						is_net_stream;

	int							frameNr;
	int							lastUploadNr;
	int							logLevel;
	int							frameBufferSize;
	int 						response;
	int 						video_stream_index;
	int							showDecIt;
	int							showDecTimeInt;

    unsigned int				nrTexBuffers;
    unsigned int				nrBufferedFrames;
    unsigned int				nrPboBufs;
    unsigned int 				pboIndex;

	double 						startTime;
	double						swsScaleTime;
	double 						eps_zero;
	double						vTimeBaseDiv;
	double						vFrameDur;

	double*						ptss;
	double						lastPtss;

	unsigned int				totNumFrames;
    uint8_t** 					buffer;
    uint8_t*					cpu_buffer = NULL;
    int							cpu_buffer_size;


	StopWatch					uplWatch;
	StopWatch 					conversionWatch;

	glFrame*					glFrames;
	unsigned int				nrGlBufferFrames;
};


#endif /* FFMPEGDECODE_H_ */
