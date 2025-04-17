/*
 * NoiseTexNV.h
 *
 *  Created on: 16.09.2016
 *      Author: sven
 */

#pragma once

#include "glb_common/glb_common.h"

namespace ara {

class NoiseTexNV {
public:
    NoiseTexNV(int w, int h, int d, GLint _internalFormat);
    ~NoiseTexNV();
    GLuint getTex();

private:
    int    width          = 0;
    int    height         = 0;
    int    depth          = 0;
    GLint  internalFormat = 0;
    GLuint tex            = 0;
};

}  // namespace ara
