// *************************************************************************
// NAME: glutcam/device.c
/*
DESCRIPTION:

this code opens the given video device and finds its capabilities
it's drawn from the v4l2 sample code in capture.c.
see http://v4l2spec.bytesex.org/spec/capture-example.html

PROCESS:

there are two main entry points into the code in this file:

 * init_source_device(argstruct, &sourceparams, &capabilities)
- open the source device called for in argstruct,
- initialize it as specified in argstruct, sourceparams
- return its capabilities in the capabilities structure

		set_device_capture_parms(&sourceparams, &capabilities)
- set up the device to capture images according to the contents of
sourceparams, capabilities

 */


#include "V4L_Device.h"
#include <iostream>

namespace tav
{

//----------------------------------------------------------------------------

V4L_Device::V4L_Device(Cmdargs_t* argstruct, Sourceparams_t* sourceparams, Videocapabilities_t* capabilities) :
		nrBuffers(6), actBufPtr(0)
{
	if (init_source_device(argstruct, sourceparams, capabilities) == 0)
	{
		if (set_device_capture_parms(sourceparams, capabilities) == 0)
		{
			if( connect_source_buffers(sourceparams) == 0)
			{
				start_capture_device(sourceparams);
			} else {
				printf("V4L_Device error connect_source_buffers\n");
			}
		} else {
			printf("V4L_Device error set_device_capture_parms\n");
		}
	} else {
		printf("V4L_Device error initing source device \n");
	}
}

//----------------------------------------------------------------------------

V4L_Device::~V4L_Device()
{
	if(decompressor != 0)
	{
		if(tjDestroy(decompressor) == -1)
		{
			std::cerr << "Failed to destroy TurboJPEG decompressor! TurboJPEG error: '" << tjGetErrorStr() << "'" << std::endl;
		}
	}
}

//----------------------------------------------------------------------------
// open the video device given in argstruct.devicename;
// store its capabilities in capabilities
//
// return 0 if all's well
// -1 if malloc fails or we can't read the device's
// capture capabilities

int V4L_Device::init_source_device(Cmdargs_t* argstruct, Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int retval, buffersize;

	// open it and make sure it's a character device
	int fd = verify_and_open_device(argstruct->devicename);

	if (0 > fd)
	{
		retval = -1; // error

	} else
	{
		// fill in sourceparams with the image size and encoding
		// from the command line.
		sourceparams->source = LIVESOURCE;
		sourceparams->fd = fd;
		sourceparams->encoding = argstruct->encoding;
		sourceparams->image_width = argstruct->image_width;
		sourceparams->image_height = argstruct->image_height;

		// start here
		// now allocate a buffer to hold the data we read from
		// this device.
		buffersize = compute_bytes_per_frame(argstruct->image_width,
				argstruct->image_height,
				argstruct->encoding);

		sourceparams->captured.start = NULL;
		sourceparams->captured.length = buffersize;


		// now get the device capabilities and select the io method
		// based on them.
		retval = get_device_capabilities(argstruct->devicename, fd, capabilities);

		if (0 == retval)
			select_io_method(sourceparams, capabilities);
	}

	return(retval);
}

//----------------------------------------------------------------------------
//                 given the devicename, do "stat" on it to make sure
//		 it's a character device file.
//
//		 if it is, open it and  return the filedescriptor.
//		 if it's not, complain and return -1.

int V4L_Device::verify_and_open_device(char * devicename)
{
	int fd;
	struct stat buff;
	char errstring[ERRSTRINGLEN];

	if (-1 == stat(devicename, &buff))
	{
		sprintf(errstring, "Error: can't 'stat' given device file '%s'",
				devicename);
		perror(errstring);
		fd = -1;
	}
	else if (!S_ISCHR (buff.st_mode))
	{
		fprintf (stderr, "%s is not a character device\n", devicename);
		fd = -1;
	}
	else
	{
		fd = open(devicename,
				O_RDWR // required
				| O_NONBLOCK, 0);

		if (-1 == fd)
		{
			sprintf(errstring, "Error: can't 'open' given device file '%s'",
					devicename);
			perror(errstring);
		}
	}

	return(fd);
}

//----------------------------------------------------------------------------
// do an ioctl call; if the call gets interrupted by
// a signal before it finishes, try again.

int V4L_Device::xioctl(int fd, int request, void * arg)
{
	int r;

	do
		r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

//----------------------------------------------------------------------------
// get the capture capabilities of the devicename and return
// them in capabilities. the capabililities data will
// allow us to determine how we can talk to the
// device.
//
// bits will be set in capabilities.capture.capabilities
// including:
//
// * V4L2_CAP_VIDEO_CAPTURE - it's capture device
// * V4L2_CAP_READWRITE - it can pass data with read
// * V4L2_CAP_STREAMING - it can do streaming i/o
// * V4L2_CAP_ASYNCIO - it can do asynchronous i/o
//
// (see the v4l2 specification for the complete set)
//
// return -1 of devicename isn't a v4l2 device or
// if there was an error using the
// VIDIOC_QUERYCAP ioctl.

int V4L_Device::get_device_capabilities(char * devicename, int device_fd,
		Videocapabilities_t * capabilities)
{
	int retval, querystatus, common_found;
	char errstring[ERRSTRINGLEN];

	memset(capabilities, 0, sizeof(*capabilities));

	querystatus = xioctl (device_fd, VIDIOC_QUERYCAP, &(capabilities->capture));

	if (-1 == querystatus)
	{
		switch (errno)
		{
		case EINVAL:
			sprintf(errstring, "Error-- is '%s' really a v4l2 device or v4l1?",
					devicename);
			break;

		default:
			sprintf(errstring, "Error doing VIDIOC_QUERYCAP on '%s'",
					devicename);
			break;
		}
		perror(errstring);
		retval = -1;
	}
	else
	{
		fprintf(stderr, "\nInfo: '%s' connects to %s using the %s driver\n\n",
				devicename, capabilities->capture.card,
				capabilities->capture.driver);
		describe_capture_capabilities((char*) "Device has the following capabilities",
				&capabilities->capture);
		describe_device_controls((char*)"Device has the following controls available",
				devicename, device_fd);

		common_found = 0;
		collect_supported_image_formats(device_fd, capabilities);
		if (1 == capabilities->supports_yuv420)
		{
			fprintf(stderr, "device supports YUV420\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_yuv422)
		{
			fprintf(stderr, "device supports YUV422\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_greyscale)
		{
			fprintf(stderr, "device supports GRAY\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_rgb)
		{
			fprintf(stderr, "device supports RGB\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_rgb_bayer_gbrg)
		{
			fprintf(stderr, "device supports RGB Bayer GBRG\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_mjpeg)
		{
			fprintf(stderr, "device supports MJPEG\n");
			common_found = 1;
		}
		if (1 == capabilities->supports_rgb32)
		{
			fprintf(stderr, "device supports RGB32\n");
			common_found = 1;
		}


		if (0 == common_found)
		{
			fprintf(stderr, "******************************************\n");
			fprintf(stderr,
					"Source doesn't supply a format this program understands\n");
			fprintf(stderr, "******************************************\n");
			retval = -1;
		}
		else
		{
			retval = 0;
		}

	}
	return(retval);
}

//----------------------------------------------------------------------------
//                 given the capabilities of the source device, select the
//		 iomethod this program will use to talk to it.
//
//		 we'll try streaming first because it's more efficient.
//		 if streaming is available, set the iomethod to be mmap.
//		 if streaming isn't available, check for read and set
//		 iomethod for that.
//
//		 the V4L2 standard says a device must support either streaming
//		 or read i/o.
//
//		 modifies sourceparams->iomethod,
//		 aborts with error message if the function can't figure out
//		 which method to use.
void V4L_Device::select_io_method(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	struct v4l2_capability * capture_capabilties;

	//  IO_METHOD_READ IO_METHOD_MMAP work
	// IO_METHOD_USERPTR not supported by the driver i have
	// sourceparams->iomethod = IO_METHOD_USERPTR;
	capture_capabilties = &(capabilities->capture);

	if (V4L2_CAP_STREAMING & capture_capabilties->capabilities)
	{
		sourceparams->iomethod = IO_METHOD_MMAP;

	} else if (V4L2_CAP_READWRITE & capture_capabilties->capabilities)
	{
		sourceparams->iomethod = IO_METHOD_READ;

	} else
	{
		fprintf(stderr,
				"Fatal Error in %s: can't find an IO method to get the images\n",
				__FUNCTION__);
		abort();
	}
	// if you want to override the logic, put your selection here
	//  sourceparams->iomethod = IO_METHOD_READ;
}

//----------------------------------------------------------------------------
// based whether the source is test pattern or live
// (sourceparams->source) and if live, what the input
// method is, create a buffer for the input data
// and point sourceparams->captured.start at that buffer
//
// if this function doesn't recognize the input method,
// print a warning to stderr and abort.
//
//	return 0 on success, -1 on error

int V4L_Device::connect_source_buffers(Sourceparams_t * sourceparams)
{
	int retval;

	switch(sourceparams->iomethod)
	{
	case IO_METHOD_READ:
		retval = allocate_capture_buffer(sourceparams);
		break;

	case IO_METHOD_MMAP:
		// point captured.start at an empty buffer that we can draw
		// until we get data
		sourceparams->captured.start = sourceparams->buffers[0].start;
		retval = 0; // we're fine
		break;

	case IO_METHOD_USERPTR:
		sourceparams->captured.start = sourceparams->buffers[0].start;
		retval = 0;
		break;

	default:
		fprintf(stderr, "Fatal Error in %s: can't find an IO method to get the images\n", __FUNCTION__);
		abort();
		break;
	}

	return(retval);
}

//----------------------------------------------------------------------------
// malloc a buffer of sourceparams->captured.length
// bytes and point sourceparams->captured.start
// to it.
//
// on error complain to stderr and return -1, on success return 0

int V4L_Device::allocate_capture_buffer(Sourceparams_t * sourceparams)
{
	int retval;

	sourceparams->captured.start =  malloc(sourceparams->captured.length);

	if (NULL == sourceparams->captured.start)
	{
		fprintf(stderr,
				"Error: unable to malloc %d bytes for capture buffer",
				(int) sourceparams->captured.length);
		sourceparams->captured.length = 0;
		retval = -1; // error

	} else
	{
		retval = 0;
	}

	return(retval);
}

//----------------------------------------------------------------------------
// set the device capture parameters according to what's in
// sourceparams and capabilities. this includes:
//
// * trying to set the device cropping and scaling to
//		   show the entire picture
// * setting the image size and encoding to what's in
//		   sourceparams
// * setting the iomethod (that we use to get data from
//		   the device)
//
//	return 0 if all's well, -1 on error

int V4L_Device::set_device_capture_parms(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int status;

	// set whole picture, dimensions, encoding
	try_reset_crop_scale(sourceparams);
	status = set_image_size_and_format(sourceparams);

	if (0 == status)
		status = set_io_method(sourceparams, capabilities);

	return(status);
}

//----------------------------------------------------------------------------
//                 try to reset the cropping area to its default to
//		 display the entire picture.
//
//		 this works by using the VIDIOC_CROPCAP ioctl to
//		 get the default area, when using VIDIOC_S_CROP
//		 to set that as the area we want to see.
//
//		 on error, complain.

void V4L_Device::try_reset_crop_scale(Sourceparams_t * sourceparams)
{
	int status;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	char errstring[ERRSTRINGLEN];

	memset(&cropcap, 0, sizeof(cropcap));

	// set crop/scale to show capture whole picture
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// get the area for the whole picture in cropcap.defrect
	status = xioctl (sourceparams->fd, VIDIOC_CROPCAP, &cropcap);

	if (0 ==  status)
	{
		// set the area to that whole picture
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; // reset to default

		if (-1 == xioctl (sourceparams->fd, VIDIOC_S_CROP, &crop))
		{
			switch (errno) {
			case EINVAL:
				sprintf(errstring, "Warning: VIDIOC_S_CROP cropping not supported");
				// Cropping not supported.
				break;
			default:
				// Errors ignored.
				sprintf(errstring, "Warning: ignoring VIDIOC_S_CROP error ");
				break;
			}

			perror(errstring);
		}
	}
	else
	{
		// Errors ignored.
		perror("Warning: ignoring error when trying to retrieve crop area");
	}

}

//----------------------------------------------------------------------------
//
//                 given the encoding enumeration for this program
//		 (a subset of the v4l2 formats), return the format
//		 specifier for each encoding.
//
//		 if this isn't kept up to date (ie a new encoding
//		 added to the Encodingmethod_t enumeration), the
//		 default clause will catch and let the user know the
//		 function needs to be updated.

__u32 V4L_Device::encoding_format(Encodingmethod_t encoding)
{
	unsigned int format;

	switch (encoding)
	{
	case LUMA:
		format = V4L2_PIX_FMT_GREY; break;
	case YUV420:
		format = V4L2_PIX_FMT_YUV420; break;
	case YUV422:
		format = V4L2_PIX_FMT_YUYV; break;
	case MJPEG :
		format = V4L2_PIX_FMT_MJPEG; break;
	case RGB:
		format = V4L2_PIX_FMT_RGB24; break;
	case RGB32:
		format = V4L2_PIX_FMT_RGB32; break;
	case RGB_BAYER_GBRG:
		format = V4L2_PIX_FMT_SGBRG8; break;

	default:
		fprintf(stderr, "Error: no format for encoding %d in %s\n",
				encoding, __FUNCTION__);
		fprintf(stderr, "  fix that and recompile\n");
		abort();
		break;
	}

	return(format);
}


//----------------------------------------------------------------------------
// set the image size and pixel format of the
// device using the VIDIOC_S_FMT ioctl.
//
// modifies sourceparams image_width, image_height
// to whatever the camera supplies.
//
// return 0 if all's well
// -1 on error

int V4L_Device::set_image_size_and_format(Sourceparams_t * sourceparams)
{
	struct v4l2_format format;
	int retval;
	char errstring[ERRSTRINGLEN];
	unsigned int requested_height, requested_width;
	unsigned int supplied_height, supplied_width;
	memset(&format, 0, sizeof(format));

	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// first get the current format, then change the parts we want
	// to change...
	retval = xioctl (sourceparams->fd, VIDIOC_G_FMT, &format);

	if (-1 == retval)
	{
		sprintf(errstring, "Fatal error trying to GET format");
		perror(errstring);
	}
	else
	{
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.fmt.pix.width  = sourceparams->image_width;
		format.fmt.pix.height  = sourceparams->image_height;
		format.fmt.pix.pixelformat = encoding_format(sourceparams->encoding);
		format.fmt.pix.field = V4L2_FIELD_ANY;

		// V4L2 spec says interlaced is typical, so let's do that.
		// format.fmt.pix.field = V4L2_FIELD_INTERLACED;
		// handle interlace another day; just do progressive scan
		// format.fmt.pix.field = V4L2_FIELD_INTERLACED;

		retval = xioctl (sourceparams->fd, VIDIOC_S_FMT, &format);
		if (-1 == retval)
		{
			sprintf(errstring,
					"Fatal error trying to set format %s: format not supported\n",
					get_encoding_string(sourceparams->encoding));
			perror(errstring);
		}
		else
		{
			// we got an answer back; we asked for a height and width
			// and pixel format; the driver returns what height and
			// width it's going to supply; tell the user if the
			// supplied height and width are not what was requested.
			requested_height = sourceparams->image_height;
			requested_width = sourceparams->image_width;
			supplied_height = format.fmt.pix.height;
			supplied_width = format.fmt.pix.width;

			if ((requested_height != supplied_height) ||
					(requested_width != supplied_width))
			{
				fprintf(stderr, "Warning: program requested size %d x %d; ",
						sourceparams->image_width, sourceparams->image_height);
				fprintf(stderr, " source offers %d x %d\n",
						format.fmt.pix.width, format.fmt.pix.height);
				fprintf(stderr, "Adjusting to %d x %d...\n",
						format.fmt.pix.width,
						format.fmt.pix.height);
				sourceparams->image_width = format.fmt.pix.width;
				sourceparams->image_height = format.fmt.pix.height;
			}
		}
	}

	act_width = format.fmt.pix.width;
	act_height = format.fmt.pix.height;

	std::cerr << "V4L_Device act size: " << act_width << ", " << act_height << std::endl;

	if (format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
	{
		std::cerr << "V4L_Device pix format is MJPEG" << std::endl;

		needsDecoding = true;
		decodeOnGpu = false;

		decompressor = tjInitDecompress();
		if(decompressor == 0)
			std::cerr << "V4L_Device Failed to initialize TurboJPEG decompressor! TurboJPEG error: '" << tjGetErrorStr() << "'" << std::endl;

    	numBytes = act_width * act_height * 3;
		buffer = new uint8_t*[nrBuffers];
    	for (unsigned int i=0; i<nrBuffers; i++)
			buffer[i] = new uint8_t[numBytes];
	}

	return(retval);
}

//----------------------------------------------------------------------------
// this uses the  EXPERIMENTAL V4L2 hook
// VIDIOC_ENUM_FRAMESIZES to find the available frame sizes
// and print them to stderr. the intention is to help the
// user in picking a size...

#ifdef VIDIOC_ENUM_FRAMESIZES
void V4L_Device::print_supported_framesizes(int device_fd, __u32 pixel_format, char * label)
{
	struct v4l2_frmsizeenum sizes;
	int retval, indx, found_a_size;

	indx = 0;
	found_a_size = 0;

	fprintf(stderr, "%s:\n", label);
	do {

		memset(&sizes, 0, sizeof(sizes));
		sizes.index = indx;
		sizes.pixel_format = pixel_format;

		retval = xioctl (device_fd, VIDIOC_ENUM_FRAMESIZES, &sizes);

		if (0 == retval)
		{
			found_a_size = 1;
			switch (sizes.type)
			{
			case V4L2_FRMSIZE_TYPE_DISCRETE:
				fprintf(stderr, "   [%d] %d x %d\n", sizes.index,
						sizes.discrete.width,
						sizes.discrete.height);
				break;

			case V4L2_FRMSIZE_TYPE_CONTINUOUS: // fallthrough
			case V4L2_FRMSIZE_TYPE_STEPWISE:
				fprintf(stderr, "  [%d] %d x %d to %d x %d in %x %d steps",
						sizes.index,
						sizes.stepwise.min_width,
						sizes.stepwise.min_height,
						sizes.stepwise.max_width,
						sizes.stepwise.max_height,
						sizes.stepwise.step_width,
						sizes.stepwise.step_height
				);
				break;

			default:
				fprintf(stderr,
						"Error: VIDIOC_ENUM_FRAMESIZES gave unknown type %d to %s",
						sizes.type,  __FUNCTION__);
				fprintf(stderr, "  fix that and recompile\n");
				abort();
				break;
			}
			indx = indx + 1;
		}
		else
		{
			// VIDIOC_ENUM_FRAMESIZES returns -1 and sets errno to EINVAL
			// when you've run out of sizes. so only tell the user we
			// have an error if we didn't find _any_ sizes to report

			if (0 == found_a_size)
			{
				perror("  Warning: can't get size information");
			}
		}

	} while (0 == retval);

}
#endif // VIDIOC_ENUM_FRAMESIZES

//----------------------------------------------------------------------------
// print out the list of image formats that the
// source supports to help the user in guessing
// which ones to ask for.  if the version of
// V4L2 we're running supports VIDIOC_ENUM_FRAMESIZES,
// then print out the image sizes the source
// will supply in each format.

void V4L_Device::collect_supported_image_formats(int device_fd,
		Videocapabilities_t * capabilities)
{
	int retval, indx;
	struct v4l2_fmtdesc format;
	char labelstring[ERRSTRINGLEN];
	indx = 0;

	std::string device_name(reinterpret_cast<char*>(capabilities->capture.card));

	fprintf(stderr, "Source supplies the following formats:\n");

	do {
		memset(&format, 0, sizeof(format));
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		format.index = indx;

		retval = xioctl(device_fd, VIDIOC_ENUM_FMT, &format);

		if (0 == retval)
		{
			fprintf(stderr, "[%d] %s: %d \n", indx, format.description, format.pixelformat);
#ifdef VIDIOC_ENUM_FRAMESIZES
			sprintf(labelstring, "   For %s source offers the following sizes:",
					format.description);
			print_supported_framesizes(device_fd, format.pixelformat,
					labelstring);
#endif // VIDIOC_ENUM_FRAMESIZES
			if (V4L2_PIX_FMT_YUV420 == format.pixelformat)
			{
				capabilities->supports_yuv420 = 1;
			}
			else if (V4L2_PIX_FMT_YUYV == format.pixelformat)
			{
				capabilities->supports_yuv422  = 1;
			}
			else if (V4L2_PIX_FMT_GREY == format.pixelformat)
			{
				capabilities->supports_greyscale   = 1;
			}
			else if (V4L2_PIX_FMT_RGB24 == format.pixelformat)
			{
				capabilities->supports_rgb = 1;
			}
			else if (V4L2_PIX_FMT_SGBRG8 == format.pixelformat)
			{
				capabilities->supports_rgb_bayer_gbrg = 1;
			}
			else if (V4L2_PIX_FMT_MJPEG == format.pixelformat)
			{
				capabilities->supports_mjpeg = 1;
			}
			// hack for DFK 23UX249,
			else if (V4L2_PIX_FMT_RGB32 == format.pixelformat || (std::strcmp(device_name.c_str(), "DFK 23UX249") == 0 && 0 == format.pixelformat) )
			{
				capabilities->supports_rgb32 = 1;
			}

			indx = indx + 1;
		}
	} while (0 == retval);


}

//----------------------------------------------------------------------------
// return a human-readable string that matches the
// enumerated value of encoding.

char * V4L_Device::get_encoding_string(Encodingmethod_t encoding)
{
	char * name;

	switch (encoding)
	{
	case LUMA:
		name = (char*) "LUMA";
		break;
	case YUV420:
		name = (char*) "YUV420";
		break;
	case YUV422:
		name = (char*) "YUV422";
		break;
	case RGB:
		name = (char*) "RGB";
		break;
	case RGB32:
		name = (char*) "RGB32";
		break;
	case RGB_BAYER_GBRG:
		name = (char*) "RGB_BAYER_GBRG";
		break;
	case MJPEG:
		name = (char*) "MJPEG";
		break;

	default:
		fprintf(stderr,
				"Warning: unknown encoding %d in %s: fix it.\n",
				(int)encoding, __FUNCTION__);
		name = (char*) " ";
		break;
	}
	return(name);
}


//----------------------------------------------------------------------------
//                 set the iomethod the device will use to supply data:
//
// * IO_METHOD_READ -- device expects to be read from
//		   (eg read(sourceparams->fd,...)
//
// * IO_METHOD_MMAP - the device's buffers will be
//		   mmap-ed into user space in
//		   sourceparams->buffers[0..sourceparams->buffercount -1]
//
// * IO_METHOD_USERPTR - memory will be allocated to
//		   sourceparams->buffers[0..sourceparams->buffercount -1]
//		   and the device will store data there directly.
//
//
//		the read method involves a context swap to get the data.
//	        the mmap method gives the process a pointer into kernel
//		    space to get the data.
//		the userptr method tells the driver to store the data in
//		    the process' own memory space directly. (not all drivers
//		    can do userptr.)
//
//		if the switch statement is not kept up to date,
//		   the default case will catch and remind the user
//		   to fix it.
//
//	        returns 0 if all's well
//		       -1 on error

int V4L_Device::set_io_method(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int retval;

	switch (sourceparams->iomethod)
	{
	case IO_METHOD_READ:
		retval = init_read_io(sourceparams, capabilities);
		break;
	case IO_METHOD_MMAP:
		retval = init_mmap_io(sourceparams, capabilities);
		break;
	case IO_METHOD_USERPTR:
		retval = init_userptr_io(sourceparams, capabilities);
		break;

	default:
		fprintf(stderr, "Error: %s doesn't have a case for %d\n",
				__FUNCTION__, sourceparams->iomethod);
		fprintf(stderr, "  add one and recompile\n");
		abort();
		break;
	}

	return(retval);
}

//----------------------------------------------------------------------------
//                 set up the device to do read io.
//
//		 first, check the capabilities and see if it's capable
//		 of read io. if not, complain and tell what modes it
//		 is capable of.
//
//		 if it is capable of read io, allocate a buffer in
//		 sourceparams->buffers[0] of the correct size.
//
//		 return -1 if the device can't do read io or if malloc
//		         runs out of memory
//			 0 if all's well.

int V4L_Device::init_read_io(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int retval, status;

	if (!(V4L2_CAP_READWRITE & capabilities->capture.capabilities))
	{
		describe_capture_capabilities((char*) "Error: device can't do read I/O",
				&(capabilities->capture));
		retval = -1;
	}
	else
	{
		// allocate a single buffer in user space (malloc)

		sourceparams->buffercount = 1;
		status = userspace_buffer_mode(sourceparams);

		if (-1 == status)
		{
			retval = -1;
		}
		else
		{
			retval = 0;
		}
	}

	return(retval);
}

//----------------------------------------------------------------------------
//                 set up the device for mmap access
//
//		 first check to see if the device supports streaming
//		 io (needed for mmap and user-pointer access).
//
//		 if it doesn't, complain, tell what capabilities it does have.
//
//		 if it does, get the device's buffers set up for mmap access.
//
//
//		 return 0 if all's well
//		       -1 if streaming io is not supported.

int V4L_Device::init_mmap_io(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int retval;

	if (!(V4L2_CAP_STREAMING & capabilities->capture.capabilities))
	{
		describe_capture_capabilities((char*) "Error: device can't do streaming I/O",
				&(capabilities->capture));
		retval = -1;
	}
	else
	{
		retval = request_and_mmap_io_buffers(sourceparams);

	}

	return(retval);
}

//----------------------------------------------------------------------------
//                 request access to the device's buffers so we can
//		 mmap them into user space.
//
//		 return 0 if all's well
//		       -1 on error

int V4L_Device::request_and_mmap_io_buffers(Sourceparams_t * sourceparams)
{
	int retval, buffercount;

	// find out how many buffers are available for mmap access
	buffercount = request_video_buffer_access(sourceparams->fd,
			V4L2_MEMORY_MMAP);

	if (-1 == buffercount) // mmap not supported
	{
		retval = -1; // error
	}
	else
	{
		// ioctl succeeds: let's see how many buffers we can work with
		// we need at least two.
		if (2 > buffercount)
		{
			fprintf(stderr, "Error: couldn't get enough video buffers from");
			fprintf(stderr, "  the video device. Requested %d, would have ",
					MAX_VIDEO_BUFFERS);
			fprintf(stderr, "settled for 2, got %d\n", buffercount);
			retval = -1; // error
		}
		else
		{
			// we got at least two buffers; call mmap so we can
			// get access to them from user space.
			sourceparams->buffercount = buffercount;
			retval = mmap_io_buffers(sourceparams);

			// now sourceparams->buffers[0..buffercount -1] point
			// to the device's video buffers, so we can get video
			// data from them.
		}

	}

	return(retval);
}

//----------------------------------------------------------------------------
// use VIDIOC_REQBUFS to find out if we can get access to the
// device's buffers in the given mode.
//
// return -1 if that mode is not supported
// else N for the number of buffers we can access

int V4L_Device::request_video_buffer_access(int device_fd, enum v4l2_memory memory)
{
	struct v4l2_requestbuffers request;
	int status, retval;
	char * mmap_error =  (char*) "Error: video device doesn't support memory mapping";
	char * userptr_error= (char*) "Error: video device doesn't support user pointer I/O";
	char * errstring;

	memset(&request, 0, sizeof(request));

	// ask for MAX_VIDEO_BUFFERS for video capture: we may not
	// get all the ones we ask for

	request.count = MAX_VIDEO_BUFFERS;
	request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	request.memory = memory;

	status = xioctl (device_fd, VIDIOC_REQBUFS, &request);

	if (-1 == status) // error
	{
		// assign errstring to the error message we can use if errno is
		// EINVAL
		if (V4L2_MEMORY_MMAP == memory)
		{
			errstring = mmap_error;
		} else
		{
			errstring = userptr_error;
		}

		// okay, print an error message
		if (EINVAL == errno)
		{
			fprintf(stderr, "%s\n", errstring);
		} else
		{
			perror("Error trying to request video buffers from device\n");
		}

		retval = -1; // error return
	} else
	{
		retval = request.count; // the number of buffers available
	}

	return(retval);
}

//----------------------------------------------------------------------------
//                 for each buffer in buffercount
//		     use the VIDIOC_QUERYBUF to find
// * the buffer's offset from start of video device memory
// * the length of the buffer
//
//		     call mmap with the video device's file descriptor and
//		          that offset and length to get an address we can
//			  use from process space.
//		     assign that address and length to
//		         sourceparam->buffers[i].start and .length
//
//		 return 0 if all's well
//		       -1 if the ioctl returns an error or
//		          if mmap returns an error

int V4L_Device::mmap_io_buffers(Sourceparams_t * sourceparams)
{
	int i, status, retval;
	struct v4l2_buffer buf;
	void * mmapped_buffer;

	status = 0;
	retval = 0;

	for (i = 0; (i < sourceparams->buffercount) && (0 == status); i++)
	{
		memset(&buf, 0, sizeof(buf));

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		status =  xioctl (sourceparams->fd, VIDIOC_QUERYBUF, &buf);

		if (-1 == status)
		{
			perror("Error trying to get buffer info in VIDIOC_QUERYBUF");
			retval = -1;
		}
		else
		{
			mmapped_buffer = mmap (NULL, // start anywhere
					buf.length,
					PROT_READ | PROT_WRITE, // required
					MAP_SHARED, // recommended
					sourceparams->fd, buf.m.offset);
			if (MAP_FAILED != mmapped_buffer)
			{
				sourceparams->buffers[i].length = buf.length;
				sourceparams->buffers[i].start = mmapped_buffer;
			}
			else
			{
				perror("Error: can't mmap the video buffer");
				retval = -1;
				status = -1;
			}
		}

	}
	return(retval);
}

//----------------------------------------------------------------------------
// set up the video device for user-pointer access
//
// if the video device is capable if streaming io
// request access to video buffers for user-pointer
// access,
//
// for each video buffer, allocate a user-space buffer
// that the device will store data in.
//
// at the end of this the sourceparams->buffers will have
// malloc-ed memory that the video device will store data in
//
// return 0 if all's well
// -1 if the device doesn't to streaming io
// if the device doesn't support user-pointer access
// if there are fewer than 2 buffers available

int V4L_Device::init_userptr_io(Sourceparams_t * sourceparams,
		Videocapabilities_t * capabilities)
{
	int buffercount;
	int retval;

	if (!(V4L2_CAP_STREAMING & capabilities->capture.capabilities))
	{
		describe_capture_capabilities((char*) "Error: device can't do streaming I/O",
				&(capabilities->capture));
		retval = -1;
	}
	else
	{
		// okay, how many buffers do we have to work with
		buffercount = request_video_buffer_access(sourceparams->fd,
				V4L2_MEMORY_USERPTR);
		if(-1 == buffercount) // error
		{
			retval = -1; // ioctl fails
		}
		else if (2 > buffercount) // too few buffers
		{
			fprintf(stderr, "Error: couldn't get enough video buffers from");
			fprintf(stderr, "  the video device. Requested %d, would have ",
					MAX_VIDEO_BUFFERS);
			fprintf(stderr, "settled for 2, got %d\n", buffercount);
			retval = -1;
		}
		else
		{
			// okay, malloc space for those buffers
			sourceparams->buffercount = buffercount;
			retval = userspace_buffer_mode(sourceparams);
		}
	}

	return(retval);

}

//----------------------------------------------------------------------------
//                 malloc sourceparams->buffercount buffer and
//		 connect them to sourceparams->buffers[*].start
//		 and sourceparams->buffers[*].length

int V4L_Device::userspace_buffer_mode(Sourceparams_t * sourceparams)
{
	int i, imagesize, retval;
	void * buffer;
	char errstring[ERRSTRINGLEN];

	// figure out how big an image is

	imagesize = compute_bytes_per_frame(sourceparams->image_width,
			sourceparams->image_height,
			sourceparams->encoding);

	retval = 0;

	for (i = 0; (i < sourceparams->buffercount) && (0 == retval); i++)
	{
		buffer = malloc(imagesize);

		if (NULL != buffer)
		{
			sourceparams->buffers[i].start = buffer;
			sourceparams->buffers[i].length = imagesize;
		}
		else
		{
			retval = -1; // error
			sprintf(errstring,
					"Error: failed to allocate %d bytes for video buffer",
					imagesize);
			perror(errstring);
		}

	}
	return(retval);
}

//----------------------------------------------------------------------------
//                 start the capture device supplying data.
//
//		 nothing special is required for read.
//		 for mmmap or user pointer mode,
//		    queue up the buffers to be filled
//		    start streaming
//
//		if the iomethod is not recognized,
//		   given an error message and abort
//
//	        return 0 on success
//		      -1 on error

int V4L_Device::start_capture_device(Sourceparams_t * sourceparams)
{
	int retval;

	switch (sourceparams->iomethod)
	{
	case IO_METHOD_READ:

		// no special action required to start
		retval = 0;
		break;

	case IO_METHOD_MMAP:
		retval = enqueue_mmap_buffers(sourceparams);
		if (0 == retval)
		{
			retval = start_streaming(sourceparams);
		}
		break;

	case IO_METHOD_USERPTR:
		retval = enqueue_userpointer_buffers(sourceparams);
		if (0 == retval)
		{
			retval = start_streaming(sourceparams);
		}
		break;

	default:
		fprintf(stderr, "Error: %s doesn't have a case for iomethod %d\n",
				__FUNCTION__, sourceparams->iomethod);
		fprintf(stderr, "add one and recompile\n");
		abort();
		break;
	}

	return(retval);
}

//----------------------------------------------------------------------------

//                 queue up all the buffers so the driver can fill them with
//		 data.  in particular, buf.index gets used in
//		 harvest_mmap_device_buffer to tell which buffer has the
//		 data in it.
//
//		 works by side effect
//
//		 return 0 if all's well, -1 on error

int V4L_Device::enqueue_mmap_buffers(Sourceparams_t * sourceparams)
{
	int i;
	int status;
	struct v4l2_buffer buf;

	status = 0;

	for(i = 0; (i < sourceparams->buffercount) && (0 == status); i++)
	{
		memset(&buf, 0, sizeof(buf));
		memset(&(buf.timestamp), 0, sizeof(struct timeval));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = (unsigned int)i;
		status =   xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);
	}
	if (-1 == status)
	{
		perror("Error enqueueing mmap-ed buffers with VIDIOC_QBUF");
	}

	return(status);
}

//----------------------------------------------------------------------------
// tell the device to start supplying data through the memory
// mapped buffers.

int V4L_Device::start_streaming(Sourceparams_t * sourceparams)
{
	enum v4l2_buf_type type;
	int status;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	status = xioctl(sourceparams->fd, VIDIOC_STREAMON, &type);

	if (-1 == status)
	{
		perror("Error starting streaming with VIDIOC_STREAMON");
	}

	return(status);
}

//----------------------------------------------------------------------------
// tell the device to stop supplying data by memory
// mapped buffers.

int V4L_Device::stop_streaming(Sourceparams_t * sourceparams)
{
	enum v4l2_buf_type type;
	int status;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	status = xioctl(sourceparams->fd, VIDIOC_STREAMOFF, &type);

	if (-1 == status)
	{
		perror("Error stopping streaming with VIDIOC_STREAMOFF");
	}

	return(status);
}

//----------------------------------------------------------------------------
// queue up the user space buffers so the driver can put
//	data into them.
//
// if the driver supplies data to buffers in user process
// space (ie not kernel space) we'd use IO_METHOD_USERPTR
// and queue up buffers allocated with malloc. this is
// where we handle that case. in particular, the driver
// needs to know the start and length of the buffer.
// (we get buf.m.userptr back in harvest_userptr_device_buffer
// to tell us where to pull data from.)

int V4L_Device::enqueue_userpointer_buffers(Sourceparams_t * sourceparams)
{
	int i;
	int status;
	struct v4l2_buffer buf;

	status = 0;

	for(i = 0; (i < sourceparams->buffercount) && (0 == status); i++)
	{
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.m.userptr = (unsigned long)(sourceparams->buffers[i].start);
		buf.length = sourceparams->buffers[i].length;
		status =   xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);
	}
	if (-1 == status)
	{
		perror("Error enqueueing mmap-ed buffers with VIDIOC_QBUF");
	}

	return(status);
}

//----------------------------------------------------------------------------
// tell the device to stop supplying data

int V4L_Device::stop_capture_device(Sourceparams_t * sourceparams)
{
	int retval;


	switch (sourceparams->iomethod)
	{
	case IO_METHOD_READ:
		retval = 0;
		// no stop function for this; just don't read anymore
		break;

	case IO_METHOD_MMAP: // fallthrough
	case IO_METHOD_USERPTR:
		retval = stop_streaming(sourceparams);
		break;

	default:
		fprintf(stderr, "Error: %s doesn't have a case for iomethod %d\n",
				__FUNCTION__, sourceparams->iomethod);
		fprintf(stderr, "add one and recompile\n");
		abort();
		break;
	}

	return(retval);
}

//----------------------------------------------------------------------------
// collect the next image from the source.
//
// if we're reading, just read
//
// if we're doing IO_METHOD_MMAP or IO_METHOD_USERPTR
// collect the ready buffer
//
// if this function doesn't recognize the given iomethod
// print an error message and abort.
//

void * V4L_Device::next_device_frame(Sourceparams_t * sourceparams, int * nbytesp)
{
	int nbytes;
	void *datap;
	int data_ready;

	data_ready = wait_for_input(sourceparams->fd , DATA_TIMEOUT_INTERVAL);
	if (-1 == data_ready)
	{
		// we had an error waiting on data
		datap = NULL;
		*nbytesp = -1;
		std::cout << "V4L_Device::next_device_frame error waiting on data" << std::endl;

	} else if (0 == data_ready)
	{
		// no data available in timeout interval
		datap = NULL;
		*nbytesp = 0;

		std::cout << "V4L_Device::next_device_frame no data available in timeout interval" << std::endl;

	} else
	{
		switch (sourceparams->iomethod)
		{
		case IO_METHOD_READ:
			nbytes = read_video_frame(sourceparams->fd, &(sourceparams->captured));
			if (0 < nbytes)
			{
				datap = sourceparams->captured.start;
				*nbytesp = nbytes;
			} else
			{
				datap = NULL;
				*nbytesp = nbytes;
			}

			break;

		case IO_METHOD_MMAP:
			nbytes = harvest_mmap_device_buffer(sourceparams);
			if (0 < nbytes)
			{
				datap = sourceparams->captured.start;
				*nbytesp = nbytes;
			} else
			{
				datap = NULL;
				*nbytesp = nbytes;
			}
			break;

		case IO_METHOD_USERPTR:
			nbytes = harvest_userptr_device_buffer(sourceparams);
			if (0 < nbytes)
			{
				datap = sourceparams->captured.start;
				*nbytesp = nbytes;
			} else
			{
				datap = NULL;
				*nbytesp = nbytes;
			}
			break;

		default:
			fprintf(stderr, "Error: %s doesn't have a case for iomethod %d\n",
					__FUNCTION__, sourceparams->iomethod);
			fprintf(stderr, "add one and recompile\n");
			abort();
			break;
		}
	}
	return(datap);
}



//----------------------------------------------------------------------------
// read buffer->length bytes of data from fd into
// buffer->start.
// returns # bytes read

int V4L_Device::read_video_frame(int fd, Videobuffer_t * buffer)
{
	int nread;

	nread = read(fd, buffer->start, buffer->length);

	if (-1 == nread)
	{
		if (EAGAIN == errno) // non-blocking io selected, no data
			nread = 0;
		else
			perror("Error reading data from video device");
	}
	return(nread);
}

//----------------------------------------------------------------------------
// take a filled buffer of video data off the queue and
// copy the data into sourceparams->captured.start.
//
// returns the #bytes of data
// -1 on error

int V4L_Device::harvest_mmap_device_buffer(Sourceparams_t * sourceparams)
{
	int retval, status;
	struct v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	status = xioctl (sourceparams->fd, VIDIOC_DQBUF, &buf);

	if (-1 == status)
	{
		// error dequeueing the buffer
		switch(errno)
		{
		case EAGAIN:
			// nonblocking and no data ready
#ifdef DEBUG_MMAP
			perror("VIDIOC_DQBUF: no data ready");
#endif //  DEBUG_MMAP
			retval = 0;
			break;

		case EIO: // fallthrough
			// transient [?] error
#ifdef DEBUG_MMAP
			perror("VIDIOC_DQBUF: EIO");
#endif //  DEBUG_MMAP
		default:
			perror("Error dequeueing mmap-ed buffer from device");
			retval = -1;
			break;
		}
	}
	else
	{
		// point captured.start at where the data starts in the
		// memory mapped buffer
		sourceparams->captured.start = sourceparams->buffers[buf.index].start;

		// queue the buffer back up again
		status = xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);

		if (-1 == status)
		{
			// error re-queueing the buffer
			switch(errno)
			{
			case EAGAIN:
				// nonblocking and no data ready
#ifdef DEBUG_MMAP
				perror("VIDIOC_QBUF: no data ready");
#endif //  DEBUG_MMAP
				retval = 0;
				break;

			case EIO: // fallthrough
				// transient [?] error
#ifdef DEBUG_MMAP
				perror("VIDIOC_DQBUF: EIO");
#endif //  DEBUG_MMAP
			default:
				perror("Error requeueing mmap-ed buffer from device");
				retval = -1;
				break;
			}
		}
		else
		{
			retval = sourceparams->captured.length;
		}
	}

	return(retval);
}

//----------------------------------------------------------------------------
// wait until either data is ready on the given file descriptor
// (fd) or until the given number of microseconds elapses.

int V4L_Device::wait_for_input(int fd, int useconds)
{
	fd_set readfds;
	int select_result, retval;
	struct timeval timeout;

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	timeout.tv_sec = 0;
	timeout.tv_usec = useconds;

	select_result = select(fd + 1, &readfds, NULL, NULL, &timeout);

	if (-1 == select_result)
	{
		// error return from select
		perror("Error trying to select looking for input");
		retval = -1; // error
	}
	else if (0 < select_result)
	{
		// we have data
		retval = 1;
	}
	else
	{
		// we have a timeout
		retval = 0;
	}
	return(retval);
}

//----------------------------------------------------------------------------
// take a filled buffer of video data off the queue and
// copy the data into sourceparams->captured.start.

int V4L_Device::harvest_userptr_device_buffer(Sourceparams_t * sourceparams)
{

	int retval, status;
	struct v4l2_buffer buf;
	void * image_source;

	memset(&buf, 0, sizeof(buf));

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_USERPTR;

	status = xioctl (sourceparams->fd, VIDIOC_DQBUF, &buf);

	if (-1 == status)
	{
		// error dequeueing the buffer
		switch(errno)
		{
		case EAGAIN:
			// nonblocking and no data ready
			retval = 0;
			break;

		case EIO: // fallthrough
			// transient [?] error
		default:
			perror("Error dequeueing mmap-ed buffer from device");
			retval = -1;
			break;
		}
	}
	else
	{
		// copy the data from the buffer to sourceparams->captured
		image_source = (void *)(buf.m.userptr);

		memcpy(sourceparams->captured.start, image_source,
				sourceparams->captured.length);

		// queue the buffer back up again
		status = xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);

		if (-1 == status)
		{
			// error re-queueing the buffer
			switch(errno)
			{
			case EAGAIN:
				// nonblocking and no data ready
				retval = 0;
				break;

			case EIO: // fallthrough
				// transient [?] error
			default:
				perror("Error requeueing mmap-ed buffer from device");
				retval = -1;
				break;
			}
		}
		else
		{
			retval = sourceparams->captured.length;
		}

	}

	return(retval);
}

//----------------------------------------------------------------------------

int V4L_Device::compute_bytes_per_frame(int image_width, int image_height,
		Encodingmethod_t encoding)
{
	int bytes_per_frame;

	switch(encoding)
	{
	case LUMA:
		// greyscale: 1 byte per pixel
		bytes_per_frame = image_width * image_height;
		break;
	case YUV420:
		// planar format: each pixel has 1 byte of Y + every 4 pixels
		// share a byte of Cr and a byte of Cb: so 1.5 bytes per pixel
		bytes_per_frame = image_width * image_height * 2; // 1.5;
		break;
	case YUV422:
		// 4 bytes represents 2 pixels: YUYV
		bytes_per_frame = image_width * image_height * 2;
		break;
	case MJPEG:
		bytes_per_frame = image_width * image_height * 3;
		break;
	case RGB:
		// 1 byte each of RGB per pixel: 3 bytes per pixel
		bytes_per_frame = image_width * image_height * 3;
		break;
	case RGB32:
		bytes_per_frame = image_width * image_height * 4;
		break;
	case RGB_BAYER_GBRG:
		// 1 byte each of RGB per pixel: 3 bytes per pixel
		bytes_per_frame = image_width * image_height * 3;
		break;
	default:
		fprintf(stderr, "Error: unknown encoding %d in %s\n", encoding, __FUNCTION__);
		fprintf(stderr, "make sure the function has a case for each value\n");
		fprintf(stderr, "in Encodingmethod_t. update and recompile.\n");
		abort();
		break;
	}

	return(bytes_per_frame);
}

//----------------------------------------------------------------------------

void V4L_Device::describe_device_controls(char * label, char * devicename, int device_fd)
{
	struct v4l2_queryctrl queryctrl;
	lastDevice_fd = device_fd;	// Hack, assuming only one device

	fprintf(stderr, "%s", label);
	fprintf(stderr, " using device file %s\n", devicename);

	memset (&queryctrl, 0, sizeof (queryctrl));

	for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1;
			queryctrl.id++)
	{
		if (0 == ioctl (device_fd, VIDIOC_QUERYCTRL, &queryctrl))
		{
			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;
			else
				explain_control_type((char*) "", queryctrl, device_fd);

			//std::cout << "---------->label: " << queryctrl.name << std::endl;
		} else
		{
			if (errno == EINVAL) continue;
			perror ("VIDIOC_QUERYCTRL");
			exit (EXIT_FAILURE);
		}
	}

	for (queryctrl.id = V4L2_CID_PRIVATE_BASE; ; queryctrl.id++)
	{
		if (0 == ioctl (device_fd, VIDIOC_QUERYCTRL, &queryctrl))
		{
			if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;
			else
				explain_control_type((char*) "", queryctrl, device_fd);
		} else
		{
			if (errno == EINVAL) break;
			perror ("VIDIOC_QUERYCTRL");
			exit (EXIT_FAILURE);
		}
	}
}

//----------------------------------------------------------------------------

void V4L_Device::enumerate_menu (char * label, int fd, struct v4l2_queryctrl queryctrl)
{
	struct v4l2_querymenu querymenu;
	// take from V4L2 spec

	fprintf (stderr, "%s:\n", label);
	fprintf (stderr, "  Menu items:\n");

	memset (&querymenu, 0, sizeof (querymenu));
	querymenu.id = queryctrl.id;

	for (querymenu.index = queryctrl.minimum;
			querymenu.index <= (unsigned int)queryctrl.maximum;
			querymenu.index++)
	{
		if (0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu))
		{
			fprintf (stderr, "  %s\n", querymenu.name);
		}
		else
		{
			perror ("VIDIOC_QUERYMENU");
			exit (EXIT_FAILURE);
		}
	}
}

//----------------------------------------------------------------------------

void V4L_Device::explain_control_type(char * label, struct v4l2_queryctrl queryctrl,
		int fd)
{
	fprintf(stderr, "%s ", label);
	fprintf (stderr, "Control %s:", queryctrl.name);

	std::string osc_label(reinterpret_cast<char*>(queryctrl.name));

	// remove whitespaces
	osc_label.erase( std::remove_if(osc_label.begin(), osc_label.end(), [](char x){return std::isspace(x);}),
			osc_label.end() );
	// remove punctation
	osc_label.erase( std::remove_if(osc_label.begin(), osc_label.end(), [](char x){return std::ispunct(x);}),
			osc_label.end() );

	fprintf(stderr, " (%s) ", osc_label.c_str());

	//push the control into the map
	osc_controls[osc_label] = queryctrl;

	switch (queryctrl.type)
	{
	case V4L2_CTRL_TYPE_INTEGER:
		// tell me the range
		fprintf (stderr, "integer %d to %d in increments of %d\n",
				queryctrl.minimum, queryctrl.maximum, queryctrl.step);
		break;

	case V4L2_CTRL_TYPE_BOOLEAN:
		fprintf (stderr, "boolean %d or %d\n", queryctrl.minimum,
				queryctrl.maximum);
		break;

	case V4L2_CTRL_TYPE_MENU:
		enumerate_menu((char *)queryctrl.name, fd, queryctrl);
		break;

	case V4L2_CTRL_TYPE_BUTTON:
		; // empty statement
		fprintf(stderr, "(button)\n");
		break;
#ifdef V4L2_CTRL_TYPE_INTEGER64
	case V4L2_CTRL_TYPE_INTEGER64:
		fprintf(stderr, "value is a 64-bit integer\n");
		break;
#endif // V4L2_CTRL_TYPE_INTEGER64
#ifdef V4L2_CTRL_TYPE_CTRL_CLASS
	case V4L2_CTRL_TYPE_CTRL_CLASS:
		;// empty statement
		break;
#endif // V4L2_CTRL_TYPE_CTRL_CLASS
	default:
		fprintf(stderr, "Warning: unknown control type in %s\n",
				__FUNCTION__);
		break;
	}
}

//----------------------------------------------------------------------------

void V4L_Device::describe_capture_capabilities(char *errstring,
		struct v4l2_capability * cap)
{
	fprintf(stderr, "%s\n", errstring);
	fprintf(stderr, "Device: '%s' Driver: '%s'\n", cap->card, cap->driver);

	if (V4L2_CAP_VIDEO_CAPTURE & cap->capabilities)
		fprintf(stderr, "Device supports video capture.\n");
	else
		fprintf(stderr, "Device does NOT support video capture.\n");

	if (V4L2_CAP_READWRITE & cap->capabilities)
		fprintf(stderr, "Device can supply data by read\n");
	else
		fprintf(stderr, "Device can NOT supply data by read\n");

	if (V4L2_CAP_STREAMING & cap->capabilities)
		fprintf(stderr, "Device supports streaming I/O\n");
	else
		fprintf(stderr, "Device does NOT support streaming I/O\n");

	if (V4L2_CAP_ASYNCIO  & cap->capabilities)
		fprintf(stderr, "Device supports asynchronous I/O\n");
	else
		fprintf(stderr, "Device does NOT support asynchronous I/O\n");
}

//----------------------------------------------------------------------------

void V4L_Device::convert_frame(Sourceparams_t* sourceparams, int* nbytesp)
{
	// decode if necessary
	if (needsDecoding)
	{
		switch(sourceparams->encoding)
		{
		case MJPEG:
			if (*nbytesp > 0 && decompressor)
			{
				int r = tjDecompress2(decompressor,
						(uint8_t*) sourceparams->captured.start,
						*nbytesp,
						buffer[actBufPtr],
						sourceparams->image_width,
						sourceparams->image_width * 3,
						sourceparams->image_height,
						TJPF_BGR,
						0);

				if (r != 0)
					std::cerr << "V4L_Device::convert_frame Error: could not decode MJPEG frame: " << std::endl;
			}
			break;
		default:
			break;
		}
	}

	actBufPtr = ++actBufPtr % nrBuffers;
}
//----------------------------------------------------------------------------

uint8_t* V4L_Device::get_act_decode_buf()
{
	return buffer[(actBufPtr -1 + nrBuffers) % nrBuffers];
}

//----------------------------------------------------------------------------

void V4L_Device::setCtrl(std::string ctrl_label, float val)
{
	struct v4l2_control argp;

	if (osc_controls.size() > 0 && osc_controls.find( ctrl_label ) != osc_controls.end())
	{
		struct v4l2_queryctrl queryctrl = osc_controls[ctrl_label];

		argp.id = queryctrl.id;
		argp.value = (__s32) val;

		switch (queryctrl.type)
		{
		case V4L2_CTRL_TYPE_INTEGER:
			if (-1 == ioctl(lastDevice_fd, VIDIOC_S_CTRL, &argp))
				std::cerr << "V4L_Device::setCtrl Error: could not set control" << std::endl;
			break;
		case V4L2_CTRL_TYPE_BOOLEAN:
			break;

		case V4L2_CTRL_TYPE_MENU:
			break;

		case V4L2_CTRL_TYPE_BUTTON:
			break;
#ifdef V4L2_CTRL_TYPE_INTEGER64
		case V4L2_CTRL_TYPE_INTEGER64:
			break;
#endif // V4L2_CTRL_TYPE_INTEGER64
#ifdef V4L2_CTRL_TYPE_CTRL_CLASS
		case V4L2_CTRL_TYPE_CTRL_CLASS:
			break;
#endif // V4L2_CTRL_TYPE_CTRL_CLASS
		default:
			fprintf(stderr, " V4L_Device::setCtrl Warning: unknown control type in %s\n",
					__FUNCTION__);
			break;
		}
	}
}

//----------------------------------------------------------------------------

unsigned int V4L_Device::getWidth()
{
	return act_width;
}

//----------------------------------------------------------------------------

unsigned int V4L_Device::getHeight()
{
	return act_height;
}

}
