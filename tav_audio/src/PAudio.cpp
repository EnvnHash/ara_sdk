/*
 *  PAudio.cpp
 *  tav
 *
 *  Created by user on 25.05.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#include "PAudio.h"


using namespace tav;

PAudio::PAudio(std::mutex *_mutex, bool* _isReady, int _frameSize, int _rec_buf_size,
			   int _fft_size, int _sample_rate, int _monRefreshRate, int _maxNrChans)
: mutex(_mutex), isReady(_isReady), frames_per_buffer(_frameSize),
rec_buf_size(_rec_buf_size), fft_size(_fft_size), sample_rate(_sample_rate),
monRefreshRate(_monRefreshRate), hasInputDevice(true), maxInputChannels(_maxNrChans),
doPostProc(true)
{
	isPlaying = false;
	writeToDisk = false;
}

//------------------------------------------------------------------------------------------------------------

void PAudio::initAudio()
{
	counter = 0;

	PaStreamParameters  outputParameters;
	PaStreamParameters  inputParameters;
    PaError             err = paNoError;

    //-------- init audio -----------------
#ifdef __linux__
    PaJack_SetClientName("Tav");
#endif

    err = Pa_Initialize();
    if( err != paNoError ) std::cout << "device initialisation error" << std::endl;

#ifdef __linux__

    bool found = false;
    int foundIndx = 0;
    PaHostApiIndex nrHostApi = Pa_GetHostApiCount();
   // printf("nrHostApis: %d \n", nrHostApi);


	inputParameters.device = Pa_GetDefaultInputDevice();

    for (int i=0;i<nrHostApi;i++)
    {
    	 const PaHostApiInfo* api = Pa_GetHostApiInfo(i);
    	 const PaDeviceInfo* devInfo;

    	 printf("api name %d: %s nrDevices: %d\n", i, api->name, api->deviceCount);

    	 if(strcmp(api->name, "JACK Audio Connection Kit") == 0 || i==2)
    	 {
    		 printf("PAudio using jack \n");


        	 for(int j=0;j<Pa_GetDeviceCount();j++)
        	 {
        		 devInfo = Pa_GetDeviceInfo(j);
            	 std::cout << "PAudio device name: " << devInfo->name << std::endl;

        		 if (std::strcmp(devInfo->name, "SuperCollider") == 0)
        		 {
            		 foundIndx = j;
            		 inputParameters.device = (PaDeviceIndex)foundIndx;

            		 printf("found SuperCollider!!!! using dev nr %d name: %s maxNrInputChans: %d \n", j, devInfo->name, devInfo->maxInputChannels);
        		 }

        		 printf("dev nr %d name: %s maxNrInputChans: %d \n", j, devInfo->name, devInfo->maxInputChannels);
        	 }



        	 //std::cout << "default device: " << api->defaultInputDevice << std::endl;
    		 //inputParameters.device = api->defaultInputDevice;
    		 found = true;
    	 }
    }

    if(!found)
    {
    	std::cout << "assigning default input device" << std::endl;
    	inputParameters.device = Pa_GetDefaultInputDevice();
    } else {
    	std::cout << "PAudio using device " << inputParameters.device << std::endl;
    }

#elif __APPLE__
    inputParameters.device = Pa_GetDefaultInputDevice();
#endif

    if (inputParameters.device == paNoDevice)
    {
        waveData.numChannels = 0;
        std::cerr << "PAudio Error: No Default input device." << std::endl;
    } else
    {
        devInfo = Pa_GetDeviceInfo(inputParameters.device);

        std::cout << "start init parameters, fft_size:" << fft_size << " frames_per_buffer:" << frames_per_buffer;
        std::cout << " rec_buf_size:" << rec_buf_size << " sample_rate:" << sample_rate << std::endl;

        nrSepBands = 4;
        volumeMed = 1.f;
        fftUpdtFreq = fft_size / frames_per_buffer;

        abios = new AubioAnalyzer(maxInputChannels, frames_per_buffer, frames_per_buffer * rec_buf_size, sample_rate);

        medVol = new float[maxInputChannels];
        dataToFft = new SAMPLE*[maxInputChannels];
        sampData = new SAMPLE*[maxInputChannels];
        for ( int i=0; i<maxInputChannels; i++)
        {
            medVol[i] = 0.f;
            dataToFft[i] = new SAMPLE[fft_size];
            sampData[i] = new SAMPLE[frames_per_buffer * rec_buf_size];
        }

        memset(&dataToFft[0][0], 0, maxInputChannels * fft_size * sizeof(SAMPLE) );
        memset(&sampData[0][0], 0, maxInputChannels * frames_per_buffer * rec_buf_size * sizeof(SAMPLE) );

        mixBuf = new SAMPLE[maxInputChannels * frames_per_buffer];
        memset(&mixBuf[0], 0, maxInputChannels * frames_per_buffer * sizeof(SAMPLE) );

        pll = new PaPhaseLockLoop(fft_size / 2, maxInputChannels);
        ffts = new MultiChanFFT(fft_size, maxInputChannels, sample_rate);
        sepBandPll = new PaPhaseLockLoop*[nrSepBands];
        for (int i=0; i<nrSepBands; i++)
          	 sepBandPll[i] = new PaPhaseLockLoop(fft_size / 2, maxInputChannels);

        // --- init postProcData ------------

        postProcData.sampData = sampData;

        // --- init waveData ------------

        waveData.numChannels = maxInputChannels;
        waveData.frameIndex = 0;
        waveData.maxFrameIndex = frames_per_buffer * rec_buf_size;
        waveData.frames_per_buffer = frames_per_buffer;
        waveData.rec_buf_size = rec_buf_size;
        waveData.sampData = sampData;
        waveData.inMutex = mutex;
        waveData.blockCounter = 0;
        waveData.doUpdate = true;
        waveData.buffersFilled = false;
        waveData.isReady = isReady;
        waveData.isWriting = &isWriting;
        waveData.mediaRecCb = NULL;
        waveData.postProcCond = &postProcCond;
        waveData.postProcData = &postProcData;
        waveData.inSampQ = NULL;
        waveData.mixBuf = mixBuf;


#ifdef __APPLE__
        // --- init aiff writer --
        writeThread = new AiffWriteThread(&writeThreadMutex, 2, 16, sample_rate);
        waveData.writeThread = writeThread;
        waveData.writeThreadMutex = &writeThreadMutex;
        waveData.writeToDisk = &writeToDisk;
#endif
        // -----------------------

        inputParameters.channelCount = maxInputChannels;
        inputParameters.sampleFormat = pa_sample_type;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        // -----------------------


        /*
        outputParameters.device = Pa_GetDefaultOutputDevice(); // default output device
        if (outputParameters.device == paNoDevice) {
        	std::cerr << "PAudio Error: No default output device." << std::endl;
        }
        outputParameters.channelCount = 2;       // stereo output
        outputParameters.sampleFormat = pa_sample_type; // 32 bit floating point output
//        outputParameters.sampleFormat = paFloat32; // 32 bit floating point output
        outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
        outputParameters.hostApiSpecificStreamInfo = NULL;

        waveData.outNumChannels = outputParameters.channelCount;
*/
        // Record some audio. --------------------------------------------

        std::cout << "paudio open stream" << std::endl;

        err = Pa_OpenStream(&stream,
                            &inputParameters,
							NULL,
                            sample_rate,
                            frames_per_buffer,
                            paClipOff,      // we won't output out of range samples so don't bother clipping them
                            streamCallback,
                            &waveData);

        if ( err != paNoError ) std::cerr << "PAudio Stream initialisation error" << std::endl;

        err = Pa_StartStream( stream );
        isPlaying = true;

        if ( err != paNoError ) std::cerr << "PAudio Stream start error" << err;

        std::cout << "Now recording!!" << std::endl;
    }
}

//------------------------------------------------------------------------------------------------------------

PAudio::~PAudio()
{
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
}

//------------------------------------------------------------------------------------------------------------

void PAudio::pause()
{
	Pa_StopStream(stream);
	isPlaying = false;
}

//------------------------------------------------------------------------------------------------------------

void PAudio::resume()
{
	Pa_StartStream(stream);
	isPlaying = true;
}

//------------------------------------------------------------------------------------------------------------

void PAudio::recordToFile(std::string _path)
{
	std::string path = _path.substr( 0, _path.length() - 3 );
	path += "aif";
#ifdef __APPLE__
	waveData.writeThread->initFile(path);
	waveData.writeThread->start(0);
	writeToDisk = true;
#endif
}

//------------------------------------------------------------------------------------------------------------

void PAudio::stopRecordToFile()
{
#ifdef __APPLE__
	writeThread->stop();
	writeToDisk = false;
	std::cout << "close audio file" << std::endl;
#endif
}

//------------------------------------------------------------------------------------------------------------

void PAudio::start()
{
	try {
		m_Thread = std::thread(&PAudio::initAudio, this);
		postProc_thread = std::thread(&PAudio::postProcLoop, this);
	}
	catch (...)
	{
		std::cout << "An unknown exception occured." << std::endl;
	}
}

//------------------------------------------------------------------------------------------------------------

void PAudio::join()
{
	m_Thread.join();
}

//------------------------------------------------------------------------------------------------------------

int PAudio::streamCallback(const void *input, void *output,
						   unsigned long frameCount,
						   const PaStreamCallbackTimeInfo* timeInfo,
						   PaStreamCallbackFlags statusFlags,
						   void *userData )
{
	paAudioInData *waveData         = (paAudioInData*)(userData);
	SAMPLE** sampData               = (SAMPLE**) (waveData->sampData);
	int frames_per_buffer           = (int)(waveData->frames_per_buffer);
	int rec_buf_size                = (int)(waveData->rec_buf_size);
	bool* isWriting                 = (bool*)(waveData->isWriting);

	const SAMPLE *rptr = (const SAMPLE*) input;
	float *outPtr = (float*) output;

	float *mixBuf = (SAMPLE*) waveData->mixBuf;

    (void) output; // Prevent unused variable warnings.
    (void) timeInfo;
    (void) statusFlags;

	waveData->inMutex->lock();

    // ------------- process output ----------------------------------------------------

	/*
    bool sndFileRouteToIn = false;

	// if we do have an active SoundFileplayer get the actual buffer and write to output
	std::vector< std::function<void(float*, bool*, bool*)> > sndFilePlCb =
		(std::vector< std::function<void(float*, bool*, bool*)> >)waveData->sndFilePlCb;

	if (sndFilePlCb.size() > 0)
	{
		bool add = false;	// the first callback that writes will set this to true
		unsigned int ind = 0;
		for (std::vector< std::function<void(float*, bool*, bool*)> >::iterator it = sndFilePlCb.begin();
				it!= sndFilePlCb.end(); ++it )
		{
			(*it)(mixBuf, &sndFileRouteToIn, &add);
			ind++;
		}
	}

	// copy mix buf to output
	memcpy(outPtr, mixBuf, waveData->numChannels * frames_per_buffer * sizeof(SAMPLE));

	//if(sndFileRouteToIn)
		outPtr = (float*) output;
*/
	// ------------- process input ----------------------------------------------------

	*waveData->isReady = true;

	if ( waveData->blockCounter >= rec_buf_size ) waveData->buffersFilled = true;
	if ( waveData->blockCounter >= (rec_buf_size+1) ) *waveData->isReady = true;

	waveData->inMutex->unlock();

	bool copyToIn = false;
	if (waveData->sndFileSampCb.size() > 0)
	{
		std::function<float(int, int)> sndFileSampCbFirst = waveData->sndFileSampCb[0];
		outPtr = (float*) output;
		copyToIn = true;
	}

	if ( waveData->doUpdate )
	{
		waveData->inMutex->lock();

		*isWriting = true;

		// copy input from audio hardware
		if( input != NULL )
		{
			for( unsigned int i=0; i<frameCount; i++ )
			{
				for( int j=0; j<waveData->numChannels; j++ )
				{
					if (!copyToIn)
						sampData[j][ waveData->frameIndex ] = *rptr;
					else {
						sampData[j][ waveData->frameIndex ] = *rptr + waveData->sndFileSampCb[0](i, j);
						outPtr++;
					}
					*rptr++;
				}
				waveData->frameIndex = ( waveData->frameIndex +1 ) % waveData->maxFrameIndex;
			}
		}

		int offset = ( waveData->frameIndex - frames_per_buffer + waveData->maxFrameIndex )
					  % waveData->maxFrameIndex;

		*isWriting = false;

#ifdef PAUDIO_POSTPROC

		// fill postProcData
		waveData->postProcData->codecNrChans = waveData->codecNrChans;
		waveData->postProcData->frameCount = frameCount;
		waveData->postProcData->frameIndex = waveData->frameIndex;
		waveData->postProcData->inSampQ = waveData->inSampQ;
		waveData->postProcData->maxFrameIndex = waveData->maxFrameIndex;
		waveData->postProcData->mixDownMap = waveData->mixDownMap;
		waveData->postProcData->numChannels = waveData->numChannels;
		waveData->postProcData->offset = offset;

		waveData->inMutex->unlock();

		// unblock post processing loop
		waveData->postProcCond->notify_one();

		// if there is a MediaRecorder Instance, its callback has to be fed with new data
		if (waveData->mediaRecCb)
			waveData->mediaRecCb(waveData->postProcData);
#endif

		waveData->inMutex->unlock();


		waveData->blockCounter++;
	}


	return paContinue;
}

//--------------------------------------------------------------------------------------------

void PAudio::postProcLoop()
{
	while(doPostProc)
	{
	    std::unique_lock<std::mutex> lock(pp_mutex);
		postProcCond.wait(lock);


	//  for( int j=0; j<waveData->numChannels; j++ )
	//  	abios->process(&sampData[j][offset], j);

		if ( waveData.frameIndex == 0 )
		{
			waveData.inMutex->lock();

			// kopiere die neuen samples in ein array mit der kompletten verfügbaren länge
			// in der richtigen reihenfolge
			for ( int j=0; j<waveData.numChannels; j++ )
				memcpy(dataToFft[j], &sampData[j][postProcData.offset], fft_size * sizeof(SAMPLE));

			waveData.inMutex->unlock();

			if ( waveData.buffersFilled )
			{
				for( int j=0; j< waveData.numChannels; j++ )
				{
			       // abios->process(j, waveData->dataToFft);

					// do the fourier analyse
			        // is also copying the sepbands
					ffts->execFFT(j, dataToFft);

					ffts->detectOnSet(j);

					// set phases to 0
					ffts->setPhaseToZeroMed(j);

					// do inverse fourier transformation
					ffts->execIFFT(j);

					// create a smoothed endless loop
					pll->getIfft( ffts->demirrored[j], j, fft_size/2 );

					medVol[j] = (medVol[j] * volumeMed + pll->maxVol[j]) / (volumeMed +1.f);

			        // process separated ffts
			        ffts->procSepBands(j);
			        ffts->setSepBandPhaseToZeroMed(j);
			        ffts->execSepBandIFFT(j);
			        for (int k=0; k<nrSepBands; k++)
			        	sepBandPll[k]->getIfft( ffts->deMirroredSepBand[j][k], j, fft_size/2 );
				}
			}

	        // threaded, but actually isn't faster on my system...
			// if ( waveData->buffersFilled )  {
			//     ffts->forwZeroBack(waveData->dataToFft);
			//  	for(int j=0; j<waveData->numChannels; j++) pll->getIfft( ffts->demirrored[j], j, fft_size/2 );
			//		pll->phaseLock( waveDatas->sepChans );
			//  }
		}
	}
}

//--------------------------------------------------------------------------------------------

float PAudio::getActFftBin(int chanNr, int binNr)
{
	return ffts->getSmoothSpecBin(chanNr, binNr);
//	return ffts->medFft[ ffts->ampBufferPtr[chanNr] ][chanNr][sampNr];
}

//--------------------------------------------------------------------------------------------

float* PAudio::getActFft(int chanNr)
{
	return ffts->actMedFft[chanNr];
}

//--------------------------------------------------------------------------------------------

float** PAudio::getActFft()
{
    return ffts->actMedFft;
}

//--------------------------------------------------------------------------------------------

float* PAudio::getActPhases(int chanNr)
{
	return ffts->actPhases[chanNr];
}

//--------------------------------------------------------------------------------------------

bool PAudio::getOnset(int chanNr)
{
//    return ffts->onsets[chanNr];
    return abios->getOnset(chanNr) > 0.f;
}

//--------------------------------------------------------------------------------------------

bool PAudio::getCatchedOnset(int chanNr)
{
    bool out = false;
    if (abios->catchedOnset[chanNr])
    {
        out = true;
        abios->catchedOnset[chanNr] = false;
    }
//    if (ffts->catchedOnset[chanNr])
//    {
//        out = true;
//        ffts->catchedOnset[chanNr] = false;
//    }
    return out;
}

//--------------------------------------------------------------------------------------------

float PAudio::getOnsetAmp(int chanNr)
{
    return abios->getOnset(chanNr);
}

//--------------------------------------------------------------------------------------------

float PAudio::getCatchedAmp(int chanNr)
{
    return abios->catchedAmp[chanNr];
    //return ffts->catchedAmp[chanNr];
}

//--------------------------------------------------------------------------------------------

float PAudio::getPitch(int chanNr)
{
    return abios->getPitch(chanNr);
}

//--------------------------------------------------------------------------------------------

float* PAudio::getSmoothSpectr(int chanNr)
{
    return ffts->actMedFft[chanNr];
}

//--------------------------------------------------------------

float PAudio::getSmoothSpectrAt(int chanNr, float bandInd)
{
    return ffts->actMedFft[chanNr][static_cast<int>(bandInd * static_cast<float>(ffts->binSize))];
}

//--------------------------------------------------------------

float PAudio::getSepBandEnergy(int chanNr, int bandNr)
{
    return ffts->getSepBandEnergy(chanNr, bandNr);
}

//--------------------------------------------------------------

float PAudio::getLoHiRelation(int chanNr)
{
    return ffts->getLoHiRelation(chanNr);
}

//--------------------------------------------------------------------------------------------

float PAudio::getMedAmp(int chanNr)
{
    return medVol[chanNr];
}

//--------------------------------------------------------------------------------------------

int PAudio::getMaxNrInChannels()
{
    return maxInputChannels;
}

//--------------------------------------------------------------------------------------------

int PAudio::getNrSepBands()
{
    return nrSepBands;
}

//--------------------------------------------------------------------------------------------

unsigned int PAudio::getFrameNr()
{
    return waveData.blockCounter;
}

//--------------------------------------------------------------------------------------------

float** PAudio::getPll()
{
    return pll->getPll();
}

//--------------------------------------------------------------------------------------------

float* PAudio::getPllAt(int chanNr)
{
    return pll->getPllAt(chanNr);
}

//--------------------------------------------------------------------------------------------

float PAudio::getPllAtPos(int chanNr, float pos)
{
    return pll->getAmpAtPos(chanNr, fmod(pos, 1.f));
}

//--------------------------------------------------------------------------------------------

float PAudio::getPllSepBandAtPos(int chanNr, int bandNr, float pos)
{
    return sepBandPll[bandNr % nrSepBands]->getAmpAtPos(chanNr, fmod(pos, 1.f));
}

//--------------------------------------------------------------------------------------------

float* PAudio::getPllSepBand(int chanNr, int bandNr)
{
    return sepBandPll[bandNr % nrSepBands]->getPllAt(chanNr);
}

//--------------------------------------------------------------------------------------------

float PAudio::getSampDataAtPos(int chanNr, float pos)
{
    return sampData[chanNr][static_cast<int>(pos * static_cast<float>(frames_per_buffer * rec_buf_size))];
}

//--------------------------------------------------------------------------------------------

unsigned int PAudio::getOutNrChannels(){
	return waveData.outNumChannels;
}

//--------------------------------------------------------------------------------------------

float* PAudio::getFft(int chanNr, int timeOffs)
{
	int ptr = (
			   ffts->ampBufferPtr[chanNr]
			   -1 
               + std::max( timeOffs, ffts->nrAmpBuffers * -1 )
			   + ffts->nrAmpBuffers
			   ) % ffts->nrAmpBuffers;
	
	return ffts->medFft[ptr][chanNr];
}

//--------------------------------------------------------------------------------------------

float PAudio::getFft(int chanNr, int timeOffs, int sampNr)
{
	int ptr = ( 
			   ffts->ampBufferPtr[chanNr]
			   -1 
			   + std::max( timeOffs, (ffts->nrAmpBuffers - 1) * -1 )
			   + ffts->nrAmpBuffers
			   ) % ffts->nrAmpBuffers;
	
	return ffts->medFft[ptr][chanNr][sampNr];
}

//--------------------------------------------------------------------------------------------

void PAudio::setPllDataMed(float _val)
{
	pll->setDataMed(_val);
}

//--------------------------------------------------------------------------------------------

void PAudio::setPllLoP(float _val)
{
	pll->setLoP(_val);
}

//--------------------------------------------------------------------------------------------
/*
void PAudio::setExtCallback(void (*cbType)(unsigned int numChans, int frames, int offset, int codecFrameSize,
		unsigned int codecNrChans, std::vector< std::vector<unsigned int> >* mixDownMap, float** sampData,
		std::vector<float>* inSampQ), std::vector<float>* sampQ, int _codecFrameSize, unsigned int _codecNrChans,
		std::vector< std::vector<unsigned int> >* mixDownMap)*/

void PAudio::setExtCallback(std::function<void(paPostProcData*)> _cb)
{
	waveData.mediaRecCb = _cb;
	/*
	waveData.inSampQ = sampQ;
	waveData.codecFrameSize = _codecFrameSize;
	waveData.codecNrChans = _codecNrChans;
	waveData.mixDownMap = mixDownMap;
	*/
}

//--------------------------------------------------------------------------------------------

void PAudio::addSndFilePlayerCb(std::function<void(float*, bool*, bool*)> _sndFilePlCb){
	waveData.sndFilePlCb.push_back( _sndFilePlCb );
}

//--------------------------------------------------------------------------------------------

void PAudio::addSndFilePlayerSampCb(std::function<float(int, int)> _sndFileSampCb){
	waveData.sndFileSampCb.push_back( _sndFileSampCb );
}

//--------------------------------------------------------------------------------------------

void PAudio::removeExtCallback()
{
	waveData.mediaRecCb = NULL;
}
