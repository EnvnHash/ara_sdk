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


#include "GeoPrimitives/Sphere.h"
#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

using namespace std;
using namespace glm;

namespace ara {
Sphere::Sphere(float _radius, int _nrSlices, bool _cclockw, bool _triangulate, bool _genTexCoord)
    : GeoPrimitive(), genTexCoord(_genTexCoord), cclockw(_cclockw), triangulate(_triangulate), radius(_radius),
      numberSlices(_nrSlices) {
    instAttribs    = nullptr;
    maxNrInstances = 1;
    Sphere::init();
}

void Sphere::init() {
    unsigned int i, j;

    unsigned int numberParallels = numberSlices / 2;
    numberVertices               = (numberParallels + 1) * (numberSlices + 1);
    unsigned int numberIndices   = numberParallels * numberSlices * 6;

    float angleStep = (2.0f * static_cast<float>(M_PI)) / static_cast<float>(numberSlices);

    // used later to help us to calculate tangents vectors
    glm::vec4 helpVector = glm::vec4(1.f, 0.f, 0.f, 0.f);
    glm::quat helpQuaternion;

    if (numberSlices < 3 || numberVertices > MAX_VERTICES || numberIndices > MAX_INDICES) {
        std::cerr << "Sphere initialization error!!!!" << std::endl;
    }

    vertices.resize(numberVertices);
    normals.resize(numberVertices);
    texCoords.resize(numberVertices);
    colors.resize(numberVertices);
    indices.resize(numberIndices);

    auto tangents = std::vector<glm::vec4>(numberVertices);

    for (i = 0; i < numberParallels + 1; i++) {
        for (j = 0; j < numberSlices + 1; j++) {
            auto  baseInd = (i * (numberSlices + 1) + j);

            vertices[baseInd].x = radius * sinf(angleStep * static_cast<GLfloat>(i)) * sinf(angleStep * static_cast<GLfloat>(j));
            vertices[baseInd].y = radius * cosf(angleStep * static_cast<GLfloat>(i));
            vertices[baseInd].z = radius * sinf(angleStep * static_cast<GLfloat>(i)) * cosf(angleStep * static_cast<GLfloat>(j));

            normals[baseInd].x = vertices[baseInd].x / radius;
            normals[baseInd].y = vertices[baseInd].y / radius;
            normals[baseInd].z = vertices[baseInd].z / radius;

            colors[baseInd].r = 1.f;
            colors[baseInd].g = 1.f;
            colors[baseInd].b = 1.f;
            colors[baseInd].a = 1.f;

            texCoords[baseInd].x = static_cast<GLfloat>(j) / static_cast<GLfloat>(numberSlices);
            texCoords[baseInd].y = 1.0f - static_cast<GLfloat>(i) / static_cast<GLfloat>(numberParallels);

            // use quaternion to get the tangent vector
            glm::rotate(helpQuaternion, glm::vec4(texCoords[baseInd].x) * static_cast<GLfloat>(M_PI * 2.0));
            auto helpMatrix = glm::mat4(helpQuaternion);
            tangents[baseInd] = helpMatrix * helpVector;
        }
    }

    // generate indices
    unsigned int indexIndices = 0;
    if (cclockw) {
        for (i = 0; i < numberParallels; i++) {
            for (j = 0; j < numberSlices; j++) {
                indices[indexIndices++] = i * (numberSlices + 1) + j;
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + j;
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);

                indices[indexIndices++] = i * (numberSlices + 1) + j;
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);
                indices[indexIndices++] = i * (numberSlices + 1) + (j + 1);
            }
        }
    } else {
        for (i = 0; i < numberParallels; i++) {
            for (j = 0; j < numberSlices; j++) {
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + j;
                indices[indexIndices++] = i * (numberSlices + 1) + j;

                indices[indexIndices++] = i * (numberSlices + 1) + (j + 1);
                indices[indexIndices++] = (i + 1) * (numberSlices + 1) + (j + 1);
                indices[indexIndices++] = i * (numberSlices + 1) + j;
            }
        }
    }

    // everything ready now convert the Mesh to a VAO -> upload to GPU
    GLenum usage = instAttribs ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    m_format = genTexCoord ? "position:3f,normal:3f,texCoord:2f,color:4f" : "position:3f,normal:3f,color:4f";

    m_vao = std::make_unique<VAO>(m_format, usage, instAttribs, maxNrInstances, true);

    if (triangulate) {
        numberVertices = numberIndices;

        tVertices.resize(numberVertices);
        tNormals.resize(numberVertices);
        tTexCoords.resize(numberVertices);
        tColors.resize(numberVertices);

        for (unsigned int l = 0; l < numberIndices; l++) {
            for (unsigned int k = 0; k < 3; k++) {
                tVertices[l + k] = vertices[indices[l] + k];
                tNormals[l + k]  = normals[indices[l] + k];
            }

            for (unsigned int k = 0; k < 2; k++) {
                tTexCoords[l + k] = texCoords[indices[l] + k];
            }

            for (unsigned int k = 0; k < 4; k++) {
                tColors[l + k] = colors[indices[l] + k];
            }
        }

        m_vao->upload(CoordType::Position, &tVertices[0][0], numberVertices);
        m_vao->upload(CoordType::Normal, &tNormals[0][0], numberVertices);
        if (genTexCoord) {
            m_vao->upload(CoordType::TexCoord, &tTexCoords[0][0], numberVertices);
        }
        m_vao->upload(CoordType::Color, &tColors[0][0], numberVertices);
    } else {
        m_vao->upload(CoordType::Position, &vertices[0][0], numberVertices);
        m_vao->upload(CoordType::Normal, &normals[0][0], numberVertices);
        if (genTexCoord) {
            m_vao->upload(CoordType::TexCoord, &texCoords[0][0], numberVertices);
        }
        m_vao->upload(CoordType::Color, &colors[0][0], numberVertices);
        m_vao->setElemIndices(numberIndices, &indices[0]);
    }
}

#ifndef __EMSCRIPTEN__

void Sphere::draw(TFO *_tfo) {
    if (!triangulate) {
        m_vao->drawElements(GL_TRIANGLES, _tfo, GL_TRIANGLES);
    } else {
        m_vao->draw(GL_TRIANGLES, _tfo, GL_TRIANGLES);
    }
}

#else
void Sphere::draw() {
    if (!triangulate)
        m_vao->drawElements(GL_TRIANGLES);
    else
        m_vao->draw(GL_TRIANGLES);
}
#endif

void Sphere::remove() {}

GLfloat *Sphere::getPositions() {
    return triangulate ? &tVertices[0][0] : &vertices[0][0];
}

GLfloat *Sphere::getNormals() {
    return triangulate ? &tNormals[0][0] : &normals[0][0];
}

GLfloat *Sphere::getTexCoords() {
    return triangulate ? &tTexCoords[0][0] : &texCoords[0][0];
}

GLfloat *Sphere::getColors() {
    return triangulate ? &tColors[0][0] : &colors[0][0];
}

GLuint *Sphere::getIndices() {
    return &indices[0];
}

unsigned int Sphere::getNrVertices() const {
    return numberVertices;
}

}  // namespace ara
