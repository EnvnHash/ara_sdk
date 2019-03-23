/*
 * VideoTextureGstVideoUtils.cpp
 *
 *  Created on: Jun 29, 2018
 *      Author: sven
 */

#include "VideoTextureGstVideoUtils.h"

using namespace std;

namespace tav
{

VideoTextureGstUtils::ofGstMainLoopThread * VideoTextureGstUtils::mainLoop;

void VideoTextureGstUtils::startGstMainLoop()
{
	static bool initialized = false;

	if(!initialized){
		mainLoop = new ofGstMainLoopThread;
		mainLoop->start();
		initialized=true;
	}
}

GMainLoop * VideoTextureGstUtils::getGstMainLoop(){
	return mainLoop->getMainLoop();
}

void VideoTextureGstUtils::quitGstMainLoop(){
	mainLoop->quit();
	delete mainLoop;
	mainLoop=0;
}



//-------------------------------------------------
//----------------------------------------- gstUtils
//-------------------------------------------------

static bool plugin_registered = false;
static bool gst_inited = false;

//------------------------------------------------------------------------------------

// called when the appsink notifies us that there is a new buffer ready for
// processing

static GstFlowReturn on_new_buffer_from_source (GstAppSink * elt, void * data){

	shared_ptr<GstSample> buffer(gst_app_sink_pull_sample (GST_APP_SINK (elt)),&gst_sample_unref);
	return ((VideoTextureGstUtils*)data)->buffer_cb(buffer);
}

//------------------------------------------------------------------------------------

static GstFlowReturn on_new_preroll_from_source (GstAppSink * elt, void * data){

	shared_ptr<GstSample> buffer(gst_app_sink_pull_preroll(GST_APP_SINK (elt)),&gst_sample_unref);

	return ((VideoTextureGstUtils*)data)->preroll_cb(buffer);
}

//------------------------------------------------------------------------------------

static void on_eos_from_source (GstAppSink * elt, void * data){
	((VideoTextureGstUtils*)data)->eos_cb();
}

//------------------------------------------------------------------------------------

static gboolean appsink_plugin_init (GstPlugin * plugin)
{
  gst_element_register (plugin, "appsink", GST_RANK_NONE, GST_TYPE_APP_SINK);

  return TRUE;
}

//------------------------------------------------------------------------------------

VideoTextureGstUtils::VideoTextureGstUtils() {
	bLoaded 					= false;
	speed 						= 1;
	bPaused						= false;
	bIsMovieDone				= false;
	bPlaying					= false;
	loopMode					= OF_LOOP_NONE;
	bFrameByFrame 				= false;

	gstPipeline					= NULL;
	gstSink						= NULL;

	durationNanos				= 0;

	isAppSink					= false;
	isStream					= false;

	appsink						= NULL;
	closing 					= false;

	busWatchID					= 0;

#if GLIB_MINOR_VERSION<32
	if(!g_thread_supported()){
		g_thread_init(NULL);
	}
#endif

	if(!gst_inited){
#ifdef TARGET_WIN32
		string gst_path = g_getenv("GSTREAMER_1_0_ROOT_X86");
		//putenv(("GST_PLUGIN_PATH_1_0=" + ofFilePath::join(gst_path, "lib\\gstreamer-1.0") + ";.").c_str());
		// to make it compatible with gcc and C++11 standard
		SetEnvironmentVariableA("GST_PLUGIN_PATH_1_0", ofFilePath::join(gst_path, "lib\\gstreamer-1.0").c_str());
#endif
		gst_init (NULL, NULL);
		gst_inited=true;
		ofLogVerbose("VideoTextureGstUtils") << "gstreamer inited";
	}
	if(!plugin_registered){
		gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR,
					"appsink", (char*)"Element application sink",
					appsink_plugin_init, "0.1", "LGPL", "ofVideoPlayer", "openFrameworks",
					"http://openframeworks.cc/");
		plugin_registered=true;
	}

}

//------------------------------------------------------------------------------------

VideoTextureGstUtils::~VideoTextureGstUtils() {
	close();
}

//------------------------------------------------------------------------------------


GstFlowReturn VideoTextureGstUtils::preroll_cb(shared_ptr<GstSample> buffer){

	bIsMovieDone = false;
	if(appsink) return appsink->on_preroll(buffer);
	else return GST_FLOW_OK;
}

//------------------------------------------------------------------------------------

GstFlowReturn VideoTextureGstUtils::buffer_cb(shared_ptr<GstSample> buffer){

	bIsMovieDone = false;
	if(appsink) return appsink->on_buffer(buffer);
	else return GST_FLOW_OK;
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::eos_cb(){
	bIsMovieDone = true;
	if(appsink && !isAppSink) appsink->on_eos();
	if(closing){
		std::unique_lock<std::mutex> lck(eosMutex);
		eosCondition.notify_all();
	}
}

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::setPipelineWithSink(string pipeline, string sinkname, bool isStream){
	VideoTextureGstUtils::startGstMainLoop();

	GError * error = NULL;
	gstPipeline = gst_parse_launch (pipeline.c_str(), &error);
	g_object_ref_sink(gstPipeline);
	ofLogNotice("VideoTextureGstUtils") << "setPipelineWithSink(): gstreamer pipeline: " << pipeline;

	if(error!=NULL){
		ofLogError("VideoTextureGstUtils") << "setPipelineWithSink(): couldn't create pipeline: " << error->message;
		return false;
	}

	if(sinkname!=""){
		gstSink = gst_bin_get_by_name(GST_BIN(gstPipeline),sinkname.c_str());

		if(!gstSink){
			ofLogError("VideoTextureGstUtils") << "setPipelineWithSink(): couldn't get sink from string pipeline";
		}
	}

	return setPipelineWithSink(gstPipeline,gstSink,isStream);
}

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::setPipelineWithSink(GstElement * pipeline, GstElement * sink, bool isStream_){
	VideoTextureGstUtils::startGstMainLoop();

	gstPipeline = pipeline;
	gstSink = sink;
	isStream = isStream_;

	if(gstSink){
		gst_base_sink_set_sync(GST_BASE_SINK(gstSink), true);
	}

	if(gstSink && string(gst_plugin_feature_get_name( GST_PLUGIN_FEATURE(gst_element_get_factory(gstSink))))=="appsink"){
		isAppSink = true;
	}else{
		isAppSink = false;
	}

	return true;
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setFrameByFrame(bool _bFrameByFrame){
	bFrameByFrame = _bFrameByFrame;
	if(gstSink){
		g_object_set (G_OBJECT (gstSink), "sync", !bFrameByFrame, (void*)NULL);
	}
}

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::isFrameByFrame() const{
	return bFrameByFrame;
}

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::startPipeline(){

	bPaused 			= true;
	speed 				= 1.0f;


	GstBus * bus = gst_pipeline_get_bus (GST_PIPELINE(gstPipeline));

	if(bus){
		busWatchID = gst_bus_add_watch (bus, (GstBusFunc) busFunction, this);
	}

	gst_object_unref(bus);

	if(isAppSink){
		ofLogVerbose("VideoTextureGstUtils") << "startPipeline(): attaching callbacks";
		// set the appsink to not emit signals, we are using callbacks instead
		// and frameByFrame to get buffers by polling instead of callback
		g_object_set (G_OBJECT (gstSink), "emit-signals", FALSE, "sync", !bFrameByFrame, (void*)NULL);
		// gst_app_sink_set_drop(GST_APP_SINK(gstSink),1);
		// gst_app_sink_set_max_buffers(GST_APP_SINK(gstSink),2);

		if(!bFrameByFrame){
			GstAppSinkCallbacks gstCallbacks;
			gstCallbacks.eos = &on_eos_from_source;
			gstCallbacks.new_preroll = &on_new_preroll_from_source;
			gstCallbacks.new_sample = &on_new_buffer_from_source;

			gst_app_sink_set_callbacks(GST_APP_SINK(gstSink), &gstCallbacks, this, NULL);
		}
	}

	// pause the pipeline
	//GstState targetState;
	GstState state;
	auto ret = gst_element_set_state(GST_ELEMENT(gstPipeline), GST_STATE_PAUSED);
    switch (ret) {
		case GST_STATE_CHANGE_FAILURE:
			ofLogError("VideoTextureGstUtils") << "startPipeline(): unable to pause pipeline";
			return false;
			break;
		case GST_STATE_CHANGE_NO_PREROLL:
			ofLogVerbose() << "Pipeline is live and does not need PREROLL waiting PLAY";
			gst_element_set_state(GST_ELEMENT(gstPipeline), GST_STATE_PLAYING);
			if(isAppSink){
				gst_app_sink_set_max_buffers(GST_APP_SINK(gstSink),1);
				gst_app_sink_set_drop (GST_APP_SINK(gstSink),true);
			}
			break;
		case GST_STATE_CHANGE_ASYNC:
			ofLogVerbose() << "Pipeline is PREROLLING";
			//targetState = GST_STATE_PAUSED;
			if(!isStream && gst_element_get_state(gstPipeline,&state,NULL,5*GST_SECOND)!=GST_STATE_CHANGE_SUCCESS){
				ofLogError("VideoTextureGstUtils") << "startPipeline(): unable to pause pipeline after 5s";
				return false;
			}else{
				ofLogVerbose() << "Pipeline is PREROLLED";
			}
			break;
		case GST_STATE_CHANGE_SUCCESS:
			ofLogVerbose() << "Pipeline is PREROLLED";
			break;
    }

	// wait for paused state to query the duration
	if(!isStream){
		bPlaying = true;
		bLoaded = true;
	}

	return true;
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::play(){
	setPaused(false);

	//this is if we set the speed first but it only can be set when we are playing.
	if(!isStream) setSpeed(speed);
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setPaused(bool _bPause){
	bPaused = _bPause;
	//timeLastIdle = ofGetElapsedTimeMillis();
	if(bLoaded){
		if(bPlaying){
			if(bPaused){
				gst_element_set_state (gstPipeline, GST_STATE_PAUSED);
			}else{
				gst_element_set_state (gstPipeline, GST_STATE_PLAYING);
			}
		}else{
			GstState state = GST_STATE_PAUSED;
			gst_element_set_state (gstPipeline, state);
			gst_element_get_state(gstPipeline,&state,NULL,2*GST_SECOND);
			if(!bPaused){
				gst_element_set_state (gstPipeline, GST_STATE_PLAYING);
			}
			bPlaying = true;
		}
	}
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::stop(){
	if(!bLoaded) return;
	GstState state;

	if(!bPaused){
		state = GST_STATE_PAUSED;
		gst_element_set_state (gstPipeline, state);
		gst_element_get_state(gstPipeline,&state,NULL,2*GST_SECOND);
	}

	state = GST_STATE_READY;
	gst_element_set_state (gstPipeline, state);
	gst_element_get_state(gstPipeline,&state,NULL,2*GST_SECOND);
	bPlaying = false;
	bPaused = true;
}

//------------------------------------------------------------------------------------

float VideoTextureGstUtils::getPosition() const{
	if(gstPipeline){
		gint64 pos=0;
		if(!gst_element_query_position(GST_ELEMENT(gstPipeline),GST_FORMAT_TIME,&pos)){
			ofLogVerbose("VideoTextureGstUtils") << "getPosition(): couldn't query position";
			return -1;
		}
		return (float)pos/(float)durationNanos;
	}else{
		return -1;
	}
}

//------------------------------------------------------------------------------------

float VideoTextureGstUtils::getSpeed() const{
	return speed;
}

//------------------------------------------------------------------------------------

float VideoTextureGstUtils::getDuration() const{
	return (float)getDurationNanos()/(float)GST_SECOND;
}

//------------------------------------------------------------------------------------

int64_t VideoTextureGstUtils::getDurationNanos() const{
	GstFormat format = GST_FORMAT_TIME;

	if(!gst_element_query_duration(getPipeline(),format,&durationNanos))
		ofLogWarning("VideoTextureGstUtils") << "getDurationNanos(): couldn't query time duration";

	return durationNanos;
}

//------------------------------------------------------------------------------------

bool  VideoTextureGstUtils::getIsMovieDone() const{
	if(isAppSink){
		return gst_app_sink_is_eos(GST_APP_SINK(gstSink));
	}else{
		return bIsMovieDone;
	}
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setPosition(float pct){
	//pct = CLAMP(pct, 0,1);// check between 0 and 1;
	GstFormat format = GST_FORMAT_TIME;
	GstSeekFlags flags = (GstSeekFlags) (GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH);
	gint64 pos = (guint64)((double)pct*(double)durationNanos);

	/*if(bPaused){
		seek_lock();
		gst_element_set_state (gstPipeline, GST_STATE_PLAYING);
		posChangingPaused=true;
		seek_unlock();
	}*/
	if(speed>0){
		if(!gst_element_seek(GST_ELEMENT(gstPipeline),speed, 	format,
				flags,
				GST_SEEK_TYPE_SET,
				pos,
				GST_SEEK_TYPE_SET,
				-1)) {
		ofLogWarning("VideoTextureGstUtils") << "setPosition(): unable to seek";
		}
	}else{
		if(!gst_element_seek(GST_ELEMENT(gstPipeline),speed, 	format,
				flags,
				GST_SEEK_TYPE_SET,
				0,
				GST_SEEK_TYPE_SET,
				pos)) {
		ofLogWarning("VideoTextureGstUtils") << "setPosition(): unable to seek";
		}
	}
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setVolume(float volume){
	gdouble gvolume = volume;
	g_object_set(G_OBJECT(gstPipeline), "volume", gvolume, (void*)NULL);
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setLoopState(ofLoopType state){
	loopMode = state;
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setSpeed(float _speed){
	GstFormat format = GST_FORMAT_TIME;
	GstSeekFlags flags = (GstSeekFlags) (GST_SEEK_FLAG_SKIP | GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH);
	gint64 pos;

	if(_speed==0){
		gst_element_set_state (gstPipeline, GST_STATE_PAUSED);
		return;
	}

	if(!gst_element_query_position(GST_ELEMENT(gstPipeline),format,&pos) || pos<0){
		//ofLogError("VideoTextureGstUtils") << "setSpeed(): couldn't query position";
		return;
	}

	speed = _speed;
	//pos = (float)gstData.lastFrame * (float)fps_d / (float)fps_n * GST_SECOND;

	if(!bPaused)
		gst_element_set_state (gstPipeline, GST_STATE_PLAYING);

	if(speed>0){
		if(!gst_element_seek(GST_ELEMENT(gstPipeline),speed, 	format,
				flags,
				GST_SEEK_TYPE_SET,
				pos,
				GST_SEEK_TYPE_SET,
				-1)) {
			ofLogWarning("VideoTextureGstUtils") << "setSpeed(): unable to change speed";
		}
	}else{
		if(!gst_element_seek(GST_ELEMENT(gstPipeline),speed, 	format,
				flags,
				GST_SEEK_TYPE_SET,
				0,
				GST_SEEK_TYPE_SET,
				pos)) {
			ofLogWarning("VideoTextureGstUtils") << "setSpeed(): unable to change speed";
		}
	}

	ofLogVerbose("VideoTextureGstUtils") << "setSpeed(): speed changed to " << speed;

}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::close(){
	if(bPlaying){
		if(!bIsMovieDone && !bPaused && !isStream){
			std::unique_lock<std::mutex> lck(eosMutex);
			closing = true;
			gst_element_send_event(gstPipeline,gst_event_new_eos());
			if(eosCondition.wait_for(lck,std::chrono::milliseconds(5000))==std::cv_status::timeout){
				ofLogWarning("VideoTextureGstUtils") << "didn't received EOS in 5s, closing pipeline anyway";
			}
			closing = false;
		}
	}
	stop();

	if(bLoaded){
		gst_element_set_state(GST_ELEMENT(gstPipeline), GST_STATE_NULL);
		gst_element_get_state(gstPipeline,NULL,NULL,2*GST_SECOND);

		if(busWatchID!=0) g_source_remove(busWatchID);

		gst_object_unref(gstPipeline);
		gstPipeline = NULL;
		gstSink = NULL;
	}

	bLoaded = false;
}

/*static string getName(GstState state){
	switch(state){
	case   GST_STATE_VOID_PENDING:
		return "void pending";
	case   GST_STATE_NULL:
		return "null";
	case   GST_STATE_READY:
		return "ready";
	case   GST_STATE_PAUSED:
		return "paused";
	case   GST_STATE_PLAYING:
		return "playing";
	default:
		return "";
	}
}*/

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::busFunction(GstBus * bus, GstMessage * message, VideoTextureGstUtils * gstUtils){
	return gstUtils->gstHandleMessage(bus,message);
}

//------------------------------------------------------------------------------------

bool VideoTextureGstUtils::gstHandleMessage(GstBus * bus, GstMessage * msg){
	if(appsink && appsink->on_message(msg)) return true;

		/*ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): got " << GST_MESSAGE_TYPE_NAME(msg)
			<< " message from " << GST_MESSAGE_SRC_NAME(msg);*/

	switch (GST_MESSAGE_TYPE (msg)) {

		case GST_MESSAGE_BUFFERING:
			gint pctBuffered;
			gst_message_parse_buffering(msg,&pctBuffered);
			ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): buffering " << pctBuffered;
			if(pctBuffered<100){
				gst_element_set_state (gstPipeline, GST_STATE_PAUSED);
			}else if(!bPaused){
				gst_element_set_state (gstPipeline, GST_STATE_PLAYING);
			}
		break;

		case GST_MESSAGE_DURATION_CHANGED:
			gst_element_query_duration(gstPipeline,GST_FORMAT_TIME,&durationNanos);
			break;

		case GST_MESSAGE_STATE_CHANGED:{
			GstState oldstate, newstate, pendstate;
			gst_message_parse_state_changed(msg, &oldstate, &newstate, &pendstate);
			if(isStream && newstate==GST_STATE_PAUSED && !bPlaying ){
				bLoaded = true;
				bPlaying = true;
				if(!bPaused){
					//ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): setting stream pipeline to play";
					play();
				}
			}

			/*ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): " << GST_MESSAGE_SRC_NAME(msg) << " state changed from "
					<< getName(oldstate) << " to " << getName(newstate) << " (" + getName(pendstate) << ")";*/
		}break;

		case GST_MESSAGE_ASYNC_DONE:
			ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): async done";
		break;

		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;
			gst_message_parse_error(msg, &err, &debug);
			gchar * name = gst_element_get_name(GST_MESSAGE_SRC (msg));

			ofLogError("VideoTextureGstUtils") << "gstHandleMessage(): embedded video playback halted for plugin, module "
				<< name << "  reported: " << err->message;

			g_free(name);
			g_error_free(err);
			g_free(debug);

			gst_element_set_state(GST_ELEMENT(gstPipeline), GST_STATE_NULL);

		}break;

		case GST_MESSAGE_EOS:{
			ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): end of the stream";
			bool isClosing = closing;
			eos_cb();

			if(isClosing){
				busWatchID = 0;
				return false;
			}

			switch(loopMode){

				case OF_LOOP_NORMAL:{
					GstFormat format = GST_FORMAT_TIME;
					GstSeekFlags flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT);
					if(speed>0){
						if(!gst_element_seek(GST_ELEMENT(gstPipeline),
											speed,
											format,
											flags,
											GST_SEEK_TYPE_SET,
											0,
											GST_SEEK_TYPE_SET,
											-1)) {
							ofLogWarning("VideoTextureGstUtils") << "gstHandleMessage(): unable to seek";
						}
					}else if(speed<0){
						if(!gst_element_seek(GST_ELEMENT(gstPipeline),speed, 	format,
								flags,
								GST_SEEK_TYPE_SET,
								0,
								GST_SEEK_TYPE_SET,
								durationNanos-1000000)) {
							ofLogWarning("VideoTextureGstUtils") << "gstHandleMessage(): unable to seek";
						}
					}
				}break;

				case OF_LOOP_PALINDROME:{
					GstFormat format = GST_FORMAT_TIME;
					GstSeekFlags flags = (GstSeekFlags) (GST_SEEK_FLAG_FLUSH |GST_SEEK_FLAG_KEY_UNIT);
					gint64 pos;
					gst_element_query_position(GST_ELEMENT(gstPipeline),format,&pos);
					float loopSpeed;
					if(pos>0)
						loopSpeed=-speed;
					else
						loopSpeed=speed;
					if(!gst_element_seek(GST_ELEMENT(gstPipeline),
										loopSpeed,
										GST_FORMAT_UNDEFINED,
										flags,
										GST_SEEK_TYPE_NONE,
										0,
										GST_SEEK_TYPE_NONE,
										0)) {
						ofLogWarning("VideoTextureGstUtils") << "gstHandleMessage(): unable to seek";
					}
				}break;

				default:
				break;
			}

		}break;
		case GST_MESSAGE_LATENCY:
			gst_bin_recalculate_latency (GST_BIN (getPipeline()));
			break;
		case GST_MESSAGE_REQUEST_STATE:	{
			GstState state;
			gchar *name = gst_object_get_path_string (GST_MESSAGE_SRC (msg));

			gst_message_parse_request_state (msg, &state);
			gst_element_set_state (getPipeline(), state);

			g_free (name);
			break;
		}

		case GST_MESSAGE_HAVE_CONTEXT:{
			GstContext *context;
			const gchar *context_type;
			gchar *context_str;

			gst_message_parse_have_context (msg, &context);

			context_type = gst_context_get_context_type (context);
			context_str = gst_structure_to_string (gst_context_get_structure (context));
			ofLogNotice("VideoTextureGstUtils","Got context from element '%s': %s=%s\n",
				GST_ELEMENT_NAME (GST_MESSAGE_SRC (msg)), context_type,
				context_str);
			g_free (context_str);
			gst_context_unref (context);
			break;
		}

		default:
			ofLogVerbose("VideoTextureGstUtils") << "gstHandleMessage(): unhandled message from " << GST_MESSAGE_SRC_NAME(msg);
		break;
	}

	return true;
}

//------------------------------------------------------------------------------------

GstElement 	* VideoTextureGstUtils::getPipeline() const{
	return gstPipeline;
}

//------------------------------------------------------------------------------------

GstElement 	* VideoTextureGstUtils::getSink() const{
	return gstSink;
}

//------------------------------------------------------------------------------------

GstElement 	* VideoTextureGstUtils::getGstElementByName(const string & name) const{
	return gst_bin_get_by_name(GST_BIN(gstPipeline),name.c_str());
}

//------------------------------------------------------------------------------------

void VideoTextureGstUtils::setSinkListener(ofGstAppSink * appsink_){
	appsink = appsink_;
}

//------------------------------------------------------------------------------------

uint64_t VideoTextureGstUtils::getMinLatencyNanos() const{
	GstClockTime minlat=0, maxlat=0;
	GstQuery * q = gst_query_new_latency();
	if (gst_element_query (gstPipeline, q)) {
		 gboolean live;
		 gst_query_parse_latency (q, &live, &minlat, &maxlat);
	}
	gst_query_unref (q);
	return minlat;
}

//------------------------------------------------------------------------------------

uint64_t VideoTextureGstUtils::getMaxLatencyNanos() const{
	GstClockTime minlat=0, maxlat=0;
	GstQuery * q = gst_query_new_latency();
	if (gst_element_query (gstPipeline, q)) {
		 gboolean live;
		 gst_query_parse_latency (q, &live, &minlat, &maxlat);
	}
	gst_query_unref (q);
	return maxlat;
}



//-------------------------------------------------
//----------------------------------------- videoUtils
//-------------------------------------------------



VideoTextureGstVideoUtils::VideoTextureGstVideoUtils(){
	bIsFrameNew					= false;
	bHavePixelsChanged			= false;
	bBackPixelsChanged			= false;
	GstMapInfo initMapinfo		= {0,};
	mapinfo 					= initMapinfo;
	internalPixelFormat			= GL_RGB;

	#ifdef USE_GST_GL
	glDisplay = NULL;
	glContext = NULL;
#endif
	copyPixels = false;
}

//------------------------------------------------------------------------------------

VideoTextureGstVideoUtils::~VideoTextureGstVideoUtils(){
	close();
}

//------------------------------------------------------------------------------------

void VideoTextureGstVideoUtils::close(){
	VideoTextureGstUtils::close();
	std::unique_lock<std::mutex> lock(mutex);
	pixels.clear();
	backPixels.clear();
	eventPixels.clear();
	bIsFrameNew					= false;
	bHavePixelsChanged			= false;
	bBackPixelsChanged			= false;
	frontBuffer.reset();
	backBuffer.reset();

	while(!bufferQueue.empty()) bufferQueue.pop();
}
//------------------------------------------------------------------------------------

bool VideoTextureGstVideoUtils::isInitialized() const{
	return isLoaded();
}

//------------------------------------------------------------------------------------

bool VideoTextureGstVideoUtils::isFrameNew() const{
	return bIsFrameNew;
}

//------------------------------------------------------------------------------------

ofPixels& VideoTextureGstVideoUtils::getPixels(){
	return pixels;
}

//------------------------------------------------------------------------------------

const ofPixels & VideoTextureGstVideoUtils::getPixels() const{
	return pixels;
}

//------------------------------------------------------------------------------------

TextureManager * VideoTextureGstVideoUtils::getTexture(){
#ifdef USE_GST_GL
	if(frontTexture.isAllocated()){
		return &frontTexture;
	}else{
		return NULL;
	}
#else
	return NULL;
#endif
}

//------------------------------------------------------------------------------------

void VideoTextureGstVideoUtils::update(){
	if (isLoaded()){
		if(!isFrameByFrame()){
			std::unique_lock<std::mutex> lock(mutex);
			bHavePixelsChanged = bBackPixelsChanged;
			if (bHavePixelsChanged){
				bBackPixelsChanged=false;
				swap(pixels,backPixels);
				#ifdef USE_GST_GL
				if(backTexture.isAllocated()){
					frontTexture.getTextureData() = backTexture.getTextureData();
					frontTexture.setTextureMinMagFilter(GL_LINEAR,GL_LINEAR);
					frontTexture.setTextureWrap(GL_CLAMP_TO_EDGE,GL_CLAMP_TO_EDGE);
				}
				#endif
				if(!copyPixels){
					frontBuffer = backBuffer;
				}
			}
		}else{

			GstBuffer * buffer;
			GstSample * sample;

			//get the buffer from appsink
			if(isPaused()){
				sample = gst_app_sink_pull_preroll (GST_APP_SINK (getSink()));
			}else{
				sample = gst_app_sink_pull_sample (GST_APP_SINK (getSink()));
			}
			buffer = gst_sample_get_buffer(sample);

			if(buffer){
				if(pixels.isAllocated()){
					gst_buffer_map (buffer, &mapinfo, GST_MAP_READ);
					//TODO: stride = mapinfo.size / height;
					pixels.setFromExternalPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getNumChannels());
					backBuffer = shared_ptr<GstSample>(sample,gst_sample_unref);
					bHavePixelsChanged=true;
					gst_buffer_unmap(buffer,&mapinfo);
				}
			}
		}
	}else{
		ofLogWarning("VideoTextureGstVideoUtils") << "update(): VideoTextureGstVideoUtils not loaded";
	}
	bIsFrameNew = bHavePixelsChanged;
	bHavePixelsChanged = false;
}

//------------------------------------------------------------------------------------

float VideoTextureGstVideoUtils::getHeight() const{
	return pixels.getHeight();
}

//------------------------------------------------------------------------------------

float VideoTextureGstVideoUtils::getWidth() const{
	return pixels.getWidth();
}

//------------------------------------------------------------------------------------

string::getGstFormatName(GLenum format){
	switch(format){
	case GL_RED:
		return "GRAY8";
	case GL_RGBA:
		return "RGBA";
	case GL_BGRA:
		return "BGRA";
	case GL_RGB565:
		return "RGB16";
	case GL_NV12:
		return "NV12";
	case GL_NV21:
		return "NV21";
	case GL_YV12:
		return "YV12";
	case GL_I420:
		return "I420";
	case GL_YUY2:
		return "YUY2";
	case GL_RGB:
		return "RGB";
	case GL_BGR:
		return "BGR";
	default:
		return "UNKNOWN";
	}
}
//------------------------------------------------------------------------------------

GstVideoFormat	VideoTextureGstVideoUtils::getGstFormat(GLenum format)
{
	switch(format)
	{
	case GL_RED:
		return GST_VIDEO_FORMAT_GRAY8;

	case GL_RGB:
		return GST_VIDEO_FORMAT_RGB;

	case GL_BGR:
		return GST_VIDEO_FORMAT_BGR;

	case GL_RGBA:
		return GST_VIDEO_FORMAT_RGBA;

	case GL_BGRA:
		return GST_VIDEO_FORMAT_BGRA;

	case GL_RGB565:
		return GST_VIDEO_FORMAT_RGB16;

	case GL_NV12:
		return GST_VIDEO_FORMAT_NV12;

	case GL_NV21:
		return GST_VIDEO_FORMAT_NV21;

	case GL_YV12:
		return GST_VIDEO_FORMAT_YV12;

	case GL_I420:
		return GST_VIDEO_FORMAT_I420;

	case GL_YUY2:
		return GST_VIDEO_FORMAT_YUY2;

	default:
		return GST_VIDEO_FORMAT_UNKNOWN;
	}
}

//------------------------------------------------------------------------------------

GLenum VideoTextureGstVideoUtils::getOFFormat(GstVideoFormat format)
{
	switch(format){
	case GST_VIDEO_FORMAT_GRAY8:
		return GL_GRAY;

	case GST_VIDEO_FORMAT_RGB:
		return GL_RGB;

	case GST_VIDEO_FORMAT_BGR:
		return GL_BGR;

	case GST_VIDEO_FORMAT_RGBA:
		return GL_RGBA;

	case GST_VIDEO_FORMAT_BGRA:
		return GL_BGRA;

	case GST_VIDEO_FORMAT_RGB16:
		return GL_RGB565;

	case GST_VIDEO_FORMAT_NV12:
		return GL_NV12;

	case GST_VIDEO_FORMAT_NV21:
		return GL_NV21;

	case GST_VIDEO_FORMAT_YV12:
		return GL_YV12;

	case GST_VIDEO_FORMAT_I420:
		return GL_I420;

	case GST_VIDEO_FORMAT_YUY2:
		return GL_YUY2;

	default:
		ofLogError() << "non supported format " << format;
		return GL_UNKNOWN;
	}
}

/*
gboolean VideoTextureGstVideoUtils::sync_bus_call (GstBus * bus, GstMessage * msg, gpointer data)
{
	switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_NEED_CONTEXT:
		{
			ofGstVideoPlayer * player = (ofGstVideoPlayer*)data;
			const gchar *context_type;

			gst_message_parse_context_type (msg, &context_type);
			ofLogNotice("ofGstVideoPlayer","got need context %s\n", context_type);

			if (g_strcmp0 (context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
				GstContext *display_context =
				gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
				gst_context_set_gl_display (display_context, player->glDisplay);
				//GstStructure *s = gst_context_writable_structure (display_context);
				//gst_structure_set (s, "context", GST_GL_TYPE_CONTEXT, player->glContext,	NULL);
				gst_element_set_context (GST_ELEMENT (msg->src), display_context);
				return TRUE;
			} else if (g_strcmp0 (context_type, "gst.gl.app_context") == 0) {
				GstContext *app_context = gst_context_new ("gst.gl.app_context", TRUE);
				GstStructure *s = gst_context_writable_structure (app_context);
				gst_structure_set (s, "context", GST_GL_TYPE_CONTEXT, player->glContext,	NULL);
				gst_element_set_context (GST_ELEMENT (msg->src), app_context);
				return TRUE;
			}
			break;
		}
		default:
		break;
	}
	return FALSE;
}*/

//------------------------------------------------------------------------------------

void VideoTextureGstVideoUtils::setCopyPixels(bool copy){
	copyPixels = copy;
}

//------------------------------------------------------------------------------------

bool VideoTextureGstVideoUtils::setPipeline(string pipeline, GLenum pixelFormat, bool isStream, int w, int h){
	internalPixelFormat = pixelFormat;
#ifndef USE_GST_GL
	string caps;

	if(pixelFormat!=GL_NATIVE){
		caps="video/x-raw, format="+getGstFormatName(pixelFormat);
	}else{
		caps = "video/x-raw,format={RGBA,BGRA,RGB,BGR,RGB16,GRAY8,YV12,I420,NV12,NV21,YUY2}";
	}

	if(w!=-1 && h!=-1){
		caps+=", width=" + ofToString(w) + ", height=" + ofToString(h);
	}

	string pipeline_string =
		pipeline + " ! appsink name=ofappsink enable-last-sample=0 caps=\"" + caps + "\"";

	if((w==-1 || h==-1) || pixelFormat==GL_NATIVE || allocate(w,h,pixelFormat)){
		return setPipelineWithSink(pipeline_string,"ofappsink",isStream);
	}else{
		return false;
	}
#else
	string pipeline_string =
		pipeline + " ! glcolorscale name=gl_filter ! appsink name=ofappsink enable-last-sample=0 caps=\"video/x-raw,format=RGBA\"";

	bool ret;
	if((w==-1 || h==-1) || pixelFormat==GL_NATIVE || allocate(w,h,pixelFormat)){
		ret = setPipelineWithSink(pipeline_string,"ofappsink",isStream);
	}else{
		ret = false;
	}

	auto glfilter = gst_bin_get_by_name(GST_BIN(getPipeline()),"gl_filter");

#if defined(TARGET_LINUX) && !defined(TARGET_OPENGLES)
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

	glXMakeCurrent (ofGetX11Display(), ofGetX11Window(), ofGetGLXContext());

#elif defined(TARGET_OPENGLES)

	cout << "current display " << ofGetEGLDisplay() << endl;
	eglMakeCurrent (eglGetDisplay(EGL_DEFAULT_DISPLAY), 0,0, 0);
	glDisplay = (GstGLDisplay *)gst_gl_display_egl_new_with_egl_display(eglGetDisplay(EGL_DEFAULT_DISPLAY));
	glContext = gst_gl_context_new_wrapped (glDisplay, (guintptr) ofGetEGLContext(),
	    		  GST_GL_PLATFORM_GLX, GST_GL_API_OPENGL);

	g_object_set (G_OBJECT (glfilter), "other-context", glContext, NULL);
	// FIXME: this seems to be the way to add the context in 1.4.5
	//
	// GstBus * bus = gst_pipeline_get_bus (GST_PIPELINE(gstPipeline));
	// gst_bus_enable_sync_message_emission (bus);
	// g_signal_connect (bus, "sync-message", G_CALLBACK (sync_bus_call), this);
	// gst_object_unref(bus);

	eglMakeCurrent (ofGetEGLDisplay(), ofGetEGLSurface(), ofGetEGLSurface(), ofGetEGLContext());

#endif

	return ret;
#endif
}

//------------------------------------------------------------------------------------

bool VideoTextureGstVideoUtils::setPixelFormat(GLenum pixelFormat){
	internalPixelFormat = pixelFormat;
	return true;
}

//------------------------------------------------------------------------------------

GLenum VideoTextureGstVideoUtils::getPixelFormat() const{
	return internalPixelFormat;
}

//------------------------------------------------------------------------------------

bool VideoTextureGstVideoUtils::allocate(int w, int h, GLenum pixelFormat){
	std::unique_lock<std::mutex> lock(mutex);

	if(pixelFormat!=internalPixelFormat){
		ofLogNotice("VideoTextureGstVideoUtils") << "allocating with " << w << "x" << h << " " << getGstFormatName(pixelFormat);
	}

	pixels.allocate(w,h,pixelFormat);
	backPixels.allocate(w,h,pixelFormat);
	pixels.set(0);
	backPixels.set(0);

	bHavePixelsChanged = false;
	bBackPixelsChanged = true;

	internalPixelFormat = pixelFormat;
	return pixels.isAllocated();
}

//------------------------------------------------------------------------------------

void VideoTextureGstVideoUtils::reallocateOnNextFrame(){
	std::unique_lock<std::mutex> lock(mutex);
	pixels.clear();
	backPixels.clear();
	bIsFrameNew					= false;
	bHavePixelsChanged			= false;
	bBackPixelsChanged			= false;
	frontBuffer.reset();
	backBuffer.reset();
	while(!bufferQueue.empty()) bufferQueue.pop();
}

//------------------------------------------------------------------------------------

static GstVideoInfo getVideoInfo(GstSample * sample){
    GstCaps *caps = gst_sample_get_caps(sample);
    GstVideoInfo vinfo;
    if(caps){
		gst_video_info_from_caps (&vinfo, caps);
    }else{
    	ofLogError() << "couldn't get sample caps";
    }
    return vinfo;
}

//------------------------------------------------------------------------------------

GstFlowReturn VideoTextureGstVideoUtils::process_sample(shared_ptr<GstSample> sample){
	GstBuffer * _buffer = gst_sample_get_buffer(sample.get());

#ifdef USE_GST_GL
	if (gst_buffer_map (_buffer, &mapinfo, (GstMapFlags)(GST_MAP_READ | GST_MAP_GL))){
		if (gst_is_gl_memory (mapinfo.memory)) {
			bufferQueue.push(sample);
			gst_buffer_unmap(_buffer, &mapinfo);
			bool newTexture=false;
			std::unique_lock<std::mutex> lock(mutex);
			while(bufferQueue.size()>2){
				backBuffer = bufferQueue.front();
				bufferQueue.pop();
				newTexture = true;
			}
			if(newTexture){
				GstBuffer * _buffer = gst_sample_get_buffer(backBuffer.get());
				gst_buffer_map (_buffer, &mapinfo, (GstMapFlags)(GST_MAP_READ | GST_MAP_GL));
				auto texId = *(guint*)mapinfo.data;
				backTexture.setUseExternalTextureID(texId);
				TextureManagerData & texData = backTexture.getTextureData();
				texData.bAllocated = true;
				texData.bFlipTexture = false;
				texData.glTypeInternal = GL_RGBA;
				texData.height = getHeight();
				texData.width = getWidth();
				texData.magFilter = GL_LINEAR;
				texData.minFilter = GL_LINEAR;
				texData.tex_h = getHeight();
				texData.tex_w = getWidth();
				texData.tex_u = 1;
				texData.tex_t = 1;
				texData.textureID = texId;
				texData.textureTarget = GL_TEXTURE_2D;
				texData.wrapModeHorizontal = GL_CLAMP_TO_EDGE;
				texData.wrapModeVertical = GL_CLAMP_TO_EDGE;
				bBackPixelsChanged=true;
				gst_buffer_unmap(_buffer,&mapinfo);
			}
			return GST_FLOW_OK;
		}
	}
#endif

	// video frame has normal texture
	gst_buffer_map (_buffer, &mapinfo, GST_MAP_READ);
	guint size = mapinfo.size;

	int stride = 0;
	if(pixels.isAllocated() && pixels.getTotalBytes()!=(int)size){
		GstVideoInfo v_info = getVideoInfo(sample.get());
		stride = v_info.stride[0];

		if(stride == (pixels.getWidth() * pixels.getBytesPerPixel())) {
			ofLogError("VideoTextureGstVideoUtils") << "buffer_cb(): error on new buffer, buffer size: " << size << "!= init size: " << pixels.getTotalBytes();
			return GST_FLOW_ERROR;
		}
	}
	mutex.lock();
	if(!copyPixels){
		backBuffer = sample;
	}

	if(pixels.isAllocated()){
		if(stride > 0) {
			if(pixels.getPixelFormat() == GL_I420){
				GstVideoInfo v_info = getVideoInfo(sample.get());
				std::vector<int> strides{v_info.stride[0],v_info.stride[1],v_info.stride[2]};
				backPixels.setFromAlignedPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getPixelFormat(),strides);
			} else {
				backPixels.setFromAlignedPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getPixelFormat(),stride);
			}
		} else if(!copyPixels){
			backPixels.setFromExternalPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getPixelFormat());
			eventPixels.setFromExternalPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getPixelFormat());
		}else{
			backPixels.setFromPixels(mapinfo.data,pixels.getWidth(),pixels.getHeight(),pixels.getPixelFormat());
		}

		bBackPixelsChanged=true;
		mutex.unlock();
		if(stride == 0) {
			ofNotifyEvent(prerollEvent,eventPixels);
		}
	}else{
		mutex.unlock();
		if(appsink){
			appsink->on_stream_prepared();
		}else{
			GstVideoInfo v_info = getVideoInfo(sample.get());
			allocate(v_info.width,v_info.height,getOFFormat(v_info.finfo->format));
		}
	}
	gst_buffer_unmap(_buffer, &mapinfo);
	return GST_FLOW_OK;
}

//------------------------------------------------------------------------------------

GstFlowReturn VideoTextureGstVideoUtils::preroll_cb(shared_ptr<GstSample> sample){
	GstFlowReturn ret = process_sample(sample);
	if(ret==GST_FLOW_OK){
		return VideoTextureGstUtils::preroll_cb(sample);
	}else{
		return ret;
	}
}

//------------------------------------------------------------------------------------

GstFlowReturn VideoTextureGstVideoUtils::buffer_cb(shared_ptr<GstSample> sample){
	GstFlowReturn ret = process_sample(sample);
	if(ret==GST_FLOW_OK){
		return VideoTextureGstUtils::buffer_cb(sample);
	}else{
		return ret;
	}
}

//------------------------------------------------------------------------------------

void VideoTextureGstVideoUtils::eos_cb(){
	VideoTextureGstUtils::eos_cb();
	ofEventArgs args;
	ofNotifyEvent(eosEvent,args);
}

#endif

}
