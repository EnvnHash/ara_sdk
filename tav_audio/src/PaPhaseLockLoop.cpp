/*
 *  PaPhaseLockLoop.cpp
 *  ta_visualizer
 *
 *  Created by user on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 * die ersten zwei samples werden ignoriert, weil sie durch den lopass filter unstetigkeiten ergeben können,
 * wenn sich die wellenform innerhalb des buffers wiederholt
 *
 * geringste mögliche frequenz samplerate / frames_per_buffer * 2.0
 *
 */
#include <iostream>
#include <math.h>

#include "PaPhaseLockLoop.h"

//--------------------------------------------------------------

PaPhaseLockLoop::PaPhaseLockLoop(int _framesPerBuffer, int _numChans)
{
	preLoP = 0.14f;
	loP = 0.13f;
	dataMed = 2.0f;
	compareTolerance = 0.3;
	fadeSmooth = 28.0f; //geht gar nicht meht wenn über 8
	numRefVal = 3;

	ifftMed = 1.0f;
	numChans = _numChans;
	framesPerBuffer = _framesPerBuffer;
	sampsPerWaveLoop = _framesPerBuffer;

	tempSampAr =		new float*[numChans];
	nOLSampsBufSize =	10;
	nOLSampsBuf =		new int*[numChans];
	nOLSampsBufPtr =	new int[numChans]; 
	loPassAr =			new float*[numChans];
	pllData =			new float*[numChans];
	pllData2 =			new float*[numChans];
	maxVol =			new float[numChans];
	minAmp =			new float[numChans];

	zeros.resize(numChans);

	for (int i=0;i<numChans;i++)
	{		
		tempSampAr[i] =		new float[framesPerBuffer];
		loPassAr[i] =		new float[framesPerBuffer];
		nOLSampsBuf[i] =	new int[nOLSampsBufSize];
		pllData[i] =		new float[framesPerBuffer];
		pllData2[i] =		new float[framesPerBuffer];
		nOLSampsBufPtr[i] = 0;
		maxVol[i] =			0.0f;
		minAmp[i] =			0.0f;

		for (int j=0; j<nOLSampsBufSize; j++) 
			nOLSampsBuf[i][j] = 0;

		for (int j=0; j<framesPerBuffer; j++) 
		{
			pllData[i][j] = 0.0f;
			pllData2[i][j] = 0.0f;
			loPassAr[i][j] = 0.0f;
			tempSampAr[i][j] = 0.0f;
		}		
	}

	pllInd = 0;
	fadeVal = 0.0f;
}

//--------------------------------------------------------------

PaPhaseLockLoop::~PaPhaseLockLoop() 
{
	for (int i=0;i<numChans;i++)
	{
		delete [] tempSampAr;
		delete [] nOLSampsBuf;
		delete [] nOLSampsBufPtr;
		delete [] pllData2;
		delete [] pllData;
		delete [] loPassAr;
	}	
	delete tempSampAr;
	delete nOLSampsBuf;
	delete nOLSampsBufPtr;
	delete loPassAr;
	delete pllData2;
	delete pllData;
}

//--------------------------------------------------------------

void PaPhaseLockLoop::getIfft(float* waveData, int chanNr, int nrSamples)
{
	maxVol[chanNr] = 0.0f;
	minAmp[chanNr] = 0.0f;

	for (int i=0; i<framesPerBuffer; i++) 
	{
		pllInd = static_cast<float> (i) / static_cast<float> (framesPerBuffer - 1);		

		float fWaveDataInd = (nrSamples -1) * pllInd;		

		lowerSrcInd = floor( fWaveDataInd );
		upperSrcInd = fmin( lowerSrcInd + 1.0f, static_cast<float> (framesPerBuffer - 1) );

		weight = fWaveDataInd - lowerSrcInd;

		if ( weight < 0.0000001f ) 
		{
			newSamp = waveData[ static_cast<int>(lowerSrcInd) ];
		} else {
			newSamp = waveData[static_cast<int>(lowerSrcInd)] * (1.0f - weight) 
							+ waveData[static_cast<int>(upperSrcInd)] * weight;
		}

		pllData[chanNr][i] = ( 
				( newSamp * 1.0f )
				+ (pllData[chanNr][i] * ifftMed)
		)
		/ ( ifftMed + 1.0f );

		if ( fabs( pllData[chanNr][i] ) > maxVol[chanNr] ) maxVol[chanNr] = fabs( pllData[chanNr][i] );
		if ( pllData[chanNr][i] < minAmp[chanNr] ) minAmp[chanNr] = pllData[chanNr][i];
	}	
}

//--------------------------------------------------------------

float PaPhaseLockLoop::getAmpAtPos(int chanNr, float pos)
{
	float fInd = pos * static_cast<float>(sampsPerWaveLoop);
	int lInd = static_cast<int>(fInd);
	float floor = static_cast<float>(lInd);
	float diff = fInd - floor;
	float v1 = pllData[chanNr][lInd];
	float v2 = pllData[chanNr][(lInd+1) % sampsPerWaveLoop];
	return v1 * (1.f - diff) + v2 * diff;
}

//--------------------------------------------------------------

float* PaPhaseLockLoop::getPllAt(int chanNr)
{
	return pllData[chanNr];
}

//--------------------------------------------------------------

float** PaPhaseLockLoop::getPll()
{
	return pllData;
}

//--------------------------------------------------------------

void PaPhaseLockLoop::phaseLock(float** waveData)
{
	float lastVal = 0.0f;
	float newVal = 0.0f;
	float numOfLoopSamps;
	float lastSamp = 0.0f;

	// die null punkte werden anhand des gefilterten signals bestimmt -> weniger sprünge
	// brutale glättung
	for (int chanNr=0; chanNr<numChans; chanNr++) 
	{
		// lopass auf das was rein kommt
		for (int i=0; i<framesPerBuffer; i++) 
		{
			//loPassAr[chanNr][i] = waveData[chanNr][i];
			if ( i>0 ) loPassAr[chanNr][i] = (loPassAr[chanNr][i-1] * preLoP) + waveData[chanNr][i] * (1.0f - preLoP);
		}


		// such die nulldurchgänge
		zeros[chanNr].clear();

		int j = 1;
		lastVal = loPassAr[chanNr][0];

		while ( j < framesPerBuffer ) 
		{
			newVal = loPassAr[chanNr][j];
			if ( lastVal < 0.0f && newVal >= 0.0f ) zeros[chanNr].push_back(j);
			j++;
			lastVal = newVal;
		}		

		// wenn nulldurchgänge gefunden rechne die maximale länge verfügbarer samples aus
		if ( zeros[chanNr].size() > 0 )
		{	
			if ( zeros[chanNr].size() > 1 ) 
			{
				numOfLoopSamps = static_cast<float>(zeros[chanNr].back() - zeros[chanNr].front());
			} else {
				numOfLoopSamps = static_cast<float>(framesPerBuffer - zeros[chanNr].front());
			}
			if ( numOfLoopSamps <= 0 ) numOfLoopSamps = framesPerBuffer;			


			// ausreisser beseitigung, wenn die neue länge sich signifikant unterscheidet 
			// und dieser unterschied nur bei den letzten zwei werten nicht vorhanden war, 
			// nimm dieses loop nicht

			// such den unterschied zu den letzten x einträgen
			double sum = 0.0;
			for (int i=0; i<numRefVal; i++) 
			{
				sum += fabs ( 
						(
								static_cast<double> (numOfLoopSamps)
								/ static_cast<double> (
										nOLSampsBuf[chanNr][
															(
																	nOLSampsBufPtr[chanNr]
																				   - i
																				   + nOLSampsBufSize
															)
															% nOLSampsBufSize
															]
								)
						) - 1.0
				);
			}
			sum /= static_cast<double> (numRefVal);

			maxVol[chanNr] = 0.0f;

			if ( sum < compareTolerance || nOLSampsBuf[chanNr][4] == 0 )
			{
				// kopier die samples zwischen den looppunkten, interpolier sie auf alle verfügbaren plätze
				for (int i=0;i<sampsPerWaveLoop;i++ )
				{
					ind = static_cast<float>(i) / static_cast<float>(sampsPerWaveLoop-1);
					lowerSrcInd = floor( ind * numOfLoopSamps );
					upperSrcInd = lowerSrcInd + 1.0;
					weight = ind * numOfLoopSamps - lowerSrcInd;

					upperSamp = waveData[chanNr][static_cast<int>(upperSrcInd) + zeros[chanNr].front()];
					lowerSamp = waveData[chanNr][static_cast<int>(lowerSrcInd) + zeros[chanNr].front()];

					newSamp = lowerSamp * (1.0f - weight) + upperSamp * weight;
					if ( i > 0 ) newSamp = (lastSamp * loP) + newSamp * (1.0f - loP);
					pllData2[chanNr][i] = ( newSamp + (pllData2[chanNr][i] * dataMed) ) / (dataMed + 1.0f);

					// nach max lautstärke checken
					if ( fabs( pllData2[chanNr][i] ) > maxVol[chanNr] ) maxVol[chanNr] = fabs( pllData2[chanNr][i] );

					tempSampAr[chanNr][i] = newSamp;
					lastSamp = newSamp;
				}
			} else {
				for (int i=0;i<sampsPerWaveLoop;i++ )
				{
					pllData2[chanNr][i] = ( 
							tempSampAr[chanNr][i]
											   + (pllData2[chanNr][i] * fadeSmooth)
					)
					/ ( fadeSmooth + 1.0f );

					// nach max lautstärke checken
					if ( fabs( pllData2[chanNr][i] ) > maxVol[chanNr] ) maxVol[chanNr] = fabs( pllData2[chanNr][i] );
				}
			}

			// speicher die letzte gefundene länge 
			nOLSampsBufPtr[chanNr] = (nOLSampsBufPtr[chanNr] + 1) % nOLSampsBufSize;
			nOLSampsBuf[chanNr][ nOLSampsBufPtr[chanNr] ] = numOfLoopSamps;
		}
	}
}

//--------------------------------------------------------------

void PaPhaseLockLoop::setDataMed(float _val)
{
	dataMed = _val;
}

//--------------------------------------------------------------

void PaPhaseLockLoop::setLoP(float _val)
{
	ifftMed = (_val) * 15.f;
}
