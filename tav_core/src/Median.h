//
//  MedianValue.h
//  tav_core
//
//  Created by Sven Hahne on 11/9/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <stdio.h>

namespace tav
{
template<typename T>
class Median
{
public:
	Median(float _medAmt, unsigned int _bufferSize = 2)
	{
		bufferSize = _bufferSize;
		medAmt = _medAmt;
		medDiv = 1.f / (medAmt + 1.f);
		ptr = 0;
		filled = false;
		values = new T[_bufferSize];
	}

	~Median()
	{
	}

	void update(T newVal)
	{
		if (filled)
			values[ptr] = (newVal
					+ values[(ptr - 1 + bufferSize) % bufferSize] * medAmt)
					* medDiv;
		else
			values[ptr] = newVal;

		ptr++;
		if (!filled && ptr == 2)
			filled = true;
		ptr %= bufferSize;
	}

	void add(T _newVal)
	{
		values[ptr % bufferSize] = _newVal;
		ptr++;
	}
	void calcMed()
	{
		T tmp;
		for (int i = 0; i < bufferSize; i++)
			tmp += values[i];
		tmp /= static_cast<float>(bufferSize);
		medValue = tmp;
	}
	T get()
	{
		return values[(ptr - 1 + bufferSize) % bufferSize];
	}
	T getMed()
	{
		return medValue;
	}
	void setInitVal(T _newVal){
		for (unsigned int i=0; i<bufferSize; i++)
			values[i] = _newVal;
	}
	void clear()
	{
		delete[] values;
		values = new T[bufferSize];
		filled = false;
	}

private:
	T* values;
	T medValue;
	float medAmt;
	float medDiv;
	bool filled;
	unsigned short ptr;
	unsigned int bufferSize;
};
}
