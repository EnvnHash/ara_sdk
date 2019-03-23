/*
 * Video4Linux2 C++ Wrapper
 * Supports GREY, YUV420, YUV422, RGB, RGB_BAYER_GBRG, MJPEG
 *
 * "Lightweight" implementation without FFmpeg
 * except MJPEG all decodig is done on the gpu
 * for MJPEG decoding libturbojpeg is used
 *
 * Shows all available controls on startup
 * they can be accessed via setCtrl and a char* with is displayed in brackets () E.g. (Brightness)
 *
 */

#include "V4L.h"

#define STRINGIFY(A) #A

namespace tav
{

V4L::V4L(ShaderCollector* _shCol, char* deviceName, Encodingmethod_t codec, int _dst_width, int _dst_height):
						shCol(_shCol), dst_width(_dst_width), dst_height(_dst_height), nrDownloadBuffers(3), num_pbos(3)
{
	Cmdargs_t argstruct; // command line parms
	Videocapabilities_t capabilities;

	memset(&sourceparams, 0, sizeof(sourceparams));

	argstruct.devicename = deviceName;
	argstruct.encoding = codec; // LUMA, RGB_BAYER, YUV422, YUV420, RGB, MJPEG
	argstruct.image_width = _dst_width;
	argstruct.image_height = _dst_height;
	argstruct.source = LIVESOURCE;

	device = new V4L_Device(&argstruct, &sourceparams, &capabilities);
	isInited = true;

	//-------------------------------------

	if(device->decodeOnGpu)
	{
		shdr_Header = "#version 410 core\n#pragma optimize(on)\n";

		//	laplacian = new GLfloat[9];
		//	laplacian[0] = -1.0f;  laplacian[1] = -1.0f;  laplacian[2] = -1.0f;
		//	laplacian[3] = -1.0f;  laplacian[4] = 8.0f;   laplacian[5] = -1.0f;
		//	laplacian[6] = -1.0f;  laplacian[7] = -1.0f;  laplacian[8] = -1.0f;
		conv_kern_size_sqr = convolution_kernel_size * convolution_kernel_size;

		luma_texture_coordinate_offsets = new GLfloat[conv_kern_size_sqr * 2];
		initialize_texture_coord_offsets(luma_texture_coordinate_offsets, convolution_kernel_size,
				_dst_width, _dst_height);

		chroma_texture_coordinate_offsets = new GLfloat[conv_kern_size_sqr * 2];
		initialize_texture_coord_offsets(chroma_texture_coordinate_offsets,
				convolution_kernel_size, _dst_width / 2,  _dst_height / 2);

		glDownloadFmt = GL_BGR;
		nDownloadBytes = _dst_width * _dst_height * 3;
		pbos = new GLuint[num_pbos];
		glGenBuffers(num_pbos, pbos);

		for (unsigned int i=0; i<num_pbos; ++i)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
			glBufferData(GL_PIXEL_PACK_BUFFER, nDownloadBytes, NULL, GL_STREAM_READ);
		}

		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		downloadBuffers = new uint8_t*[nrDownloadBuffers];
		for (unsigned int i=0; i<nrDownloadBuffers; ++i)
			downloadBuffers[i] = new uint8_t[nDownloadBytes];
	}

	int nrBuffers=1;

	switch(sourceparams.encoding)
	{
	case LUMA :
		nrBuffers = 1;
		dispShdr = initShdrGrey();
		break;
	case YUV420 :
		nrBuffers = 3;
		dispShdr = initShdrYuv420();
		break;
	case YUV422 :
		nrBuffers = 1;
		dispShdr = initShdrYuv422();
		break;
	case RGB :
		nrBuffers = 1;
		dispShdr = initShdrRgb();
		break;
	case RGB32 :
		nrBuffers = 1;
		dispShdr = initShdrRgb();
		break;
	case RGB_BAYER_GBRG :
		nrBuffers = 1;
		dispShdr = initShdrRgbBayerGBRG();
		break;
	case MJPEG :
		nrBuffers = 1;
		if(device->decodeOnGpu)
			std::cerr << "tav::V4L error No implementation for decoding MJPEG on GPU" << std::endl;
		break;
	default :
		nrBuffers = 3;
		break;
	}

	if(device->decodeOnGpu)
	{
		texIDs = new GLuint[nrBuffers];
		textures = new TextureManager[nrBuffers];
		if(device->decodeOnGpu)
			ppFbo = new PingPongFbo(_shCol, _dst_width, _dst_height, GL_RGBA8, GL_TEXTURE_2D);

		for (int i=0;i<nrBuffers;i++)
		{
			texIDs[i] = textures[i].allocate(device->getWidth(), device->getHeight(),
					texture_pixel_internal_format(sourceparams.encoding),
					texture_pixel_format(sourceparams.encoding),
					GL_TEXTURE_2D);
			textures[i].setWraping(GL_CLAMP_TO_BORDER);
		}

		if(device->decodeOnGpu)
			quad = new Quad(-1.0f, -1.f, 2.f, 2.f,
					glm::vec3(0.f, 0.f, 1.f),
					1.f, 1.f, 1.f, 1.f);
	}

	//-------------------------------------

	isRunning = true;
	m_Thread = new std::thread(&V4L::processQueue, this);
}

//------------------------------------------------------------------------------------

void V4L::join()
{
	isRunning = false;
	m_Thread->join();
	delete m_Thread;
}

//------------------------------------------------------------------------------------

void V4L::processQueue()
{
	void* retval=0;
	int framesize;

	while (isRunning)
	{
		switch (sourceparams.source)
		{
		case LIVESOURCE:
			//mutex.lock();
			retval = device->next_device_frame(&sourceparams, &framesize);
			if (!device->decodeOnGpu && retval)
				device->convert_frame(&sourceparams, &framesize);
			frameNr++;

			//mutex.unlock();
			break;

		default:
			fprintf(stderr, "Error: %s doesn't have a case for encoding %d\n",
					__FUNCTION__, sourceparams.encoding);
			fprintf(stderr, "add one and recompile\n");
			abort();
			break;
		}
	}
}

//------------------------------------------------------------------------------------

void V4L::loadFrameToTexture()
{
	if (isInited && frameNr != lastUploadNr)
	{
		if (device->decodeOnGpu)
		{
			// take framesize bytes of data from sourceparams->captured.start
			// and transfer it into displaydata->texture
			// use shaders to convert the color format to rgb

			// if the video is encoded as YUV420 it's in three separate areas in
			// memory (planar-- not interlaced). so if we have three areas of
			// data, set up one texture unit for each one. in the if statement
			// we'll set up the texture units for chrominance (U & V) and we'll
			// put the luminance (Y) data in GL_TEXTURE0 after the if.

			if (YUV420 == sourceparams.encoding)
			{
				int chroma_width, chroma_height, luma_size, chroma_size;
				char * u_texture;
				char * v_texture;

				luma_size = device->getWidth() * device->getHeight();

				chroma_width = device->getWidth() / 2;
				chroma_height = device->getHeight() / 2;
				chroma_size = chroma_width * chroma_height;

				u_texture = ((char *)sourceparams.captured.start) + luma_size;
				v_texture = u_texture + chroma_size;

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, texIDs[1]);
				glTexSubImage2D(GL_TEXTURE_2D,
						0,
						0, 0,
						chroma_width,
						chroma_height,
						texture_pixel_format(sourceparams.encoding),
						GL_UNSIGNED_BYTE,
						v_texture);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, texIDs[2]);
				glTexSubImage2D(GL_TEXTURE_2D,
						0,
						0, 0,
						chroma_width,
						chroma_height,
						texture_pixel_format(sourceparams.encoding),
						GL_UNSIGNED_BYTE,
						u_texture);
			}

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texIDs[0]);
			glTexSubImage2D(GL_TEXTURE_2D,
					0,
					0, 0,
					device->getWidth(),
					device->getHeight(),
					texture_pixel_format(sourceparams.encoding),
					GL_UNSIGNED_BYTE,
					sourceparams.captured.start);

			// -------------------------------------------------------------

			if (sourceparams.encoding != RGB && sourceparams.encoding != RGB32)
			{
				dispShdr->begin();
				dispShdr->setUniform1i("tex_unit", 0);

				if (YUV420 == sourceparams.encoding)
				{
					dispShdr->setUniform1i("u_tex_unit", 1);
					dispShdr->setUniform1i("v_tex_unit", 2);
				}

				if (YUV420 == sourceparams.encoding || YUV422 == sourceparams.encoding )
				{
					dispShdr->setUniform1f("texel_width", 1.f / (float) device->getWidth());
					dispShdr->setUniform1f("texture_width", (float) device->getWidth());
					dispShdr->setUniform2fv("luma_texcoord_offsets", &luma_texture_coordinate_offsets[0], conv_kern_size_sqr);
				}

				if (YUV422 == sourceparams.encoding)
					dispShdr->setUniform2fv("chroma_texcoord_offsets", &chroma_texture_coordinate_offsets[0], conv_kern_size_sqr);

				if (RGB_BAYER_GBRG == sourceparams.encoding)
				{
					dispShdr->setUniform4f("sourceSize",
							(float) device->getWidth(), (float) device->getHeight(),
							1.f / (float) device->getWidth(), 1.f / (float) device->getHeight() );

					dispShdr->setUniform2f("firstRed", 0.f, 1.f);
				}

				ppFbo->dst->bind();

				quad->draw();

				ppFbo->dst->unbind();
				ppFbo->swap();

				dispShdr->end();
			}

		} else
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texIDs[0]);
			glTexSubImage2D(GL_TEXTURE_2D,
					0,
					0, 0,
					device->getWidth(),
					device->getHeight(),
					GL_BGR,
					GL_UNSIGNED_BYTE,
					device->get_act_decode_buf());
		}

		lastUploadNr = frameNr;
	}
}

//----------------------------------------------------------------------------

GLenum V4L::texture_pixel_format(Encodingmethod_t encoding)
{
	int format;

	switch (encoding)
	{
	case LUMA:
		// greyscale, 1 byte/pixel
		format = GL_RED;
		break;
	case YUV420:
		// color, 1.5 bytes/pixel, call it 1 for the luminance *Y)
		// part. (we'll generate other textures for the U & V parts)
		// the shader will turn it into color
		format =  GL_RED;
		break;
	case YUV422:
		// color, 2 bytes/pixel
		format = GL_RG;
		break;
	case MJPEG:
		// color, 3 bytes/pixel
		format = GL_RGB;
		break;
	case  RGB_BAYER_GBRG:
		format = GL_RED;
		break;
	case  RGB:
		// color, 3 bytes/pixel
		format = GL_BGR;
		break;
	case  RGB32:
		// color, 4 bytes/pixel
		format = GL_RG;
		break;
	default:
		fprintf(stderr, "Error: %s doesn't have a case for encoding %d\n",
				__FUNCTION__, encoding);
		fprintf(stderr, "add one and recompile\n");
		abort();
		break;
	}

	return(format);
}

//----------------------------------------------------------------------------

GLenum V4L::texture_pixel_internal_format(Encodingmethod_t encoding)
{
	int format;

	switch (encoding)
	{
	case LUMA:
		// greyscale, 1 byte/pixel
		format = GL_R8;
		break;
	case YUV420:
		// color, 1.5 bytes/pixel, call it 1 for the luminance *Y)
		// part. (we'll generate other textures for the U & V parts)
		// the shader will turn it into color
		format =  GL_R8;
		break;
	case YUV422:
		// color, 2 bytes/pixel
		format = GL_RGB8;
		break;
	case MJPEG:
		format = GL_RGB8;
		break;
	case  RGB_BAYER_GBRG:
		format = GL_R8;
		break;
	case  RGB:
		// color, 3 bytes/pixel
		format = GL_RGB8;
		break;
	case  RGB32:
		// color, 4 bytes/pixel
		format = GL_RGBA8;
		break;
	default:
		fprintf(stderr, "Error: %s doesn't have a case for encoding %d\n",
				__FUNCTION__, encoding);
		fprintf(stderr, "add one and recompile\n");
		abort();
		break;
	}

	return(format);
}

//----------------------------------------------------------------------------

std::string V4L::getStdVertShdr()
{
	std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;
	out vec2 tex_coord;
	void main() {
		tex_coord = texCoord;
		gl_Position = position;
	});
	return "// V4L vertex shader\n" +shdr_Header +stdVert;
}

//----------------------------------------------------------------------------

Shaders* V4L::initShdrGrey()
{
	std::string frag = STRINGIFY(uniform sampler2D tex_unit;
	in vec2 tex_coord;
	layout (location = 0) out vec4 fragColor;
	void main() {
		float bright = texture(tex_unit, tex_coord);
		fragColor = texture(bright, bright, bright, 1.0);
	});

	frag = "// V4L grey fragment shader\n"+shdr_Header+frag;

	return shCol->addCheckShaderText("V4L_grey_shader", getStdVertShdr().c_str(), frag.c_str());
}

//----------------------------------------------------------------------------

Shaders* V4L::initShdrYuv420()
{
	std::string frag = STRINGIFY(
			// adapted from YUV420P-OpenGL-GLSLang.c
			// by  Peter Bengtsson, Dec 2004.
			//
			// YUV420 is a planar (non-packed) format.
			// the first plane is the Y with one byte per pixel.
			// the second plane us U with one byte for each 2x2 square of pixels
			// the third plane is V with one byte for each 2x2 square of pixels
			//
			// image_texture_unit - contains the Y (luminance) component of the
			//    image. this is a texture unit set up by the OpenGL program.
			// u_texture_unit, v_texture_unit - contain the chrominance parts of
			//    the image. also texture units  set up by the OpenGL program.
			uniform sampler2D tex_unit; // Y component
	uniform sampler2D u_tex_unit; // U component
	uniform sampler2D v_tex_unit; // V component

	// luma_texcoord_offsets - the offsets in texture coordinates to
	//   apply to our current coordinates to get the neighboring
	//   pixels (up, down, left, right).
	//
	uniform vec2 luma_texcoord_offsets[9];
	in vec2 tex_coord;
	layout (location = 0) out vec4 fragColor;

	//
	// float laplace_luma()
	//
	//  do laplacian edge detection on the luminance. this makes regions of
	//  constant color 0 while leaving the borders non-zero.
	//
	//  take the luminance value at gl_TexCoord[0].st and multiply it by
	//  the following kernel:
	//
	//   -1  -1  -1
	//   -1   8  -1
	//   -1  -1  -1
	//
	// (which is to say that the luminance value at gl_TexCoord[0].st
	//  is multiplied by 8 and the values of the adjacent texels are
	//  subtracted from that.)
	//
	// to cut down on the algebra here, i'll start with 8 * the 5th element
	// then subtract off the other elements.
	//
	// the result is returned

	float laplace_luma()
	{
		int i;
		float y;

		y = 8.0 * (texture(image_tex_unit,
				tex_coord + luma_texcoord_offsets[4]).r
				- 0.0625) *  1.1643;

		for (i = 0; i < 4; i++)
		{
			y -= (texture(image_tex_unit,
					tex_coord + luma_texcoord_offsets[i]).r
					- 0.0625) *  1.1643;
			y -= (texture(image_tex_unit,
					tex_coord + luma_texcoord_offsets[i + 5]).r
					- 0.0625) *  1.1643;
		}

		return(y);
	}

	void main(void)
	{
		float r;
		float g;
		float b;
		float y;
		float u;
		float v;
		int i;

		// compute the brightness based on laplace
		// edge detection.
		y = laplace_luma();

		// do the math to turn YUV into RGB
		u = texture2D(u_tex_unit, tex_coord).r - 0.5;
		v = texture2D(v_tex_unit, tex_coord).r - 0.5;

		r = y + 1.5958 * v;
		g = y - 0.39173 * u - 0.81290 * v;
		b = y + 2.017 * u;

		fragColor = vec4(r, g, b, 1.0);
	});

	frag = "// SNTrustLogo fragment shader\n"+shdr_Header+frag;

	return shCol->addCheckShaderText("V4L_yuv420_shader", getStdVertShdr().c_str(), frag.c_str());
}

//----------------------------------------------------------------------------

Shaders* V4L::initShdrYuv422()
{

	std::string frag = STRINGIFY(
			uniform sampler2D tex_unit;
	uniform float texel_width;
	uniform float texture_width;

	// luma_texcoord_offsets - the offsets in texture coordinates to
	// apply to our current coordinates to get the neighboring
	// pixels (up, down, left, right).
	uniform vec2 luma_texcoord_offsets[9];
	uniform vec2 chroma_texcoord_offsets[9];

	in vec2 tex_coord;
	layout (location = 0) out vec4 fragColor;

	// float laplace_luma()
	//
	//  do laplacian edge detection on the luminance. this makes regions of
	//  constant color 0 while leaving the borders non-zero.
	//
	//  take the luminance value at gl_TexCoord[0].st and multiply it by
	//  the following kernel:
	//
	//   -1  -1  -1
	//   -1   8  -1
	//   -1  -1  -1
	//
	// (which is to say that the luminance value at gl_TexCoord[0].st
	//  is multiplied by 8 and the values of the adjacent texels are
	//  subtracted from that.)
	//
	// to cut down on the algebra here, i'll start with 8 * the 5th element
	// then subtract off the other elements.
	//
	// the result is returned

	float laplace_luma()
	{
		int i;
		float y;

		y = 8.0 * (texture(tex_unit,
				tex_coord + luma_texcoord_offsets[4]).r
				- 0.0625) *  1.1643;

		for (i = 0; i < 4; i++)
		{
			y -= (texture(tex_unit,
					tex_coord + luma_texcoord_offsets[i]).r
					- 0.0625) *  1.1643;
			y -= (texture(tex_unit,
					tex_coord + luma_texcoord_offsets[i + 5]).r
					- 0.0625) *  1.1643;
		}

		return(y);
	}

	vec3 laplace_yuv422()
	{
		int i;
		vec3 yuv;
		float luma;
		float chroma0;
		float chroma1;
		vec2 location;
		vec4 luma_chroma;

		location = tex_coord;

		luma_chroma = texture2D(tex_unit,
				location + luma_texcoord_offsets[4]);

		luma = 8.0 * luma_chroma.r;
		chroma0 = 8.0 * luma_chroma.a;

		for (i = 0; i < 4; i++)
		{
			luma -= texture(tex_unit,
					location + luma_texcoord_offsets[i]).r;

			luma -= texture(tex_unit,
					location + luma_texcoord_offsets[i + 5]).r;
		}

		for (i = 0; i < 4; i++)
		{
			chroma0 -= texture(tex_unit,
					location + chroma_texcoord_offsets[i]).a;;
			chroma0 -= texture(tex_unit,
					location + chroma_texcoord_offsets[i + 5]).a;
		}

		location.x += texel_width;
		chroma1 = 8.0 * texture(tex_unit,
				location + chroma_texcoord_offsets[4]).a;

		for (i = 0; i < 4; i++)
		{
			chroma1 -= texture(tex_unit,
					location + chroma_texcoord_offsets[i]).a;
			chroma1 -= texture(tex_unit,
					location + chroma_texcoord_offsets[i + 5]).a;
		}

		yuv.r = luma * 1.1643;
		yuv.g = chroma0;
		yuv.b = chroma1;

		return(yuv);
	}

	void main()
	{
		float red;
		float green;
		float blue;
		vec4 luma_chroma;
		float luma;
		float chroma_u;
		float chroma_v;
		float pixelx;
		float pixely;
		float xcoord;
		float ycoord;
		vec3 yuv;

		// note: pixelx, pixely are 0.0 to 1.0 so "next pixel horizontally"
		//  is not just pixelx + 1; rather pixelx + texel_width.
		pixelx = tex_coord.x;
		pixely = tex_coord.y;

		// if pixelx is even, then that pixel contains [Y U] and the
		//    next one contains [Y V] -- and we want the V part.
		// if  pixelx is odd then that pixel contains [Y V] and the
		//     previous one contains  [Y U] -- and we want the U part.

		// note: only valid for images whose width is an even number of
		// pixels
		xcoord = floor (pixelx * texture_width);

		luma_chroma = texture(tex_unit, tex_coord);

		// just look up the brightness
		luma = (luma_chroma.r - 0.0625) * 1.1643;

		if (0.0 == mod(xcoord , 2.0)) // even
		{
			chroma_u = luma_chroma.g;
			chroma_v = texture(tex_unit,
					vec2(pixelx + texel_width, pixely)).g;
		} else // odd
		{
			chroma_v = luma_chroma.g;
			chroma_u = texture(tex_unit,
					vec2(pixelx - texel_width, pixely)).g;
		}
		chroma_u = chroma_u - 0.5;
		chroma_v = chroma_v - 0.5;

		red = luma + 1.5958 * chroma_v;
		green = luma - 0.39173 * chroma_u - 0.81290 * chroma_v;
		blue = luma + 2.017 * chroma_u;

		// set the color based on the texture color
		fragColor = vec4(red, green, blue, 1.0);
	});

	frag = "// V4L Yuv422 fragment shader\n"+shdr_Header+frag;

	std::string vert =  getStdVertShdr();
	return shCol->addCheckShaderText("V4L_yuv422_shader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------------------------------

Shaders* V4L::initShdrRgb()
{
	std::string frag = STRINGIFY(
	uniform sampler2D tex_unit;
	in vec2 tex_coord;
	out vec4 fragColor;
	void main() {
		fragColor = texture(tex_unit, tex_coord);
	});

	frag = "// V4L rgb fragment shader\n"+shdr_Header+frag;

	return shCol->addCheckShaderText("V4L_rgb_shader", getStdVertShdr().c_str(), frag.c_str());
}

//----------------------------------------------------------------------------

Shaders* V4L::initShdrRgbBayerGBRG()
{
	std::string vert = STRINGIFY(layout( location = 0 ) in vec4 position;
	layout( location = 1 ) in vec4 normal;
	layout( location = 2 ) in vec2 texCoord;

	uniform vec4 sourceSize;	// (w,h,1/w,1/h)

	/** Pixel position of the first red pixel in the Bayer pattern.  [{0,1}, {0, 1}]*/
	uniform vec2 firstRed;

	out VS_FS {
		/** .xy = Pixel being sampled in the fragment shader on the range [0, 1]
		    .zw = ...on the range [0, sourceSize], offset by firstRed */
		vec4  center;

		/** center.x + (-2/w, -1/w, 1/w, 2/w); These are the x-positions of the adjacent pixels.*/
		vec4 xCoord;

		/** center.y + (-2/h, -1/h, 1/h, 2/h); These are the y-positions of the adjacent pixels.*/
		vec4 yCoord;
	} out_vert;

	void main(void)
	{
		out_vert.center.xy = texCoord;
		out_vert.center.zw = texCoord * sourceSize.xy + firstRed;

		vec2 invSize = sourceSize.zw;

		out_vert.xCoord = out_vert.center.x + vec4(-2.0 * invSize.x, -invSize.x, invSize.x, 2.0 * invSize.x);
		out_vert.yCoord = out_vert.center.y + vec4(-2.0 * invSize.y, -invSize.y, invSize.y, 2.0 * invSize.y);

		gl_Position = position;
	});

	vert = "// V4L vertex shader\n" +shdr_Header +vert;




	std::string frag = STRINGIFY(
			out vec4 fragColor;\n	// Monochrome RGBA or GL_LUMINANCE Bayer encoded texture.
			uniform sampler2D source;\n
			in VS_FS {\n
				vec4 center;\n
				vec4 xCoord;\n
				vec4 yCoord;\n
			} in_vert;\n

			void main(void)\n
			{\n

				float C = texture(source, in_vert.center.xy).r; // (0, 0)
			const vec4 kC = vec4(4.0,  6.0,  5.0,  5.0) / 8.0;

			// Determine which of four types of pixels we are on.
			vec2 alternate = mod(floor(in_vert.center.zw), 2.0);

			vec4 Dvec = vec4(
					fetch(in_vert.xCoord[1], in_vert.yCoord[1]),  // (-1,-1)
					fetch(in_vert.xCoord[1], in_vert.yCoord[2]),  // (-1, 1)
					fetch(in_vert.xCoord[2], in_vert.yCoord[1]),  // ( 1,-1)
					fetch(in_vert.xCoord[2], in_vert.yCoord[2])); // ( 1, 1)

			vec4 PATTERN = (kC.xyz * C).xyzz;

			// Can also be a dot product with (1,1,1,1) on hardware where that is
			// specially optimized.
			// Equivalent to: D = Dvec[0] + Dvec[1] + Dvec[2] + Dvec[3];
			Dvec.xy += Dvec.zw;
			Dvec.x  += Dvec.y;

			vec4 value = vec4(
					fetch(in_vert.center.x, in_vert.yCoord[0]),   // ( 0,-2)
					fetch(in_vert.center.x, in_vert.yCoord[1]),   // ( 0,-1)
					fetch(in_vert.xCoord[0], in_vert.center.y),   // (-1, 0)
					fetch(in_vert.xCoord[1], in_vert.center.y));  // (-2, 0)

			vec4 temp = vec4(
					fetch(in_vert.center.x, in_vert.yCoord[3]),   // ( 0, 2)
					fetch(in_vert.center.x, in_vert.yCoord[2]),   // ( 0, 1)
					fetch(in_vert.xCoord[3], in_vert.center.y),   // ( 2, 0)
					fetch(in_vert.xCoord[2], in_vert.center.y));  // ( 1, 0)

			// Even the simplest compilers should be able to constant-fold these to avoid the division.
			// Note that on scalar processors these constants force computation of some identical products twice.
			const vec4 kA = vec4(-1.0, -1.5,  0.5, -1.0) / 8.0;
			const vec4 kB = vec4( 2.0,  0.0,  0.0,  4.0) / 8.0;
			const vec4 kD = vec4( 0.0,  2.0, -1.0, -1.0) / 8.0;

			value += temp;

			// There are five filter patterns (identity, cross, checker,
			// theta, phi).  Precompute the terms from all of them and then
			// use swizzles to assign to color channels.
			//
			// Channel   Matches
			//   x       cross   (e.g., EE G)
			//   y       checker (e.g., EE B)
			//   z       theta   (e.g., EO R)
			//   w       phi     (e.g., EO R)

			// Avoid zero elements. On a scalar processor this saves two MADDs and it has no
			// effect on a vector processor.
			PATTERN.yzw += (kD.yz * D).xyy;

			PATTERN += (kA.xyz * A).xyzx + (kE.xyw * E).xyxz;
			PATTERN.xw  += kB.xw * B;
			PATTERN.xz  += kF.xz * F;


			fragColor.rgb = (alternate.y == 0.0) ?
					((alternate.x == 0.0) ?
							vec3(C, PATTERN.xy) :
							vec3(PATTERN.z, C, PATTERN.w)) :
							((alternate.x == 0.0) ?
									vec3(PATTERN.w, C, PATTERN.z) :
									vec3(PATTERN.yx, C));
			fragColor.a = 1.0;
			});

	frag = "// V4L rgb gbrg fragment shader\n"+shdr_Header+"#define fetch(x, y) texture(source, vec2(x, y)).r\n#define kE (kA.xywz)\n#define kF (kB.xywz)\n#define A (value[0])\n#define B (value[1])\n#define D (Dvec.x)\n#define E (value[2])\n#define F (value[3])\n"+frag;

	return shCol->addCheckShaderText("V4L_rgb_bayer_gbrg_shader", vert.c_str(), frag.c_str());
}

//----------------------------------------------------------------------------

void V4L::initialize_texture_coord_offsets(GLfloat *offsets, int kernel_size,
		GLfloat texture_width, GLfloat texture_height)
{
	int i, j;
	GLfloat texel_width, texel_height;

	texel_width = 1.0 / texture_width;
	texel_height =  1.0 / texture_height;

	for (i = 0; i < kernel_size; i++)
	{
		for (j = 0; j < kernel_size; j++)
		{
			offsets[(((i * kernel_size)+j)*2)+0] =
					((GLfloat)(i - 1)) * texel_width;

			offsets[(((i * kernel_size)+j)*2)+1] =
					((GLfloat)(j - 1) * texel_height);
		}
	}
}

//----------------------------------------------------------------------------

void V4L::setCtrl(std::string ctrl_label, float val)
{
	if(device)
		device->setCtrl(ctrl_label, val);
}

//----------------------------------------------------------------------------

unsigned int V4L::getWidth()
{
	if (device->decodeOnGpu)
		return dst_width;
	else
		return device->getWidth();
}

//----------------------------------------------------------------------------

unsigned int V4L::getHeight()
{
	if (device->decodeOnGpu)
		return dst_height;
	else
		return device->getHeight();
}

//----------------------------------------------------------------------------

GLuint V4L::getActTexId()
{
	if(device->decodeOnGpu && (sourceparams.encoding != RGB && sourceparams.encoding != RGB32))
		return ppFbo->getSrcTexId();
	else
		return texIDs[0];
}

//----------------------------------------------------------------------------

uint8_t* V4L::getActBuf()
{
	if(device->decodeOnGpu)
		return downloadFrame();
	else
		return device->get_act_decode_buf();
}

//----------------------------------------------------------------------------

uint8_t* V4L::downloadFrame()
{
	uint8_t* out=0;

	if (isInited && frameNr != lastUploadNr)
		loadFrameToTexture();


	if ( downloadNr != lastUploadNr )
	{
		downloadNr = lastUploadNr;
		downloadBufReadPtr = (downloadBufReadPtr +1) % nrDownloadBuffers;

		// bind fbo to read
		ppFbo->src->bind();
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[dx]);

		if (num_downloads < num_pbos)
		{
			// First we need to make sure all our pbos are bound, so glMap/Unmap will
			// read from the oldest bound buffer first.
			glReadPixels(0, 0, ppFbo->src->getWidth(), ppFbo->src->getHeight(),
					glDownloadFmt, GL_UNSIGNED_BYTE, 0);  // When a GL_PIXEL_PACK_BUFFER is bound,																						// the last 0 is used as offset into the buffer to read into.
			num_downloads++;
		} else
		{
			// Read from the oldest bound pbo.
			ptr = (unsigned char*) glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
			if (ptr != NULL)
			{
				memcpy(downloadBuffers[downloadBufReadPtr], ptr, nDownloadBytes);
				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
				out = downloadBuffers[downloadBufReadPtr];
			} else
				std::cerr << "V4L::downloadFrame Failed to map the buffer" << std::endl;

			// Trigger the next read.
			glReadPixels(0, 0, ppFbo->src->getWidth(), ppFbo->src->getHeight(),
					glDownloadFmt, GL_UNSIGNED_BYTE, 0);
		}

		dx = ++dx % num_pbos;
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		ppFbo->src->unbind();

	} else
	{
		// if there are no new frames and the pbos were filled, return the last downloaded Frame
		return downloadBuffers[downloadBufReadPtr];
	}

	return out;
}

//----------------------------------------------------------------------------

int V4L::getActFrameNr()
{
	return frameNr;
}

//----------------------------------------------------------------------------

V4L::~V4L()
{
	// Free the RGB image
	for (int i=0; i<nrBufferFrames; i++)
	{
		delete [] buffer[i];
	}

	for (unsigned int i=0; i<nrDownloadBuffers; ++i)
		if (downloadBuffers[i])
			delete [] downloadBuffers[i];

	delete [] downloadBuffers;
}

}
