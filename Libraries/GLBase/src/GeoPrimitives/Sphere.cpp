//
//  Sphere.cpp
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "GeoPrimitives/Sphere.h"

#include <Meshes/Mesh.h>
#include <Utils/VAO.h>

namespace ara {
Sphere::Sphere(float _radius, int _nrSlices, bool _cclockw, bool _triangulate, bool _genTexCoord)
    : GeoPrimitive(), radius(_radius), cclockw(_cclockw), numberSlices(_nrSlices), triangulate(_triangulate),
      genTexCoord(_genTexCoord) {
    instAttribs    = nullptr;
    maxNrInstances = 1;
    init();
}

void Sphere::init() {
    unsigned int i, j;
    unsigned int indexIndices;

    unsigned int numberParallels = numberSlices / 2;
    numberVertices               = (numberParallels + 1) * (numberSlices + 1);
    unsigned int numberIndices   = numberParallels * numberSlices * 6;

    float angleStep = (2.0f * (float)M_PI) / ((float)numberSlices);
    // float angleStepY = (2.0f * M_PI) / ((float) numberSlices-1);

    // used later to help us calculating tangents vectors
    glm::vec4 helpVector = glm::vec4(1.f, 0.f, 0.f, 0.f);
    glm::quat helpQuaternion;
    glm::mat4 helpMatrix;

    if (numberSlices < 3 || numberVertices > MAX_VERTICES || numberIndices > MAX_INDICES)
        std::cerr << "Sphere initialization error!!!!" << std::endl;

    vertices  = new GLfloat[3 * numberVertices];
    normals   = new GLfloat[3 * numberVertices];
    texCoords = new GLfloat[2 * numberVertices];
    colors    = new GLfloat[4 * numberVertices];
    indices   = new GLuint[numberIndices];

    glm::vec4 *tangents = new glm::vec4[numberVertices];

    for (i = 0; i < numberParallels + 1; i++) {
        for (j = 0; j < numberSlices + 1; j++) {
            GLuint baseInd        = (i * (numberSlices + 1) + j);
            GLuint vertexIndex    = baseInd * 3;
            GLuint normalIndex    = baseInd * 3;
            GLuint colorIndex     = baseInd * 4;
            GLuint texCoordsIndex = baseInd * 2;
            GLuint tangentIndex   = baseInd;

            vertices[vertexIndex + 0] = radius * sinf(angleStep * (GLfloat)i) * sinf(angleStep * (GLfloat)j);
            vertices[vertexIndex + 1] = radius * cosf(angleStep * (GLfloat)i);
            vertices[vertexIndex + 2] = radius * sinf(angleStep * (GLfloat)i) * cosf(angleStep * (GLfloat)j);

            normals[normalIndex + 0] = vertices[vertexIndex + 0] / radius;
            normals[normalIndex + 1] = vertices[vertexIndex + 1] / radius;
            normals[normalIndex + 2] = vertices[vertexIndex + 2] / radius;

            colors[colorIndex + 0] = 1.f;
            colors[colorIndex + 1] = 1.f;
            colors[colorIndex + 2] = 1.f;
            colors[colorIndex + 3] = 1.f;

            texCoords[texCoordsIndex + 0] = (GLfloat)j / (GLfloat)numberSlices;
            texCoords[texCoordsIndex + 1] = 1.0f - (GLfloat)i / (GLfloat)numberParallels;

            // use quaternion to get the tangent vector
            glm::rotate(helpQuaternion, glm::vec4(texCoords[texCoordsIndex + 0]) * GLfloat(M_PI * 2.0));
            helpMatrix             = glm::mat4(helpQuaternion);
            tangents[tangentIndex] = helpMatrix * helpVector;
        }
    }

    // generate indices
    indexIndices = 0;
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
    GLenum usage = GL_STATIC_DRAW;
    if (instAttribs) usage = GL_DYNAMIC_DRAW;
    if (genTexCoord)
        m_format = "position:3f,normal:3f,texCoord:2f,color:4f";
    else
        m_format = "position:3f,normal:3f,color:4f";

    m_vao = std::make_unique<VAO>(m_format, usage, instAttribs, maxNrInstances, true);

    if (triangulate) {
        numberVertices = numberIndices;

        tVertices  = new GLfloat[numberVertices * 3];
        tNormals   = new GLfloat[numberVertices * 3];
        tTexCoords = new GLfloat[numberVertices * 2];
        tColors    = new GLfloat[numberVertices * 4];

        for (unsigned int j = 0; j < numberIndices; j++) {
            for (unsigned int k = 0; k < 3; k++) {
                tVertices[j * 3 + k] = vertices[indices[j] * 3 + k];
                tNormals[j * 3 + k]  = normals[indices[j] * 3 + k];
            }

            for (unsigned int k = 0; k < 2; k++) tTexCoords[j * 2 + k] = texCoords[indices[j] * 2 + k];

            for (unsigned int k = 0; k < 4; k++) tColors[j * 4 + k] = colors[indices[j] * 4 + k];
        }

        m_vao->upload(CoordType::Position, &tVertices[0], numberVertices);
        m_vao->upload(CoordType::Normal, &tNormals[0], numberVertices);
        if (genTexCoord) m_vao->upload(CoordType::TexCoord, &tTexCoords[0], numberVertices);
        m_vao->upload(CoordType::Color, &tColors[0], numberVertices);
    } else {
        m_vao->upload(CoordType::Position, &vertices[0], numberVertices);
        m_vao->upload(CoordType::Normal, &normals[0], numberVertices);
        if (genTexCoord) m_vao->upload(CoordType::TexCoord, &texCoords[0], numberVertices);
        m_vao->upload(CoordType::Color, &colors[0], numberVertices);
        m_vao->setElemIndices(numberIndices, &indices[0]);
    }
}

#ifndef __EMSCRIPTEN__

void Sphere::draw(TFO *_tfo) {
    if (!triangulate)
        m_vao->drawElements(GL_TRIANGLES, _tfo, GL_TRIANGLES);
    else
        m_vao->draw(GL_TRIANGLES, _tfo, GL_TRIANGLES);
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
    if (triangulate)
        return &tVertices[0];
    else
        return &vertices[0];
}

GLfloat *Sphere::getNormals() {
    if (triangulate)
        return &tNormals[0];
    else
        return &normals[0];
}

GLfloat *Sphere::getTexCoords() {
    if (triangulate)
        return &tTexCoords[0];
    else
        return &texCoords[0];
}

GLfloat *Sphere::getColors() {
    if (triangulate)
        return &tColors[0];
    else
        return &colors[0];
}

GLuint *Sphere::getIndices() { return &indices[0]; }

unsigned int Sphere::getNrVertices() { return numberVertices; }

}  // namespace ara
