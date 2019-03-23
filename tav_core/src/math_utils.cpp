/*
 *  math_utils.cpp
 *  Timeart Visualizer modern OpenGL framework
 *
 *  Created by Sven Hahne on 09.09.13.
 *  Copyright 2013 Sven Hahne. All rights reserved.
 *
 */

#include "pch.h"
#include "math_utils.h"

namespace tav
{

// calculates value in between the indices of an array
// inInd ranges from 0.0f to arraySize
float interpolVal(float inInd, int arraySize, float* array)
{
	float outVal = 0.0f;
	int lowerInd = static_cast<int>(floor(inInd));
	int upperInd = static_cast<int>(fmin(lowerInd + 1.0f,
			static_cast<float>(arraySize - 1)));
	float weight = inInd - lowerInd;

	if (weight == 0.0)
	{
		outVal = array[lowerInd];
	}
	else
	{
		outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
	}
	return outVal;
}

//----------------------------------------------------

// calculates value in between the indices of an array
// index ranges from 0-1 eingeben
float interpolVal2(float inInd, int arraySize, float* array)
{
	float outVal = 0.0f;
	float fArraySize = static_cast<float>(arraySize);
	float fInd = fmod(inInd, 1.0f) * (fArraySize - 1.0f);

	int lowerInd = static_cast<int>(floor(fInd));
	int upperInd = static_cast<int>(fmin(lowerInd + 1.0f, fArraySize - 1.0f));
	float weight = fInd - lowerInd;

	if (weight == 0.0)
	{
		outVal = array[lowerInd];
	}
	else
	{
		outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
	}
	return outVal;
}

//----------------------------------------------------

// calculates value in between the indices of an array
// index ranges from 0-1 eingeben
float interpolVal(float inInd, int arraySize, std::vector<float>* array)
{
	float outVal = 0.0f;
	float fArraySize = static_cast<float>(arraySize);
	float fInd = fmod(inInd, 1.0f) * (fArraySize - 1.0f);

	int lowerInd = static_cast<int>(floor(fInd));
	int upperInd = static_cast<int>(fmin(lowerInd + 1.0f, fArraySize - 1.0f));
	float weight = fInd - lowerInd;

	if (weight == 0.0)
	{
		outVal = array->at(lowerInd);
	}
	else
	{
		outVal = array->at(lowerInd) * (1.0f - weight)
				+ array->at(upperInd) * weight;
	}
	return outVal;
}

//----------------------------------------------------

float distPointLine(glm::vec2 _point, glm::vec2 _lineP1, glm::vec2 _lineP2)
{
	// return minimum distance between linesegment P1P2 and point
	float l2 = glm::length(_lineP2 - _lineP1);
	l2 *= l2;
	if (l2 == 0.f)
		return glm::distance(_point, _lineP1);

	// consider the line extending the segment, paramneterized as v + t (P2 - P1).
	// we find projection of point p onto the line
	// if falls where t = [(p - P1) . (P2 - P1)] / |P2 - P1|^2
	float t = glm::dot(_point - _lineP1, _lineP2 - _lineP1) / l2;
	if (t < 0.f)
		return glm::distance(_point, _lineP1); // beyond the P1 End of Segment
	else if (t > 1.f)
		return glm::distance(_point, _lineP2); // beyond the P2 End of Segment

	glm::vec2 proj = _lineP1 + t * (_lineP2 - _lineP1);
	return glm::distance(_point, proj);
}

//----------------------------------------------------

float getRandF(float min, float max)
{
	float outVal = 0.0f;
	outVal = rand() % 100000;
	outVal *= 0.00001f;

	outVal *= max - min;
	outVal += min;

	return outVal;
}

//----------------------------------------------------

void init_number_generator()
{
	static bool done = false;
	if (!done)
	{
		done = true;
		std::srand(static_cast<unsigned int>(std::time(0)));
	}
}

//----------------------------------------------------

int random_number_generator()
{
	init_number_generator();
	return std::rand();
}

//----------------------------------------------------

float mapFloat(float value, float inputMin, float inputMax, float outputMin,
		float outputMax, bool clamp)
{
	if (std::fabs(inputMin - inputMax) < FLT_EPSILON)
	{
		std::cout << ("ofMath")
				<< "ofMap(): avoiding possible divide by zero, check inputMin and inputMax: "
				<< inputMin << " " << inputMax;
		return outputMin;
	}
	else
	{
		float outVal = ((value - inputMin) / (inputMax - inputMin)
				* (outputMax - outputMin) + outputMin);

		if (clamp)
		{
			if (outputMax < outputMin)
			{
				if (outVal < outputMax)
					outVal = outputMax;
				else if (outVal > outputMin)
					outVal = outputMin;
			}
			else
			{
				if (outVal > outputMax)
					outVal = outputMax;
				else if (outVal < outputMin)
					outVal = outputMin;
			}
		}
		return outVal;
	}
}

//----------------------------------------------------

float clamp(float value, float min, float max)
{
	return value < min ? min : value > max ? max : value;
}

//----------------------------------------------------

void makeMatr(glm::mat4* _matr, bool* _inited, float xOffs, float yOffs,
		float zOffs, float rotX, float rotY, float rotZ, float scaleX,
		float scaleY, float scaleZ)
{
	/*
	 glm::mat4 rotM, transScaleM, matrix;
	 
	 rotM = rotM.createRotationAroundAxis(
	 rotX / 360.0f * TWO_PI, 
	 rotY / 360.0f * TWO_PI, 
	 rotZ / 360.0f * TWO_PI
	 );
	 transScaleM.setTranslation(xOffs, yOffs, zOffs);
	 transScaleM.setScaling(scaleX, scaleY, scaleZ);
	 rotM = rotM * transScaleM;
	 
	 *_matr = vmath::Matrix4f(rotM);
	 
	 if ( xOffs != 0.0f || yOffs != 0.0f || zOffs != 0.0f 
	 || rotX != 0.0f || rotY != 0.0f || rotZ != 0.0f
	 || scaleX != 1.0f || scaleY != 1.0f || scaleZ != 1.0f
	 )
	 *_inited = true;
	 */
}

//----------------------------------------------------

float frand()
{
	return rand() / (float) RAND_MAX;
}

//----------------------------------------------------

float sfrand()
{
	return frand() * 2.0f - 1.0f;
}
}
