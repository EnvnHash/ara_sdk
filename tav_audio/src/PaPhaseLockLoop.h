/*
 *  PaPhaseLockLoop.h
 *  PEBRE
 *
 *  Created by Sven Hahne on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#include <vector>

#ifndef _PAPHASELOCKLOOP_
#define _PAPHASELOCKLOOP_

class PaPhaseLockLoop
{
	public :
		PaPhaseLockLoop(int _framesPerBuffer, int _numChans);
		~PaPhaseLockLoop();
		void phaseLock(float** waveData );
		void getIfft(float* waveData, int chanNr, int nrSamples);
        float getAmpAtPos(int chanNr, float pos);
        float* getPllAt(int chanNr);
        float** getPll();
        void setDataMed(float _val);
        void setLoP(float _val);

        int sampsPerWaveLoop;
		float** pllData;
		float** pllData2;
		float* maxVol;
		float* minAmp;
	private :
		int numChans;
		int framesPerBuffer;
		std::vector< std::vector<int> > zeros;
		float** loPassAr;
		float** tempSampAr;
		int** nOLSampsBuf;
		int* nOLSampsBufPtr;
		int nOLSampsBufSize;
		float preLoP;
		float loP;
		float dataMed;
		double compareTolerance;
		float fadeSmooth;
		int numRefVal;
		float ifftMed;
		float ind, upperSrcInd, lowerSrcInd, weight, lowerSamp, upperSamp, newSamp;
		float fadeVal;
		float pllInd;
};
#endif
