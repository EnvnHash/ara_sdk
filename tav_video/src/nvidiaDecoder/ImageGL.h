/*
 * Copyright 1993-2015 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#ifndef NV_IMAGE_GL
#define NV_IMAGE_GL

#include "dynlink_cuda.h" // <cuda.h>
#include "dynlink_cudaGL.h" //<cudaGL.h>

#include "NvDecoder_def.h" // use the tav gl headers

#define PAD_ALIGN(x,mask) ( (x + mask) & ~mask )

const int Format2Bpp[] = { 1, 4, 0 };

class ImageGL
{
public:
	enum PixelFormat
	{
		LUMINANCE_PIXEL_FORMAT,
		BGRA_PIXEL_FORMAT,
		UNKNOWN_PIXEL_FORMAT
	};

	ImageGL(unsigned int nDispWidth, unsigned int nDispHeight,
			unsigned int nTexWidth,  unsigned int nTexHeight,
			PixelFormat ePixelFormat = BGRA_PIXEL_FORMAT);

	// Destructor
	~ImageGL();

	void registerAsCudaResource();
	void unregisterAsCudaResource();
	void setTextureFilterMode(GLuint nMINfilter, GLuint nMAGfilter);
	void setCUDAcontext(CUcontext oContext);
	void setCUDAdevice(CUdevice oDevice);
	int Bpp() {
		return Format2Bpp[(int)e_PixFmt_];
	}

	bool isCudaResource() const;

	// Map this image's DX surface into CUDA memory space.
	// Parameters:
	//      ppImageData - point to point to image data. On return this
	//          pointer references the mapped data.
	//      pImagePitch - pointer to image pitch. On return of this
	//          pointer contains the pitch of the mapped image surface.
	//      field_num   - optional, if we are going to deinterlace and display fields separately
	// Note:
	//      This method will fail, if this image is not a registered CUDA resource.
	void map(CUdeviceptr *ppImageData, size_t *pImagePitch, int field_num = 0);

	void unmap();

	// Clear the image.
	// Parameters:
	//      nClearColor - the luminance value to clear the image to. Default is white.
	// Note:
	//      This method will not work if this image is not registered as a CUDA resource at the
	//      time of this call.
	void clear(unsigned char nClearColor = 0xff);

	unsigned int nWidth() const {
		return nWidth_;
	}

	unsigned int nHeight() const {
		return nHeight_;
	}

	unsigned int nTexWidth() const {
		return nTexWidth_;
	}

	unsigned int nTexHeight() const {
		return nTexHeight_;
	}

	void unpackPbo(int field_num) const;

	GLuint getPBO() {
		return gl_pbo_;
	}

	GLuint getTexID() {
		return gl_texid_;
	}

	double getPts() { return pts; }
	void setPts(double _val) { pts = _val; }
private:
	GLuint gl_pbo_;     // OpenGL pixel buffer object
	GLuint gl_texid_;   // Texture resource for rendering
	GLuint gl_shader_;

	unsigned int nWidth_;
	unsigned int nHeight_;
	unsigned int nTexWidth_;
	unsigned int nTexHeight_;
	double pts;
	PixelFormat e_PixFmt_;
	bool bIsCudaResource_;

	CUcontext oContext_;
	CUdevice  oDevice_;
};

#endif // IMAGE_GL

