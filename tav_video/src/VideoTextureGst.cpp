/*
 * VIdeoTextureGst.cpp
 *
 *  Created on: Jun 29, 2018
 *      Author: sven
 *      adapted from Openframeworks
 */

#include "VideoTextureGst.h"

using namespace std;

namespace tav
{

VideoTextureGst::VideoTextureGst(){
	nFrames						= 0;
	//internalPixelFormat			= OF_PIXELS_RGB;
	internalPixelFormat			= GL_RGB;
	bIsStream					= false;
	bIsAllocated				= false;
	threadAppSink				= false;
	bAsyncLoad					= false;
	//videoUtils.setSinkListener(this);
	fps_d = 1;
	fps_n = 1;
}

//------------------------------------------------------------------------------------

VideoTextureGst::~VideoTextureGst(){
	close();
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::setPixelFormat(GLenum pixelFormat)
{
	internalPixelFormat = pixelFormat;
	return true;
}

//------------------------------------------------------------------------------------

GLenum VideoTextureGst::getPixelFormat() const {
	return internalPixelFormat;
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::createPipeline(string name)
{

#ifndef USE_GST_GL

	string mime="video/x-raw";

	GstCaps *caps;
	if(internalPixelFormat==OF_PIXELS_NATIVE){
		caps = gst_caps_from_string((mime + ",format={RGBA,BGRA,RGB,BGR,RGB16,GRAY8,YV12,I420,NV12,NV21,YUY2}").c_str());
	}else{
		string format = ofGstVideoUtils::getGstFormatName(internalPixelFormat);
		caps = gst_caps_new_simple(mime.c_str(),
				"format", G_TYPE_STRING, format.c_str(),
				NULL);
	}

	GstElement * gstPipeline = gst_element_factory_make("playbin","player");

	g_object_ref_sink(gstPipeline);
	g_object_set(G_OBJECT(gstPipeline), "uri", name.c_str(), (void*)NULL);

	// create the oF appsink for video rgb without sync to clock
	GstElement * gstSink = gst_element_factory_make("appsink", "app_sink");
	gst_app_sink_set_caps(GST_APP_SINK(gstSink), caps);
	gst_caps_unref(caps);

	if(threadAppSink){
		GstElement * appQueue = gst_element_factory_make("queue","appsink_queue");
		g_object_set(G_OBJECT(appQueue), "leaky", 0, "silent", 1, (void*)NULL);
		GstElement* appBin = gst_bin_new("app_bin");
		gst_bin_add(GST_BIN(appBin), appQueue);
		GstPad* appQueuePad = gst_element_get_static_pad(appQueue, "sink");
		GstPad* ghostPad = gst_ghost_pad_new("app_bin_sink", appQueuePad);
		gst_object_unref(appQueuePad);
		gst_element_add_pad(appBin, ghostPad);

		gst_bin_add(GST_BIN(appBin), gstSink);
		gst_element_link(appQueue, gstSink);

		g_object_set (G_OBJECT(gstPipeline),"video-sink",appBin,(void*)NULL);
	}else{
		g_object_set (G_OBJECT(gstPipeline),"video-sink",gstSink,(void*)NULL);
	}

#ifdef TARGET_WIN32
	GstElement *audioSink = gst_element_factory_make("directsoundsink", NULL);
	g_object_set (G_OBJECT(gstPipeline),"audio-sink",audioSink,(void*)NULL);
#endif

	return videoUtils.setPipelineWithSink(gstPipeline,gstSink,bIsStream);

#else
	/*auto gstPipeline = gst_parse_launch(("uridecodebin uri=" + name + " ! glcolorscale name=gl_filter ! appsink name=app_sink").c_str(),NULL);
	auto gstSink = gst_bin_get_by_name(GST_BIN(gstPipeline),"app_sink");
	auto glfilter = gst_bin_get_by_name(GST_BIN(gstPipeline),"gl_filter");
	gst_app_sink_set_caps(GST_APP_SINK(gstSink), caps);
	gst_caps_unref(caps);

	glXMakeCurrent (ofGetX11Display(), None, 0);
	glDisplay = (GstGLDisplay *)gst_gl_display_x11_new_with_display(ofGetX11Display());
	glContext = gst_gl_context_new_wrapped (glDisplay, (guintptr) ofGetGLXContext(),
	    		  GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);

	g_object_set (G_OBJECT (glfilter), "other-context", glContext, NULL);

	// FIXME: this seems to be the way to add the context in 1.4.5
	//
	// GstBus * bus = gst_pipeline_get_bus (GST_PIPELINE(gstPipeline));
	// gst_bus_enable_sync_message_emission (bus);
	// g_signal_connect (bus, "sync-message", G_CALLBACK (sync_bus_call), this);
	// gst_object_unref(bus);

	auto ret = videoUtils.setPipelineWithSink(gstPipeline,gstSink,bIsStream);
	glXMakeCurrent (ofGetX11Display(), ofGetX11Window(), ofGetGLXContext());
	return ret;*/

	return videoUtils.setPipeline("uridecodebin uri=" + name,internalPixelFormat,bIsStream,-1,-1);
	//return videoUtils.setPipeline("filesrc location=" + name + " ! qtdemux ",internalPixelFormat,bIsStream,-1,-1);
#endif
}

//------------------------------------------------------------------------------------

void VideoTextureGst::loadAsync(string name){
	bAsyncLoad = true;
	load(name);
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::load(string name){
	if( name.find( "file://",0 ) != string::npos){
		bIsStream = bAsyncLoad;
	}else if( name.find( "://",0 ) == string::npos){
		GError * err = NULL;
		gchar* name_ptr = gst_filename_to_uri(ofToDataPath(name).c_str(),&err);
		name = name_ptr;
		g_free(name_ptr);
		if(err) g_free(err);
		//name = ofToDataPath(name);
		bIsStream = bAsyncLoad;
	}else{
		bIsStream = true;
	}
	ofLogVerbose("VideoTextureGst") << "loadMovie(): loading \"" << name << "\"";

	if(isInitialized()){
		gst_element_set_state (videoUtils.getPipeline(), GST_STATE_READY);
		if(!bIsStream){
			gst_element_get_state (videoUtils.getPipeline(), NULL, NULL, -1);
		}
		internalPixelFormat = OF_PIXELS_NATIVE;
		bIsAllocated = false;
		videoUtils.reallocateOnNextFrame();
		g_object_set(G_OBJECT(videoUtils.getPipeline()), "uri", name.c_str(), (void*)NULL);
		gst_element_set_state (videoUtils.getPipeline(), GST_STATE_PAUSED);
		if(!bIsStream){
			gst_element_get_state (videoUtils.getPipeline(), NULL, NULL, -1);
			return allocate();
		}else{
			return true;
		}
	}else{
		ofGstUtils::startGstMainLoop();
		return createPipeline(name) &&
				videoUtils.startPipeline() &&
				(bIsStream || allocate());
	}
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setThreadAppSink(bool threaded){
	threadAppSink = threaded;
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::allocate(){
	if(bIsAllocated){
		return true;
	}

	guint64 durationNanos = videoUtils.getDurationNanos();

	nFrames		  = 0;
	if(GstPad* pad = gst_element_get_static_pad(videoUtils.getSink(), "sink")){
#if GST_VERSION_MAJOR==0
		int width,height;
		if(gst_video_get_size(GST_PAD(pad), &width, &height)){
			if(!videoUtils.allocate(width,height,internalPixelFormat)) return false;
		}else{
			ofLogError("VideoTextureGst") << "allocate(): couldn't query width and height";
			return false;
		}

		const GValue *framerate = gst_video_frame_rate(pad);
		fps_n=0;
		fps_d=0;
		if(framerate && GST_VALUE_HOLDS_FRACTION (framerate)){
			fps_n = gst_value_get_fraction_numerator (framerate);
			fps_d = gst_value_get_fraction_denominator (framerate);
			nFrames = (float)(durationNanos / (float)GST_SECOND) * (float)fps_n/(float)fps_d;
			ofLogVerbose("VideoTextureGst") << "allocate(): framerate: " << fps_n << "/" << fps_d;
		}else{
			ofLogWarning("VideoTextureGst") << "allocate(): cannot get framerate, frame seek won't work";
		}
		bIsAllocated = true;
#else
		if(GstCaps *caps = gst_pad_get_current_caps (GST_PAD (pad))){
			GstVideoInfo info;
			gst_video_info_init (&info);
			if (gst_video_info_from_caps (&info, caps)){
				ofPixelFormat format = ofGstVideoUtils::getOFFormat(GST_VIDEO_INFO_FORMAT(&info));
				if(format!=internalPixelFormat){
					ofLogVerbose("VideoTextureGst") << "allocating as " << info.width << "x" << info.height << " " << info.finfo->description << " " << info.finfo->name;
					internalPixelFormat = format;
				}
				if(!videoUtils.allocate(info.width,info.height,format)) return false;
			}else{
				ofLogError("VideoTextureGst") << "allocate(): couldn't query width and height";
				return false;
			}

			fps_n = info.fps_n;
			fps_d = info.fps_d;
			nFrames = (float)(durationNanos / (float)GST_SECOND) * (float)fps_n/(float)fps_d;
			gst_caps_unref(caps);
			bIsAllocated = true;
		}else{
			ofLogError("VideoTextureGst") << "allocate(): cannot get pipeline caps";
			bIsAllocated = false;
		}
#endif
		gst_object_unref(GST_OBJECT(pad));
	}else{
		ofLogError("VideoTextureGst") << "allocate(): cannot get sink pad";
		bIsAllocated = false;
	}
	return bIsAllocated;
}

//------------------------------------------------------------------------------------

void VideoTextureGst::on_stream_prepared(){
	if(!bIsAllocated) allocate();
}

//------------------------------------------------------------------------------------

int	VideoTextureGst::getCurrentFrame() const {
	int frame = 0;

	// zach I think this may fail on variable length frames...
	float pos = getPosition();
	if(pos == -1) return -1;


	float  framePosInFloat = ((float)getTotalNumFrames() * pos);
	int    framePosInInt = (int)framePosInFloat;
	float  floatRemainder = (framePosInFloat - framePosInInt);
	if (floatRemainder > 0.5f) framePosInInt = framePosInInt + 1;
	//frame = (int)ceil((getTotalNumFrames() * getPosition()));
	frame = framePosInInt;

	return frame;
}

//------------------------------------------------------------------------------------

int	VideoTextureGst::getTotalNumFrames() const {
	return nFrames;
}

//------------------------------------------------------------------------------------

void VideoTextureGst::firstFrame(){
	setFrame(0);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::nextFrame(){
	gint64 currentFrame = getCurrentFrame();
	if(currentFrame!=-1) setFrame(currentFrame + 1);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::previousFrame(){
	gint64 currentFrame = getCurrentFrame();
	if(currentFrame!=-1) setFrame(currentFrame - 1);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setFrame(int frame){ // frame 0 = first frame...
	float pct = (float)frame / (float)nFrames;
	setPosition(pct);
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isStream() const {
	return bIsStream;
}

//------------------------------------------------------------------------------------

void VideoTextureGst::update(){
	videoUtils.update();
}

//------------------------------------------------------------------------------------

void VideoTextureGst::play(){
	videoUtils.play();
}

//------------------------------------------------------------------------------------

void VideoTextureGst::stop(){
	videoUtils.stop();
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setPaused(bool bPause){
	videoUtils.setPaused(bPause);
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isPaused() const {
	return videoUtils.isPaused();
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isLoaded() const {
	return videoUtils.isLoaded();
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isPlaying() const {
	return videoUtils.isPlaying();
}

//------------------------------------------------------------------------------------

float VideoTextureGst::getPosition() const {
	return videoUtils.getPosition();
}

//------------------------------------------------------------------------------------

float VideoTextureGst::getSpeed() const {
	return videoUtils.getSpeed();
}

//------------------------------------------------------------------------------------

float VideoTextureGst::getDuration() const {
	return videoUtils.getDuration();
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::getIsMovieDone() const {
	return videoUtils.getIsMovieDone();
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setPosition(float pct){
	videoUtils.setPosition(pct);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setVolume(float volume){
	videoUtils.setVolume(volume);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setLoopState(ofLoopType state){
	videoUtils.setLoopState(state);
}

//------------------------------------------------------------------------------------

ofLoopType VideoTextureGst::getLoopState() const {
	return videoUtils.getLoopState();
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setSpeed(float speed){
	videoUtils.setSpeed(speed);
}

//------------------------------------------------------------------------------------

void VideoTextureGst::close(){
	bIsAllocated = false;
	videoUtils.close();
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isFrameNew() const {
	return videoUtils.isFrameNew();
}

//------------------------------------------------------------------------------------

uint8_t& VideoTextureGst::getPixels(){
	return videoUtils.getPixels();
}

//------------------------------------------------------------------------------------

const uint8_t& VideoTextureGst::getPixels() const {
	return videoUtils.getPixels();
}

//------------------------------------------------------------------------------------

TextureManager* VideoTextureGst::getTexturePtr(){
	return videoUtils.getTexture();
}

//------------------------------------------------------------------------------------

float VideoTextureGst::getHeight() const {
	return videoUtils.getHeight();
}

//------------------------------------------------------------------------------------

float VideoTextureGst::getWidth() const {
	return videoUtils.getWidth();
}

//------------------------------------------------------------------------------------

VideoTextureGstUtils * VideoTextureGst::getGstVideoUtils(){
	return &videoUtils;
}

//------------------------------------------------------------------------------------

void VideoTextureGst::setFrameByFrame(bool frameByFrame){
	videoUtils.setFrameByFrame(frameByFrame);
}

//------------------------------------------------------------------------------------

bool VideoTextureGst::isThreadedAppSink() const{
	return threadAppSink;
}

bool VideoTextureGst::isFrameByFrame() const{
	return videoUtils.isFrameByFrame();
}

}
