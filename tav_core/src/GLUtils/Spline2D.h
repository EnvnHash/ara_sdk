//
//  Spline2D.h
//  Tav_App
//
//  Created by Sven Hahne on 16/6/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#ifndef Tav_App_Spline2D_h
#define Tav_App_Spline2D_h

#include <glm/glm.hpp>
#include <vector>
#include "spline.h"

namespace tav
{
class Spline2D
{
public:
	Spline2D()
	{};

	//--------------------------------------------------------------
	// 0 - 1 : 0.0 = first Entry, 1.0 = last Entry
	glm::vec2 sampleAt(double t)
	{
		glm::vec2 newPos;
		if (rebuildSpline)
			buildSpl();

		newPos = glm::vec2(
				xCor(_distX.front() + t * (_distX.back() - _distX.front())),
				yCor(_distY.front() + t * (_distY.back() - _distY.front())));
		return newPos;
	}
	;

	//--------------------------------------------------------------
	void buildSpl()
	{
		xCor.set_points(_distX, _dataX, cubicInterp);
		yCor.set_points(_distY, _dataY, cubicInterp);
		rebuildSpline = false;
	}
	;

	//--------------------------------------------------------------
	void setCubicInterp(bool _val)
	{
		cubicInterp = _val;
	}
	;

	//--------------------------------------------------------------
	void push_back(const glm::vec2& newData)
	{
		_dataX.push_back(static_cast<double>(newData.x));
		if ((int) _distX.size() > 0)
			_distX.push_back(_distX.back() + 1.0);
		else
			_distX.push_back(0);

		_dataY.push_back(static_cast<double>(newData.y));
		if ((int) _distY.size() > 0)
			_distY.push_back(_distY.back() + 1.0);
		else
			_distY.push_back(0);

		rebuildSpline = true;
	}
	;

	//--------------------------------------------------------------
	void push_back(const glm::vec2& newData, double time)
	{
		_dataX.push_back(static_cast<double>(newData.x));
		_distX.push_back(time);
		_dataY.push_back(static_cast<double>(newData.y));
		_distY.push_back(time);

		rebuildSpline = true;
	}
	;

	//--------------------------------------------------------------
	void pop_back()
	{
		_dataX.pop_back();
		_distX.pop_back();
		_dataY.pop_back();
		_distY.pop_back();

		rebuildSpline = true;
	}
	;

	void insert(int pos, glm::vec2& v);

	//--------------------------------------------------------------
	int size() const
	{
		return static_cast<int>(_dataX.size());
	}
	;

	//--------------------------------------------------------------
	void erase(int i)
	{
		if (static_cast<int>(_dataX.size()) > i)
		{
			_dataX.erase(_dataX.begin() + i);
			_distX.erase(_distX.begin() + i);
			_dataY.erase(_dataY.begin() + i);
			_distY.erase(_distY.begin() + i);
		}

		rebuildSpline = true;
	}
	;

	//--------------------------------------------------------------
	void clear()
	{
		_dataX.clear();
		_distX.clear();
		_dataY.clear();
		_distY.clear();
	}
	;

protected:
	bool rebuildSpline;
	bool cubicInterp;
	std::vector<double> _dataX;
	std::vector<double> _distX;
	std::vector<double> _dataY;
	std::vector<double> _distY;
	tk::spline xCor;
	tk::spline yCor;
};
}

#endif
