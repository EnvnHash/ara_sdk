/*
 *  FFT.h
 *	PEBRE
 *
 *  Created by Sven Hahne on 01.08.13.
 *  Copyright 2013 the PEBRE Community. All rights reserved.
 *
 *  in parts adapted from ofxFft
 */

#pragma once

#ifndef _FFT_
#define _FFT_

#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>
#include <float.h>
#include "fftw3.h"

#ifndef PI
#define PI       3.14159265358979323846
#endif

#ifndef TWO_PI
#define TWO_PI   6.28318530717958647693
#endif

enum fftWindowType
{
	PBR_FFT_WINDOW_RECTANGULAR,
	PBR_FFT_WINDOW_BARTLETT,
	PBR_FFT_WINDOW_HANN,
	PBR_FFT_WINDOW_HAMMING,
	PBR_FFT_WINDOW_SINE
};

enum fftImplementation
{
	PBR_FFT_BASIC,
	PBR_FFT_FFTW
};

namespace tav
{

class FFT
{
public :
	FFT(int signalSize = 512, fftWindowType windowType = PBR_FFT_WINDOW_HAMMING);
	~FFT();
	void        setSignal(const std::vector<float>& signal);
	void        setSignal(const float* signal);
	void        setBinAmp(unsigned int bandNr, float amp);
	void        setRawFFT(float* _rawFftIn);
	float*      getSignal();
	float*      getAmplitude();
	int         getBinSize();
	float*      getRawFFT();
	float*      getReal();
	float*      getImaginary();
	float*      getPhase();
	int         getSignalSize();
	float       getAmplitudeAtBin(float bin);
	float       getBinFromFrequency(float frequency, float sampleRate = 44100);
	float       getFrequencyFromBin(int bin, float sampleRate = 44100);
	float       getAmplitudeAtFrequency(float frequency, float sampleRate = 44100);
	float       getMaxAmp();

	void        executeIfft();
	float*      ifftOut;
	void        setPolar(float* amplitude, float* phase = NULL);
	void        setPolarPart(float lowerLim, float upperLim, float* amplitude, float* phase = NULL);
	void        normalizePolar();
	void updateCartesian();
	void        normalizeCartesian();
	void        prepareCartesian();
	void updateSignal();
	void        imagToZero();
	void        clampSignal();
	float clamp(float value, float min, float max);
	float map(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp=false);
	float*      signal;
	// frequency domain data and methods
	int signalSize, binSize;

	void preparePolar();

private :
	float *fftIn, *fftOut, *ifftIn;
	fftwf_plan fftPlan, ifftPlan;

	void setCartesian(float* real, float* imag = NULL);

protected:
	void executeFft();

	void clear();

	// time domain data and methods
	fftWindowType windowType;
	float windowSum;
	float *window, *inverseWindow;

	void setWindowType(fftWindowType windowType);

	inline void runWindow(float* signal) {
		if(windowType != PBR_FFT_WINDOW_RECTANGULAR)
			for(int i = 0; i < signalSize; i++)
				signal[i] *= window[i];
	}

	inline void runInverseWindow(float* signal) {
		if(windowType != PBR_FFT_WINDOW_RECTANGULAR)
			for(int i = 0; i < signalSize; i++)
				signal[i] *= inverseWindow[i];
	}

	bool signalUpdated, signalNormalized;
	void prepareSignal();
	void normalizeSignal();
	void copySignal(const float* signal);
	void copySignal(const float* signal, int offset, int blockSize);

	float *real, *imag;
	bool cartesianUpdated, cartesianNormalized;
	void copyReal(float* real);
	void copyImaginary(float* imag);

	float *amplitude, *phase;
	bool polarUpdated, polarNormalized;
	void updatePolar();
	void copyAmplitude(float* amplitude);
	void copyAmplitudePart(float lowerLim, float upperLim, float* amplitude);
	void copyPhase(float* phase);
	void copyPhasePart(float lowerLim, float upperLim, float* phase);

	void clearUpdates();

	inline float cartesianToAmplitude(float x, float y) {
		return sqrtf(x * x + y * y);
	}

	inline float cartesianToPhase(float x, float y) {
		return atan2f(y, x);
	}
};

}

#endif
