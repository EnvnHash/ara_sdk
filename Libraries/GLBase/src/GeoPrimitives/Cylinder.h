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
    void init() override;

    void createTopRings(std::vector<GLfloat>& positions, std::vector<GLfloat>& normals, const std::vector<GLfloat>& ringPos,
                        float cylRadius, unsigned int& ind) const;
    static void createCapCenters(std::vector<GLfloat>& positions, std::vector<GLfloat>& normals, unsigned int& ind);
    void createCapRings(std::vector<GLfloat>& positions, std::vector<GLfloat>& normals, const std::vector<GLfloat>& ringPos,
                        float cylRadius, unsigned int& ind) const;

    void remove() {}

private:
    std::vector<CoordType> *m_instAttribs    = nullptr;
    int                     m_maxNrInstances = 1;
    unsigned int            m_nrSegs         = 0;
    unsigned int            m_totNrPoints    = 0;
};

}  // namespace ara
