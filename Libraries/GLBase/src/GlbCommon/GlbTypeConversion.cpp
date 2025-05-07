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


#include <GlbCommon/GlbCommon.h>

namespace ara {

GLenum postGLError(bool silence) {
    GLenum glErr = glGetError();

    static std::unordered_map<GLenum, std::string> errCodeMap = {
        { GL_INVALID_ENUM, "GL ERROR: GL_INVALID_ENUM" },
        { GL_INVALID_VALUE, "GL ERROR: GL_INVALID_VALUE, A numeric argument is out of range"},
        { GL_INVALID_OPERATION, "GL ERROR: GL_INVALID_OPERATION, The specified operation is not allowed in the current state."},
        { GL_INVALID_FRAMEBUFFER_OPERATION, "GL ERROR: GL_INVALID_FRAMEBUFFER_OPERATION, The framebuffer object is not complete."},
#ifndef ARA_USE_GLES31
        { GL_STACK_OVERFLOW, "GL ERROR: GL_STACK_OVERFLOW, Given when a stack pushing operation cannot be done because it would overflow the limit of that stack's size."},
        { GL_STACK_UNDERFLOW, "GL ERROR: GL_STACK_UNDERFLOW, Given when a stack popping operation cannot be done because the stack is already at its lowest point." },
#endif
        { GL_OUT_OF_MEMORY, "GL ERROR: GL_OUT_OF_MEMORY, GL_OUT_OF_MEMORY" }
    };

    if (glErr != GL_NO_ERROR && !silence) {
        LOGE << "";
        LOGE << (errCodeMap.find(glErr) != errCodeMap.end() ? errCodeMap[glErr] : "GL ERROR");
        LOGE << "\n";
        return glErr;
    }

    return GL_NO_ERROR;
}

#ifdef ARA_USE_EGL
std::string eglErrorString(EGLint err) {
    static std::unordered_map<GLenum, std::string> errCodeMap = {
        { EGL_SUCCESS, "no error" },
        { EGL_NOT_INITIALIZED, "EGL not, or could not be, initialized" },
        { EGL_BAD_ACCESS, "access violation" },
        { EGL_BAD_ALLOC, "could not allocate resources" },
        { EGL_BAD_ATTRIBUTE, "invalid attribute" },
        { EGL_BAD_CONTEXT, "invalid context specified" },
        { EGL_BAD_CONFIG, "invald m_frame buffer configuration specified" },
        { EGL_BAD_CURRENT_SURFACE, "current window : return pbuffer or pixmap surface is no longer valid" },
        { EGL_BAD_DISPLAY, "invalid display specified" },
        { EGL_BAD_SURFACE, "invalid surface specified" },
        { EGL_BAD_MATCH, "bad argument match" },
        { EGL_BAD_PARAMETER, "invalid paramater" },
        { EGL_BAD_NATIVE_PIXMAP, "invalid NativePixmap" },
        { EGL_BAD_NATIVE_WINDOW, "invalid NativeWindow" },
        { EGL_CONTEXT_LOST, "APM event caused context loss" }
    }
    return errCodeMap.find(err) != errCodeMap.end() : errCodeMap[err] : "unknown error " + std::to_string(err);
}
#endif

GLenum getExtType(GLenum tp) {
    static std::unordered_map<GLenum, GLenum> extTypeMap = {
        { GL_R8,  GL_RED },
{ GL_R8_SNORM,  GL_RED },
#ifndef ARA_USE_GLES31
        { GL_R16,  GL_RED },
        { GL_R16_SNORM,  GL_RED },
#endif
        { GL_RG8,  GL_RG },
        { GL_RG8_SNORM,  GL_RG },
#ifndef ARA_USE_GLES31
        { GL_RG16,  GL_RG },
        { GL_RG16_SNORM,  GL_RG },
        { GL_R3_G3_B2,  GL_RGB },
        { GL_RGB4,  GL_RGB },
        { GL_RGB5,  GL_RGB },
#endif
        { GL_RGB565,  GL_RGB },
        { GL_RGB8,  GL_RGB },
        { GL_RGB8_SNORM,  GL_RGB },
#ifndef ARA_USE_GLES31
        { GL_RGB10,  GL_RGB },
        { GL_RGB12,  GL_RGB },
        { GL_RGB16,  GL_RGB },
        { GL_RGB16_SNORM,  GL_RGB },
        { GL_RGBA2,  GL_RGBA },
#endif
        { GL_RGBA4,  GL_RGBA },
        { GL_RGB5_A1,  GL_RGBA },
        { GL_RGBA8, GL_BGRA },
        { GL_RGBA8_SNORM,  GL_RGBA },
        { GL_RGB10_A2,  GL_RGBA },
        { GL_RGB10_A2UI,  GL_RGBA },
#ifndef ARA_USE_GLES31
        { GL_RGBA12,  GL_RGBA },
        { GL_RGBA16,  GL_RGBA },
        { GL_RGBA16_SNORM,  GL_RGBA },
#endif
        { GL_SRGB8,  GL_RGB },
        { GL_SRGB8_ALPHA8,  GL_RGBA },
        { GL_R16F,  GL_RED },
        { GL_RG16F,  GL_RG },
        { GL_RGB16F,  GL_RGB },
        { GL_RGBA16F,  GL_RGBA },
        { GL_R32F,  GL_RED },
        { GL_RG32F,  GL_RG },
        { GL_RGB32F,  GL_RGB },
        { GL_RGBA32F,  GL_RGBA },
        { GL_R11F_G11F_B10F,  GL_RGB },
        { GL_RGB9_E5,  GL_RGB },
        { GL_R8I,  GL_RED_INTEGER },
        { GL_R8UI,  GL_RED_INTEGER },
        { GL_R16I,  GL_RED_INTEGER },
        { GL_R16UI,  GL_RED_INTEGER },
        { GL_R32I,  GL_RED_INTEGER },
        { GL_R32UI,  GL_RED_INTEGER },
        { GL_RG8I,  GL_RG_INTEGER },
        { GL_RG8UI,  GL_RG_INTEGER },
        { GL_RG16I,  GL_RG_INTEGER },
        { GL_RG16UI,  GL_RG_INTEGER },
        { GL_RG32I,  GL_RG_INTEGER },
        { GL_RG32UI,  GL_RG_INTEGER },
        { GL_RGB8I,  GL_RGB_INTEGER },
        { GL_RGB8UI,  GL_RGB_INTEGER },
        { GL_RGB16I,  GL_RGB_INTEGER },
        { GL_RGB16UI,  GL_RGB_INTEGER },
        { GL_RGB32I,  GL_RGB_INTEGER },
        { GL_RGB32UI,  GL_RGB_INTEGER },
        { GL_RGBA8I,  GL_RGBA_INTEGER },
        { GL_RGBA8UI,  GL_RGBA_INTEGER },
        { GL_RGBA16I,  GL_RGBA_INTEGER },
        { GL_RGBA16UI,  GL_RGBA_INTEGER },
        { GL_RGBA32I,  GL_RGBA_INTEGER },
        { GL_RGBA32UI,  GL_RGBA_INTEGER },
        { GL_DEPTH_COMPONENT24,  GL_DEPTH_COMPONENT },
        { GL_DEPTH24_STENCIL8,  GL_DEPTH_STENCIL },
        { GL_DEPTH32F_STENCIL8,  GL_DEPTH_STENCIL },
        { GL_DEPTH_COMPONENT32F,  GL_DEPTH_COMPONENT },
    };
    return extTypeMap.find(tp) != extTypeMap.end() ? extTypeMap[tp] : 0;
}

GLenum getPixelType(GLenum tp) {
    static std::unordered_map<GLenum, GLenum> typeMap = {
        { GL_R8, GL_UNSIGNED_BYTE },
        { GL_R8_SNORM, GL_UNSIGNED_BYTE },
#ifndef ARA_USE_GLES31
        { GL_R16, GL_UNSIGNED_SHORT },
        { GL_R16_SNORM, GL_UNSIGNED_SHORT },
#endif
        { GL_RG8, GL_UNSIGNED_BYTE },
        { GL_RG8_SNORM, GL_UNSIGNED_BYTE },
#ifndef ARA_USE_GLES31
        { GL_RG16, GL_UNSIGNED_SHORT },
        { GL_RG16_SNORM, GL_UNSIGNED_SHORT },
        { GL_R3_G3_B2, GL_UNSIGNED_BYTE },
        { GL_RGB4, GL_UNSIGNED_BYTE },
        { GL_RGB5, GL_UNSIGNED_BYTE },
        { GL_RGB565, GL_UNSIGNED_BYTE },
#endif
        { GL_RGB8, GL_UNSIGNED_BYTE },
        { GL_RGB8_SNORM, GL_UNSIGNED_BYTE },
#ifndef ARA_USE_GLES31
        { GL_RGB10, GL_UNSIGNED_BYTE },
        { GL_RGB12, GL_UNSIGNED_BYTE },
        { GL_RGB16, GL_UNSIGNED_SHORT },
        { GL_RGB16_SNORM, GL_UNSIGNED_SHORT },
        { GL_RGBA2, GL_UNSIGNED_BYTE },
#endif
        { GL_RGBA4, GL_UNSIGNED_BYTE },
        { GL_RGB5_A1, GL_UNSIGNED_BYTE },
        { GL_RGBA8, GL_UNSIGNED_BYTE },
        { GL_RGBA8_SNORM, GL_UNSIGNED_BYTE },
        { GL_RGB10_A2, GL_UNSIGNED_BYTE },
        { GL_RGB10_A2UI, GL_UNSIGNED_BYTE },
#ifndef ARA_USE_GLES31
        { GL_RGBA12, GL_UNSIGNED_BYTE },
        { GL_RGBA16, GL_UNSIGNED_SHORT },
        { GL_RGBA16_SNORM, GL_UNSIGNED_SHORT },
#endif
        { GL_SRGB8, GL_UNSIGNED_BYTE },
        { GL_SRGB8_ALPHA8, GL_UNSIGNED_BYTE },
        { GL_R16F, GL_HALF_FLOAT },
        { GL_RG16F, GL_HALF_FLOAT },
        { GL_RGB16F, GL_HALF_FLOAT },
        { GL_RGBA16F, GL_HALF_FLOAT },
        { GL_R32F, GL_FLOAT },
        { GL_RG32F, GL_FLOAT },
        { GL_RGB32F, GL_FLOAT },
        { GL_RGBA32F, GL_FLOAT },
        { GL_R11F_G11F_B10F, GL_FLOAT },
        { GL_RGB9_E5, GL_UNSIGNED_BYTE },
        { GL_R8I, GL_BYTE },
        { GL_R8UI, GL_UNSIGNED_BYTE },
        { GL_R16I, GL_SHORT },
        { GL_R16UI, GL_UNSIGNED_SHORT },
        { GL_R32I, GL_INT },
        { GL_R32UI, GL_UNSIGNED_INT },
        { GL_RG8I, GL_BYTE },
        { GL_RG8UI, GL_UNSIGNED_BYTE },
        { GL_RG16I, GL_SHORT },
        { GL_RG16UI, GL_UNSIGNED_SHORT },
        { GL_RG32I, GL_INT },
        { GL_RG32UI, GL_UNSIGNED_INT },
        { GL_RGB8I, GL_BYTE },
        { GL_RGB8UI, GL_UNSIGNED_BYTE },
        { GL_RGB16I, GL_SHORT },
        { GL_RGB16UI, GL_UNSIGNED_SHORT },
        { GL_RGB32I, GL_INT },
        { GL_RGB32UI, GL_UNSIGNED_INT },
        { GL_RGBA8I, GL_BYTE },
        { GL_RGBA8UI, GL_UNSIGNED_BYTE },
        { GL_RGBA16I, GL_SHORT },
        { GL_RGBA16UI, GL_UNSIGNED_SHORT },
        { GL_RGBA32I, GL_INT },
        { GL_RGBA32UI, GL_UNSIGNED_INT },
        { GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT },
        { GL_DEPTH24_STENCIL8, GL_UNSIGNED_INT_24_8 },
        { GL_DEPTH32F_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV },
        { GL_DEPTH_COMPONENT32F, GL_FLOAT },
    };
    return typeMap.find(tp) != typeMap.end() ? typeMap[tp] : GLenum{};
}

short getNrColChans(GLenum tp) {
    static std::unordered_map<short, GLenum> typeMap = {
        { GL_R8, 1 },
        { GL_R8_SNORM, 1 },
#ifndef ARA_USE_GLES31
        { GL_R16, 1 },
        { GL_R16_SNORM, 1 },
#endif
        { GL_RG8, 2 },
        { GL_RG8_SNORM, 2 },
#ifndef ARA_USE_GLES31
        { GL_RG16, 2 },
        { GL_RG16_SNORM, 2 },
        { GL_R3_G3_B2, 3 },
        { GL_RGB4, 3 },
        { GL_RGB5, 3 },
#endif
        { GL_RGB565, 3 },
        { GL_RGB8, 3 },
        { GL_RGB8_SNORM, 3 },
#ifndef ARA_USE_GLES31
        { GL_RGB10, 3 },
        { GL_RGB12, 3 },
        { GL_RGB16, 3 },
        { GL_RGB16_SNORM, 3 },
        { GL_RGBA2, 4 },
#endif
        { GL_RGBA4, 4 },
        { GL_RGB5_A1, 4 },
        { GL_RGBA8, 4 },
        { GL_RGBA8_SNORM, 4 },
        { GL_RGB10_A2, 4 },
        { GL_RGB10_A2UI, 4 },
#ifndef ARA_USE_GLES31
        { GL_RGBA12, 4 },
        { GL_RGBA16, 4 },
        { GL_RGBA16_SNORM, 4 },
#endif
        { GL_SRGB8, 3 },
        { GL_SRGB8_ALPHA8, 4 },
        { GL_R16F, 1 },
        { GL_RG16F, 2 },
        { GL_RGB16F, 3 },
        { GL_RGBA16F, 4 },
        { GL_R32F, 1 },
        { GL_RG32F, 2 },
        { GL_RGB32F, 3 },
        { GL_RGBA32F, 4 },
        { GL_R11F_G11F_B10F, 3 },
        { GL_RGB9_E5, 4 },
        { GL_R8I, 1 },
        { GL_R8UI, 1 },
        { GL_R16I, 1 },
        { GL_R16UI, 1 },
        { GL_R32I, 1 },
        { GL_R32UI, 1 },
        { GL_RG8I, 2 },
        { GL_RG8UI, 2 },
        { GL_RG16I, 2 },
        { GL_RG16UI, 2 },
        { GL_RG32I, 2 },
        { GL_RG32UI, 2 },
        { GL_RGB8I, 3 },
        { GL_RGB8UI, 3 },
        { GL_RGB16I, 3 },
        { GL_RGB16UI, 3 },
        { GL_RGB32I, 3 },
        { GL_RGB32UI, 3 },
        { GL_RGBA8I, 4 },
        { GL_RGBA8UI, 4 },
        { GL_RGBA16I, 4 },
        { GL_RGBA16UI, 4 },
        { GL_RGBA32I, 4 },
        { GL_RGBA32UI, 4 },
    };
    return typeMap.find(tp) != typeMap.end() ? typeMap[tp] : 0;
}

uint getBitCount(GLenum tp) {
    static std::unordered_map<uint, GLenum> typeMap = {
        { GL_R8, 8 },
        { GL_R8_SNORM, 8 },
#ifndef ARA_USE_GLES31
        { GL_R16, 16 },
        { GL_R16_SNORM, 16 },
#endif
        { GL_RG8, 16 },
        { GL_RG8_SNORM, 16 },
#ifndef ARA_USE_GLES31
        { GL_RG16, 32 },
        { GL_RG16_SNORM, 32 },
        { GL_R3_G3_B2, 8 },
        { GL_RGB4, 12 },
        { GL_RGB5, 15 },
#endif
        { GL_RGB565, 16 },
        { GL_RGB8, 24 },
        { GL_RGB8_SNORM, 24 },
#ifndef ARA_USE_GLES31
        { GL_RGB10, 30 },
        { GL_RGB12, 36 },
        { GL_RGB16, 48 },
        { GL_RGB16_SNORM, 48 },
        { GL_RGBA2, 26 },
#endif
        { GL_RGBA4, 28 },
        { GL_RGB5_A1, 16 },
        { GL_RGBA8, 32 },
        { GL_RGBA8_SNORM, 32 },
        { GL_RGB10_A2, 32 },
        { GL_RGB10_A2UI, 32 },
#ifndef ARA_USE_GLES31
        { GL_RGBA12, 48 },
        { GL_RGBA16, 64 },
        { GL_RGBA16_SNORM, 64 },
#endif
        { GL_SRGB8, 24 },
        { GL_SRGB8_ALPHA8, 32 },
        { GL_R16F, 16 },
        { GL_RG16F, 32 },
        { GL_RGB16F, 48 },
        { GL_RGBA16F, 64 },
        { GL_R32F, 32 },
        { GL_RG32F, 64 },
        { GL_RGB32F, 96 },
        { GL_RGBA32F, 128 },
        { GL_R11F_G11F_B10F, 32 },
        { GL_RGB9_E5, 32 },
        { GL_R8I, 8 },
        { GL_R8UI, 8 },
        { GL_R16I, 16 },
        { GL_R16UI, 16 },
        { GL_R32I, 32 },
        { GL_R32UI, 32 },
        { GL_RG8I, 16 },
        { GL_RG8UI, 16 },
        { GL_RG16I, 32 },
        { GL_RG16UI, 32 },
        { GL_RG32I, 64 },
        { GL_RG32UI, 64 },
        { GL_RGB8I, 24 },
        { GL_RGB8UI, 24 },
        { GL_RGB16I, 48 },
        { GL_RGB16UI, 48 },
        { GL_RGB32I, 96 },
        { GL_RGB32UI, 96 },
        { GL_RGBA8I, 32 },
        { GL_RGBA8UI, 32 },
        { GL_RGBA16I, 64 },
        { GL_RGBA16UI, 64 },
        { GL_RGBA32I, 128 },
        { GL_RGBA32UI, 128 },
    };
    return typeMap.find(tp) != typeMap.end() ? typeMap[tp] : 0;
}

}