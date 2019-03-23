/*
 *  MultiChanFFT.h
 *  PEBRE
 *
 *  Created by user on 01.08.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#ifndef _MULTICHANFFT_
#define _MULTICHANFFT_

#include <thread>
#include <mutex>
#include <cmath>
#include <numeric>
#include "FFT.h"

namespace tav
{

class MultiChanFFT
{
public :
	MultiChanFFT(int _fftSize, int _nrChan, int _sample_rate);
	~MultiChanFFT();

	void    execFFT(int chanNr, float** waveData);
	void    detectOnSet(int chanNr);
	void    procSepBands(int chanNr);

	void    forwZeroBack(float** waveData);
	void    pForZeroBack(FFT** ffts, int chanNr, float** waveData);

	void    execIFFT(int chanNr);
	void    execSepBandIFFT(int chanNr);
	void    setPhaseToZero(int chanNr);
	void    setPhaseToZeroMed(int chanNr);
	void    setSepBandPhaseToZeroMed(int chanNr);

	float*  getSmoothSpectr(int chanNr);
	float   getSmoothSpecBin(int chanNr, int binNr);
	float   getSepBandEnergy(int chanNr, int bandNr);
	float   getLoHiRelation(int chanNr);

	float**     demirrored;
	float***    deMirroredSepBand;


	FFT**       fft;
	FFT***      sepBandFFt;
	int         nrSepBands;

	float*      onsets;
	bool*       catchedOnset;
	float*      onsetAmp;
	float*      catchedAmp;
	bool        doFFT;
	int         binSize;
	float***    amplitudes;
	float**     curFft;
	float***    medFft;
	float**     actMedFft;
	float**     actPhases;
	float*      intrpDiff;
	int*        intrpUpperInds;
	int*        intrpLowerInds;
	float       fftMed;
	float       freqDist;
	float       scaleAmp;
	int         maxFreq;
	int         fftSize;
	int         loopCounter;
	int         nrAmpBuffers;
	int*        ampBufferPtr;

private :

	void            processSignal(int chanNr);
	int             nrChan;
	int             sample_rate;
	float**         newAmp;
	float**         newPhase;
	float**         sepBandRelEnergy;
	float*          loHiRelation;
	float*          phases;
	float*          ampSum;
	float           fInd;
	float           deMirrorMed;
	std::thread** 	_threads;
	std::thread** 	fft_Threads;

	//    OnsetsDS*       ods;
	//    onsetsds_odf_types odftype;
	float**         odsdata;
	float           relaxtime;
};
}

#endif
