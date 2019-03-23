/*
 * SoundFilePlayer.h
 *
 *  Created on: Jun 7, 2018
 *      Author: sven
 */

#ifndef SRC_SOUNDFILEPLAYER_H_
#define SRC_SOUNDFILEPLAYER_H_

#include <iostream>
#include <functional>
#include <sndfile.hh>
#include <cmath>
#include <mutex>
#include <thread>
#include <limits.h>
#include <string.h>

namespace tav
{

class SoundFilePlayer
{
public:
	SoundFilePlayer(unsigned int _hwBufferFrameSize, unsigned int _hwNrChannels);
	virtual ~SoundFilePlayer();

	void open_file(const char * fname);
	void play();
	void stop();

	virtual void readThread(unsigned int* ptr, SndfileHandle* _sndFile);

	void close_file();
	void pa_play_cb(float* outPtr, bool* routeToIn, bool* _add);
	float pa_play_samp_cb(int framePosPtr, int channel);

	bool isPlaying();
	float getDuration();

	void setLoop(bool value);
	void setRouteToIn(bool value) { routeToIn = value; }
	void setStopCallback(std::function<void(void)> _cb, double _offset) {
		stopCallbackCalled = false; stopCbOffset = _offset; stopCallback = _cb;
	}
	void setVolume(float vol);

	void myNanoSleep(uint32_t ns);

private:
	bool						loop;
	bool						playing;
	bool						routeToIn;
	bool						idle;
	bool						debug;
	bool						stopCallbackCalled;

	short**						buffer;
	float**						fbuffer;
	float*						f0buffer;

	float						vol;

	double						stopCbOffset;

	SndfileHandle*				file;
	std::string					filename;
	unsigned int				fileNrFrames;
	float						fileDuration;
	unsigned int				fileNrChannels;
	unsigned int				fileSampleRate;

	unsigned int				hwBufferFrameSize;
	unsigned int 				hwNrChannels;

	unsigned int 				redBlocks;
	unsigned int 				nrBufferedBlocks;
	unsigned int 				readBufPtr;
	unsigned int 				writeBufPtr;

	unsigned int				nrPreloadFrames;

	std::thread*				mReadThread;
	std::mutex					mtx;

	float 						shortLimitInv;

	std::function<void(void)>	stopCallback;
};

} /* namespace tav */

#endif /* SRC_SOUNDFILEPLAYER_H_ */
