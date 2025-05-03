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

    ~Sphere() override = default;

    void init() override;

#ifndef __EMSCRIPTEN__

    void draw(TFO *_tfo = nullptr) override;

#else
    void draw();
#endif

    static void remove();

    GLfloat *getPositions();
    GLfloat *getNormals();
    GLfloat *getTexCoords();
    GLfloat *getColors();
    GLuint *getIndices();

    unsigned int getNrVertices() const;

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

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec4> colors;
    std::vector<GLuint> indices;

    std::vector<glm::vec3> tVertices;
    std::vector<glm::vec3> tNormals;
    std::vector<glm::vec2> tTexCoords;
    std::vector<glm::vec4> tColors;
};
}  // namespace ara
