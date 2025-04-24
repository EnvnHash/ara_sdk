/*
 * NoiseTexNV.cpp
 *
 *  Created on: 16.09.2016
 *      Author: sven
 */

#include "Utils/NoiseTexNV.h"

namespace ara {
NoiseTexNV::NoiseTexNV(int w, int h, int d, GLint _internalFormat)
    : width(w), height(h), depth(d), internalFormat(_internalFormat) {
    std::vector<uint8_t> data(w * h * d * 4);
    auto ptr  = data.begin();
    for (int z = 0; z < d; z++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *ptr++ = rand() & 0xff;
                *ptr++ = rand() & 0xff;
                *ptr++ = rand() & 0xff;
                *ptr++ = rand() & 0xff;
            }
        }
    }

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RGBA, GL_BYTE, data.data());
}

GLuint NoiseTexNV::getTex() { return tex; }

NoiseTexNV::~NoiseTexNV() {}

}  // namespace ara
