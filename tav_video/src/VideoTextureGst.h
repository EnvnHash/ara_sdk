/*
 * VIdeoTextureGst.h
 *
 *  Created on: Jun 29, 2018
 *      Author: sven
 */

#ifndef SRC_VIDEOTEXTUREGST_H_
#define SRC_VIDEOTEXTUREGST_H_


#include <string>

#include <header/gl_header.h>
#include <GLUtils/TextureManager.h>

#include "VideoTextureGstUtils.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
//#include "ofConstants.h"
//#include "ofGstUtils.h"


namespace tav
{

class VideoTextureGst
{
public:


	VideoTextureGst();
	virtual ~VideoTextureGst();

	/// needs to be called before loadMovie
	bool 	setPixelFormat(GLenum pixelFormat);
	GLenum	getPixelFormat() const;

	void	loadAsync(std::string name);
	bool 	load(std::string uri);

	void 	update();

	int		getCurrentFrame() const;
	int		getTotalNumFrames() const;

	void 	firstFrame();
	void 	nextFrame();
	void 	previousFrame();
	void 	setFrame(int frame);  // frame 0 = first frame...

	bool	isStream() const;

	void 	play();
	void 	stop();
	void 	setPaused(bool bPause);
	bool 	isPaused() const;
	bool 	isLoaded() const;
	bool 	isPlaying() const;

	float	getPosition() const;
	float 	getSpeed() const;
	float 	getDuration() const;
	bool  	getIsMovieDone() const;

	void 	setPosition(float pct);
	void 	setVolume(float volume);
	void 	setLoopState(VideoTextureGst::LoopType state);
	VideoTextureGst::LoopType 	getLoopState() const;
	void 	setSpeed(float speed);
	void 	close();

	bool 			isFrameNew() const;

	uint8_t&		getPixels();
	const uint8_t&  getPixels() const;
	TextureManager* getTexturePtr();

	float 			getHeight() const;
	float 			getWidth() const;

	void setFrameByFrame(bool frameByFrame);
	void setThreadAppSink(bool threaded);
	bool isThreadedAppSink() const;
	bool isFrameByFrame() const;

	//ofGstVideoUtils * getGstVideoUtils();

protected:
	bool allocate();
	bool createPipeline(std::string uri);
	void on_stream_prepared();

	// return true to set the message as attended so upstream doesn't try to process it
	virtual bool on_message(GstMessage* msg){return false;};

private:
	GLenum					internalPixelFormat;
	long					nFrames;
	int 					fps_n, fps_d;
	bool					bIsStream;
	bool					bIsAllocated;
	bool					bAsyncLoad;
	bool					threadAppSink;
	VideoTextureGstUtils	videoUtils;
};

} /* namespace tav */

#endif /* SRC_VIDEOTEXTUREGST_H_ */
