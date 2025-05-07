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

#include <Utils/TextureData.h>
#include <Utils/ImageIO/FreeImageHandler.h>

#ifndef __EMSCRIPTEN__
#include <FreeImage.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <emscripten/emscripten.h>
#endif

namespace ara {

class GLBase;

class Texture {
public:
    Texture();
    explicit Texture(GLBase *glbase);
    ~Texture();

#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    static FIBITMAP *ImageLoader(const char *path, int flag);
#endif

    GLuint loadTexture2D(const std::string &filename, int nrMipMaps = 8, bool flipH = false) {
        return loadFromFile(filename, GL_TEXTURE_2D, nrMipMaps, flipH);
    }

    GLuint loadTextureCube(std::string filename, int nrMipMaps = 8, bool flipH = false) {
        m_filename = std::move(filename);
        return loadFromFile(m_filename, GL_TEXTURE_CUBE_MAP, nrMipMaps, flipH);
    }

    GLuint loadTextureRect(std::string filename, bool flipH = false);
    std::array<float, 2> getFileImageSize(const std::string &filename);
    GLuint loadFromFile(const std::string &filename, GLenum textTarget, int nrMipMaps, bool flipH = false);
    GLuint loadFromMemPtr(void *ptr, size_t size, GLenum textTarget, int nrMipMaps, bool flipH = false);
    GLuint procTextureData();

#ifndef __EMSCRIPTEN__
#ifdef ARA_USE_FREEIMAGE
    GLuint loadTexture2D(FIBITMAP *fib, int nrMipMaps, bool flipH = false) {
        return loadFromFib(fib, GL_TEXTURE_2D, nrMipMaps, flipH);
    }

    GLuint loadFromFib(FIBITMAP *fib, GLenum textTarget, int nrMipMaps, bool flipH = false);
#endif
    GLuint allocate1D(uint32_t w, GLenum internalGlDataType, GLenum extGlDataType, GLenum pixelType);
    GLuint allocate3D(uint32_t w, uint32_t h, uint32_t d, GLenum internalGlDataType, GLenum extGlDataType,
                      GLenum textTarget, GLenum pixelType);
#else
    GLuint loadSDL(GLenum _textTarget, int nrMipMaps);
#endif

    void genTexture(GLuint& id);
    GLuint allocate2D(uint32_t w, uint32_t h, GLenum internalGlDataType, GLenum extGlDataType, GLenum textTarget,
                      GLenum pixelType, uint32_t samples = 1, GLboolean fixedsamplelocations = GL_FALSE);
    GLuint gen(GLenum target);
    void   upload(const void *dataPtr) const;
    void   upload(const void *dataPtr, int width, int height, int depth, int xOffs, int yOffs, int zOffs, GLenum uplFormat = 0, GLenum uplPixType = 0) const;

    void setSamplerFiltering(int a_tfMagnification, int a_tfMinification);
    void setFiltering(GLenum magFilter, GLenum minFilter) const;
    void setWraping(GLfloat wrap) const;
    void bind() const;
    void bind(GLuint texUnit) const;
    void bind(GLuint su, GLuint si, GLuint tu);
    void unbind() const;
    void releaseTexture();
    void generateSampler();

    void keepBitmap(bool val)               { m_keepBitmap = val; }
    void setFileName(const std::string &fn) { m_filename = fn; }
    void setWidth(float w)                  { m_texData.width = static_cast<uint32_t>(w); }
    void setHeight(float h)                 { m_texData.height = static_cast<uint32_t>(h); }
    void setGlbase(GLBase *glbase)          { m_glbase = glbase; }

    static void getGlFormatAndType(GLenum glInternalFormat, GLenum &glFormat, GLenum &type);

    [[nodiscard]] glm::vec2 getCoordFromPercent(float xPct, float yPct) const;

    std::string         &getFileName() { return m_filename; }
    [[nodiscard]] uint   getMipMapLevels() const { return m_mipmapLevels; }
    static GLenum        getWrapMode() { return GL_REPEAT; }
    [[nodiscard]] GLenum getInternalFormat() const { return m_texData.internalFormat; }
    [[nodiscard]] bool   isAllocated() const { return m_texData.bAllocated; }
    [[nodiscard]] uint   getId() const { return m_texData.textureID; }
    [[nodiscard]] uint   getHeight() const { return m_texData.height; }
    [[nodiscard]] uint   getWidth() const { return m_texData.width; }
    [[nodiscard]] uint   getDepth() const { return m_texData.depth; }
    [[nodiscard]] uint   getNrChans() const { return m_texData.nrChan; }

    int* getSize() {
        m_sizeInt[0] = static_cast<int32_t>(m_texData.width);
        m_sizeInt[1] = static_cast<int32_t>(m_texData.height);
        return &m_sizeInt[0];
    }

    [[nodiscard]] unsigned char *getBits() const { return m_texData.bits; }
    [[nodiscard]] uint32_t       getBpp() const { return m_texData.bpp; }
    [[nodiscard]] float          getHeightF() const { return static_cast<float>(m_texData.height); }
    [[nodiscard]] float          getWidthF() const { return static_cast<float>(m_texData.width); }
    [[nodiscard]] int            getMinificationFilter() const { return m_tfMinification; }
    [[nodiscard]] int            getMagnificationFilter() const { return m_tfMagnification; }

#ifdef __EMSCRIPTEN__
    Uint32       get_pixel32(SDL_Surface *surface, int x, int y);
    void         put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel);
    SDL_Surface *flip_surface(SDL_Surface *surface, int flags);
#else
#ifdef ARA_USE_FREEIMAGE

    static void saveTexToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, GLenum internalFormat, GLint texNr);
    void saveBufToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan, uint8_t *buf);
    static void saveFrontBuffer(const std::string &filename, int w, int h, int nrChan);

#endif
#endif

protected:
    enum ETextureFiltering {
        TEXTURE_FILTER_MAG_NEAREST = 0,      // Nearest criterion for magnification
        TEXTURE_FILTER_MAG_BILINEAR,         // Bilinear criterion for magnification
        TEXTURE_FILTER_MIN_NEAREST,          // Nearest criterion for minification
        TEXTURE_FILTER_MIN_BILINEAR,         // Bilinear criterion for minification
        TEXTURE_FILTER_MIN_NEAREST_MIPMAP,   // Nearest criterion for
                                             // minification, but on closest
                                             // mipmap
        TEXTURE_FILTER_MIN_BILINEAR_MIPMAP,  // Bilinear criterion for
                                             // minification, but on closest
                                             // mipmap
        TEXTURE_FILTER_MIN_TRILINEAR,        // Bilinear criterion for minification on
                                             // two closest mipmaps, then averaged
    };

#ifdef __EMSCRIPTEN__
    const int FLIP_VERTICAL   = 1;
    const int FLIP_HORIZONTAL = 2;
#endif

    GLuint              m_texture = 0;
    TextureData         m_texData;
    GLBase*             m_glbase = nullptr;
    GLuint              m_samplerID   = 0;
    GLint               m_samplerUnit = 0;
    bool                m_generateMips    = false;
    bool                m_keepBitmap      = false;
    int                 m_tfMinification  = 0;
    int                 m_tfMagnification = 0;
    uint                m_mipmapLevels    = 1;
    GLsizei             m_maxTexSize      = 0;
    std::string         m_filename;
    int                 m_sizeInt[2]{0, 0};
    FIBITMAP*           m_saveBufCont = nullptr;
};
}  // namespace ara
