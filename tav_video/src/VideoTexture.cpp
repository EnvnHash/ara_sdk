/*
 * VideoTexture.cpp
 *
 *  Created on: 09.12.2016
 *      Copyright by Sven Hahne
 */

#include "VideoTexture.h"

namespace tav
{
VideoTexture::VideoTexture(char* file, int initFrameRate) :
	nrBufferFrames(15), dst_pix_fmt(AV_PIX_FMT_BGRA)
{
	unsigned i;
	bool valid = false;

	std::cout << "VideoTexture now opening: " << file << std::endl;

	init();

	avformat_network_init();
	av_register_all();	// register all codecs, demux and protocols
	av_log_set_level(AV_LOG_ERROR);

	// register a callback function for synchronization
	//av_lockmgr_register(&LockCallBack);

	// interrupt callback
	interrupt_metadata.timeout_after_ms = LIBAVFORMAT_INTERRUPT_OPEN_TIMEOUT_MS;
	get_monotonic_time(&interrupt_metadata.value);

	ic = avformat_alloc_context();
	ic->interrupt_callback.callback = _opencv_ffmpeg_interrupt_callback;
	ic->interrupt_callback.opaque = &interrupt_metadata;

	av_dict_set(&dict, "rtsp_transport", "tcp", 0);
	int err = avformat_open_input(&ic, file, NULL, &dict);

	if (err < 0)
	{
		std::cerr << ("Error opening file") << std::endl;
		goto exit_func;
	}
	err = avformat_find_stream_info(ic, NULL);

	if (err < 0)
	{
		std::cerr << ("Could not find codec parameters") << std::endl;
		goto exit_func;
	}

	for(i = 0; i < ic->nb_streams; i++)
	{
		AVCodec* codec = avcodec_find_decoder(ic->streams[i]->codecpar->codec_id);
		AVCodecContext* enc = avcodec_alloc_context3(codec);
		//AVCodecContext *enc = ic->streams[i]->codec;
		//enc->thread_count = get_number_of_cpus();

		if( AVMEDIA_TYPE_VIDEO == enc->codec_type && video_stream < 0)
		{
			video_stream = i;
			video_st = ic->streams[i];

			codecCtx = avcodec_alloc_context3(codec);
			codecCtx->thread_count = get_number_of_cpus();

			// backup encoder' width/height
			int enc_width = video_st->codecpar->width;
			int enc_height = video_st->codecpar->height;
			std::cout << "codecCtx size: " << video_st->codecpar->width << ", " << video_st->codecpar->height  << std::endl;

			if (!codec || avcodec_open2(codecCtx, codec, NULL)  < 0)
				goto exit_func;

			// checking width/height (since decoder can sometimes alter it, eg. vp6f)
			if (enc_width && (video_st->codecpar->width != enc_width)) { video_st->codecpar->width = enc_width; }
			if (enc_height && (video_st->codecpar->height != enc_height)) { video_st->codecpar->height = enc_height; }

			break;
		}
	}

	if(video_stream >= 0) valid = true;

	std::cout << "now allocating textures codecCtx->width: " << video_st->codecpar->width << ", " << video_st->codecpar->height  << std::endl;

	texIDs = new GLuint[nrBufferFrames];
	textures = new TextureManager[nrBufferFrames];

	for (int i=0;i<nrBufferFrames;i++)
	{
		texIDs[i] = textures[i].allocate(video_st->codecpar->width, video_st->codecpar->height, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D);
		textures[i].setWraping(GL_CLAMP_TO_BORDER);
	}

	// Allocate an AVFrame structure
	pFrameRGB = new AVFrame*[nrBufferFrames];
	for (int i=0; i<nrBufferFrames; i++)
	{
		pFrameRGB[i] = av_frame_alloc();
		if(pFrameRGB[i] == NULL)
			std::cout << "Codec not allocate frame" << std::endl;
	}

	// Determine required buffer size and allocate buffer
	numBytes = av_image_get_buffer_size(dst_pix_fmt, video_st->codecpar->width,
			video_st->codecpar->height, 1);
	buffer = new uint8_t[numBytes];
	for (int i=0; i<nrBufferFrames; i++)
	{
		// Assign appropriate parts of buffer to image planes in pFrameRGB
		av_image_fill_arrays(pFrameRGB[i]->data, pFrameRGB[i]->linesize, buffer,
				dst_pix_fmt, video_st->codecpar->width, video_st->codecpar->height, 1);
	}

	img_convert_ctx = sws_getContext(
			video_st->codecpar->width, video_st->codecpar->height,
			(AVPixelFormat) video_st->codecpar->format,
			video_st->codecpar->width, video_st->codecpar->height,
			dst_pix_fmt,
			SWS_FAST_BILINEAR, //SWS_BICUBIC,
			NULL, NULL, NULL);

	if (img_convert_ctx == NULL)
		std::cout << "Cannot initialize the conversion context" << std::endl;

exit_func:
	// deactivate interrupt callback
	interrupt_metadata.timeout_after_ms = 0;
	if( !valid )  close();
}

//------------------------------------------------------------------------------------

void VideoTexture::init()
{
	ic = 0;
	video_stream = -1;
	video_st = 0;
	picture_pts = AV_NOPTS_VALUE_;
	first_frame_number = -1;

	rgb_picture = av_frame_alloc();
	picture = av_frame_alloc();

	//memset( &rgb_picture, 0, sizeof(rgb_picture) );
	filename = 0;
	memset(&packet, 0, sizeof(packet));
	av_init_packet(&packet);
	img_convert_ctx = 0;

	avcodec = 0;
	frame_number = 0;
	eps_zero = 0.000025;

	dict = NULL;
}

//------------------------------------------------------------------------------------

void VideoTexture::close()
{
	std::cout << " VideoTexture::close()" << std::endl;

	if( img_convert_ctx )
	{
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = 0;
	}

	if( picture ) av_frame_free(&picture);

	if( video_st )
	{
	//	avcodec_close(avcodec);
		video_st = NULL;
	}

	if( ic )
	{
		avformat_close_input(&ic);
		ic = NULL;
	}

#if USE_AV_FRAME_GET_BUFFER
	av_frame_unref(&rgb_picture);
#else
	if( rgb_picture->data[0] )
	{
		free( rgb_picture->data[0] );
		rgb_picture->data[0] = 0;
	}
#endif

	// free last packet if exist
	if (packet.data) {
		av_packet_unref(&packet);
		packet.data = NULL;
	}

	if (dict != NULL)  av_dict_free(&dict);

	init();
}

//------------------------------------------------------------------------------------

bool VideoTexture::grabFrame()
{
	bool valid = false;
	int got_picture;
	int count_errs = 0;
	const int max_number_of_attempts = 1 << 9;

	if( !ic || !video_st )  return false;

	if( ic->streams[video_stream]->nb_frames > 0 &&
		frame_number > ic->streams[video_stream]->nb_frames )
		return false;

	picture_pts = AV_NOPTS_VALUE_;

	// activate interrupt callback
	get_monotonic_time(&interrupt_metadata.value);
	interrupt_metadata.timeout_after_ms = LIBAVFORMAT_INTERRUPT_READ_TIMEOUT_MS;

	// get the next frame
	while (!valid)
	{
		av_packet_unref(&packet);

		if (interrupt_metadata.timeout)
		{
			valid = false;
			break;
		}

		int ret = av_read_frame(ic, &packet);
		if (ret == AVERROR(EAGAIN)) continue;

		if( packet.stream_index != video_stream )
		{
			av_packet_unref(&packet);
			count_errs++;
			if (count_errs > max_number_of_attempts)
				break;
			continue;
		}

		// Decode video frame
		ret = avcodec_decode_video2(codecCtx, picture, &got_picture, &packet);

		// Did we get a video frame?
		if(got_picture)
		{
			//picture_pts = picture->best_effort_timestamp;
			if( picture_pts == AV_NOPTS_VALUE_ )
				picture_pts = packet.pts != AV_NOPTS_VALUE_ && packet.pts != 0 ? packet.pts : packet.dts;

			readBuffer = actBuffer;

			// Convert the image from its native format to RGB
			sws_scale(img_convert_ctx,
					picture->data, picture->linesize, 0, ic->streams[video_stream]->codecpar->height,
					pFrameRGB[actBuffer]->data, pFrameRGB[actBuffer]->linesize);

			actBuffer = (actBuffer + 1) % nrBufferFrames;

			loadFrameToTexture();

			frame_number++;
			valid = true;
		}
		else
		{
			count_errs++;
			if (count_errs > max_number_of_attempts)
				break;
		}
	}

	if( valid && first_frame_number < 0 )
		first_frame_number = dts_to_frame_number(picture_pts);

	// deactivate interrupt callback
	interrupt_metadata.timeout_after_ms = 0;

	// return if we have a new picture or not
	return valid;
}

//------------------------------------------------------------------------------------

bool VideoTexture::retrieveFrame()
{
	if( !video_st || !picture->data[0] )
		return false;

	if( img_convert_ctx == NULL )
	{
		// Some sws_scale optimizations have some assumptions about alignment of data/step/width/height
		// Also we use coded_width/height to workaround problem with legacy ffmpeg versions (like n0.8)
		int buffer_width = codecCtx->coded_width, buffer_height = codecCtx->coded_height;
		std::cout << "codecCtx->coded_width: " << codecCtx->coded_width << ", " << codecCtx->coded_height << std::endl;

		img_convert_ctx = sws_getCachedContext(
				img_convert_ctx,
				buffer_width, buffer_height,
				(AVPixelFormat) video_st->codecpar->format,
				buffer_width, buffer_height,
				dst_pix_fmt,
				SWS_BICUBIC,
				NULL, NULL, NULL
				);

		if (img_convert_ctx == NULL)
			return false;//CV_Error(0, "Cannot initialize the conversion context!");


		av_frame_unref(rgb_picture);
		rgb_picture->format = dst_pix_fmt;
		rgb_picture->width = buffer_width;
		rgb_picture->height = buffer_height;

		if (av_frame_get_buffer(rgb_picture, 32) != 0)
		{
			std::cerr << ("OutOfMemory") << std::endl;
			return false;
		}
	}


	std::cout << sws_scale(
			img_convert_ctx,
			picture->data,
			picture->linesize,
			0, video_st->codecpar->height,
			rgb_picture->data,
			rgb_picture->linesize
			) << std::endl;

	loadFrameToTexture();

	return true;
}

//------------------------------------------------------------------------------------

void VideoTexture::loadFrameToTexture()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texIDs[0]);
	glTexSubImage2D(GL_TEXTURE_2D,             // target
				0,                          // First mipmap level
				0, 0,                       // x and y offset
				video_st->codecpar->width,              // width and height
				video_st->codecpar->height,
				GL_BGRA,
				GL_UNSIGNED_BYTE,
				pFrameRGB[readBuffer]->data[0]);
}

//------------------------------------------------------------------------------------

double VideoTexture::get_duration_sec()
{
	double sec = (double)ic->duration / (double)AV_TIME_BASE;

	if (sec < eps_zero)
	{
		sec = (double)ic->streams[video_stream]->duration * r2d(ic->streams[video_stream]->time_base);
	}

	if (sec < eps_zero)
	{
		sec = (double)ic->streams[video_stream]->duration * r2d(ic->streams[video_stream]->time_base);
	}

	return sec;
}

//------------------------------------------------------------------------------------

int VideoTexture::get_bitrate()
{
	return ic->bit_rate;
}

//------------------------------------------------------------------------------------

double VideoTexture::get_fps()
{
	double fps = r2d(ic->streams[video_stream]->avg_frame_rate);

	if (fps < eps_zero)
	{
		fps = r2d(ic->streams[video_stream]->avg_frame_rate);
	}

//        if (fps < eps_zero)
//        {
//            fps = 1.0 / r2d(ic->streams[video_stream]->codec->time_base);
//        }

	return fps;
}

//------------------------------------------------------------------------------------

int64_t VideoTexture::get_total_frames()
{
	int64_t nbf = ic->streams[video_stream]->nb_frames;

	if (nbf == 0)
	{
		nbf = (int64_t)floor(get_duration_sec() * get_fps() + 0.5);
	}
	return nbf;
}

//------------------------------------------------------------------------------------

double VideoTexture::r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

//------------------------------------------------------------------------------------

int64_t VideoTexture::dts_to_frame_number(int64_t dts)
{
	double sec = dts_to_sec(dts);
	return (int64_t)(get_fps() * sec + 0.5);
}

//------------------------------------------------------------------------------------

double VideoTexture::dts_to_sec(int64_t dts)
{
	return (double)(dts - ic->streams[video_stream]->start_time) *
		r2d(ic->streams[video_stream]->time_base);
}

//------------------------------------------------------------------------------------

VideoTexture::~VideoTexture()
{
	close();
}
}


/*





void CvCapture_FFMPEG::seek(int64_t _frame_number)
{
    _frame_number = std::min(_frame_number, get_total_frames());
    int delta = 16;

    // if we have not grabbed a single frame before first seek, let's read the first frame
    // and get some valuable information during the process
    if( first_frame_number < 0 && get_total_frames() > 1 )
        grabFrame();

    for(;;)
    {
        int64_t _frame_number_temp = std::max(_frame_number-delta, (int64_t)0);
        double sec = (double)_frame_number_temp / get_fps();
        int64_t time_stamp = ic->streams[video_stream]->start_time;
        double  time_base  = r2d(ic->streams[video_stream]->time_base);
        time_stamp += (int64_t)(sec / time_base + 0.5);
        if (get_total_frames() > 1) av_seek_frame(ic, video_stream, time_stamp, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(ic->streams[video_stream]->codec);
        if( _frame_number > 0 )
        {
            grabFrame();

            if( _frame_number > 1 )
            {
                frame_number = dts_to_frame_number(picture_pts) - first_frame_number;
                //printf("_frame_number = %d, frame_number = %d, delta = %d\n",
                //       (int)_frame_number, (int)frame_number, delta);

                if( frame_number < 0 || frame_number > _frame_number-1 )
                {
                    if( _frame_number_temp == 0 || delta >= INT_MAX/4 )
                        break;
                    delta = delta < 16 ? delta*2 : delta*3/2;
                    continue;
                }
                while( frame_number < _frame_number-1 )
                {
                    if(!grabFrame())
                        break;
                }
                frame_number++;
                break;
            }
            else
            {
                frame_number = 1;
                break;
            }
        }
        else
        {
            frame_number = 0;
            break;
        }
    }
}

void CvCapture_FFMPEG::seek(double sec)
{
    seek((int64_t)(sec * get_fps() + 0.5));
}

bool CvCapture_FFMPEG::setProperty( int property_id, double value )
{
    if( !video_st ) return false;

    switch( property_id )
    {
    case CV_FFMPEG_CAP_PROP_POS_MSEC:
    case CV_FFMPEG_CAP_PROP_POS_FRAMES:
    case CV_FFMPEG_CAP_PROP_POS_AVI_RATIO:
        {
            switch( property_id )
            {
            case CV_FFMPEG_CAP_PROP_POS_FRAMES:
                seek((int64_t)value);
                break;

            case CV_FFMPEG_CAP_PROP_POS_MSEC:
                seek(value/1000.0);
                break;

            case CV_FFMPEG_CAP_PROP_POS_AVI_RATIO:
                seek((int64_t)(value*ic->duration));
                break;
            }

            picture_pts=(int64_t)value;
        }
        break;
    default:
        return false;
    }

    return true;
}

bool OutputMediaStream_FFMPEG::open(const char* fileName, int width, int height, double fps)
{
    fmt_ = 0;
    oc_ = 0;
    video_st_ = 0;

    // auto detect the output format from the name and fourcc code
    #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 2, 0)
        fmt_ = av_guess_format(NULL, fileName, NULL);
    #else
        fmt_ = guess_format(NULL, fileName, NULL);
    #endif
    if (!fmt_)
        return false;

    CV_CODEC_ID codec_id = CV_CODEC(CODEC_ID_H264);

    // alloc memory for context
    #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 2, 0)
        oc_ = avformat_alloc_context();
    #else
        oc_ = av_alloc_format_context();
    #endif
    if (!oc_)
        return false;

    // set some options
    oc_->oformat = fmt_;
    snprintf(oc_->filename, sizeof(oc_->filename), "%s", fileName);

    oc_->max_delay = (int)(0.7 * AV_TIME_BASE); // This reduces buffer underrun warnings with MPEG

    // set a few optimal pixel formats for lossless codecs of interest..
    AVPixelFormat codec_pix_fmt = AV_PIX_FMT_YUV420P;
    int bitrate_scale = 64;

    // TODO -- safe to ignore output audio stream?
    video_st_ = addVideoStream(oc_, codec_id, width, height, width * height * bitrate_scale, fps, codec_pix_fmt);
    if (!video_st_)
        return false;

    // set the output parameters (must be done even if no parameters)
    #if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 2, 0)
        if (av_set_parameters(oc_, NULL) < 0)
            return false;
    #endif

    // now that all the parameters are set, we can open the audio and
    // video codecs and allocate the necessary encode buffers

    #if LIBAVFORMAT_BUILD > 4628
        AVCodecContext* c = (video_st_->codec);
    #else
        AVCodecContext* c = &(video_st_->codec);
    #endif

    c->codec_tag = MKTAG('H', '2', '6', '4');
    c->bit_rate_tolerance = c->bit_rate;

    // open the output file, if needed
    if (!(fmt_->flags & AVFMT_NOFILE))
    {
        #if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 2, 0)
            int err = url_fopen(&oc_->pb, fileName, URL_WRONLY);
        #else
            int err = avio_open(&oc_->pb, fileName, AVIO_FLAG_WRITE);
        #endif

        if (err != 0)
            return false;
    }

    // write the stream header, if any
    #if LIBAVFORMAT_BUILD < CALC_FFMPEG_VERSION(53, 2, 0)
        av_write_header(oc_);
    #else
        avformat_write_header(oc_, NULL);
    #endif

    return true;
}



struct OutputMediaStream_FFMPEG* create_OutputMediaStream_FFMPEG(const char* fileName, int width, int height, double fps)
{
    OutputMediaStream_FFMPEG* stream = (OutputMediaStream_FFMPEG*) malloc(sizeof(OutputMediaStream_FFMPEG));

    if (stream->open(fileName, width, height, fps))
        return stream;

    stream->close();
    free(stream);

    return 0;
}

void release_OutputMediaStream_FFMPEG(struct OutputMediaStream_FFMPEG* stream)
{
    stream->close();
    free(stream);
}

void write_OutputMediaStream_FFMPEG(struct OutputMediaStream_FFMPEG* stream, unsigned char* data, int size, int keyFrame)
{
    stream->write(data, size, keyFrame);
}

//
 * For CUDA decoder


enum
{
    VideoCodec_MPEG1 = 0,
    VideoCodec_MPEG2,
    VideoCodec_MPEG4,
    VideoCodec_VC1,
    VideoCodec_H264,
    VideoCodec_JPEG,
    VideoCodec_H264_SVC,
    VideoCodec_H264_MVC,

    // Uncompressed YUV
    VideoCodec_YUV420 = (('I'<<24)|('Y'<<16)|('U'<<8)|('V')),   // Y,U,V (4:2:0)
    VideoCodec_YV12   = (('Y'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,V,U (4:2:0)
    VideoCodec_NV12   = (('N'<<24)|('V'<<16)|('1'<<8)|('2')),   // Y,UV  (4:2:0)
    VideoCodec_YUYV   = (('Y'<<24)|('U'<<16)|('Y'<<8)|('V')),   // YUYV/YUY2 (4:2:2)
    VideoCodec_UYVY   = (('U'<<24)|('Y'<<16)|('V'<<8)|('Y'))    // UYVY (4:2:2)
};

enum
{
    VideoChromaFormat_Monochrome = 0,
    VideoChromaFormat_YUV420,
    VideoChromaFormat_YUV422,
    VideoChromaFormat_YUV444
};

struct InputMediaStream_FFMPEG
{
public:
    bool open(const char* fileName, int* codec, int* chroma_format, int* width, int* height);
    void close();

    bool read(unsigned char** data, int* size, int* endOfFile);

private:
    InputMediaStream_FFMPEG(const InputMediaStream_FFMPEG&);
    InputMediaStream_FFMPEG& operator =(const InputMediaStream_FFMPEG&);

    AVFormatContext* ctx_;
    int video_stream_id_;
    AVPacket pkt_;

#if USE_AV_INTERRUPT_CALLBACK
    AVInterruptCallbackMetadata interrupt_metadata;
#endif
};

bool InputMediaStream_FFMPEG::open(const char* fileName, int* codec, int* chroma_format, int* width, int* height)
{
    int err;

    ctx_ = 0;
    video_stream_id_ = -1;
    memset(&pkt_, 0, sizeof(AVPacket));

#if USE_AV_INTERRUPT_CALLBACK
    // interrupt callback
    interrupt_metadata.timeout_after_ms = LIBAVFORMAT_INTERRUPT_OPEN_TIMEOUT_MS;
    get_monotonic_time(&interrupt_metadata.value);

    ctx_ = avformat_alloc_context();
    ctx_->interrupt_callback.callback = _opencv_ffmpeg_interrupt_callback;
    ctx_->interrupt_callback.opaque = &interrupt_metadata;
#endif

    #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 13, 0)
        avformat_network_init();
    #endif

    #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 6, 0)
        err = avformat_open_input(&ctx_, fileName, 0, 0);
    #else
        err = av_open_input_file(&ctx_, fileName, 0, 0, 0);
    #endif
    if (err < 0)
        return false;

    #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 6, 0)
        err = avformat_find_stream_info(ctx_, 0);
    #else
        err = av_find_stream_info(ctx_);
    #endif
    if (err < 0)
        return false;

    for (unsigned int i = 0; i < ctx_->nb_streams; ++i)
    {
        #if LIBAVFORMAT_BUILD > 4628
            AVCodecContext *enc = ctx_->streams[i]->codec;
        #else
            AVCodecContext *enc = &ctx_->streams[i]->codec;
        #endif

        if (enc->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_id_ = static_cast<int>(i);

            switch (enc->codec_id)
            {
            case CV_CODEC(CODEC_ID_MPEG1VIDEO):
                *codec = ::VideoCodec_MPEG1;
                break;

            case CV_CODEC(CODEC_ID_MPEG2VIDEO):
                *codec = ::VideoCodec_MPEG2;
                break;

            case CV_CODEC(CODEC_ID_MPEG4):
                *codec = ::VideoCodec_MPEG4;
                break;

            case CV_CODEC(CODEC_ID_VC1):
                *codec = ::VideoCodec_VC1;
                break;

            case CV_CODEC(CODEC_ID_H264):
                *codec = ::VideoCodec_H264;
                break;

            default:
                return false;
            };

            switch (enc->pix_fmt)
            {
            case AV_PIX_FMT_YUV420P:
                *chroma_format = ::VideoChromaFormat_YUV420;
                break;

            case AV_PIX_FMT_YUV422P:
                *chroma_format = ::VideoChromaFormat_YUV422;
                break;

            case AV_PIX_FMT_YUV444P:
                *chroma_format = ::VideoChromaFormat_YUV444;
                break;

            default:
                return false;
            }

            *width = enc->coded_width;
            *height = enc->coded_height;

            break;
        }
    }

    if (video_stream_id_ < 0)
        return false;

    av_init_packet(&pkt_);

#if USE_AV_INTERRUPT_CALLBACK
    // deactivate interrupt callback
    interrupt_metadata.timeout_after_ms = 0;
#endif

    return true;
}

void InputMediaStream_FFMPEG::close()
{
    if (ctx_)
    {
        #if LIBAVFORMAT_BUILD >= CALC_FFMPEG_VERSION(53, 24, 2)
            avformat_close_input(&ctx_);
        #else
            av_close_input_file(ctx_);
        #endif
    }

    // free last packet if exist
    if (pkt_.data)
        av_packet_unref(&pkt_);
}

bool InputMediaStream_FFMPEG::read(unsigned char** data, int* size, int* endOfFile)
{
    bool result = false;

#if USE_AV_INTERRUPT_CALLBACK
    // activate interrupt callback
    get_monotonic_time(&interrupt_metadata.value);
    interrupt_metadata.timeout_after_ms = LIBAVFORMAT_INTERRUPT_READ_TIMEOUT_MS;
#endif

    // free last packet if exist
    if (pkt_.data)
        av_packet_unref(&pkt_);

    // get the next frame
    for (;;)
    {
#if USE_AV_INTERRUPT_CALLBACK
        if(interrupt_metadata.timeout)
        {
            break;
        }
#endif

        int ret = av_read_frame(ctx_, &pkt_);

        if (ret == AVERROR(EAGAIN))
            continue;

        if (ret < 0)
        {
            if (ret == (int)AVERROR_EOF)
                *endOfFile = true;
            break;
        }

        if (pkt_.stream_index != video_stream_id_)
        {
            av_packet_unref(&pkt_);
            continue;
        }

        result = true;
        break;
    }

#if USE_AV_INTERRUPT_CALLBACK
    // deactivate interrupt callback
    interrupt_metadata.timeout_after_ms = 0;
#endif

    if (result)
    {
        *data = pkt_.data;
        *size = pkt_.size;
        *endOfFile = false;
    }

    return result;
}

InputMediaStream_FFMPEG* create_InputMediaStream_FFMPEG(const char* fileName, int* codec, int* chroma_format, int* width, int* height)
{
    InputMediaStream_FFMPEG* stream = (InputMediaStream_FFMPEG*) malloc(sizeof(InputMediaStream_FFMPEG));

    if (stream && stream->open(fileName, codec, chroma_format, width, height))
        return stream;

    stream->close();
    free(stream);

    return 0;
}

void release_InputMediaStream_FFMPEG(InputMediaStream_FFMPEG* stream)
{
    stream->close();
    free(stream);
}

int read_InputMediaStream_FFMPEG(InputMediaStream_FFMPEG* stream, unsigned char** data, int* size, int* endOfFile)
{
    return stream->read(data, size, endOfFile);
}
*/

