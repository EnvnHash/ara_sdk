/*
 *  MultiChanFFT.cpp
 *
 *  Created by Sven Hahne on 01.08.13.
 *  Copyright 2013 the PEBRE community. All rights reserved.
 *
 */

#include "MultiChanFFT.h"

namespace tav
{
MultiChanFFT::MultiChanFFT(int _fftSize, int _nrChan, int _sample_rate) : 
		fftSize(_fftSize), nrChan(_nrChan), sample_rate(_sample_rate)
{
	doFFT = false;
	fft = new FFT*[nrChan];
	sepBandFFt = new FFT**[nrChan];

	onsets = new float[nrChan];
	catchedOnset = new bool[nrChan];
	onsetAmp = new float[nrChan];
	catchedAmp = new float[nrChan];
	ampSum = new float[nrChan];
	sepBandRelEnergy = new float*[nrChan];
	loHiRelation = new float[nrChan];

	//    ods = new OnsetsDS[nrChan];
	//    odsdata = new float*[nrChan];

	fftMed = 2.0f;
	deMirrorMed = 2.0f;
	//	fftMed = 6.0f;
	//	deMirrorMed = 4.0f;
	scaleAmp = 1.5f; // im bezug auf die fft magnitudes

	freqDist = 6.0f; // je höher desto weiter rutschen die Tiefen nach oben
	maxFreq = 14000;
	relaxtime = 0.5f; // relax time for onset detection
	fInd = 0.0f;
	loopCounter = 0;

	nrSepBands = 4;

	for (int i=0; i<nrChan; i++)
	{
		fft[i] = new FFT(fftSize, PBR_FFT_WINDOW_HAMMING);
		sepBandFFt[i] = new FFT*[nrSepBands];
		for (int j=0; j<nrSepBands; j++)
			sepBandFFt[i][j] = new FFT(fftSize, PBR_FFT_WINDOW_HAMMING);

		onsets[i] = false;
		catchedOnset[i] = false;
		onsetAmp[i] = 0.f;
		catchedAmp[i] = 0.f;
		ampSum[i] = 0.f;
		loHiRelation[i] = 0.f;
		sepBandRelEnergy[i] = new float[nrSepBands];

		/*
        // init onsetds
        // choose detector type
        ods[i].floor = 0.001f; // lowest value
        ods[i].thresh = 0.15f; // threshold
        ods[i].mingap = 0; // minimum pause between detections in fftsize
        odftype = ODS_ODF_PHASE;
        // Allocate contiguous memory using malloc or whatever is reasonable.
        odsdata[i] = (float*) malloc( onsetsds_memneeded(odftype, fftSize, 11) ); // median span 11
        // Now initialise the OnsetsDS struct and its associated memory
        onsetsds_init(&ods[i], odsdata[i], ODS_FFT_FFTW3_HC, odftype, fftSize, 11, static_cast<float>(_sample_rate));
        onsetsds_setrelax(&ods[i], relaxtime, binSize);
		 */
	}

	binSize = fft[0]->getBinSize();


	//interpRawWave = new float*[nrChan];

	demirrored = new float*[nrChan];
	deMirroredSepBand = new float**[nrChan];

	curFft = new float*[nrChan];

	nrAmpBuffers = 6;
	ampBufferPtr = new int[nrChan];
	medFft = new float**[nrAmpBuffers];

	for ( int i=0;i<nrAmpBuffers;i++ ) 
	{
		medFft[i] = new float*[nrChan];
		for (int j=0; j<nrChan; j++) 
		{
			ampBufferPtr[j] = 0;
			medFft[i][j] = new float[binSize];
			for (int k=0;k<binSize; k++) medFft[i][j][k] = 0.0f;
		}
	}

	newAmp = new float*[nrChan];
	newPhase = new float*[nrChan];
	actMedFft = new float*[nrChan];
	actPhases = new float*[nrChan];

	intrpDiff = new float[binSize];
	intrpUpperInds = new int[binSize];	
	intrpLowerInds = new int[binSize];

	phases = new float[binSize];

	for (int i=0; i<nrChan; i++)
	{
		actMedFft[i] = new float[binSize];
		actPhases[i] = new float[binSize];
		newAmp[i] = new float[binSize];
		newPhase[i] = new float[binSize];
		demirrored[i] = new float[binSize -1];
		deMirroredSepBand[i] = new float*[nrSepBands];
		for (int j=0; j<nrSepBands; j++)
		{
			deMirroredSepBand[i][j] = new float[binSize -1];
			for (int k=0; k<binSize; k++)
				if ( k < binSize -2 ) deMirroredSepBand[i][j][k] = 0.0f;
		}

		curFft[i] = new float[binSize];

		for (int j=0; j<binSize; j++)
		{
			actMedFft[i][j] = 0.0f;
			actPhases[i][j] = 0.0f;
			newAmp[i][j] = 0.0f;
			newPhase[i][j] = -0.5f * PI;
			curFft[i][j] = 0.0f;
			if ( j < binSize -2 ) demirrored[i][j] = 0.0f;
		}
	}


	// init phases of sepband fft
	for (int i=0; i<nrChan; i++)
	{
		for (int j=0; j<nrSepBands; j++)
		{
			sepBandFFt[i][j]->prepareCartesian();
			sepBandFFt[i][j]->setPolar( newAmp[i], newPhase[i] );
		}
	}

	int maxBin = fft[0]->getBinFromFrequency( maxFreq, sample_rate );

	for (int i=0; i<binSize; i++) 
	{		
		phases[i] = 0.0f;

		// convert linear to logarithmical freq scale
		float cpsOct = static_cast<float>( 
				std::exp(
						( static_cast<float>(i)
								/ static_cast<float>(binSize-1)
								- 1.0f
						)
						* freqDist
				)
		);
		cpsOct = fmax( 0.0, fmin( cpsOct, 1.0 ) );

		intrpDiff[i] = cpsOct * static_cast<float>( maxBin );
		intrpLowerInds[i] = static_cast<int> ( floor( intrpDiff[i] ) );
		intrpUpperInds[i] = static_cast<int> ( 
				fmin(
						floor( intrpDiff[i] ) + 1.0f,
						static_cast<float>( binSize -1 )
				)
		);
		intrpDiff[i] -= static_cast<float> (intrpLowerInds[i]);
	}
}

//--------------------------------------------------------------

void MultiChanFFT::processSignal(int chanNr)
{
	fft[chanNr]->preparePolar();
	newAmp[chanNr] = fft[chanNr]->getAmplitude();	
	newAmp[chanNr][0] = 0.0f;

	// von linearer lautstärke auf logarithmische
	for (int i=0; i<binSize; i++) 
		curFft[chanNr][i] = static_cast<float>( sqrt( static_cast<double>(newAmp[chanNr][i]) ) ) * scaleAmp;

	int lastAmpBufferPtr = ampBufferPtr[chanNr];
	ampBufferPtr[chanNr] = (ampBufferPtr[chanNr] + 1) % nrAmpBuffers;

	float newVal = 0.0f;
	float newSum = 0.0f;
	ampSum[chanNr] = 0.f;

	// passe die frequenz skalierung an
	for (int i=0; i<binSize; i++) 
	{
		newVal = curFft[chanNr][ intrpLowerInds[i] ] * (1.0f - intrpDiff[i]) + curFft[chanNr][ intrpUpperInds[i] ] * intrpDiff[i];

		newSum = 0.0f;

		for ( int j=0;j<nrAmpBuffers-1;j++ )
			newSum += medFft[ ( ( ampBufferPtr[chanNr] -j -1 ) + nrAmpBuffers ) % nrAmpBuffers ][chanNr][i];

		newSum /= static_cast<float>( nrAmpBuffers -1 );

		medFft[ampBufferPtr[chanNr]][chanNr][i] = ( newVal + ( medFft[lastAmpBufferPtr][chanNr][i] * fftMed ) ) / (fftMed +1.0f);

		// zähle alle amp im spektrum zusammen -> messe die gesamt energie
		ampSum[chanNr] += medFft[ampBufferPtr[chanNr]][chanNr][i];
	}
	//memcpy(actMedFft[chanNr], medFft[lastAmpBufferPtr][chanNr], binSize * sizeof(float));
	memcpy(actMedFft[chanNr], medFft[ampBufferPtr[chanNr]][chanNr], binSize * sizeof(float));


	// phases sehen nicht besonders gut aus, deshalb hier auskommentiert,
	// da eigentlich nirgendwo gebraucht, bei bedarf wieder einkommentieren
	//	memcpy(actPhases[chanNr], fft[chanNr]->getPhase(), binSize * sizeof(float));
	actPhases[chanNr] = fft[chanNr]->getPhase();

	// unterteile das spektrum in n bänder
	// kopiere für jedes band separat die magnitudes
	// und zähle jeweils die magnitude summen zusammen, um einen
	// durchschnittslautstärken wert zu erhalten
	for(auto i=0;i<nrSepBands;i++)
	{
		sepBandRelEnergy[chanNr][i] = 0.f;
		for (int j=(i*binSize/nrSepBands); j<(i+1)*(binSize/nrSepBands); j++)
		{
			sepBandRelEnergy[chanNr][i] += actMedFft[chanNr][j];
		}
		sepBandRelEnergy[chanNr][i] /= ampSum[chanNr];
	}

	// berechne die relation von niedrigen frequenzen zu hohen
	loHiRelation[chanNr] = 0.f;
	for (int j=0; j<binSize/2; j++)
		loHiRelation[chanNr] += actMedFft[chanNr][j];
	loHiRelation[chanNr] /= ampSum[chanNr];
}

//--------------------------------------------------------------

void MultiChanFFT::procSepBands(int chanNr)
{
	// copy act fft auf sepbandfft
	for(auto i=0;i<nrSepBands;i++)
	{
		float lower = static_cast<float>(i) / static_cast<float>(std::max(nrSepBands, 1));
		float upper = static_cast<float>(i+1) / static_cast<float>(std::max(nrSepBands, 1));

		sepBandFFt[chanNr][i]->setPolarPart( lower, upper, fft[chanNr]->getAmplitude() );
		sepBandFFt[chanNr][i]->updateCartesian();
	}
}

//--------------------------------------------------------------

void MultiChanFFT::pForZeroBack(FFT** ffts, int chanNr, float** waveData)
{
	ffts[chanNr]->setSignal( waveData[chanNr] );
	processSignal(chanNr);
	setPhaseToZeroMed(chanNr);
	execIFFT(chanNr);
}

//--------------------------------------------------------------

void MultiChanFFT::forwZeroBack(float** waveData)
{
	//	nice(-10);	// set thread priority -10 - 10

	_threads = new std::thread*[nrChan];

	for (int chanNr=0; chanNr<nrChan; chanNr++) 
		_threads[chanNr] = new std::thread(&MultiChanFFT::pForZeroBack, this, fft, chanNr, waveData);

	for (int chanNr=0; chanNr<nrChan; chanNr++) 
	{
		_threads[chanNr]->join();
		delete _threads[chanNr];
	}

	delete _threads;
}

//--------------------------------------------------------------

void MultiChanFFT::execFFT(int chanNr, float** waveData)
{
	fft[chanNr]->setSignal( waveData[chanNr] );
	processSignal(chanNr);
}

//--------------------------------------------------------------

void MultiChanFFT::detectOnSet(int chanNr)
{
	/*
    // Grab your 512- or 1024-point, 50%-overlap, nicely-windowed FFT data, into "fftdata"
    // Then detect. "onset" will be true when there's an onset, false otherwise
    onsets[chanNr] = onsetsds_process(&ods[chanNr], fft[chanNr]->getRawFFT()) ? 1.f : 0.f;
    onsetAmp[chanNr] = fft[chanNr]->getMaxAmp();
    if (onsets[chanNr]) {
       // std::cout << "onset! chanNr " << chanNr << std::endl;
        catchedOnset[chanNr] = true;
        catchedAmp[chanNr] = fft[chanNr]->getMaxAmp();
   }
	 */
}

//--------------------------------------------------------------

void MultiChanFFT::execIFFT(int chanNr)
{
	float newVal = 0.0f;
	fft[chanNr]->executeIfft();

	// mix left and right side of the mirrored result
	for (int i=0;i<binSize-1;i++)
	{
		fInd = 1.0f - fabs( static_cast<float> (i) / static_cast<float> (binSize-2) * 2.0f - 1.0f );

		newVal = (fft[chanNr]->ifftOut[i] + fft[chanNr]->ifftOut[i + binSize -1]) * 0.0425f;
		newVal *= pow(fInd, 2.0f) * 8.0f + 1.0f;

		demirrored[chanNr][i] = ( newVal + ( demirrored[chanNr][i] * deMirrorMed ) ) / (deMirrorMed +1.0f);
	}
}

//--------------------------------------------------------------

void MultiChanFFT::execSepBandIFFT(int chanNr)
{
	float newVal = 0.0f;

	for (auto bandNr = 0;bandNr<nrSepBands;bandNr++)
	{
		sepBandFFt[chanNr][bandNr]->executeIfft();

		// mix left and right side of the mirrored result
		for (int i=0;i<binSize-1;i++)
		{
			fInd = 1.0f - fabs( static_cast<float> (i) / static_cast<float> (binSize-2) * 2.0f - 1.0f );

			newVal = (sepBandFFt[chanNr][bandNr]->ifftOut[i] + sepBandFFt[chanNr][bandNr]->ifftOut[i + binSize -1]) * 0.0425f;
			newVal *= pow(fInd, 2.0f) * 8.0f + 1.0f;

			deMirroredSepBand[chanNr][bandNr][i] = ( newVal + ( deMirroredSepBand[chanNr][bandNr][i] * deMirrorMed ) ) / (deMirrorMed +1.0f);
		}
	}
}

//--------------------------------------------------------------

void MultiChanFFT::setPhaseToZeroMed(int chanNr) 
{
	/*
	phases = fft[chanNr]->getPhase();

	for (int i=0; i<fft[chanNr]->getBinSize(); i++)
	{
		newAmp[chanNr][i] = 0.0f;
		for (int j=0; j<nrAmpBuffers; j++) newAmp[chanNr][i] += amplitudes[chanNr][j][i];
		newAmp[chanNr][i] /= (nrAmpBuffers * 0.25);
	}
	 */

	fft[chanNr]->setPolar( newAmp[chanNr], newPhase[chanNr] );
	fft[chanNr]->updateCartesian();
}

//--------------------------------------------------------------

void MultiChanFFT::setSepBandPhaseToZeroMed(int chanNr)
{
	for (auto bandNr = 0;bandNr<nrSepBands;bandNr++)
	{
		sepBandFFt[chanNr][bandNr]->setPolar( sepBandFFt[chanNr][bandNr]->getAmplitude(), newPhase[chanNr] );
		sepBandFFt[chanNr][bandNr]->updateCartesian();
	}
}

//--------------------------------------------------------------

void MultiChanFFT::setPhaseToZero(int chanNr)
{
	phases = fft[chanNr]->getPhase();

	fft[chanNr]->setPolar( amplitudes[chanNr][ ampBufferPtr[chanNr] ], newPhase[chanNr] );
	fft[chanNr]->updateCartesian();
}

//--------------------------------------------------------------

float* MultiChanFFT::getSmoothSpectr(int chanNr)
{
	return actMedFft[chanNr];
}

//--------------------------------------------------------------

float MultiChanFFT::getSmoothSpecBin(int chanNr, int binNr)
{
	return medFft[ ampBufferPtr[chanNr] ][chanNr][binNr];
}

//--------------------------------------------------------------

float MultiChanFFT::getSepBandEnergy(int chanNr, int bandNr)
{
	return sepBandRelEnergy[chanNr][bandNr];
}

//--------------------------------------------------------------

float MultiChanFFT::getLoHiRelation(int chanNr)
{
	return loHiRelation[chanNr];
}

//--------------------------------------------------------------

MultiChanFFT::~MultiChanFFT()
{	

	for (int i=0; i<nrAmpBuffers; i++) 
	{
		for (int j=0; j<nrChan; j++) 
			delete medFft[i][j];
		delete medFft[i];
	}
	delete [] medFft;

	for (int i=0; i<nrChan; i++) 
	{
		for (int j=0; j<nrAmpBuffers; j++) 
			delete amplitudes[i][j];

		for (int j=0; j<nrSepBands; j++)
			delete sepBandFFt[i][j];

		delete fft[i];
		delete sepBandFFt[i];
		delete curFft[i];
		delete amplitudes[i];
		delete demirrored[i];
		//        delete ods[i].data;
	}

	delete [] fft;
	delete [] sepBandFFt;
	delete [] curFft;
	delete [] amplitudes;
	delete [] demirrored;
	delete [] ampBufferPtr;
	delete [] intrpDiff;
	delete [] intrpUpperInds;
	delete [] intrpLowerInds;
}
}
