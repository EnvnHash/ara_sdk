/*
 * Cylinder.h
 *
 *  Created on: Jul 12, 2018
 *      Author: sven
 */

#ifndef CYLINDER_H_
#define CYLINDER_H_

#include "GeoPrimitives/GeoPrimitive.h"

namespace tav
{

class Cylinder : public GeoPrimitive
{
public:
	Cylinder(unsigned int _nrSegs, std::vector<coordType>* _instAttribs = NULL,
			int _maxNrInstances=1);
	virtual ~Cylinder();
	void init();
	void remove();

private:
	std::vector<std::pair<glm::vec3, float> > 	sides;
	std::vector<coordType>*						instAttribs;
	int 										maxNrInstances = 1;
	int 										nrSubDiv;
	unsigned int 								nrSegs;
	unsigned int 								totNrPoints;
};

} /* namespace tav */

#endif /* CYLINDER_H_ */
