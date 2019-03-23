//
//  Line.h
//  tav_gl4
//
//  Created by Sven Hahne on 20.08.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
//

#pragma once

#include <vector>
#include <utility>
#include "GeoPrimitives/GeoPrimitive.h"
#include "GLUtils/glm_utils.h"

namespace tav
{
class Line: public GeoPrimitive
{
public:
	Line();
	Line(int _nrSegments);
	Line(int _nrSegments, float _r, float _g, float _b, float _a);
	Line(int _nrSegments, float _r, float _g, float _b, float _a,
			std::vector<coordType>* _instAttribs, int _nrInstance);
	~Line();
	void init();
	void remove();
	int nrSegments;
private:
	std::vector<coordType>* instAttribs;
	int maxNrInstances = 1;
	bool indexed;
};
}
