#pragma once

#ifndef _VideoTexture_
#define _VideoTexture_

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <limits>

#include "GLUtils/TextureManager.h"

#include <thread>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#if defined WIN32 || defined _WIN32
#include <windows.h>
#elif defined __linux__ || defined __APPLE__
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#if defined __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif
#endif


#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if defined(__APPLE__)
#define AV_NOPTS_VALUE_ ((int64_t)0x8000000000000000LL)
#else
#define AV_NOPTS_VALUE_ ((int64_t)AV_NOPTS_VALUE)
#endif

#define LIBAVFORMAT_INTERRUPT_OPEN_TIMEOUT_MS 30000
#define LIBAVFORMAT_INTERRUPT_READ_TIMEOUT_MS 30000


namespace tav
{

#ifdef WIN32
// http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows

static
inline LARGE_INTEGER get_filetime_offset()
{
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return t;
}

static
inline void get_monotonic_time(timespec *tv)
{
	LARGE_INTEGER           t;
	FILETIME				f;
	double                  microseconds;
	static LARGE_INTEGER    offset;
	static double           frequencyToMicroseconds;
	static int              initialized = 0;
	static BOOL             usePerformanceCounter = 0;

	if (!initialized)
	{
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter)
		{
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
		}
		else
		{
			offset = get_filetime_offset();
			frequencyToMicroseconds = 10.;
		}
	}

	if (usePerformanceCounter)
	{
		QueryPerformanceCounter(&t);
	} else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = microseconds;
	tv->tv_sec = t.QuadPart / 1000000;
	tv->tv_nsec = (t.QuadPart % 1000000) * 1000;
}
#else
static
inline void get_monotonic_time(timespec *time)
{
#if defined(__APPLE__) && defined(__MACH__)
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	time->tv_sec = mts.tv_sec;
	time->tv_nsec = mts.tv_nsec;
#else
	clock_gettime(CLOCK_MONOTONIC, time);
#endif
}
#endif

static
inline timespec get_monotonic_time_diff(timespec start, timespec end)
{
	timespec temp;
	if (end.tv_nsec - start.tv_nsec < 0)
	{
		temp.tv_sec = end.tv_sec - start.tv_sec - 1;
		temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
	}
	else
	{
		temp.tv_sec = end.tv_sec - start.tv_sec;
		temp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return temp;
}

static
inline double get_monotonic_time_diff_ms(timespec time1, timespec time2)
{
	timespec delta = get_monotonic_time_diff(time1, time2);
	double milliseconds = delta.tv_sec * 1000 + (double)delta.tv_nsec / 1000000.0;

	return milliseconds;
}

static int get_number_of_cpus(void)
{
#if defined WIN32 || defined _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf( _SC_NPROCESSORS_ONLN );
#elif defined __APPLE__
	int numCPU=0;
	int mib[4];
	size_t len = sizeof(numCPU);

	// set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

	// get the number of CPUs from the system
	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if( numCPU < 1 )
	{
		mib[1] = HW_NCPU;
		sysctl( mib, 2, &numCPU, &len, NULL, 0 );

		if( numCPU < 1 )
			numCPU = 1;
	}

	return (int)numCPU;
#endif
}

struct AVInterruptCallbackMetadata
{
	timespec value;
	unsigned int timeout_after_ms;
	int timeout;
};

static
inline void _opencv_ffmpeg_free(void** ptr)
{
	if(*ptr) free(*ptr);
	*ptr = 0;
}

static
inline int _opencv_ffmpeg_interrupt_callback(void *ptr)
{
	AVInterruptCallbackMetadata* metadata = (AVInterruptCallbackMetadata*)ptr;
	assert(metadata);

	if (metadata->timeout_after_ms == 0)
	{
		return 0; // timeout is disabled
	}

	timespec now;
	get_monotonic_time(&now);

	metadata->timeout = get_monotonic_time_diff_ms(metadata->value, now) > metadata->timeout_after_ms;

	return metadata->timeout ? -1 : 0;
}

class VideoTexture
{
public:
	VideoTexture(char * file, int initFrameRate);
	~VideoTexture();

	void close();

	double getProperty(int);
	bool setProperty(int, double);
	bool grabFrame();
	bool retrieveFrame();
    void loadFrameToTexture();

    GLuint getTex(){ return texIDs[0]; }
	void init();

	void    seek(int64_t frame_number);
	void    seek(double sec);
	bool    slowSeek( int framenumber );

	int64_t get_total_frames();
	double  get_duration_sec();
	double  get_fps();
	int     get_bitrate();

	double  r2d(AVRational r);
	int64_t dts_to_frame_number(int64_t dts);
	double  dts_to_sec(int64_t dts);

	AVFormatContext* 	ic;
	AVCodecContext*  	codecCtx;
	AVCodec* 			avcodec;
	int               	video_stream;
	AVStream*			video_st;
	AVFrame* 			picture;
	AVFrame*           	rgb_picture;
    AVFrame**           pFrameRGB;
	int64_t           	picture_pts;
    uint8_t*	        buffer;
    AVPixelFormat       dst_pix_fmt;

	AVPacket          	packet;
	struct SwsContext*	img_convert_ctx;

	TextureManager*     textures;
	int					nrBufferFrames;
	int					actBuffer;
	int					readBuffer;
	int					numBytes;
    GLuint* 			texIDs;

	int64_t frame_number, first_frame_number;

	double 				eps_zero;

	//   'filename' contains the filename of the videosource,
	//   'filename==NULL' indicates that ffmpeg's seek support works
	//   for the particular file.
	//   'filename!=NULL' indicates that the slow fallback function is used for seeking,
	//   and so the filename is needed to reopen the file on backward seeking.
	char* 				filename;

	AVDictionary *dict;
	AVInterruptCallbackMetadata interrupt_metadata;
};
}
#endif
