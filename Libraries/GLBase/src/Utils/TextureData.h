//
// Created by hahne on 22.04.2025.
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

#include <glb_common/glb_common.h>

#ifdef ARA_USE_FREEIMAGE
struct FIBITMAP;
#endif

namespace ara {

class TextureData {
public:
#ifdef ARA_USE_FREEIMAGE
    FIBITMAP *pBitmap = nullptr;
#endif

    GLuint  textureID      = 0;
    GLenum  target         = GL_TEXTURE_2D;  // Texture target (2D, cube map, etc.)
    GLenum  internalFormat = GL_RGB;         // Recommended internal gpu format.
    GLenum  format         = GL_RGB;         // Format in cpu memory
    GLenum  swizzle[4]{};                  // Swizzle for RGBA
    GLenum  pixelType = GL_UNSIGNED_BYTE;  // type, e.g., GL_UNSIGNED_BYTE. should be named type
    GLsizei mipLevels = 0;                 // Number of present mipmap levels
    GLsizei slices    = 0;                 // Number of slices (for arrays)
    GLubyte *bits      = nullptr;

    float tex_t = 0.f;
    float tex_u = 0.f;

    uint32_t width   = 0;
    uint32_t height  = 0;
    uint32_t depth   = 0;
    uint32_t nrChan  = 3;
    uint32_t samples = 1;
    uint32_t bpp     = 0;

    bool      bAllocated           = false;
    GLboolean fixedsamplelocations = false;

    // separate space for the slices of a cubemap
#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    std::array<uint8_t*, 6> faceData{};
#else
    unsigned char **faceData{};
#endif

};

}