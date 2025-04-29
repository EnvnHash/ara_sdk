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

#include "Utils/NoiseTexNV.h"

namespace ara {
NoiseTexNV::NoiseTexNV(int w, int h, int d, GLint internalFormat)
    : m_width(w), m_height(h), m_depth(d), m_internalFormat(internalFormat) {
    std::vector<uint8_t> data(w * h * d * 4);
    auto ptr  = data.begin();
    for (int z = 0; z < d; z++) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                *ptr++ = static_cast<uint8_t>(getRandF(0.f, 255.f)) & 0xff;
                *ptr++ = static_cast<uint8_t>(getRandF(0.f, 255.f)) & 0xff;
                *ptr++ = static_cast<uint8_t>(getRandF(0.f, 255.f)) & 0xff;
                *ptr++ = static_cast<uint8_t>(getRandF(0.f, 255.f)) & 0xff;
            }
        }
    }

    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_3D, m_tex);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, w, h, d, 0, GL_RGBA, GL_BYTE, data.data());
}

GLuint NoiseTexNV::getTex() const {
    return m_tex;
}

NoiseTexNV::~NoiseTexNV() = default;

}  // namespace ara
