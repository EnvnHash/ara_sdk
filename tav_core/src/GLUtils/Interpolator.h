//
//  MsaInterpolator.h
//  Tav_App
//
//  Created by Sven Hahne on 20/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#ifndef __Tav_App__Interpolator__
#define __Tav_App__Interpolator__

#include <glm/glm.hpp>
#include <stdio.h>
#include "math_utils.h"

namespace tav
{
typedef enum
{
	kInterpolationLinear, kInterpolationCubic,
} InterpolationType;

template<typename T>
class InterpolatorT
{
public:

	bool verbose;

	InterpolatorT();

	// interpolate and re-sample at t position along the spline
	// where t: 0....1 based on length of spline
	T sampleAt(float t) const;

	void setInterpolation(InterpolationType i = kInterpolationCubic);
	int getInterpolation() const;

	void setUseLength(bool b);
	bool getUseLength() const;

	// return length upto data point i
	// leave blank (-1) to return length of entire data set
	// only valid if setUseLength is true
	// uses current interpolation settings for lenth calculation
	// returns cached value, no calculations done in this function
	const float getLength(int i = -1) const;

	// set number of subdivisions used to calculation length of segment
	void setLengthSubdivisions(int i = 100);
	int getLengthSubdivisions() const;

	/******************* stl::container wrapper functions *******************/
	void push_back(const T& newData);
	void pop_back();
	void insert(int pos, const T& v);
	int size() const;
	void reserve(int i);
	void erase(int i);
	void clear();
	const T& at(int i) const;
	std::vector<T>& getData();
	const std::vector<T>& getData() const;

protected:
	InterpolationType _interpolationMethod;
	bool _useLength;
	int _lengthSubdivisions;// number of subdivisions used for length calculation
	std::vector<T> _data;				// std::vector of all data
	std::vector<float> _dist;// std::vector of cumulative Lengths from i'th data point to beginning of spline

	// calculates length of segment prior to (leading up to) i'th point
	float calcSegmentLength(int i);

	// update all Lengths in _dist array
	void updateAllLengths();

	// given t(0...1) find the node index directly to the left of the point
	void findPosition(float t, int &leftIndex, float &mu) const;

	T linearInterpolate(const T& y1, const T& y2, float mu) const;

	// this function is from Paul Bourke's site
	// http://local.wasp.uwa.edu.au/~pbourke/miscellaneous/interpolation/
	T cubicInterpolate(const T& y0, const T& y1, const T& y2, const T& y3,
			float mu) const;

};

//----------------------------------------------------------------------------
//--------------------------------------------------------------

//--------------------------------------------------------------
template<typename T>
InterpolatorT<T>::InterpolatorT()
{
	setInterpolation();
	setUseLength(false);
	setLengthSubdivisions();
	verbose = false;
}

//--------------------------------------------------------------
// use catmull rom interpolation to re-sample At normT position along the spline
// where normT: 0....1 based on length of spline
template<typename T>
T InterpolatorT<T>::sampleAt(float t) const
{
	int numItems = size();
	if (numItems == 0)
	{
		//				if(verbose) printf("InterpolatorT: not enough samples", t);
		return T();
	}

	if (t > 1)
		t = 1;
	else if (t < 0)
		t = 0;
	int i0, i1, i2, i3;
	float mu;

	findPosition(t, i1, mu);

	// if less than 4 data points, force linear interpolation
	InterpolationType it = _interpolationMethod;
	if (numItems < 4)
		it = kInterpolationLinear;

	switch (it)
	{
	case kInterpolationCubic:
		i0 = i1 - 1;
		i2 = i1 + 1;
		i3 = i2 + 1;

		if (i0 < 0)
			i0 = 0;
		if (i3 >= numItems)
			i3 = numItems - 1;

		return cubicInterpolate(at(i0), at(i1), at(i2), at(i3), mu);
		break;

	case kInterpolationLinear:
		i2 = i1 + 1;
		if (i2 >= numItems)
			i2 = numItems - 1;
		return linearInterpolate(at(i1), at(i2), mu);
		break;
	}
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::setInterpolation(InterpolationType i)
{
	_interpolationMethod = i;
	updateAllLengths();
}

//--------------------------------------------------------------
template<typename T>
int InterpolatorT<T>::getInterpolation() const
{
	return _interpolationMethod;
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::setUseLength(bool b)
{
	_useLength = b;
	if (_useLength)
		updateAllLengths();
	else
		_dist.clear();
}

//--------------------------------------------------------------
template<typename T>
bool InterpolatorT<T>::getUseLength() const
{
	return _useLength;
}

//--------------------------------------------------------------
template<typename T>
const float InterpolatorT<T>::getLength(int i) const
{
	if (_useLength)
	{
		return i < 0 ? _dist[_dist.size() - 1] : _dist.at(i);
	}
	else
	{
		return 0;
	}
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::setLengthSubdivisions(int i)
{
	_lengthSubdivisions = i;
}

//--------------------------------------------------------------
template<typename T>
int InterpolatorT<T>::getLengthSubdivisions() const
{
	return _lengthSubdivisions;
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::push_back(const T& newData)
{
	_data.push_back(newData);						// add data

	if (getUseLength())
	{
		float segmentLength;
		float totalLength;

		if (size() > 1)
		{
			//				T distT		= newData - _data.at(prevIndex);	// get offset to previous node
			//				float dist		= lengthOf(distT);					// actual Length to node

			segmentLength = calcSegmentLength(size() - 1);
			totalLength = segmentLength + _dist.at(size() - 2);
		}
		else
		{
			segmentLength = 0;
			totalLength = 0;
		}

		_dist.push_back(totalLength);

		//				if(verbose) printf("segment length = %f | total length = %f\n", segmentLength, totalLength);
	}
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::pop_back()
{
	_data.pop_back();
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::insert(int pos, const T &v)
{
	typename std::vector<T>::const_iterator it = _data.begin();
	_data.insert(it + pos, v);
}

//--------------------------------------------------------------
template<typename T>
int InterpolatorT<T>::size() const
{
	return static_cast<int>(_data.size());
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::reserve(int i)
{
	_data.reserve(i);
	_dist.reserve(i);
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::erase(int i)
{
	if ((int) _data.size() > i)
		_data.erase(_data.begin() + i);
	if ((int) _dist.size() > i)
		_dist.erase(_dist.begin() + i);
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::clear()
{
	_data.clear();
	_dist.clear();
}

//--------------------------------------------------------------
template<typename T>
const T& InterpolatorT<T>::at(int i) const
{
	return _data.at(tav::clamp(i, 0, size() - 1));
}

//--------------------------------------------------------------
template<typename T>
std::vector<T>& InterpolatorT<T>::getData()
{
	return _data;
}

//--------------------------------------------------------------
template<typename T>
const std::vector<T>& InterpolatorT<T>::getData() const
{
	return _data;
}

//--------------------------------------------------------------
template<typename T>
inline float lengthOf(const T &v)
{
	return 1;
}

//--------------------------------------------------------------
template<typename T>
float InterpolatorT<T>::calcSegmentLength(int i)
{
	std::cout
			<< "msa::InterpolatorT<T>::calcSegmentLength(int i) isn't working anymore"
			<< std::endl;
	int numItems = size();

	if (numItems < 2 || i < 1 || i >= numItems)
		return 0;

	bool saveUseLength = _useLength;
	_useLength = false;

	float startPerc = (i - 1) * 1.0f / (numItems - 1);
	float endPerc = (i) * 1.0f / (numItems - 1);
	float incPerc = (endPerc - startPerc) / _lengthSubdivisions;

	T prev = sampleAt(startPerc);
	T cur;

	float segmentLength = 0;
	for (float f = startPerc; f <= endPerc; f += incPerc)
	{
		cur = sampleAt(f);
		segmentLength += lengthOf(cur - prev);
		prev = cur;
	}

	_useLength = saveUseLength;

	if (verbose)
		printf("segment length for %i is %f\n", i, segmentLength);

	return segmentLength;
}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::updateAllLengths()
{
	_dist.clear();

	float curTotal = 0;

	for (int i = 0; i < size(); i++)
	{
		curTotal += calcSegmentLength(i);
		_dist.push_back(curTotal);
	}

}

//--------------------------------------------------------------
template<typename T>
void InterpolatorT<T>::findPosition(float t, int &leftIndex, float &mu) const
{
	int numItems = size();

	switch (numItems)
	{
	case 0:
		leftIndex = 0;
		mu = 0;
		break;

	case 1:
		leftIndex = 0;
		mu = 0;
		break;

	case 2:
		leftIndex = 0;
		mu = t;
		break;

	default:
		if (_useLength)
		{									// need to use
			float totalLengthOfInterpolator = _dist.at(numItems - 1);
			float tDist = totalLengthOfInterpolator * t;// the Length we want to be from the start
			int startIndex = floor(t * (numItems - 1));	// start approximation here
			int i1 = startIndex;
			int limitLeft = 0;
			int limitRight = numItems - 1;

			float distAt1, distAt2;
			//						do {
			for (int iterations = 0; iterations < 100; iterations++)
			{	// limit iterations
				distAt1 = _dist.at(i1);
				if (distAt1 <= tDist)
				{// if Length at i1 is less than desired Length (this is good)
					distAt2 = _dist.at(
							tav::clamp(i1 + 1, 0, (int) _dist.size() - 1));
					if (distAt2 > tDist)
					{
						leftIndex = i1;
						mu = (tDist - distAt1) / (distAt2 - distAt1);
						return;
					}
					else
					{
						limitLeft = i1;
					}
				}
				else
				{
					limitRight = i1;
				}
				i1 = (limitLeft + limitRight) >> 1;
			}
			//						} while(true);

		}
		else
		{
			float actT = t * (numItems - 1);
			leftIndex = floor(actT);
			mu = actT - leftIndex;
		}
	}
}

//--------------------------------------------------------------
template<typename T>
T InterpolatorT<T>::linearInterpolate(const T& y1, const T& y2, float mu) const
{
	return (y2 - y1) * mu + y1;
}

//--------------------------------------------------------------
// this function is from Paul Bourke's site
// http://local.wasp.uwa.edu.au/~pbourke/miscellaneous/interpolation/
template<typename T>
T InterpolatorT<T>::cubicInterpolate(const T& y0, const T& y1, const T& y2,
		const T& y3, float mu) const
{
	float mu2 = mu * mu;
	T a0 = y3 - y2 - y0 + y1;
	T a1 = y0 - y1 - a0;
	T a2 = y2 - y0;
	T a3 = y1;

	return (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3);
}

//--------------------------------------------------------------
typedef InterpolatorT<float> Interpolator1D;
typedef InterpolatorT<glm::vec2> Interpolator2D;
typedef InterpolatorT<glm::vec3> Interpolator3D;

//--------------------------------------------------------------
inline float lengthOf(const glm::vec2& v)
{
	return v.length();
}

//--------------------------------------------------------------
inline float lengthOf(float f)
{
	return f;
}

//--------------------------------------------------------------
inline float lengthOf(const glm::vec3& v)
{
	return v.length();
}

//--------------------------------------------------------------
// OpenGL ES compatibility added by Rob Seward
// http://www.openframeworks.cc/forum/viewtopic.php?f=25&t=3767&p=19865
inline void drawInterpolatorRaw(Interpolator2D &spline, int dotSize = 20,
		int lineWidth = 4)
{
	int numItems = spline.size();

	if (lineWidth)
	{
		glLineWidth(lineWidth);
		GLfloat vertex[numItems * 2];
		for (int i = 0; i < numItems; i++)
		{
			vertex[i * 2] = spline.at(i).x;
			vertex[(i * 2) + 1] = spline.at(i).y;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_LINE_STRIP, 0, numItems);
	}

	if (dotSize)
	{
		glPointSize(dotSize);
		GLfloat vertex[numItems * 2];
		for (int i = 0; i < numItems; i++)
		{
			vertex[i * 2] = spline.at(i).x;
			vertex[(i * 2) + 1] = spline.at(i).y;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_POINTS, 0, numItems);
	}
}

//--------------------------------------------------------------
inline void drawInterpolatorSmooth(Interpolator2D &spline, int numSteps,
		int dotSize = 8, int lineWidth = 2)
{
	float spacing = 1.0 / numSteps;
	if (lineWidth)
	{
		glLineWidth(lineWidth);

		GLfloat vertex[numSteps * 2];
		int i = 0;
		for (float f = 0; f < 1; f += spacing)
		{
			glm::vec2 v = spline.sampleAt(f);
			vertex[i * 2] = v.x;
			vertex[(i * 2) + 1] = v.y;
			i++;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_LINE_STRIP, 0, numSteps);
	}

	if (dotSize)
	{
		glPointSize(dotSize);
		GLfloat vertex[numSteps * 2];
		int i = 0;
		for (float f = 0; f < 1; f += spacing)
		{
			glm::vec2 v = spline.sampleAt(f);
			vertex[i * 2] = v.x;
			vertex[(i * 2) + 1] = v.y;
			i++;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_POINTS, 0, numSteps);
	}
}

//--------------------------------------------------------------
// OpenGL ES compatibility added by Rob Seward
// http://www.openframeworks.cc/forum/viewtopic.php?f=25&t=3767&p=19865
inline void drawInterpolatorRaw(Interpolator3D spline, int dotSize = 20,
		int lineWidth = 4)
{
	int numItems = spline.size();
	if (numItems == 0)
		return;

	if (lineWidth)
	{
		glLineWidth(lineWidth);
		GLfloat vertex[numItems * 3];
		for (int i = 0; i < numItems; i++)
		{
			vertex[i * 3] = spline.at(i).x;
			vertex[(i * 3) + 1] = spline.at(i).y;
			vertex[(i * 3) + 2] = spline.at(i).z;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_LINE_STRIP, 0, numItems);
	}

	if (dotSize)
	{
		glPointSize(dotSize);
		GLfloat vertex[numItems * 3];
		for (int i = 0; i < numItems; i++)
		{
			vertex[i * 3] = spline.at(i).x;
			vertex[(i * 3) + 1] = spline.at(i).y;
			vertex[(i * 3) + 2] = spline.at(i).z;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_POINTS, 0, numItems);
	}
}

//--------------------------------------------------------------
inline void drawInterpolatorSmooth(Interpolator3D spline, int numSteps,
		int dotSize = 8, int lineWidth = 2)
{
	float spacing = 1.0 / numSteps;
	if (spline.size() == 0)
		return;

	if (lineWidth)
	{
		glLineWidth(lineWidth);

		GLfloat vertex[numSteps * 3];
		int i = 0;
		for (float f = 0; f < 1; f += spacing)
		{
			glm::vec3 v = spline.sampleAt(f);
			vertex[i * 3] = v.x;
			vertex[(i * 3) + 1] = v.y;
			vertex[(i * 3) + 2] = v.z;
			i++;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_LINE_STRIP, 0, numSteps);
	}

	if (dotSize)
	{
		glPointSize(dotSize);
		GLfloat vertex[numSteps * 3];
		int i = 0;
		for (float f = 0; f < 1; f += spacing)
		{
			glm::vec3 v = spline.sampleAt(f);
			vertex[i * 3] = v.x;
			vertex[(i * 3) + 1] = v.y;
			vertex[(i * 3) + 2] = v.z;
			i++;
		}
		glEnableClientState (GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertex);
		glDrawArrays(GL_POINTS, 0, numSteps);
	}
}
}

#endif /* defined(__Tav_App__MsaInterpolator__) */
