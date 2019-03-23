//
//  Aubio.cpp
//  tav_gl4
//
//  Created by user on 20.08.14.
//  Copyright (c) 2014 user. All rights reserved.
//

#include "AubioAnalyzer.h"

namespace tav
{

AubioAnalyzer::AubioAnalyzer(unsigned int _nrChans, unsigned int _hop_s, unsigned int _win_s, unsigned int _sampRate):
    		 win_s(_win_s), hop_s(_hop_s), nrChans(_nrChans), samplerate(_sampRate)
{
	pitchMed = 5.f;
	onset_threshold = 0.5f;
	silence_threshold = -70.f;
	input = new fvec_t*[nrChans];
	output = new fvec_t*[nrChans];
	pitch = new float[nrChans];
	catchedOnset = new bool[nrChans];
	catchedAmp = new float[nrChans];
	p = new aubio_pitch_t*[nrChans];
	o = new aubio_onset_t*[nrChans];

	for (unsigned int i=0;i<nrChans;i++)
	{
		input[i] = new_fvec(win_s); // input buffer
		output[i] = new_fvec(1); // output candidates
		pitch[i] = 0.0f;
		catchedOnset[i] = false;
		catchedAmp[i] = 0.f;

		// create pitch object
		p[i] = new_aubio_pitch ((char*) "default", win_s, hop_s, samplerate);

		// create onset object
		o[i] = new_aubio_onset ((char*) "energy", win_s, hop_s, samplerate);
		if (onset_threshold != 0.) aubio_onset_set_threshold (o[i], onset_threshold);
		if (silence_threshold != -90.) aubio_onset_set_silence (o[i], silence_threshold);
	}
}

//--------------------------------------------------------------

void AubioAnalyzer::process(unsigned int chanNr, float** waveData)
{
	input[chanNr]->data = waveData[chanNr];

	aubio_onset_do (o[chanNr], input[chanNr], output[chanNr]);
	if (output[chanNr]->data[0] > 0.f)
	{
		catchedOnset[chanNr] = true;
		catchedAmp[chanNr] = output[chanNr]->data[0];
	}

	aubio_pitch_do (p[chanNr], input[chanNr], output[chanNr]);
	if (output[chanNr]->data[0] > 0.f)
	{
		if (pitch[chanNr] == 0.f)
		{
			pitch[chanNr] = output[chanNr]->data[0];
		} else
		{
			pitch[chanNr] = (output[chanNr]->data[0] + (pitchMed * pitch[chanNr])) / (pitchMed + 1.f);
		}
	}

	n++; // frame counter
}

//--------------------------------------------------------------

float AubioAnalyzer::getPitch(unsigned int chanNr)
{
	return pitch[chanNr];
}

//--------------------------------------------------------------

bool AubioAnalyzer::getOnset(unsigned int chanNr)
{
	return onset[chanNr];
}

//--------------------------------------------------------------

AubioAnalyzer::~AubioAnalyzer()
{
	for (unsigned int i=0;i<nrChans;i++)
	{
		del_fvec (input[i]);
		del_fvec (output[i]);

		del_aubio_pitch (p[i]);
		del_aubio_onset (o[i]);
	}
	delete input;
	delete output;
	aubio_cleanup ();
}

}
