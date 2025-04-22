//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#pragma once

#include "GeoPrimitives/GeoPrimitive.h"

namespace ara {
class Sphere : public GeoPrimitive {
public:
    Sphere(float radius, int nrSlices, bool cclockw = true, bool triangulate = true, bool genTexCoord = true);

    virtual ~Sphere() = default;

    void init() override;

#ifndef __EMSCRIPTEN__

    void draw(TFO *_tfo = nullptr) override;

#else
    void draw();
#endif

    void remove();

    GLfloat *getPositions();

    GLfloat *getNormals();

    GLfloat *getTexCoords();

    GLfloat *getColors();

    GLuint *getIndices();

    unsigned int getNrVertices();

private:
    bool                    genTexCoord    = false;
    bool                    cclockw        = false;
    bool                    triangulate    = false;
    float                   radius         = 0.f;
    std::vector<CoordType> *instAttribs    = nullptr;
    int                     maxNrInstances = 1;
    unsigned int            numberSlices   = 0;
    const unsigned int      MAX_VERTICES   = 1048576;
    const unsigned int      MAX_INDICES    = MAX_VERTICES * 4;

    unsigned int numberVertices = 0;

    GLfloat *vertices  = nullptr;
    GLfloat *normals   = nullptr;
    GLfloat *texCoords = nullptr;
    GLfloat *colors    = nullptr;
    GLuint  *indices   = nullptr;

    GLfloat *tVertices  = nullptr;
    GLfloat *tNormals   = nullptr;
    GLfloat *tTexCoords = nullptr;
    GLfloat *tColors    = nullptr;
};
}  // namespace ara
