/*
 * FFMpegDecode.cpp
 *
 *  Created on: 29.01.2018
 *      Author: sven
 */

#include "FFMpegDecode.h"

#define STRINGIFY(A) #A

using namespace std;
using namespace tav;

FFMpegDecode* FFMpegDecode::this_dec;
enum AVPixelFormat hw_pix_fmt;

static enum AVPixelFormat find_fmt_by_hw_type(const enum AVHWDeviceType type)
{
	enum AVPixelFormat fmt;

	switch (type) {
	case AV_HWDEVICE_TYPE_VAAPI:
		fmt = AV_PIX_FMT_VAAPI;
		printf(" got hardware format AV_PIX_FMT_VAAPI \n");
		break;
	case AV_HWDEVICE_TYPE_DXVA2:
		fmt = AV_PIX_FMT_DXVA2_VLD;
		printf(" got hardware format AV_PIX_FMT_DXVA2_VLD \n");
		break;
	case AV_HWDEVICE_TYPE_D3D11VA:
		fmt = AV_PIX_FMT_D3D11;
		printf(" got hardware format AV_PIX_FMT_D3D11 \n");
		break;
	case AV_HWDEVICE_TYPE_VDPAU:
		fmt = AV_PIX_FMT_VDPAU;
		printf(" got hardware format AV_PIX_FMT_VDPAU \n");
		break;
	case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
		fmt = AV_PIX_FMT_VIDEOTOOLBOX;
		printf(" got hardware format AV_PIX_FMT_VIDEOTOOLBOX \n");
		break;
	default:
		fmt = AV_PIX_FMT_NONE;
		break;
	}

	return fmt;
}

//------------------------------------------------------------------------------------

int FFMpegDecode::hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
			NULL, NULL, 0)) < 0) {
		fprintf(stderr, "Failed to create specified HW device.\n");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return err;
}

//------------------------------------------------------------------------------------

static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
		const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
		{
		//	printf("got hw_pix_format: %d \n", *p);
			return *p;
		}
	}

	//return (AVPixelFormat)46;

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

//------------------------------------------------------------------------------------

//void _stdcall LogCallbackShim(void *ptr, int level, const char *fmt, va_list vargs)
void LogCallbackShim(void *ptr, int level, const char *fmt, va_list vargs)
{
	FFMpegDecode::this_dec->log_callback(ptr, level, fmt, vargs);
}

//------------------------------------------------------------------------

FFMpegDecode::FFMpegDecode() : pFormatContext(NULL), destWidth(0), logLevel(AV_LOG_INFO),
		showLogging(true), showDecTimeInt(80), swsScaleTime(0.0), showDecIt(0), frameNr(-1),
		frameBufferSize(1), intBufFull(false), ready(false), nrPboBufs(3), pCodecContext(NULL),
		eps_zero(0.000025), nrBufferedFrames(0), firstFramePresented(false), loop(true),
		pboIndex(0), img_convert_ctx(NULL), is_net_stream(false), pPacket(NULL),
		gl_res_inited(false), textures(NULL), bgraFrame(NULL), pFrame(NULL), shader(NULL),
		lastPtss(-1.0), endCb(NULL),
		usePbos(false) // memory if useing pbos leak....
{}

//------------------------------------------------------------------------

int FFMpegDecode::OpenFile(ShaderCollector* _shCol, std::string _filePath, int _useNrThreads,
		int _destWidth, int _destHeight, bool _useHwAccel, bool _decodeYuv420OnGpu)
{
	int ret;
	ready=false;
	filePath = _filePath;
	//texIDs = _texIds;
	destWidth = _destWidth;
	destHeight = _destHeight;
	destPixFmt = AV_PIX_FMT_BGRA;
	this_dec = this;
	decodeYuv420OnGpu = _decodeYuv420OnGpu;
	shCol = _shCol;
	useHwAccel = _useHwAccel;

	// Initialize the lavf/lavc library and register all the formats, codecs and protocols.
	// http://ffmpeg.org/doxygen/trunk/group__lavf__core.html#ga917265caec45ef5a0646356ed1a507e3
	av_register_all();
	avformat_network_init();
	//av_log_set_level(AV_LOG_DEBUG);
	av_log_set_level(AV_LOG_INFO);

	if (useHwAccel)
	{
		type = av_hwdevice_find_type_by_name("vdpau");
		hw_pix_fmt = find_fmt_by_hw_type(type);
		if (hw_pix_fmt == -1) {
			fprintf(stderr, "Cannot support vdpau in this example.\n");
			return -1;
		}
	}

	//av_log_set_callback( &LogCallbackShim );	// custom logging

	// AVFormatContext holds the header information from the format (Container)
	// Allocating memory for this component
	// http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
	pFormatContext = avformat_alloc_context();
	if (!pFormatContext) {
		//printf("ERROR could not allocate memory for Format Context\n");
		return -1;
	}

	AVDictionary *d = NULL;
	av_dict_parse_string(&d, "", ":", ",", 0);

	if(_filePath.substr(0,6) == "mms://" || _filePath.substr(0,7) == "mmsh://" || _filePath.substr(0,7) == "mmst://" || _filePath.substr(0,7) == "mmsu://" ||
			_filePath.substr(0,7) == "http://" || _filePath.substr(0,8) == "https://" ||
			_filePath.substr(0,7) == "rtmp://" || _filePath.substr(0,6) == "udp://" ||
			_filePath.substr(0,7) == "rtsp://" || _filePath.substr(0,6) == "rtp://" ||
			_filePath.substr(0,6) == "ftp://" || _filePath.substr(0,7) == "sftp://" ||
			_filePath.substr(0,6) == "tcp://" || _filePath.substr(0,7) == "unix://" ||
			_filePath.substr(0,6) == "smb://")
	{
		av_dict_set(&d, "rtsp_transport", "tcp", 0);
		//av_dict_set(&d, "max_delay", "500000", 0);

		is_net_stream = true;
	}

	if (avformat_open_input(&pFormatContext, filePath.c_str(), NULL, &d) != 0) {
		//printf("ERROR could not open the file\n");
		return -1;
	}

	// read Packets from the Format to get stream information
	// this function populates pFormatContext->streams
	// (of size equals to pFormatContext->nb_streams)
	// the arguments are: the AVFormatContext and options contains options for codec corresponding to i-th stream.
	// On return each dictionary will be filled with options that were not found.
	// https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
	if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
		printf("FFMpegDecode ERROR could not get the stream info\n");
		return -1;
	}


	av_dump_format(pFormatContext, 0, filePath.c_str(), 0);

	// the component that knows how to enCOde and DECode the stream
	// it's the codec (audio or video)
	// http://ffmpeg.org/doxygen/trunk/structAVCodec.html
	pCodec = NULL;

	// this component describes the properties of a codec used by the stream i
	// https://ffmpeg.org/doxygen/trunk/structAVCodecParameters.html
	pCodecParameters =  NULL;
	video_stream_index = -1;

	// loop though all the streams and print its main information
	for (unsigned int i = 0; i < pFormatContext->nb_streams; i++)
	{
		AVCodecParameters *pLocalCodecParameters =  NULL;
		pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

		/*
		printf(" pFormatContext->streams[i]->codecpar %p \n",  pFormatContext->streams[i]->codecpar);
		printf("AVStream->time_base before open coded %d/%d \n", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
		printf("AVStream->r_frame_rate before open coded %d/%d \n", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
		printf("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
		printf("\n");
		printf(" AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);
		printf("\n");
		 */

		vTimeBaseDiv = (double)pFormatContext->streams[0]->time_base.num / (double)pFormatContext->streams[0]->time_base.den;
		vFrameDur = (double)pFormatContext->streams[0]->r_frame_rate.den / (double)pFormatContext->streams[0]->r_frame_rate.num;

		AVCodec *pLocalCodec = NULL;

		// finds the registered decoder for a codec ID
		// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
		pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

		if (pLocalCodec==NULL) {
			printf("ERROR unsupported codec!\n");
			//return -1;
		} else
		{
			if (pLocalCodec->pix_fmts && pLocalCodec->pix_fmts[0] != -1){
				int ind = 0;
				while(pLocalCodec->pix_fmts[ind] != -1) {
					//printf("CODEC possible pix_fmts: %d \n", pLocalCodec->pix_fmts[ind]);
					ind++;
				}
			} else {
				//printf("Warning: CODEC could not get pix_fmts\n");
			}

			// when the stream is a video we store its index, codec parameters and codec
			if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				video_stream_index = i;
				pCodec = pLocalCodec;
				pCodecParameters = pLocalCodecParameters;

				//printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
			} else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
				//printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
			}

			// print its name, id and bitrate
			//printf("\tCodec %s ID %d bit_rate %ld", pLocalCodec->long_name, pLocalCodec->id, pCodecParameters->bit_rate);
			//printf("\n\n");
		}
	}


	// https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
	pCodecContext = avcodec_alloc_context3(pCodec);
	if (!pCodecContext)
	{
		//printf("failed to allocated memory for AVCodecContext\n");
		return -1;
	}

	// set number of threads here
	if (!useHwAccel)
		pCodecContext->thread_count = _useNrThreads;

	// Fill the codec context based on the values from the supplied codec parameters
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
	if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
	{
		//printf("failed to copy codec params to codec context\n");
		return -1;
	}


	if (useHwAccel)
	{
		pCodecContext->get_format  = get_hw_format;
		av_opt_set_int(pCodecContext, "refcounted_frames", 1, 0);	// what does this do?

		if (hw_decoder_init(pCodecContext, type) < 0){
			return -1;
		} else {
			//printf("hw_decoder_init successfull!! \n");
		}
	}

	// save basic codec parameters for access from outside
	srcPixFmt = pCodecContext->pix_fmt;
	srcWidth =  pCodecContext->width;
	srcHeight = pCodecContext->height;

	if (decodeYuv420OnGpu) {
		destWidth = srcWidth;
		destHeight = srcHeight;
	}

	// Initialize the AVCodecContext to use the given AVCodec.
	// https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
	if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
	{
		//printf("failed to open codec through avcodec_open2\n");
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVPacket.html
	pPacket = av_packet_alloc();
	if (!pPacket)
	{
		//printf("failed to allocated memory for AVPacket\n");
		return -1;
	}

	// https://ffmpeg.org/doxygen/trunk/structAVFrame.html
	pFrame = new AVFrame*[frameBufferSize];
	for (int i=0;i<frameBufferSize;i++)
	{
		pFrame[i] = av_frame_alloc();
		if (!pFrame[i])
		{
			printf("failed to allocated memory for AVFrame\n");
			return -1;
		}
	}

	frame = av_frame_alloc();

	if(!decodeYuv420OnGpu)
	{
		buffer = new uint8_t*[frameBufferSize];
		bgraFrame = new AVFrame*[frameBufferSize];
		for (int i=0;i<frameBufferSize;i++)
			bgraFrame[i] = alloc_picture(destPixFmt, destWidth, destHeight, buffer[i]);
	}

	// destFmt BGRA
	if (usePbos)
	{
		pbos = new GLuint[nrPboBufs];
		glGenBuffers(nrPboBufs, pbos);
		for (int i=0;i<nrPboBufs;i++)
		{
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[i]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, destWidth * destHeight * 4, NULL, GL_STREAM_DRAW);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		}
	}

	ptss = new double[frameBufferSize];
	fBlocked = new bool[frameBufferSize];
	for (int i=0;i<frameBufferSize;i++)
	{
		ptss[i] = -1.0;
		fBlocked[i] = false;
	}

	response = 0;
	totNumFrames = get_total_frames();
	ready = true;

	return 0;
}

//----------------------------------------------------

void FFMpegDecode::initShader(AVPixelFormat _srcPixFmt)
{
    if(decodeYuv420OnGpu)
    {
        std::string shdr_Header = shCol->getShaderHeader();

        std::string vert = STRINGIFY(
                                     layout( location = 0 ) in vec4 position;\n
                                     layout( location = 2 ) in vec2 texCoord;\n
                                     uniform mat4 m_pvm;\n
                                     out vec2 tex_coord;\n
                                     void main(){\n
                                         tex_coord = texCoord;\n
                                         gl_Position = m_pvm * position;\n
                                     });
        vert = "// yuv420 texture shader, vert\n" + shdr_Header + vert;

        std::string frag = STRINGIFY(
                                     // YUV420 is a planar (non-packed) format.
                                     // the first plane is the Y with one byte per pixel.
                                     // the second plane us U with one byte for each 2x2 square of pixels
                                     // the third plane is V with one byte for each 2x2 square of pixels
                                     //
                                     // tex_unit - contains the Y (luminance) component of the
                                     //    image. this is a texture unit set up by the OpenGL program.
                                     // u_texture_unit, v_texture_unit - contain the chrominance parts of
                                     //    the image. also texture units  set up by the OpenGL program.
                                     uniform sampler2D tex_unit;\n // Y component
                                     uniform sampler2D u_tex_unit;\n // U component
                                     uniform sampler2D v_tex_unit;\n // V component
                                     uniform float alpha;\n // V component
                                     \n
                                     in vec2 tex_coord;\n
                                     layout (location = 0) out vec4 fragColor;\n
                                     \n
                                     void main(void)\n
                                     {\n);

		 // NV12
		 if(_srcPixFmt == AV_PIX_FMT_NV12)
			 frag += STRINGIFY(
							   float y = texture(tex_unit, tex_coord).r; \n
							   float u = texture(u_tex_unit, tex_coord).r - 0.5; \n
							   float v = texture(u_tex_unit, tex_coord).g - 0.5; \n

							   fragColor = vec4((vec3(y + 1.4021 * v, \n
													y - 0.34482 * u - 0.71405 * v, \n
													y + 1.7713 * u) - 0.05) * 1.07, \n
												alpha); \n
							   );
		 else
			 // YUV420P
			 frag += STRINGIFY(
							   float y = texture(tex_unit, tex_coord).r; \n
							   float u = texture(u_tex_unit, tex_coord).r - 0.5; \n
							   float v = texture(v_tex_unit, tex_coord).r - 0.5; \n

							   float r = y + 1.402 * v; \n
							   float g = y - 0.344 * u - 0.714 * v; \n
							   float b = y + 1.772 * u; \n

							   fragColor = vec4(vec3(r,g,b),alpha); \n);

        frag += "}";
        frag = "// YUV420 fragment shader\n"+shdr_Header+frag;

        shader = shCol->addCheckShaderText("FFMpegDecode_yuv", vert.c_str(), frag.c_str());

    } else
    {
        shader = shCol->getStdTexAlpha();
    }
}

//------------------------------------------------------------------------

void FFMpegDecode::shaderBegin()
{
	//cout << "textures: " << textures << endl;

	if(run && shader && textures)
	{
		shader->begin();
		shader->setIdentMatrix4fv("m_pvm");

		if (decodeYuv420OnGpu)
		{
			shader->setUniform1f("alpha", 1.f); // y
			shader->setUniform1i("tex_unit", 0); // y
			shader->setUniform1i("u_tex_unit", 1); // u
			shader->setUniform1i("v_tex_unit", 2); // v

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0].getId()); // y

			if (srcPixFmt == AV_PIX_FMT_YUV420P || srcPixFmt == AV_PIX_FMT_NV12)
			{
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, textures[1].getId()); // u
			}

			if (srcPixFmt == AV_PIX_FMT_YUV420P)
			{
				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, textures[2].getId()); // v
			}
		} else
		{
			shader->setUniform1i("tex", 0); // y

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textures[0].getId()); // y
		}
	}
}

//------------------------------------------------------------------------

void FFMpegDecode::shaderEnd()
{
	if(run && shader && decodeYuv420OnGpu)
	{
		shader->end();
	}
}

//------------------------------------------------------------------------

void FFMpegDecode::start()
{
	startTime = glfwGetTime();
	run = true;
	decodeThreadEnded = false;
	decodeThread = std::thread(&FFMpegDecode::singleThreadDecodeLoop, this);
	decodeThread.detach();
}

//------------------------------------------------------------------------

void FFMpegDecode::stop()
{
	//std::unique_lock<std::mutex> lock(endThread_mutex);

	run = false;
	decodeCond.notify_all();	 // wait until the packet was needed

	while (!decodeThreadEnded)
	{
		std::cout << "waiting for thread end, endThreadCond.wait(lock)"  << std::endl;
		//endThreadCond.wait(lock);
		//lock.unlock();
		usleep(100);
	}

	std::cout << "stop done" << std::endl;

}

//-----------------------------------------------------

void FFMpegDecode::alloc_gl_res(AVPixelFormat _srcPixFmt)
{
	initShader(_srcPixFmt);

	if(decodeYuv420OnGpu)
		nrTexBuffers = _srcPixFmt == AV_PIX_FMT_NV12 ? 2 : 3;
	else
		nrTexBuffers = 1;

	textures = new TextureManager[nrTexBuffers];

	if(decodeYuv420OnGpu)
	{
		if (srcPixFmt == AV_PIX_FMT_NV12)
		{
			textures[0].allocate(srcWidth, srcHeight, GL_R8, GL_RED, GL_TEXTURE_2D);
			textures[1].allocate(srcWidth/2, srcHeight/2, GL_RG8, GL_RG, GL_TEXTURE_2D);

		} else
		{   // YUV420P
			textures[0].allocate(srcWidth, srcHeight, GL_R8, GL_RED, GL_TEXTURE_2D);
			textures[1].allocate(srcWidth/2, srcHeight/2, GL_R8, GL_RED, GL_TEXTURE_2D);
			textures[2].allocate(srcWidth/2, srcHeight/2, GL_R8, GL_RED, GL_TEXTURE_2D);
		}

	} else {
		textures[0].allocate(destWidth, destHeight, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D);
	}
}

//------------------------------------------------------------------------

AVFrame* FFMpegDecode::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height,
		uint8_t* buf)
{
	AVFrame* picture;

	picture = av_frame_alloc();
	if (!picture) return NULL;

	picture->format = pix_fmt;
	picture->width  = width;
	picture->height = height;

	//Allocate memory for the raw data we get when converting.
	int numBytes = av_image_get_buffer_size(destPixFmt, width, height, 1);
	buf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));


	// Assign appropriate parts of buffer to image planes in pFrameRGB
	av_image_fill_arrays (picture->data, picture->linesize, buf, destPixFmt, width, height, 1);

	//av_free(buffer);

	return picture;
}

//------------------------------------------------------------------------

void FFMpegDecode::singleThreadDecodeLoop()
{
	int response;

	// decode packet
	while (run)
	{
		if(pPacket && av_read_frame(pFormatContext, pPacket) >= 0)
		{
			// if it's the video stream and the buffer queue is not filled
			if (pPacket->stream_index == video_stream_index
					&& nrBufferedFrames < frameBufferSize)
			{
				// we are using multiple frames, so the frames reaching here are not
				// in a continous order!!!!!!
				unsigned int actFrameNr = (unsigned int)(double)pPacket->pts * vTimeBaseDiv / vFrameDur;

				if ((totNumFrames -1) == actFrameNr)
				{
					if (loop && !is_net_stream){
						av_seek_frame(pFormatContext, video_stream_index, 0, AVSEEK_FLAG_BACKWARD);
						//avcodec_flush_buffers(pFormatContext->streams[video_stream_index]->codec);
					}
				}

				response = decode_packet(pPacket, pCodecContext);

				if (response < 0)
					break;
			}

			// https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
			av_packet_unref(pPacket);

		} else
		{
			usleep(1000);
		}
	}

	std::cout << "while loop quit sending endThreadCond.notify_all()" << std::endl;

	decodeThreadEnded = true;
	//endThreadCond.notify_all();	 // wait until the packet was needed
}

//------------------------------------------------------------------------

int FFMpegDecode::decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext)
{
	uint8_t *buffer = NULL;
	int size;

	std::unique_lock<std::mutex> lock(pp_mutex);

	// Supply raw packet data as input to a decoder
	// https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
	int response = avcodec_send_packet(pCodecContext, pPacket);

	if (response < 0) {
		// //printf("Error while sending a packet to the decoder: %s", av_err2str(response));
		return response;
	}

	while (run && response >= 0)
	{
		// check for a free index to write
		int freeFrame = -1;
		while (run && freeFrame < frameBufferSize)
		{
			freeFrame++;
			if (!fBlocked[freeFrame])
				break;
		}

		if(freeFrame == frameBufferSize)
		{
			decodeCond.wait(lock);	 // wait until the a  frame was consumed and we have free space
			lock.unlock();
		}

		if (run && freeFrame != -1 && freeFrame != frameBufferSize)
		{
			if(useHwAccel)
			{
				response = avcodec_receive_frame(pCodecContext, frame);			// always calls av_frame_unref
			} else
			{
				mutex.lock();
				response = avcodec_receive_frame(pCodecContext, pFrame[freeFrame]);	// always calls av_frame_unref
				mutex.unlock();
			}


			if (response == AVERROR(EAGAIN)) {
				break;
			} else if (response == AVERROR_EOF) {
				//fprintf (stderr, "end of file\n");
			} else if (response < 0) {
				//fprintf(stderr, "Error while receiving a frame from the decoder: %s", av_err2str(response));
				return response;
			}


			// we got a valid packet!!
			if (run && response >= 0)
			{
				if (!decodeYuv420OnGpu)
				{
					// convert frame to desired size and format
					//mutex.lock();
					//conversionWatch.setStart();	// measure conversion time
					mutex.lock();

					if (useHwAccel && frame->format == hw_pix_fmt)
					{
						// retrieve data from GPU to CPU, dst frame must be "clean"
						if ( av_hwframe_transfer_data(pFrame[freeFrame], frame, 0)  < 0) {
							fprintf(stderr, "Error transferring the data to system memory\n");
						}

						pFrame[freeFrame]->pts = frame->pts;
						pFrame[freeFrame]->pkt_size = frame->pkt_size;
						pFrame[freeFrame]->coded_picture_number = frame->coded_picture_number;
						pFrame[freeFrame]->pict_type = frame->pict_type;
					}

                    // since now for the first time we are really sure about the pix_fmt the decode
                    // frame will have, initialize the textures and the swscale context if necessary
                    if (!img_convert_ctx && !decodeYuv420OnGpu)
                            img_convert_ctx = sws_getCachedContext(img_convert_ctx,
                                                                   pCodecContext->width, pCodecContext->height,
                                                                   (AVPixelFormat)pFrame[freeFrame]->format,
                                                                   destWidth, destHeight, destPixFmt,
                                                                   SWS_FAST_BILINEAR, //SWS_BICUBIC,
                                                                   NULL, NULL, NULL);

					response = sws_scale(img_convert_ctx,
								pFrame[freeFrame]->data, pFrame[freeFrame]->linesize, 0, pCodecContext->height,
								bgraFrame[freeFrame]->data, bgraFrame[freeFrame]->linesize);


					if (response < 0) fprintf(stderr, "FFMpegDecode ERROR, sws_scale failed!!!\n");

					mutex.unlock();

					//conversionWatch.setEnd();	// measure conversion time
					//conversionWatch.print((char*)"conversion ");	// measure conversion time
					//mutex.unlock();

					//for (int i=0; i<8; i++)
					//	printf("linesize[%d]: %d\n", i, pFrame[freeFrame]->linesize[i]);

				} else
				{
					if (useHwAccel && frame->format == hw_pix_fmt)
					{
						mutex.lock();

						// retrieve data from GPU to CPU, dst frame must be "clean"
						if ( av_hwframe_transfer_data(pFrame[freeFrame], frame, 0)  < 0) {
							fprintf(stderr, "Error transferring the data to system memory\n");
						}

						// not all parameters are copied so, do this manually
						pFrame[freeFrame]->pts = frame->pts;
						pFrame[freeFrame]->pkt_size = frame->pkt_size;
						pFrame[freeFrame]->coded_picture_number = frame->coded_picture_number;
						pFrame[freeFrame]->pict_type = frame->pict_type;

                        srcPixFmt = (AVPixelFormat)pFrame[freeFrame]->format;

						mutex.unlock();

					} else
					{
						srcPixFmt = (AVPixelFormat)pCodecContext->pix_fmt;
					}
				}

				mutex.lock();

/*
			printf("--- [%d] Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d] \n",
					freeFrame,
					pCodecContext->frame_number,
					av_get_picture_type_char(pFrame[freeFrame]->pict_type),
					pFrame[freeFrame]->pkt_size,
					pFrame[freeFrame]->pts,
					pFrame[freeFrame]->key_frame,
					pFrame[freeFrame]->coded_picture_number
			);
*/

				ptss[freeFrame] = vTimeBaseDiv * (double)pFrame[freeFrame]->pts;
				fBlocked[freeFrame] = true;
				nrBufferedFrames++;

				// call end callback if we are done
				if (endCb && ptss[freeFrame] < lastPtss && lastPtss > 0)
				{
					cout << "reached end " << endl;
					endCb();
				}

				lastPtss = ptss[freeFrame];

				mutex.unlock();

				if(run && nrBufferedFrames == frameBufferSize){
					decodeCond.wait(lock);	 // wait until the a  frame was consumed and we have free space
					lock.unlock();
				}
			}
		}
	}

	return 0;
}

//------------------------------------------------------------------------

void FFMpegDecode::loadFrameToTexture(double time)
{
	if (ready && run && nrBufferedFrames > 0)
	{
		int frameToUpload = -1;
		double actRelTime = time - startTime;
		unsigned int searchInd = 0;
		double highestPts = 0.0;

		if (!gl_res_inited)
		{
			alloc_gl_res(srcPixFmt);
			gl_res_inited = true;
		}


		// check for the first frame or a frame with a pts close to the actual time
		while (searchInd < frameBufferSize)
		{
			if (!firstFramePresented)
			{
				//printf("ptss[%d]: %f\n", searchInd % frameBufferSize, ptss[searchInd % frameBufferSize]);
				if ((ptss[searchInd % frameBufferSize] == 0.0) || is_net_stream)
				{
					firstFramePresented = true;
					startTime = time;
					frameToUpload = searchInd % frameBufferSize;
					break;
				}
			} else
			{
				//printf("ptss[%d]: %f actRelTime: %f\n", searchInd % frameBufferSize, ptss[searchInd % frameBufferSize], actRelTime);

				// if we have a timestamp with a pts that differs less than 10% of the a frame duration, present it
				if (std::fabs(ptss[searchInd % frameBufferSize] - actRelTime) < (vFrameDur * 0.25)
						|| (ptss[searchInd % frameBufferSize] != -1.0
								&& ptss[searchInd % frameBufferSize] < actRelTime))
				{
					// if we are playing in loop mode and reached the last frame, reset the startTime
					if ((unsigned int)(ptss[searchInd % frameBufferSize] / vFrameDur) == (totNumFrames -1)){
						startTime = time;
					}

					frameToUpload = searchInd % frameBufferSize;
					break;
				}
			}

			// check if there are frames that are too old
			searchInd++;
		}


		if (frameToUpload != -1)
		{
			mutex.lock();
			//uplWatch.setStart();

			if (decodeYuv420OnGpu)
			{
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				if (AV_PIX_FMT_NV12 == srcPixFmt)
				{
					// UV interleaved
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, textures[1].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,	srcWidth/2, srcHeight/2,
							GL_RG, GL_UNSIGNED_BYTE, pFrame[frameToUpload]->data[1]);

					// Y
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textures[0].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,	srcWidth, srcHeight,
							texture_pixel_format(srcPixFmt), GL_UNSIGNED_BYTE, pFrame[frameToUpload]->data[0]);
				}

				// if the video is encoded as YUV420 it's in three separate areas in
				// memory (planar-- not interlaced). so if we have three areas of
				// data, set up one texture unit for each one. in the if statement
				// we'll set up the texture units for chrominance (U & V) and we'll
				// put the luminance (Y) data in GL_TEXTURE0 after the if.

				if (AV_PIX_FMT_YUV420P == srcPixFmt)
				{
					// luminace values, whole picture
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textures[0].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, srcWidth, srcHeight,
							texture_pixel_format(srcPixFmt), GL_UNSIGNED_BYTE,
							pFrame[frameToUpload]->data[0]);

					int chroma_width = srcWidth / 2;
					int chroma_height = srcHeight / 2;

					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, textures[1].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,	chroma_width, chroma_height,
							texture_pixel_format(srcPixFmt), GL_UNSIGNED_BYTE,
							pFrame[frameToUpload]->data[1]);


					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, textures[2].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, chroma_width, chroma_height,
							texture_pixel_format(srcPixFmt), GL_UNSIGNED_BYTE,
							pFrame[frameToUpload]->data[2]);
				}

			} else
			{
				if (usePbos)
				{
					pboIndex = (pboIndex + 1) % nrPboBufs;
					unsigned int nextIndex = (pboIndex + 1) % nrPboBufs;

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D,  textures[0].getId());
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[pboIndex]);

					glTexSubImage2D(GL_TEXTURE_2D,      // target
							0,                      // First mipmap level
							0, 0,                   // x and y offset
							destWidth,              // width and height
							destHeight,
							GL_BGRA,
							GL_UNSIGNED_BYTE,
							0);


					// bind PBO to update texture source
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[nextIndex]);

					// Note that glMapBufferARB() causes sync issue.
					// If GPU is working with this buffer, glMapBufferARB() will wait(stall)
					// until GPU to finish its job. To avoid waiting (idle), you can call
					// first glBufferDataARB() with NULL pointer before glMapBufferARB().
					// If you do that, the previous data in PBO will be discarded and
					// glMapBufferARB() returns a new allocated pointer immediately
					// even if GPU is still working with the previous data.
					glBufferData(GL_PIXEL_UNPACK_BUFFER, destWidth * destHeight * 4, 0, GL_STREAM_DRAW);

					// map the buffer object into client's memory
					GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

					if(ptr)
					{
						// update data directly on the mapped buffer
						memcpy(ptr, bgraFrame[frameToUpload]->data[0], destWidth * destHeight * 4);
						glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
					}

					// it is good idea to release PBOs with ID 0 after use.
					// Once bound with 0, all pixel operations are back to normal ways.
					glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

				} else
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textures[0].getId());
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, destWidth, destHeight,
							GL_BGRA, GL_UNSIGNED_BYTE,	bgraFrame[frameToUpload]->data[0]);
				}

				//decodeCond.notify_all();	 // wait until the packet was needed
			}

			fBlocked[frameToUpload] = false;
			nrBufferedFrames--;
			decodeCond.notify_all();	 // wait until the packet was needed

			//uplWatch.setEnd();
			//uplWatch.print("upl: ");

			mutex.unlock();
		}
	}
}

//------------------------------------------------------------------------

GLenum FFMpegDecode::texture_pixel_format(AVPixelFormat srcFmt)
{
	int format;

	switch (srcFmt)
	{
	//	case LUMA:
	// greyscale, 1 byte/pixel
	//		format = GL_RED;
	//		break;
	case AV_PIX_FMT_YUV420P:
		// color, 1.5 bytes/pixel, call it 1 for the luminance *Y)
		// part. (we'll generate other textures for the U & V parts)
		// the shader will turn it into color
		format =  GL_RED;
		break;
	case AV_PIX_FMT_NV12:
		format =  GL_RED;
		break;
		/*	case YUV422:
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
		 */
	case AV_PIX_FMT_RGB24:
		// color, 3 bytes/pixel
		format = GL_BGR;
		break;
		/*
case  RGB32:
	// color, 4 bytes/pixel
	format = GL_RG;
	break;
		 */
	default:
		break;
	}

	return(format);
}

//------------------------------------------------------------------------

void FFMpegDecode::seek_frame(int64_t _frame_number, double time)
{
	/**
	 * Seek to the keyframe at timestamp.
	 * 'timestamp' in 'stream_index'.
	 *
	 * @param s media file handle
	 * @param stream_index If stream_index is (-1), a default
	 * stream is selected, and timestamp is automatically converted
	 * from AV_TIME_BASE units to the stream specific time_base.
	 * @param timestamp Timestamp in AVStream.time_base units
	 *        or, if no stream is specified, in AV_TIME_BASE units.
	 * @param flags flags which select direction and seeking mode
	 * @return >= 0 on success
	 */

	// dauert manchmal lange... geht wohl nicht besser
	startTime = time;

	av_seek_frame(pFormatContext, video_stream_index, _frame_number, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(pCodecContext);
}

//------------------------------------------------------------------------

void FFMpegDecode::seek(double sec, double time)
{
	printf("FFMpegDecode seeking to sec: %f", sec);
	seek_frame((int64_t)(sec * get_fps() + 0.5), time);
}

//------------------------------------------------------------------------

void FFMpegDecode::resetToStart(double time)
{
	seek(0.0, time);
}

//------------------------------------------------------------------------------------

double FFMpegDecode::get_duration_sec()
{
	double sec = (double)pFormatContext->duration / (double)AV_TIME_BASE;

	if (sec < eps_zero)
	{
		sec = (double)pFormatContext->streams[video_stream_index]->duration
				* r2d(pFormatContext->streams[video_stream_index]->time_base);
	}

	if (sec < eps_zero)
	{
		sec = (double)pFormatContext->streams[video_stream_index]->duration
				* r2d(pFormatContext->streams[video_stream_index]->time_base);
	}

	return sec;
}

//------------------------------------------------------------------------------------

int64_t FFMpegDecode::get_total_frames()
{
	int64_t nbf = pFormatContext->streams[video_stream_index]->nb_frames;

	if (nbf == 0)
	{
		nbf = (int64_t) std::floor(get_duration_sec() * get_fps() + 0.5);
	}
	return nbf;
}

//------------------------------------------------------------------------------------

double FFMpegDecode::get_fps()
{
	double fps = r2d(pFormatContext->streams[video_stream_index]->avg_frame_rate);

	if (fps < eps_zero)
	{
		fps = r2d(pFormatContext->streams[video_stream_index]->avg_frame_rate);
	}

	//        if (fps < eps_zero)
	//        {
	//            fps = 1.0 / r2d(ic->streams[video_stream]->codec->time_base);
	//        }

	return fps;
}

//------------------------------------------------------------------------------------

double FFMpegDecode::r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

//------------------------------------------------------------------------

void FFMpegDecode::logging(const char *fmt, ...)
{
	/*
if(showLogging)
{
	va_list args;
	f//printf( stderr, "LOG: " );
	va_start( args, fmt );
	vf//printf( stderr, fmt, args );
	va_end( args );
	f//printf( stderr, "\n" );

}
	 */
}

//------------------------------------------------------------------------

void FFMpegDecode::lockMutex()
{
	mutex.lock();
}

//------------------------------------------------------------------------

void FFMpegDecode::unLockMutex()
{
	mutex.unlock();
}

//------------------------------------------------------------------------------------

void FFMpegDecode::log_callback(void *ptr, int level, const char *fmt, va_list vargs)
{
	if (level <= logLevel)
	{
		const char *module = NULL;
		static char	message[8192];

		// Get module name
		if (ptr)
		{
			AVClass *avc = *(AVClass**) ptr;
			module = avc->item_name(ptr);
		}

		// Create the actual message
		sprintf(message, fmt, vargs);
		//		vsn//printf_s(message, sizeof(message), fmt, vargs);
		//printf(message);
	}
}

//------------------------------------------------------------------------

FFMpegDecode::~FFMpegDecode()
{
	//printf("releasing all the resources\n");

	if (pFormatContext){
		avformat_close_input(&pFormatContext);
		avformat_free_context(pFormatContext);
	}

	if(pCodecContext)
		avcodec_free_context(&pCodecContext);

	if (pFrame){
		for (int i=0;i<frameBufferSize;i++)
		{
			av_frame_unref(pFrame[i]);
			av_frame_free(&pFrame[i]);
		}
		delete [] pFrame;
	}

	if(frame){
		av_frame_unref(frame);
		av_frame_free(&frame);
	}


	if(!decodeYuv420OnGpu && bgraFrame)
	{
		for (int i=0;i<frameBufferSize;i++)
		{
			av_frame_free(&bgraFrame[i]);
			delete buffer[i];
		}

		delete [] buffer;
		delete [] bgraFrame;
		//sws_freeContext(img_convert_ctx);
	}

	av_packet_free(&pPacket);

	if (usePbos){
		glDeleteBuffers(nrPboBufs, pbos);
		delete [] pbos;
	}

	if(textures)
	{
		for (int i=0;i<nrTexBuffers;i++)
			textures[i].releaseTexture();
	}

	delete [] textures;
	delete [] ptss;
	delete [] fBlocked;

	if(hw_device_ctx)
		av_buffer_unref(&hw_device_ctx);
	//printf("done\n");

}

