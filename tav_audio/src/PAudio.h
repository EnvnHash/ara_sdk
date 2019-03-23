/*
 *  PAudio.h
 *  PEBRE
 *
 *  Created by Sven Hahne on 25.05.11.
 *  Copyright 2011 the PEBRE Community. All rights reserved.
 *
 */

#pragma once

#ifndef _PAUDIO_
#define _PAUDIO_

#include <sstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include <portaudio.h>

#ifdef __APPLE__
#include <pa_mac_core.h>
#elif __linux__
#include <pa_jack.h>
#endif

#include "PaPhaseLockLoop.h"
#include "AubioAnalyzer.h"
#include "MultiChanFFT.h"

#define PAUDIO_POSTPROC

using namespace tav;

class PAudio
{
public :

	typedef float SAMPLE;

	typedef struct {
		unsigned int							codecNrChans;
		int										frameCount;
		int										frameIndex;
		int										maxFrameIndex;
		int										numChannels;
		int										offset;
		std::vector< std::vector<unsigned int> >* mixDownMap;
		std::vector<float>* 					inSampQ;
		SAMPLE**								sampData;
	} paPostProcData;

	typedef struct
	{
		bool									doUpdate;
		bool									buffersFilled;
		bool*									isWriting;
		bool*									isReady;
		bool*									writeToDisk;
		int										blockCounter;
		int										codecFrameSize;
		unsigned int							codecNrChans;
		int										frameIndex;
		int										frames_per_buffer;
		int										maxFrameIndex;
		int										numChannels;
		int										outNumChannels;
		int										rec_buf_size;
		SAMPLE**								sampData;
		SAMPLE*									mixBuf;
		std::vector<float>*						inSampQ;
		std::vector< std::vector<unsigned int> >* mixDownMap;
		std::mutex*								inMutex;
		paPostProcData*							postProcData;
		std::function<void(paPostProcData*)> 	mediaRecCb;
		std::vector< std::function<void(float*, bool*, bool*)> >	sndFilePlCb;
		std::vector< std::function<float(int, int)> >		sndFileSampCb;
	    std::condition_variable* 				postProcCond;
	} paAudioInData;


	PAudio(std::mutex *_mutex, bool* _isReady, int _frameSize, int _rec_buf_size,
			int _fft_size, int _sample_rate, int _monRefreshRate, int _maxNrChans);
	~PAudio();

	void    		initAudio();
	void 			postProcLoop();

	void 			start();
	void 			join();

	void    		pause();
	void    		resume();
	void    		recordToFile(std::string path);
	void    		stopRecordToFile();

	bool    		getIsReady() { return *isReady; }

	float   		getActFftBin(int chanNr, int binNr);
	float*  		getActFft(int chanNr);
	float** 		getActFft();
	float*  		getActPhases(int chanNr);
	int 			getBlockCount() { return waveData.blockCounter; }
	bool    		getCatchedOnset(int chanNr);
	float   		getCatchedAmp(int chanNr);
	float*  		getFft(int chanNr, int timeOffs);
	float   		getFft(int chanNr, int timeOffs, int sampNr);
	unsigned int 	getFrameNr();
	unsigned int 	getFramesPerBuffer() { return frames_per_buffer; }
	float   		getLoHiRelation(int chanNr);
	int     		getMaxNrInChannels();
	float   		getMedAmp(int chanNr);
	int     		getNrSepBands();
	bool    		getOnset(int chanNr);
	float   		getOnsetAmp(int chanNr);
	float** 		getPll();
	float* 		 	getPllAt(int chanNr);
	float   		getPllAtPos(int chanNr, float pos);
	float   		getPllSepBandAtPos(int chanNr, int bandNr, float pos);
	float* 			getPllSepBand(int chanNr, int bandNr);
	float   		getPitch(int chanNr);
	unsigned int 	getOutNrChannels();
	int     		getSampleRate() { return sample_rate; }
	float   		getSampDataAtPos(int chanNr, float pos);
	float*  		getSmoothSpectr(int chanNr);
	float   		getSmoothSpectrAt(int chanNr, float bandInd);
	float   		getSepBandEnergy(int chanNr, int bandNr);

	void 			setPllDataMed(float _val);
	void 			setPllLoP(float _val);
	/*void    		setExtCallback(void (*cbType)(unsigned int numChans, int frames, int offset, int codecFrameSize, unsigned int codecNrChans,
						std::vector< std::vector<unsigned int> >* mixDownMap, float** sampData, std::vector<float>* inSampQ),
						std::vector<float>* sampQ, int _codecFrameSize, unsigned int _codecNrChans, std::vector< std::vector<unsigned int> >* mixDownMap);
						*/

	void    		setExtCallback(std::function<void(paPostProcData*)> _cb);

	//void	setSndFilePlayerCb(std::function<void(float*, bool*)> _sndFilePlCb);
	//void	setSndFilePlayerSampCb(std::function<float(int, int)> _sndFileSampCb);


	void			addSndFilePlayerCb(std::function<void(float*, bool*, bool*)> _sndFilePlCb);
	void			addSndFilePlayerSampCb(std::function<float(int, int)> _sndFileSampCb);

	void    		removeExtCallback();

private:
	static int streamCallback(
			const void *input, void *output,
			unsigned long frameCount,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void *userData);

	constexpr static const float 	SAMPLE_SILENCE = 0.0f;
	static const PaSampleFormat 	pa_sample_type = paFloat32;

	PaStream*           			stream;
	const PaDeviceInfo* 			devInfo;

	AubioAnalyzer*					abios;
	AubioAnalyzer*      			abAnalyzer;
	MultiChanFFT*					ffts;

	PaPhaseLockLoop*				pll;
	PaPhaseLockLoop**       		sepBandPll;
	paPostProcData					postProcData;
	paAudioInData 					waveData;

	SAMPLE**						dataToFft;
	SAMPLE**						sampData;
	SAMPLE*							mixBuf;

	std::thread 					m_Thread;
	std::thread 					postProc_thread;
	std::mutex*						mutex;
	std::mutex						pp_mutex;
    std::condition_variable 		postProcCond;

	bool        					hasInputDevice;
	bool							doPostProc;
	bool    						isPlaying;
	bool        					isWriting;
	bool							writeToDisk;

	bool*       					isReady;

	int                 			counter;
	int								fftUpdtFreq;
	int         					fft_size;
	int         					frames_per_buffer;
	int 							maxInputChannels;
	int         					monRefreshRate;
	int                     		nrSepBands;
	int         					rec_buf_size; // multiples of frames_per_buffer
	int         					sample_rate;

	float*                     		medVol;
	float                       	volumeMed;
};

#endif
