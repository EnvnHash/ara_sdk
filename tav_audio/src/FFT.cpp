/*
 *  FFT.cpp
 *  tav
 *
 *  Created by user on 01.08.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 *	benutzt libfftw (3.3.3)
 */

#include "FFT.h"

namespace tav
{

FFT::FFT(int _signalSize, fftWindowType _windowType) : signalSize(_signalSize), windowType(_windowType)
{
	// more info on setting up a forward r2r fft here:
	// http://www.fftw.org/fftw3_doc/Real_002dto_002dReal-Transforms.html
	fftIn = (float*) fftwf_malloc(sizeof(float) * signalSize);
	fftOut = (float*) fftwf_malloc(sizeof(float) * signalSize);
	fftPlan = fftwf_plan_r2r_1d(signalSize, fftIn, fftOut, FFTW_R2HC,
	                            FFTW_DESTROY_INPUT | FFTW_MEASURE);
	
	// the difference between setting up an r2r ifft and fft
	// is using the flag/kind FFTW_HC2R instead of FFTW_R2HC:
	// http://www.fftw.org/fftw3_doc/Real_002dto_002dReal-Transform-Kinds.html
	ifftIn = (float*) fftwf_malloc(sizeof(float) * signalSize);
	ifftOut = (float*) fftwf_malloc(sizeof(float) * signalSize);
	ifftPlan = fftwf_plan_r2r_1d(signalSize, ifftIn, ifftOut, FFTW_HC2R,
	                             FFTW_DESTROY_INPUT | FFTW_MEASURE);
	
	binSize = (signalSize / 2) + 1;
	
	signalNormalized = true;
	signal = new float[signalSize];
	
	cartesianUpdated = true;
	cartesianNormalized = true;
	real = new float[binSize];
	imag = new float[binSize];
	
	polarUpdated = true;
	polarNormalized = true;
	amplitude = new float[binSize];
	phase = new float[binSize];
	
	clear();
	
	window = new float[signalSize];
	inverseWindow = new float[signalSize];
	setWindowType(windowType);	
}

//--------------------------------------------------------------

void FFT::executeFft() 
{
	std::memcpy(fftIn, signal, sizeof(float) * signalSize);
	runWindow(fftIn);
	fftwf_execute(fftPlan);
	// explanation of halfcomplex format:
	// http://www.fftw.org/fftw3_doc/The-Halfcomplex_002dformat-DFT.html
	copyReal(fftOut);
	imag[0] = 0;
	for (int i = 1; i < binSize; i++)
		imag[i] = fftOut[std::max(signalSize - i, 0)];
	cartesianUpdated = true;
}

//--------------------------------------------------------------

void FFT::executeIfft() 
{
	std::memcpy(ifftIn, real, sizeof(float) * binSize);
	for (int i = 1; i < binSize; i++)
        ifftIn[std::max(signalSize - i, 0)] = imag[i];
	fftwf_execute(ifftPlan);
	runInverseWindow(ifftOut);
	copySignal(ifftOut);
}

//--------------------------------------------------------------

void FFT::setWindowType(fftWindowType windowType) 
{
	this->windowType = windowType;
	if(windowType == PBR_FFT_WINDOW_RECTANGULAR) {
		for(int i = 0; i < signalSize; i++)
			window[i] = 1; // only used for windowSum
	} else if(windowType == PBR_FFT_WINDOW_BARTLETT) {
		int half = signalSize / 2;
		for (int i = 0; i < half; i++) {
			window[i] = ((float) i / half);
			window[i + half] = (1 - ((float) i / half));
		}
	} else if(windowType == PBR_FFT_WINDOW_HANN) {
		for(int i = 0; i < signalSize; i++)
			window[i] = .5 * (1 - cos((TWO_PI * i) / (signalSize - 1)));
	} else if(windowType == PBR_FFT_WINDOW_HAMMING) {
		for(int i = 0; i < signalSize; i++)
			window[i] = .54 - .46 * cos((TWO_PI * i) / (signalSize - 1));
	} else if(windowType == PBR_FFT_WINDOW_SINE) {
		for(int i = 0; i < signalSize; i++)
			window[i] = sin((M_PI * i) / (signalSize - 1));
	}
	
	windowSum = 0;
	for(int i = 0; i < signalSize; i++)
		windowSum += window[i];
	
	for(int i = 0; i < signalSize; i++)
		inverseWindow[i] = 1. / window[i];
}

//--------------------------------------------------------------

void FFT::clear()
{
	memset(signal, 0, sizeof(float) * signalSize);
	memset(real, 0, sizeof(float) * binSize);
	memset(imag, 0, sizeof(float) * binSize);
	memset(amplitude, 0, sizeof(float) * binSize);
	memset(phase, 0, sizeof(float) * binSize);
}

//--------------------------------------------------------------

void FFT::copySignal(const float* signal)
{
	std::memcpy(this->signal, signal, sizeof(float) * signalSize);
}

//--------------------------------------------------------------

void FFT::copyReal(float* real)
{
	std::memcpy(this->real, real, sizeof(float) * binSize);
}

//--------------------------------------------------------------

void FFT::copyImaginary(float* imag)
{
	if(imag == NULL)
		memset(this->imag, 0, sizeof(float) * binSize);
	else
		std::memcpy(this->imag, imag, sizeof(float) * binSize);
}

//--------------------------------------------------------------

void FFT::copyAmplitude(float* amplitude)
{
	std::memcpy(this->amplitude, amplitude, sizeof(float) * binSize);
}

//--------------------------------------------------------------

void FFT::copyAmplitudePart(float lowerLim, float upperLim, float* amplitude)
{
    unsigned int binOffs = static_cast<unsigned int>(std::pow(lowerLim, 4.0) * static_cast<float>(binSize));
    float* startPtr = &amplitude[binOffs];
    float* dstPtr = &this->amplitude[binOffs];
    
    unsigned int newBinSize = static_cast<unsigned int>((std::pow(upperLim, 4.0) - std::pow(lowerLim, 4.0)) * static_cast<float>(binSize));
    std::memcpy(dstPtr, startPtr, sizeof(float) * newBinSize);
}

//--------------------------------------------------------------

void FFT::copyPhase(float* phase)
{
    if (phase == NULL) {
		memset(this->phase, 0, sizeof(float) * binSize);
    } else {
		std::memcpy(this->phase, phase, sizeof(float) * binSize);
    }
}

//--------------------------------------------------------------

void FFT::copyPhasePart(float lowerLim, float upperLim, float* phase)
{
    // lower and upper limit are converted log to lin (aprox.)
    
    if (phase == NULL) {
        memset(this->phase, 0, sizeof(float) * binSize);
    } else
    {
        unsigned int binOffs = static_cast<unsigned int>(std::pow(lowerLim, 4.0) * static_cast<float>(binSize));
        float* startPtr = &phase[binOffs];
        float* dstPtr = &this->phase[binOffs];
        
        unsigned int newBinSize = static_cast<unsigned int>((std::pow(upperLim, 4.0) - std::pow(lowerLim, 4.0)) * static_cast<float>(binSize));
        std::memcpy(dstPtr, startPtr, sizeof(float) * newBinSize);
    }
}

//--------------------------------------------------------------

void FFT::prepareSignal()
{
	if(!signalUpdated)
		updateSignal();
	if(!signalNormalized)
		normalizeSignal();
}

//--------------------------------------------------------------

void FFT::updateSignal()
{
	prepareCartesian();
	executeIfft();
	signalUpdated = true;
	signalNormalized = false;
}

//--------------------------------------------------------------

void FFT::normalizeSignal()
{
	float normalizer = (float) windowSum / (2 * signalSize);
	for (int i = 0; i < signalSize; i++)
		signal[i] *= normalizer;
	signalNormalized = true;
}

//--------------------------------------------------------------

float* FFT::getSignal()
{
	prepareSignal();
	return signal;
}

//--------------------------------------------------------------

void FFT::clampSignal()
{
	prepareSignal();
	for(int i = 0; i < signalSize; i++) {
		if(signal[i] > 1)
			signal[i] = 1;
		else if(signal[i] < -1)
			signal[i] = -1;
	}
}

//--------------------------------------------------------------

void FFT::prepareCartesian()
{
	if(!cartesianUpdated) {
		if(!polarUpdated)
			executeFft();
		else
			updateCartesian();
	}
	if(!cartesianNormalized)
		normalizeCartesian();
}

//--------------------------------------------------------------

float* FFT::getRawFFT()
{
    return fftOut;
}

//--------------------------------------------------------------

float* FFT::getReal()
{
	prepareCartesian();
	return real;
}

//--------------------------------------------------------------

float* FFT::getImaginary()
{
	prepareCartesian();
	return imag;
}

//--------------------------------------------------------------

void FFT::preparePolar()
{
	if(!polarUpdated)
		updatePolar();
	if(!polarNormalized)
		normalizePolar();
}

//--------------------------------------------------------------

float* FFT::getAmplitude()
{
	//preparePolar();
	return amplitude;
}

//--------------------------------------------------------------

float* FFT::getPhase()
{
	preparePolar();
	return phase;
}

//--------------------------------------------------------------

float FFT::getAmplitudeAtBin(float bin)
{
	float* amplitude = getAmplitude();
	int lowBin = clamp(floorf(bin), 0, binSize - 1);
	int highBin = clamp(ceilf(bin), 0, binSize - 1);
	return map(bin, (float)lowBin, (float)highBin, amplitude[lowBin], amplitude[highBin]);
}

//--------------------------------------------------------------

float FFT::getBinFromFrequency(float frequency, float sampleRate)
{
	return frequency * binSize / (sampleRate / 2);
}

//--------------------------------------------------------------

float FFT::getFrequencyFromBin(int bin, float sampleRate)
{
	return (static_cast<float>(bin) * ( static_cast<float>(sampleRate) * 0.5f ) ) / static_cast<float>(binSize);
}

//--------------------------------------------------------------

float FFT::getAmplitudeAtFrequency(float frequency, float sampleRate)
{
	return getAmplitudeAtBin(getBinFromFrequency(frequency, sampleRate));
}

//--------------------------------------------------------------

float FFT::getMaxAmp()
{
    float maxAmp = 0.0f;
    for(int i=0; i<binSize; i++)
        if (amplitude[i] > maxAmp) maxAmp = amplitude[i];
    return maxAmp;
}

//--------------------------------------------------------------

void FFT::updateCartesian()
{
	for(int i = 0; i < binSize; i++) {
		real[i] = cosf(phase[i]) * amplitude[i];
		imag[i] = sinf(phase[i]) * amplitude[i];
	}
	cartesianUpdated = true;
	cartesianNormalized = polarNormalized;
}

//--------------------------------------------------------------

void FFT::imagToZero()
{
	for(int i = 0; i < binSize; i++) imag[i] = 0.0f;
//	cartesianUpdated = true;
//	cartesianNormalized = polarNormalized;
}

//--------------------------------------------------------------

void FFT::normalizeCartesian()
{
	float normalizer = 2. / windowSum;
	for(int i = 0; i < binSize; i++) {
		real[i] *= normalizer;
		imag[i] *= normalizer;
	}
	cartesianNormalized = true;
}

//--------------------------------------------------------------

void FFT::updatePolar()
{
	prepareCartesian(); // hier wird executeFft aufgerufen
	for(int i = 0; i < binSize; i++) {
		amplitude[i] = cartesianToAmplitude(real[i], imag[i]);
		phase[i] = cartesianToPhase(real[i], imag[i]);
	}
	polarUpdated = true;
	polarNormalized = cartesianNormalized;
}

//--------------------------------------------------------------

void FFT::normalizePolar()
{
	float normalizer = 2. / windowSum;
	for(int i = 0; i < binSize; i++)
		amplitude[i] *= normalizer;
	polarNormalized = true;
}

//--------------------------------------------------------------

void FFT::clearUpdates()
{
	cartesianUpdated = false;
	polarUpdated = false;
	cartesianNormalized = false;
	polarNormalized = false;
	signalUpdated = false;
	signalNormalized = false;
}

//--------------------------------------------------------------

void FFT::setSignal(const std::vector<float>& signal)
{
	setSignal(&signal[0]);
}

//--------------------------------------------------------------

void FFT::setSignal(const float* signal)
{
	clearUpdates();
	copySignal(signal);
	signalUpdated = true;
	signalNormalized = true;
}

//--------------------------------------------------------------

void FFT::setBinAmp(unsigned int bandNr, float amp)
{
    amplitude[bandNr] = amp;
}

//--------------------------------------------------------------

void FFT::setCartesian(float* real, float* imag)
{
	clearUpdates();
	copyReal(real);
	copyImaginary(imag);
	cartesianUpdated = true;
	cartesianNormalized = true;
}

//--------------------------------------------------------------

void FFT::setPolar(float* amplitude, float* phase)
{
	clearUpdates();
	copyAmplitude(amplitude);
	copyPhase(phase);
	polarUpdated = true;
	polarNormalized = true;
}

//--------------------------------------------------------------

void FFT::setPolarPart(float lowerLim, float upperLim, float* amplitude, float* phase)
{
    clearUpdates();
    copyAmplitudePart(lowerLim, upperLim, amplitude);
    copyPhasePart(lowerLim, upperLim, phase);
    polarUpdated = true;
    polarNormalized = true;
}

//--------------------------------------------------------------

void FFT::setRawFFT(float* _rawFftIn)
{
    std::memcpy(fftOut, _rawFftIn, signalSize * sizeof(float));
}

//--------------------------------------------------------------

int FFT::getBinSize()
{
	return binSize;
}

//--------------------------------------------------------------

int FFT::getSignalSize()
{
	return signalSize;
}

//--------------------------------------------------------------

float FFT::clamp(float value, float min, float max) {
	return value < min ? min : value > max ? max : value;
}

//--------------------------------------------------------------

//check for division by zero???
//--------------------------------------------------
float FFT::map(float value, float inputMin, float inputMax, float outputMin, float outputMax, bool clamp)
{
	if (fabs(inputMin - inputMax) < FLT_EPSILON){
		std::cout << "ofMap(): avoiding possible divide by zero, check inputMin and inputMax: " << inputMin << " " << inputMax;
		return outputMin;
	} else {
		float outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);

		if( clamp ){
			if(outputMax < outputMin){
				if( outVal < outputMax )outVal = outputMax;
				else if( outVal > outputMin )outVal = outputMin;
			}else{
				if( outVal > outputMax )outVal = outputMax;
				else if( outVal < outputMin )outVal = outputMin;
			}
		}
		return outVal;
	}
}

//--------------------------------------------------------------

FFT::~FFT() 
{
	if (fftPlan != NULL) 
	{
		fftwf_destroy_plan(fftPlan);
		fftwf_free(fftIn);
		fftwf_free(fftOut);
		
		fftwf_destroy_plan(ifftPlan);
		fftwf_free(ifftIn);
		fftwf_free(ifftOut);
		fftwf_cleanup();
	}
	delete [] signal;
	delete [] real;
	delete [] imag;
	delete [] amplitude;
	delete [] phase;
	delete [] window;
	delete [] inverseWindow;
}
}
