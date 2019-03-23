#include <stdio.h>
#include <string.h>
#include <stdlib.h> // exit

#include <headers/gl_header.h>

#include <thread>
#include <mutex>

#include "GLUtils/TextureManager.h"
#include "GLUtils/PingPongFbo.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"
#include "GeoPrimitives/Quad.h"

#include "V4L/V4L_structs.h"
#include "V4L/V4L_Device.h"

#pragma once

namespace tav
{

class V4L
{
public:
	V4L(ShaderCollector* _shCol, char* deviceName, Encodingmethod_t codec, int _dst_width, int _dst_height);
	virtual ~V4L();

	void join();
	virtual void processQueue();
	void loadFrameToTexture();

	GLenum texture_pixel_format(Encodingmethod_t encoding);
	GLenum texture_pixel_internal_format(Encodingmethod_t encoding);

	std::string getStdVertShdr();
	Shaders* initShdrGrey();
	Shaders* initShdrYuv420();
	Shaders* initShdrYuv422();
	Shaders* initShdrRgb();
	Shaders* initShdrRgbBayerGBRG();

	void initialize_texture_coord_offsets(GLfloat *offsets, int kernel_size,
			GLfloat texture_width, GLfloat texture_height);

	void setCtrl(std::string ctrl_label, float val);
	unsigned int getWidth();
	unsigned int getHeight();
	GLuint getActTexId();
	int getActFrameNr();
	uint8_t* getActBuf();
	uint8_t* downloadFrame();

	bool 				isInited = false;
	GLuint* 			texIDs;

private:
	V4L_Device*			device=0;
	Sourceparams_t		sourceparams; // info about video source

	std::mutex    	mutex;
	std::thread*  	m_Thread;

	uint8_t**      		buffer;
	TextureManager*		textures;
	PingPongFbo*		ppFbo;
	ShaderCollector*	shCol;
	Shaders*			dispShdr;
	Quad*				quad;

	std::string			shdr_Header;
	GLfloat*			laplacian;
	GLfloat*			luma_texture_coordinate_offsets;
	GLfloat*			chroma_texture_coordinate_offsets;

	GLuint* 			pbos;
	GLenum				glDownloadFmt;
	bool				isRunning = false;

	const int 			convolution_kernel_size = 3;
	int 				conv_kern_size_sqr;
	int             	frameRate = 0;
	int 				nrBufferFrames;
	int 				actBuffer;
	int 				textureSize;
	int 				dst_width;
	int 				dst_height;
	int					frameNr = -1;

	int					lastUploadNr = -1;

	int					downloadNr = -1;
	unsigned int		nrDownloadBuffers;
	unsigned int		downloadBufReadPtr =0;
	uint64_t			num_downloads=0;
	uint64_t			num_pbos;
    uint64_t 			dx=0;
    unsigned char* 		ptr;
    unsigned int		nDownloadBytes;
	uint8_t**      		downloadBuffers;

};

}
