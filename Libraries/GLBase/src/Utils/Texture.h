//
// Generic Texture Manager
// For use with OpenGL 3.2+ and the FreeImage library
// assumes the sampler unit to be called "tex"
//
#pragma once

#ifndef _WIN32
#define _stdcall
#endif

#include "glb_common/glb_common.h"

#ifndef __EMSCRIPTEN__
#include <FreeImage.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <emscripten/emscripten.h>
#endif

namespace ara {

// This is the main image data structure. It contains all the parameters needed
// to place texture data into a texture object using OpenGL.
class TextureData {
public:
#ifdef ARA_USE_FREEIMAGE
    FIBITMAP *pBitmap = nullptr;
#endif
    GLuint textureID      = 0;
    GLenum target         = GL_TEXTURE_2D;  // Texture target (2D, cube map, etc.)
    GLenum internalFormat = GL_RGB;         // Recommended internal gpu format.
    GLenum format         = GL_RGB;         // Format in cpu memory
#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    std::array<BYTE *, 6> faceData;  // separate space for the slices of a cubemap
#else
    unsigned char **faceData;  // separate space for the slices of a cubemap
#endif
    GLenum   swizzle[4]{};                  // Swizzle for RGBA
    GLenum   pixelType = GL_UNSIGNED_BYTE;  // type, e.g., GL_UNSIGNED_BYTE. should be named type
    GLsizei  mipLevels = 0;                 // Number of present mipmap levels
    GLsizei  slices    = 0;                 // Number of slices (for arrays)
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
};

#ifdef ARA_USE_FREEIMAGE

class FreeImg_MemHandler {
public:
    FreeImg_MemHandler(void *ptr, size_t size) {
        memPtr  = ptr;
        memSize = size;
        memPos  = 0;

        fillFreeImageIO(fIO);
    }

    void                  *memPtr  = nullptr;
    size_t                 memSize = 0;
    size_t                 memPos  = 0;
    FreeImageIO            fIO{};
    [[nodiscard]] uint8_t *getPos() const { return ((uint8_t *)memPtr) + memPos; }

    static void fillFreeImageIO(FreeImageIO &io) {
        io.read_proc  = FreeImg_MemHandler::read;
        io.write_proc = FreeImg_MemHandler::write;
        io.seek_proc  = FreeImg_MemHandler::seek;
        io.tell_proc  = FreeImg_MemHandler::tell;
    }

    FreeImageIO *io() { return &fIO; }

    static unsigned _stdcall read(void *buffer, unsigned size, unsigned count, fi_handle handle) {
        auto *h = static_cast<FreeImg_MemHandler *>(handle);

        auto    *dest = static_cast<uint8_t *>(buffer);
        uint8_t *src  = h->getPos();

        for (unsigned c = 0; c < count; c++) {
            std::copy_n(src, size, dest);
            src += size;
            dest += size;
            h->memPos += size;
        }

        return count;
    }

    static unsigned _stdcall write(void *buffer, unsigned size, unsigned count, fi_handle handle) {
        return size;
    }

    static int _stdcall seek(fi_handle handle, long offset, int origin) {
        auto *h = (FreeImg_MemHandler *)handle;

        if (origin == SEEK_SET) {
            h->memPos = 0;
        } else if (origin == SEEK_CUR) {
            h->memPos = offset;
        }

        return 0;
    }

    static long _stdcall tell(fi_handle handle) {
        auto *h = (FreeImg_MemHandler *)handle;
        return (long)(h->memPos);
    }
};

#endif

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

    GLuint loadTexture2D(const char *filename, int nrMipMaps = 8, bool flipH = false) {
        m_filename = std::string(filename);
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

    GLuint allocate2D(uint32_t w, uint32_t h, GLenum internalGlDataType, GLenum extGlDataType, GLenum textTarget,
                      GLenum pixelType, uint32_t samples = 1, GLboolean fixedsamplelocations = GL_FALSE);
    GLuint gen(GLenum target);
    void   upload(void *dataPtr);
    void   upload(void *dataPtr, int width, int height, int depth, int xOffs, int yOffs, int zOffs, GLenum uplFormat = 0, GLenum uplPixType = 0);

    void setSamplerFiltering(int a_tfMagnification, int a_tfMinification);
    void setFiltering(GLenum magFilter, GLenum minFilter);
    void setWraping(GLenum _wrap);
    void bind() const { glBindTexture(m_texData.target, m_texData.textureID); }

    void bind(GLuint texUnit) const {
        glActiveTexture(GL_TEXTURE0 + texUnit);
        glBindTexture(m_texData.target, m_texData.textureID);
    }

    void bind(GLuint su, GLuint si, GLuint tu) {
        m_samplerUnit = su;
        glActiveTexture(GL_TEXTURE0 + tu);
        glBindTexture(m_texData.target, m_texData.textureID);
        glBindSampler(m_samplerUnit, si);
    }

    void unbind() const { glBindTexture(m_texData.target, 0); }

    void releaseTexture() {
        glDeleteTextures(1, &m_texData.textureID);
        m_texData.textureID = 0;
#ifdef ARA_USE_FREEIMAGE
        if (m_keepBitmap) {
            FreeImage_Unload(m_texData.pBitmap);
        }
#endif
    }

    void generateSampler() {
        glGenSamplers(1, &m_samplerID);
        glBindSampler(m_samplerUnit, m_samplerID);
    }

    void keepBitmap(bool val)               { m_keepBitmap = val; }
    void setFileName(const std::string &fn) { m_filename = fn; }
    void setWidth(float w)                  { m_texData.width = (uint32_t)w; }
    void setHeight(float h)                 { m_texData.height = (uint32_t)h; }
    void setGlbase(GLBase *glbase)          { m_glbase = glbase; }

    static void getGlFormatAndType(GLenum glInternalFormat, GLenum &glFormat, GLenum &type);

    GLfloat *getCoordFromPercent(float xPct, float yPct);

    std::string         &getFileName() { return m_filename; }
    [[nodiscard]] uint   getMipMapLevels() const { return m_mipmapLevels; }
    static GLenum        getWrapMode() { return GL_REPEAT; }
    [[nodiscard]] GLenum getInternalFormat() const { return m_texData.internalFormat; }
    [[nodiscard]] bool   isAllocated() const { return m_texData.bAllocated; }
    [[nodiscard]] uint   getId() const { return m_texData.textureID; }
    [[nodiscard]] uint   getHeight() const { return static_cast<uint>(m_texData.height); }
    [[nodiscard]] uint   getWidth() const { return static_cast<uint>(m_texData.width); }
    [[nodiscard]] uint   getDepth() const { return static_cast<uint>(m_texData.depth); }
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

    void saveTexToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, GLenum internalFormat, GLint texNr);
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

    GLuint      m_texture = 0;
    TextureData m_texData;
    GLBase     *m_glbase = nullptr;

    GLuint  m_samplerID   = 0;
    GLint   m_samplerUnit = 0;

    bool    m_generateMips    = false;
    bool    m_keepBitmap      = false;
    int     m_tfMinification  = 0;
    int     m_tfMagnification = 0;
    uint    m_mipmapLevels    = 1;
    GLsizei m_maxTexSize      = 0;

    std::string         m_filename;
    int                 m_sizeInt[2]{0, 0};
    FIBITMAP*           m_saveBufCont = nullptr;
};
}  // namespace ara
