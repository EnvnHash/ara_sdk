//
//  Created by Sven Hahne on 16.07.14.
//  Copyright (c) 2014 Sven Hahne. All rights reserved..
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
