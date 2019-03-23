/* ************************************************************************* 
* NAME: glutcam/device.h
*
* DESCRIPTION:
*
* this is the header file for the code in device.c.
* that code opens the given video device and finds its capabilities.
* it's drawn from the v4l2 sample code in capture.c.
*
* PROCESS:
*
* there are two main entry points into the code in this file:
*
* * init_source_device(argstruct, &sourceparams, &capabilities)
*   - open the source device called for in argstruct,
*   - initialize it as specified in argstruct, sourceparams
*   - return its capabilities in the capabilities structure
*
* * set_device_capture_parms(&sourceparams, &capabilities)
*   - set up the device to capture images according to the contents of
*     sourceparams, capabilities
*
* * connect_source_buffers(&sourceparams)
*   - connect data buffers only available after the device is initialized
*     to the image source buffer.
*   
* ************************************************************************* */

#include <iostream> // sprintf, perror

#include <stdio.h> // sprintf, perror
#include <string.h> // memset, memcpy
#include <stdlib.h> // abort
#include <sys/time.h> // select
#include <sys/types.h> // select, stat, open
#include <sys/stat.h> // stat, open
#include <unistd.h> // select, stat
#include <fcntl.h> // open
#include <sys/ioctl.h> // ioctl
#include <errno.h> // EINTR
#include <sys/mman.h> // mmap
#include <vector>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cstring>
#include <map>

#include <turbojpeg.h>

#include "V4L/V4L_structs.h"


#pragma once

// ERRSTRINGLEN - max length of generated error string
#define ERRSTRINGLEN 127

// DEBUG_MMAP - if this is defined you'll get more information about
// video buffers being exchanged with the video device's driver.
#define DEBUG_MMAP

// DATA_TIMEOUT_INTERVAL - number of microseconds to wait before
// declaring no data available from the input device. tune this
// to the expected speed of your camera. eg:
// 1000000.0 /60 Hz is 16666 usec
// 1000000.0 /30 HZ is 33333 usec
// 1000000.0 /15 Hz is 66666 usec
// if data is available _before_ this timeout elapses you'll get
//    it when it becomes available.
#define DATA_TIMEOUT_INTERVAL (1000000.0 / 15.0)


namespace tav
{

class V4L_Device
{
public:
	V4L_Device(Cmdargs_t* argstruct, Sourceparams_t* sourceparams, Videocapabilities_t* capabilities);
	virtual ~V4L_Device();

	int init_source_device(Cmdargs_t* argstruct,
			      Sourceparams_t* sourceparams,
			      Videocapabilities_t* capabilities);
	int set_device_capture_parms(Sourceparams_t* sourceparams,
				    Videocapabilities_t* capabilities);
	int connect_source_buffers(Sourceparams_t* sourceparams);
	int start_capture_device(Sourceparams_t* sourceparams);
	int stop_capture_device(Sourceparams_t* sourceparams);
	void* next_device_frame(Sourceparams_t* sourceparams, int* nbytesp);


	int verify_and_open_device(char* devicename);
	int get_device_capabilities(char* devicename, int device_fd,
			Videocapabilities_t* capabilities);
	static int xioctl(int fd, int request, void * arg);
	void select_io_method(Sourceparams_t * sourceparams,
			Videocapabilities_t * capabilities);
	int allocate_capture_buffer(Sourceparams_t * sourceparams);
	void try_reset_crop_scale(Sourceparams_t * sourceparams);
	int set_image_size_and_format(Sourceparams_t * sourceparams);
	void print_supported_framesizes(int device_fd, __u32 pixel_format,
			char * label);
	void collect_supported_image_formats(int device_fd,
			Videocapabilities_t * capabilities);
	int set_io_method(Sourceparams_t * sourceparams,
			Videocapabilities_t * capabilities);
	int init_read_io(Sourceparams_t * sourceparams,
			Videocapabilities_t * capabilities);
	int init_mmap_io(Sourceparams_t * sourceparams,
			Videocapabilities_t * capabilities);
	__u32 encoding_format(Encodingmethod_t encoding);
	char * get_encoding_string(Encodingmethod_t encoding);
	int mmap_io_buffers(Sourceparams_t * sourceparams);
	int request_video_buffer_access(int device_fd, enum v4l2_memory memory);
	int request_and_mmap_io_buffers(Sourceparams_t * sourceparams);
	int init_userptr_io(Sourceparams_t * sourceparams,
			Videocapabilities_t * capabilities);
	int userspace_buffer_mode(Sourceparams_t * sourceparams);
	int enqueue_mmap_buffers(Sourceparams_t * sourceparams);
	int start_streaming(Sourceparams_t * sourceparams);
	int stop_streaming(Sourceparams_t * sourceparams);
	int enqueue_userpointer_buffers(Sourceparams_t * sourceparams);
	int read_video_frame(int fd, Videobuffer_t * buffer);
	int harvest_mmap_device_buffer(Sourceparams_t * sourceparams);
	int wait_for_input(int fd, int useconds);
	int harvest_userptr_device_buffer(Sourceparams_t * sourceparams);
	int compute_bytes_per_frame(int image_width, int image_height,
				    Encodingmethod_t encoding);

	void describe_device_controls(char * label, char * devicename, int device_fd);
	void enumerate_menu (char * label, int fd,	struct v4l2_queryctrl queryctrl);
	void explain_control_type(char * label, struct v4l2_queryctrl queryctrl, int fd);

	char* removeSpace(char* s);
	void describe_capture_capabilities(char *errstring, struct v4l2_capability * cap);

	void convert_frame(Sourceparams_t* sourceparams, int* nbytesp);
	uint8_t* get_act_decode_buf();

	void setCtrl(std::string ctrl_label, float val);

	unsigned int getWidth();
	unsigned int getHeight();

	bool						decodeOnGpu=true;

private:
	bool						useTJpeg=false;
	bool 						needsDecoding=false;


    uint8_t*	        		buffer_wrap;
	int64_t           			picture_pts;
    uint8_t**	        		buffer;

    tjhandle 					decompressor;

    void*						codec_mutex;

    std::map<std::string, struct v4l2_queryctrl> osc_controls;

	int							numBytes;
	int							lastDevice_fd;

	unsigned int 				act_width;
	unsigned int 				act_height;

    unsigned int				nrBuffers;
    unsigned int				actBufPtr;
};

}
