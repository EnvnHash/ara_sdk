/*
 * Envelopse.h
 *
 *  Created on: 23.02.2017
 *      Copyright by Sven Hahne
 */

#ifndef ENVELOPE_H_
#define ENVELOPE_H_

#pragma once

#include <vector>
#include <iostream>

namespace tav
{

class Envelope
{
public:
	Envelope()
	{
	}
	~Envelope()
	{
	}
	void addTime(float _val)
	{
		times.push_back(_val);
	}
	void addValue(float _val)
	{
		values.push_back(_val);
	}
	void clear()
	{
		times.clear();
		values.clear();
	}
	float getVal(float _t)
	{
		float outVal = 0.f;
		found = false;
		ind = 0;

		while (!found && ind < static_cast<unsigned int>(times.size() - 1))
			if (times[ind] >= _t && _t <= times[ind + 1])
				found = true;

		if (found == true)
		{
			blendPos = (_t - times[ind]) / (times[ind + 1] - times[ind]);
			outVal = values[ind] * (1.f - blendPos)
					+ values[ind + 1] * blendPos;
		}

		return outVal;
	}
	void dump()
	{
		// print times
		std::cout << "[ [";
		for (unsigned int i = 0; i < static_cast<unsigned int>(times.size());
				i++)
		{
			std::cout << times[i];
			if (i != static_cast<unsigned int>(times.size() - 1))
				std::cout << ", ";
		}
		std::cout << "], ";

		// print values
		std::cout << "[";
		for (unsigned int i = 0; i < static_cast<unsigned int>(values.size());
				i++)
		{
			std::cout << values[i];
			if (i != static_cast<unsigned int>(values.size() - 1))
				std::cout << ", ";
		}
		std::cout << "] ] ";
	}
private:
	bool found;
	unsigned int ind;
	float blendPos;
	std::vector<float> times;
	std::vector<float> values;
};

}

#endif /* ENVELOPE_H_ */
