/*
 * SoundFilePlayer.cpp
 *
 *  Created on: Jun 7, 2018
 *      Author: sven
 *
 *  Player using libsndfile (GNU lGPL v2 and v3), has to be registered in PAudio instance with setSndFilePlayerCb
 *
 *	reads Microsoft WAV
 *	SGI / AppleAIFF / AIFC
 *	Sun / DEC / NeXT AU / SND
 *	Headerless RAW
 *	Paris Audio File PAF
 *	Commodore Amiga IFF / SVX
 *	Sphere Nist WAV
 *	IRCAM SF
 *	Creative VOC
 *	Sound forge W64	GNU
 *	Octave 2.0 MAT4
 *	GNU Octave 2.1 MAT5
 *	Portable Voice Format PVF
 *	Fasttracker 2 XI
 *	HMM Tool Kit HTK
 *	Apple CAF
 *	Sound Designer II SD2
 *	Free Lossless Audio Codec FLAC
 *
 */

#include "SoundFilePlayer.h"

namespace tav
{

SoundFilePlayer::SoundFilePlayer(unsigned int _hwBufferFrameSize, unsigned int _hwNrChannels) :
		file(NULL), hwBufferFrameSize(_hwBufferFrameSize), hwNrChannels(_hwNrChannels),
		readBufPtr(0), writeBufPtr(0), nrPreloadFrames(8), buffer(NULL), playing(false),
		mReadThread(NULL), loop(false), routeToIn(false), idle(false), vol(1.f),
		stopCallback(NULL), f0buffer(NULL), fbuffer(NULL), debug(true), stopCbOffset(0)
{
	shortLimitInv = 1.f / (float)SHRT_MAX;
}

//----------------------------------------------------

void SoundFilePlayer::open_file(const char * fname)
{
	mtx.lock();

	vol = 0;
	playing = false;
	filename = std::string(fname);
	std::cout << "SoundFilePlayer::open_file " << filename << std::endl;

	if(file)
	{
		std::cout << "close_file from open_file" << std::endl;
		delete file;
	}

	file = new SndfileHandle(fname);

	if (debug)
	{
		printf ("SoundFilePlayer::open_file: Opened file '%s'\n", fname) ;
		printf ("    Sample rate : %d\n", file->samplerate ()) ;
		printf ("    Channels    : %d\n", file->channels ()) ;
		printf ("    Frames      : %d\n", file->frames ()) ;
	}

	fileNrChannels = file->channels();
	fileNrFrames = (unsigned int) file->frames();
	fileDuration = (float)file->frames() / (float)file->samplerate(); // in secs
	fileSampleRate = file->samplerate();

	if (debug)
		printf ("    Length      : %d%d:%d%d min\n", (int)(fileDuration / 600.f), (int)(fileDuration / 60.f)%10,
			(int)std::fmod(fileDuration, 60.f)/10, (int)std::fmod(fileDuration, 60.f)% 10 ) ;

	// allocate memory
	if (!buffer && !fbuffer)
	{
		buffer = new short*[nrPreloadFrames];
		fbuffer = new float*[nrPreloadFrames];
		for (unsigned int i=0; i<nrPreloadFrames; i++){
			buffer[i] = new short[hwBufferFrameSize * fileNrChannels];
			fbuffer[i] = new float[hwBufferFrameSize * fileNrChannels];
		}
	}

	if (!f0buffer) {
		f0buffer = new float[hwBufferFrameSize * fileNrChannels];
		memset(f0buffer, 0, hwBufferFrameSize * fileNrChannels * sizeof(float));
	}

	readBufPtr = 0;
	writeBufPtr = 0;
	redBlocks = 0;
	nrBufferedBlocks = 0;

	mtx.unlock();
}

//----------------------------------------------------

void SoundFilePlayer::play()
{
	mtx.lock();
	playing = true;
	idle = false;

	std::cout << "play" << std::endl;

		// start buffer thread
	if (mReadThread && mReadThread->joinable())
	{
		mReadThread->join();
		delete mReadThread;
	}


	mReadThread = new std::thread(&SoundFilePlayer::readThread, this, &readBufPtr, file);
	mReadThread->detach();

	mtx.unlock();

}

//----------------------------------------------------

void SoundFilePlayer::readThread(unsigned int* ptr, SndfileHandle* _sndFile)
{
	while (playing)
	{
		mtx.lock();

		// call stop callback, can be earlier than end by stopCbOffset
		unsigned int endFrameOffset = static_cast<unsigned int>(stopCbOffset * (double)fileSampleRate
				/ (double)hwBufferFrameSize);


		if ((redBlocks +1) * hwBufferFrameSize >= (fileNrFrames - endFrameOffset))
			if(stopCallback && !stopCallbackCalled) {
				stopCallbackCalled = true;
				stopCallback();
			}

		// check if we have still frames left to load, if not we are at the end of the file
		if ((redBlocks +1) * hwBufferFrameSize < fileNrFrames)
		{
			// if we have less than nrPreloadFrames -1 preloaded, continue loading
			if (nrBufferedBlocks < nrPreloadFrames-1)
			{
				_sndFile->read(buffer[*ptr], hwBufferFrameSize * fileNrChannels); // knackst ohne +2 ...

				// convert buffer
				float* outPtr = &fbuffer[*ptr][0];
				short* inPtr = &buffer[*ptr][0];
				for (int i=0; i<hwBufferFrameSize * fileNrChannels; i++)
					*outPtr++ = (float)(*inPtr++) * shortLimitInv * vol;

				*ptr = (*ptr +1) % nrPreloadFrames;
				redBlocks++;

				nrBufferedBlocks++;
			}
			//else myNanoSleep(1000);

		} else
		{
			playing = false;

			// if loop requested, reopen the file
			if (loop)
			{
				if (_sndFile) delete _sndFile;	// delete file handle
				_sndFile = new SndfileHandle(filename.c_str());
				redBlocks = 0;
				playing = true;
			}
		}

		mtx.unlock();
	}
}

//----------------------------------------------------

void SoundFilePlayer::stop()
{
	mtx.lock();

	if (playing)
	{
		playing = false;
		idle = true;

		if (mReadThread && mReadThread->joinable())
			mReadThread->join();

		delete mReadThread;
		mReadThread = 0;

		if(stopCallback) stopCallback();

	}
	mtx.unlock();

}

//----------------------------------------------------

void SoundFilePlayer::close_file()
{
	mtx.lock();

	if (file)
	{
		vol = 0;
		std::cout << "close_file" << std::endl;
		playing = false;

		delete file;	// delete file handle
		file = NULL;
	}

	redBlocks = 0;
	nrBufferedBlocks = 0;
	idle = true;

	mtx.unlock();

}

//----------------------------------------------------

void SoundFilePlayer::pa_play_cb(float* outPtr, bool* _routeToIn, bool* _add)
{
	if (file && nrBufferedBlocks >= nrPreloadFrames-1)
	{
		mtx.lock();

		*_routeToIn = routeToIn;

		if (playing)
		{
			if (*_add)
			{
				for (unsigned int i=0; i<hwBufferFrameSize; i++)
					for (unsigned int j=0; j<fileNrChannels; j++)
						*(outPtr++) += (float)buffer[writeBufPtr][i*fileNrChannels +j] * shortLimitInv * vol;
			} else {
				for (unsigned int i=0; i<hwBufferFrameSize; i++)
					for (unsigned int j=0; j<fileNrChannels; j++)
						*(outPtr++) = (float)buffer[writeBufPtr][i*fileNrChannels +j] * shortLimitInv * vol;
			}
		}


		if (!*_add) *_add = true;

		//if (hwNrChannels <= fileNrChannels) {
//			memcpy(outPtr, &fbuffer[writeBufPtr][0], fileNrChannels * hwBufferFrameSize * sizeof(float));
		//else {

		/*
			// if we have more channels in the file than hardware channels, mixdown the file
			unsigned short fileChansPerHWChans = (unsigned short)fileNrChannels / (unsigned short)hwNrChannels;

			for (unsigned int i=0; i<hwBufferFrameSize; i++)
			{
				for (unsigned int j=0; j<fileNrChannels; j+fileChansPerHWChans)
				{
					for (int k=0; k<fileChansPerHWChans; k++)
					{
						unsigned short fChanInd = (j * fileChansPerHWChans) + k;
						if (fChanInd < fileNrChannels)
						{
							if (k == 0)
								*outPtr += (float)buffer[writeBufPtr][i * fChanInd] * shortLimitInv;
							else
								*outPtr += (float)buffer[writeBufPtr][i * fChanInd] * shortLimitInv;
						}
					}
					outPtr++;
				}
			}
			*/
		//}

		nrBufferedBlocks--;
		writeBufPtr = (writeBufPtr +1) % nrPreloadFrames;


		mtx.unlock();

	} else if (file)
	{
		if (*_add)
		{
			for (unsigned int i=0; i<hwBufferFrameSize; i++)
				for (unsigned int j=0; j<fileNrChannels; j++)
					*(outPtr++) += 0.f;
		} else {
			for (unsigned int i=0; i<hwBufferFrameSize; i++)
				for (unsigned int j=0; j<fileNrChannels; j++)
					*(outPtr++) = 0.f;
		}
	}
}

//----------------------------------------------------

float SoundFilePlayer::pa_play_samp_cb(int framePosPtr, int channel)
{

	//mtx.lock();

	if (file && playing && nrBufferedBlocks >= nrPreloadFrames-1)
	{
		if (hwNrChannels == fileNrChannels)
			return fbuffer[writeBufPtr][framePosPtr * fileNrChannels + channel];
		else
			return 0.f;
	} else return 0.f;

	//mtx.unlock();

}

//----------------------------------------------------

bool SoundFilePlayer::isPlaying()
{
	return playing;
}

//----------------------------------------------------

float SoundFilePlayer::getDuration()
{
	return fileDuration;
}

//----------------------------------------------------

void SoundFilePlayer::setLoop(bool value)
{
	loop = value;
}

//----------------------------------------------------

void SoundFilePlayer::setVolume(float _vol){
	vol = _vol;
}

//----------------------------------------------------

void SoundFilePlayer::myNanoSleep(uint32_t ns)
{
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = (long)ns;
	nanosleep(&tim, NULL);
}

//----------------------------------------------------

SoundFilePlayer::~SoundFilePlayer()
{
	close_file();
	if (f0buffer) delete f0buffer;
	if (buffer) delete [] buffer;
	if (fbuffer) delete [] fbuffer;
}

} /* namespace tav */
