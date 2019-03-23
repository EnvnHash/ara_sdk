#pragma once

#include <headers/gl_header.h>
#include <GLUtils/TextureManager.h>


//#include "ofConstants.h"
//#include "ofBaseTypes.h"
//#include "uint8_t.h"
//#include "ofTypes.h"
//#include "ofEvents.h"
//#include "ofThread.h"

#define GST_DISABLE_DEPRECATED
#include <gst/gst.h>
#include <gst/gstpad.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <glib-object.h>
#include <glib.h>

#include <algorithm>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <thread>

//#define USE_GST_GL
#ifdef USE_GST_GL
#define GST_USE_UNSTABLE_API
#include <gst/gl/gl.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if GST_VERSION_MAJOR>0
#include <gst/video/gstvideometa.h>
#endif



class GstAppSink;
typedef struct _GstElement GstElement;
typedef struct _GstBuffer GstBuffer;
typedef struct _GstMessage GstMessage;


#define guint long
#define gint64 long

//-------------------------------------------------
//----------------------------------------- VideoTextureGstUtils
//-------------------------------------------------

using namespace std;

namespace tav
{

class VideoTextureGstUtils{
public:
	enum LoopType { LOOP };

	VideoTextureGstUtils();
	virtual ~VideoTextureGstUtils();

	bool 	setPipelineWithSink(string pipeline, string sinkname="sink", bool isStream=false);
	bool 	setPipelineWithSink(GstElement * pipeline, GstElement * sink, bool isStream=false);
	bool	startPipeline();

	void 	play();
	void 	stop();
	void 	setPaused(bool bPause);
	bool 	isPaused() const {return bPaused;}
	bool 	isLoaded() const {return bLoaded;}
	bool 	isPlaying() const {return bPlaying;}

	float	getPosition() const;
	float 	getSpeed() const;
	float 	getDuration() const;
	int64_t  getDurationNanos() const;
	bool  	getIsMovieDone() const;

	void 	setPosition(float pct);
	void 	setVolume(float volume);
	void 	setLoopState(LoopType state);
	LoopType	getLoopState() const {return loopMode;}
	void 	setSpeed(float speed);

	void 	setFrameByFrame(bool bFrameByFrame);
	bool	isFrameByFrame() const;

	GstElement 	* getPipeline() const;
	GstElement 	* getSink() const;
	GstElement 	* getGstElementByName(const string & name) const;
	uint64_t getMinLatencyNanos() const;
	uint64_t getMaxLatencyNanos() const;

	virtual void close();

	void setSinkListener(GstAppSink * appsink);

	// callbacks to get called from gstreamer
	virtual GstFlowReturn preroll_cb(shared_ptr<GstSample> buffer);
	virtual GstFlowReturn buffer_cb(shared_ptr<GstSample> buffer);
	virtual void 		  eos_cb();

	static void startGstMainLoop();
	static GMainLoop * getGstMainLoop();
	static void quitGstMainLoop();
protected:
	GstAppSink * 		appsink;
	bool				isStream;
	bool				closing;

private:
	static bool			busFunction(GstBus * bus, GstMessage * message, VideoTextureGstUtils * app);
	bool				gstHandleMessage(GstBus * bus, GstMessage * message);

	bool 				bPlaying;
	bool 				bPaused;
	bool				bIsMovieDone;
	bool 				bLoaded;
	bool 				bFrameByFrame;
	LoopType			loopMode;

	GstElement  *		gstSink;
	GstElement 	*		gstPipeline;

	float				speed;
	mutable gint64		durationNanos;
	bool				isAppSink;
	std::condition_variable		eosCondition;
	std::mutex			eosMutex;
	guint				busWatchID;

	class GstMainLoopThread: public std::thread{
	public:
		GstMainLoopThread() : main_loop(NULL)
		{
		}

		void start(){
			main_loop = new std::thread(&threadedFunction, this);
			main_loop->detach();
		}

		void threadedFunction(){
			while(run) {

			}
		}

		std::thread * getMainLoop(){
			return main_loop;
		}

		void quit(){
			run = false;
			main_loop->join();
		}
	private:
		std::thread *main_loop;
		bool run = true;
	};

	static GstMainLoopThread * mainLoop;
};


//-------------------------------------------------
//----------------------------------------- videoUtils
//-------------------------------------------------

class VideoTextureGstVideoUtils: public VideoTextureGstUtils { // public ofBaseVideo
public:

	VideoTextureGstVideoUtils();
	virtual ~VideoTextureGstVideoUtils();

	bool 			setPipeline(string pipeline, GLenum pixelFormat = GL_RGB, bool isStream=false, int w=-1, int h=-1);

	bool 			setPixelFormat(GLenum pixelFormat);
	GLenum 	getPixelFormat() const;
	bool 			allocate(int w, int h, GLenum pixelFormat);
	void 			reallocateOnNextFrame();

	bool 			isFrameNew() const;
	uint8_t&		getPixels();
	const uint8_t&	getPixels() const;
	TextureManager * 	getTexture();
	void 			update();

	float 			getHeight() const;
	float 			getWidth() const;

	void 			close();

	static string			getGstFormatName(GLenum format);
	static GstVideoFormat	getGstFormat(GLenum format);
	static GLenum	getOFFormat(GstVideoFormat format);

	bool			isInitialized() const;

	// copy pixels from gst buffer to avoid
	// https://bugzilla.gnome.org/show_bug.cgi?id=737427
	void setCopyPixels(bool copy);

	// this events happen in a different thread
	// do not use them for opengl stuff
	//ofEvent<uint8_t> prerollEvent;
	//ofEvent<uint8_t> bufferEvent;
	//ofEvent<ofEventArgs> eosEvent;

protected:

	GstFlowReturn process_sample(shared_ptr<GstSample> sample);
	GstFlowReturn preroll_cb(shared_ptr<GstSample> buffer);
	GstFlowReturn buffer_cb(shared_ptr<GstSample> buffer);
	void			eos_cb();


	uint8_t		pixels;				// 24 bit: rgb
	uint8_t		backPixels;
	uint8_t		eventPixels;
private:
	static gboolean	sync_bus_call (GstBus * bus, GstMessage * msg, gpointer data);
	bool			bIsFrameNew;			// if we are new
	bool			bHavePixelsChanged;
	bool			bBackPixelsChanged;
	std::mutex		mutex;


	shared_ptr<GstSample> 	frontBuffer, backBuffer;
	queue<shared_ptr<GstSample> > bufferQueue;
	GstMapInfo mapinfo;
	#ifdef USE_GST_GL
		TextureManager		frontTexture, backTexture;
	#endif

	GLenum	internalPixelFormat;
	bool copyPixels; // fix for certain versions bug with v4l2

#ifdef USE_GST_GL
	GstGLDisplay *		glDisplay;
	GstGLContext *		glContext;
#endif
};


//-------------------------------------------------
//----------------------------------------- appsink listener
//-------------------------------------------------

class GstAppSink{
public:
	virtual ~GstAppSink(){}

	virtual GstFlowReturn on_preroll(shared_ptr<GstSample> buffer){
		return GST_FLOW_OK;
	}
	virtual GstFlowReturn on_buffer(shared_ptr<GstSample> buffer){
		return GST_FLOW_OK;
	}

	virtual void			on_eos(){}

	// return true to set the message as attended so upstream doesn't try to process it
	virtual bool on_message(GstMessage* msg){return false;};

	// pings when enough data has arrived to be able to get sink properties
	virtual void on_stream_prepared(){};
};

}

