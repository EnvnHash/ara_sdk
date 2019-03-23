/*
 *  OSCData.cpp
 *  ta_visualizer
 *
 *  Created by Sven Hahne on 06.01.11.
 *  Copyright 2011 Sven Hahne. All rights reserved.
 *
 */

#include "pch.h"
#include "OSCData.h"

namespace tav
{

OSCData theOscData;

OSCData::OSCData() :
		inited(false)
{
}

//----------------------------------------------------

OSCData::~OSCData()
{}

//----------------------------------------------------

void OSCData::initData()
{
	firstRun = true;

	seqParMed = 0.3f;
	parMed = 0.6f;

	osc_in_vals.resize(NUMOSCSCENEPAR);
	osc_vals.resize(NUMOSCSCENEPAR);

	for (int i = 0; i < NUMOSCSCENEPAR; i++)
	{
		osc_in_vals[i] = 0.0f;
		osc_vals[i] = 0.0;
	}

	theOscData.osc_vals[3] = theOscData.totalBrightness;
	theOscData.osc_in_vals[3] = theOscData.totalBrightness;
	theOscData.osc_vals[4] = theOscData.alpha;
	theOscData.osc_in_vals[4] = theOscData.alpha;
	theOscData.osc_in_vals[1] = theOscData.sceneNum1;
	theOscData.osc_in_vals[2] = theOscData.sceneNum2;

	rotYAxis = 0.0f;
	speed = 0.0f;
	actTime = 0.0;
	lastTime = 0.0;
}

//----------------------------------------------------

void OSCData::getNewVals(double dt)
{
	float med = seqParMed / std::max(dt, 0.001);
	float med2 = parMed / std::max(dt, 0.001);

	for (int i = 0; i < NUMOSCSCENEPAR; i++)
	{
		if ((i != 1) && (i != 2))
		{
			if (i == 0)
			{
				theOscData.osc_vals[i] = (theOscData.osc_in_vals[i]
						+ theOscData.osc_vals[i] * med) / (med + 1.0f);
				// schneid die werte unten ab
				theOscData.osc_vals[i] = fmax(theOscData.osc_vals[i] * 1.00001f,
						0.00001f) - 0.00001f;
			}
			else
			{
				// sprünge von 1 auf 0 und 0 auf 1 nicht glätten
				if ((theOscData.osc_in_vals[i] < 0.2f
						&& theOscData.osc_vals[i] > 0.8f)
						|| (theOscData.osc_in_vals[i] > 0.8f
								&& theOscData.osc_vals[i] < 0.2f))
				{
					theOscData.osc_vals[i] = theOscData.osc_in_vals[i];
				}
				else
				{
					theOscData.osc_vals[i] = (theOscData.osc_in_vals[i]
							+ theOscData.osc_vals[i] * med2) / (med2 + 1.0f);
				}
			}
		}
	}

	theOscData.scBlend = theOscData.osc_vals[0];
	theOscData.sceneNum1 = static_cast<int>(theOscData.osc_in_vals[1] + 0.1f);
	theOscData.sceneNum2 = static_cast<int>(theOscData.osc_in_vals[2] + 0.1f);
	theOscData.totalBrightness = theOscData.osc_vals[3];
	theOscData.alpha = theOscData.osc_vals[4];
	theOscData.feedback = theOscData.osc_vals[5];
	theOscData.blurOffs = theOscData.osc_vals[6];
	theOscData.blurFboAlpha = theOscData.osc_vals[7];
	theOscData.blurFdbk = theOscData.osc_vals[8];

	// schneid rotAxis werte unten ab
	theOscData.rotYAxis =
			theOscData.osc_vals[9] > 0.00001f ? theOscData.osc_vals[9] : 0.f;
	theOscData.zoom = theOscData.osc_vals[10];
	theOscData.speed = theOscData.osc_vals[11];
	theOscData.backColor = theOscData.osc_vals[12];
	theOscData.startStopVideo = theOscData.osc_vals[13];
	theOscData.videoSpeed = theOscData.osc_vals[14];
	theOscData.audioSmooth = theOscData.osc_vals[15];

	if (!inited)
		inited = true;
}

//----------------------------------------------------

void OSCData::addPar(std::string _name, float _min, float _max, float _step,
		float _initVal, contrShape _shape)
{
	if (theOscData.nodePar.find(_name) == theOscData.nodePar.end())
	{
		theOscData.nodePar[_name] = contrSpec();
		theOscData.nodePar[_name].min = _min;
		theOscData.nodePar[_name].max = _max;
		theOscData.nodePar[_name].step = _step;
		theOscData.nodePar[_name].initVal = _initVal;
		theOscData.nodePar[_name].shape = _shape;
		theOscData.nodePar[_name].val = _initVal;
	}
	else
	{
		std::cerr << "OSCData::addPar Warning: control Spec already exists"
				<< std::endl;
	}
}

//----------------------------------------------------

float OSCData::getPar(std::string _name)
{
	if (theOscData.nodePar.find(_name) != theOscData.nodePar.end())
	{
		return theOscData.nodePar[_name].val;
	}
	else
	{
		std::cerr << "OSCData::getPar Error: Parameter does not exist"
				<< std::endl;
	}
	return 0;
}

}
