/*
 *	Aubio.h
 *	PEBRE
 *
 *	Created by Sven Hahne on 20.08.14.
 *	Copyright (c) 2014 the PEBRE community. All rights reserved.
 */

#pragma once

#include <aubio/aubio.h>
#include <vector>

namespace tav
{

class AubioAnalyzer
{
public:
	AubioAnalyzer(unsigned int _nrChans, unsigned int _hop_s, unsigned int _win_s, unsigned int _sampRate);
	~AubioAnalyzer();
	void            process(unsigned int chanNr, float** waveData);
	float           getPitch(unsigned int chanNr);
	bool            getOnset(unsigned int chanNr);

	bool*           catchedOnset;
	float*          catchedAmp;
private:
	unsigned int    n = 0; // frame counter
	unsigned int    win_s = 1024; // window size
	unsigned int    hop_s; // hop size
	unsigned int    nrChans = 2;
	unsigned int    samplerate = 44100; // samplerate
	fvec_t**        input;
	fvec_t**        output;
	aubio_pitch_t** p;
	aubio_onset_t** o;

	float*          pitch;
	bool*           onset;
	float           pitchMed;
	float           onset_threshold;
	float           silence_threshold;
};

}
