/*
 * Cylinder.h
 *
 *  Created on: Jul 12, 2018
 *      Author: sven
 */

#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {

class Cylinder : public GeoPrimitive {
public:
    explicit Cylinder(unsigned int nrSegs, std::vector<CoordType> *instAttribs = nullptr, int maxNrInstances = 1);
    ~Cylinder() override = default;
    void init() override;

    static void createCapCenters(std::vector<glm::vec3>::iterator& pos, std::vector<glm::vec3>::iterator& norm);
    static void remove() {}

private:
    std::vector<CoordType> *m_instAttribs    = nullptr;
    int32_t                 m_maxNrInstances = 1;
    int32_t                 m_nrSegs         = 0;
    unsigned int            m_totNrPoints    = 0;
};

}  // namespace ara
