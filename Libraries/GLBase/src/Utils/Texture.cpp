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


#include "Utils/Texture.h"

#include "GLBase.h"

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
CMRC_DECLARE(ara);
#else
namespace fs = std::filesystem;
#endif

using namespace std;
using namespace glm;

namespace ara {

Texture::Texture(GLBase *glbase) : m_glbase(glbase) {}

GLuint Texture::loadTextureRect(const std::string& filename, bool flipH) {
    m_filename = filename;
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
    return loadFromFile(m_filename, GL_TEXTURE_RECTANGLE, 1, flipH);
#else
    return loadFromFile(m_filename, GL_TEXTURE_2D, 1);
#endif
}

std::array<float, 2> Texture::getFileImageSize(const std::string &filename) {
    std::array<float, 2> sz{};
#ifdef ARA_USE_CMRC
    auto fs = cmrc::ara::get_filesystem();

    if (fs.exists(filename)) {
        auto file = fs.open(filename);
        if (!file.size()) {
            return {};
        }

        size_t size = file.size();
        if (size > 0) {
            vector<uint8_t> vp(size);
            std::copy(file.begin(), file.end(), vp.begin());
            auto imgSize = FreeImage::GetSize(vp);
            sz = { static_cast<float>(imgSize[0]), static_cast<float>(imgSize[1]) };
        }
#elif !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    if (fs::exists(fs::path(filename))) {
        m_filename        = filename;
        auto imgSize = FreeImage::GetSize(filename);
        sz = { static_cast<float>(imgSize[0]), static_cast<float>(imgSize[1]) };
#else
    // TODO: implement for EMSCRIPTEN
    //if (fs::exists(fs::path(filename))) {
    //    return loadFromSDL(_textTarget, nrMipMaps);
#endif
    } else {
        LOGE << "Texture::loadFromFile Error: file (" << filename << ") does not exist!!!";
    }

    return sz;
}

GLuint Texture::loadFromFile(const std::string &filename, GLenum textTarget, int nrMipMaps, bool flipH) {
#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    if (std::filesystem::exists(std::filesystem::path(filename))) {
        m_filename        = filename;
        auto pBitmap = FreeImage::Load(filename, nullptr);
        return loadFromFib(pBitmap, textTarget, nrMipMaps, flipH);
#else
    if (fs::exists(fs::path(filename))) {
        return loadFromSDL(_textTarget, nrMipMaps);
#endif
    } else {
        LOGE << "Texture::loadFromFile Error: file (" << filename << ") does not exist!!!";
    }

    return 0;
}

GLuint Texture::loadFromMemPtr(void *ptr, size_t size, GLenum textTarget, int nrMipMaps, bool flipH) {
#ifdef ARA_USE_FREEIMAGE
    if (!ptr || !size) {
        return {};
    }
    m_texData.pBitmap = FreeImage::Load(ptr, size);
    return loadFromFib(m_texData.pBitmap, textTarget, nrMipMaps, flipH);
#else
    return 0;
#endif
}

#if defined(ARA_USE_FREEIMAGE) && !defined(__EMSCRIPTEN__)
GLuint Texture::loadFromFib(FIBITMAP *pBitmap, GLenum textTarget, int nrMipMaps, bool flipH) {
    GLboolean generateMips = std::min(nrMipMaps, m_glbase->maxTexMipMapLevels()) > 1;
    uint      width(0), height(0), BPP(0);

    if (flipH) {
        FreeImage_FlipVertical(pBitmap);
    }

#if !defined(ARA_USE_GLES31)
    if (textTarget == GL_TEXTURE_RECTANGLE || textTarget == GL_TEXTURE_CUBE_MAP) generateMips = false;
#else
    if (textTarget == GL_TEXTURE_CUBE_MAP) generateMips = false;
#endif

    m_mipmapLevels = generateMips ? std::min(nrMipMaps, m_glbase->maxTexMipMapLevels()) : 1;

    // bits = FreeImage_GetBits(pBitmap);
    m_texData.bits = reinterpret_cast<GLubyte*>(FreeImage_GetBits(pBitmap));

    width                           = FreeImage_GetWidth(pBitmap);
    height                          = FreeImage_GetHeight(pBitmap);
    BPP                             = FreeImage_GetBPP(pBitmap);

    switch (FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(pBitmap)) {
        case FIC_MINISBLACK:
            m_texData.nrChan         = 1;
            m_texData.format         = GL_RED;
            m_texData.internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F : BPP == 8 ? GL_R8 : 0;
            m_texData.pixelType      = m_texData.internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
            break;
        case FIC_MINISWHITE:
            m_texData.nrChan         = 1;
            m_texData.format         = GL_RED;
            m_texData.internalFormat = BPP == 32 ? GL_R32F : BPP == 16 ? GL_R16F : BPP == 8 ? GL_R8 : 0;
            m_texData.pixelType      = m_texData.internalFormat == GL_R8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
            break;
        case FIC_PALETTE:
            m_texData.nrChan         = 4;
            m_texData.format         = GL_BGR;
            m_texData.internalFormat = GL_RGB8;
            m_texData.pixelType      = GL_UNSIGNED_BYTE;
            break;
        case FIC_RGB:
            m_texData.nrChan         = 3;
            m_texData.format         = GL_BGR;
            m_texData.internalFormat = BPP == 96 ? GL_RGB32F : BPP == 48 ? GL_RGB16F : BPP == 24 ? GL_RGB8 : 0;
            m_texData.pixelType      = m_texData.internalFormat == GL_RGB8 ? GL_UNSIGNED_BYTE : GL_FLOAT;

            // strange effect when exporting tiff from gimp... fi says FIC_RGB,
            // but has 32 bit...
            if (BPP == 32) {
                m_texData.internalFormat = GL_RGBA8;
                m_texData.format         = GL_BGRA;
                m_texData.nrChan         = 4;
                m_texData.pixelType      = GL_UNSIGNED_BYTE;
            }
            break;
        case FIC_RGBALPHA: m_texData.nrChan = 4;
#ifndef ARA_USE_GLES31
            m_texData.format = GL_BGRA;
#else
            m_texData.format = GL_RGBA;
#endif
            m_texData.internalFormat = BPP == 128 ? GL_RGBA32F : BPP == 64 ? GL_RGBA16F : BPP == 32 ? GL_RGBA8 : 0;
            m_texData.pixelType      = m_texData.internalFormat == GL_RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
            break;
        case FIC_CMYK:
            m_texData.nrChan         = 4;
            m_texData.format         = GL_BGRA;
            m_texData.internalFormat = BPP == 128 ? GL_RGBA32F : BPP == 64 ? GL_RGBA16F : BPP == 32 ? GL_RGBA8 : 0;
            m_texData.pixelType      = m_texData.internalFormat == GL_RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT;
            break;
        default: printf("Texture::loadFromFile Error: unknown number of channels\n");
    }

    m_texData.width  = width;
    m_texData.height = height;
    m_texData.tex_t  = static_cast<float>(width) / static_cast<float>(m_texData.width);
    m_texData.tex_u  = static_cast<float>(height) / static_cast<float>(m_texData.height);
    m_texData.target = textTarget;  // assuming 2d pictures
    m_texData.textureID = 0;  // init id
    m_texData.bpp       = BPP;

    // mipmap levels may have to limited in order to be not smaller than 1
    if (generateMips) {
        m_mipmapLevels = std::min(m_mipmapLevels, static_cast<uint>(std::log2(std::max(m_texData.width, m_texData.height))));
    }

    // if the texture is a cube map, cut the input file according to a standard cubemap separation
    if (m_texData.target == GL_TEXTURE_CUBE_MAP) {
        array<FIBITMAP *, 6> faceDataBM{nullptr};

        auto stepX     = m_texData.width / 4;
        auto stepY     = m_texData.height / 3;
        std::array pos {
            ivec4{stepX * 2, stepY, stepX * 3, stepY * 2},  // 0: positive-x
            ivec4{0, stepY, stepX, stepY * 2},              // 1: negative-x
            ivec4{stepX, 0, stepX * 2, stepY},              // 3: negative-y
            ivec4{stepX, stepY * 2, stepX * 2, stepY * 3},  // 2: positive-y
            ivec4{stepX, stepY, stepX * 2, stepY * 2},      // 4: positive-z
            ivec4{stepX * 3, stepY, stepX * 4, stepY * 2}   // 5: negative-z
        };

        for (auto face = 0; face < 6; face++) {
            faceDataBM[face] = FreeImage_Copy(pBitmap, pos[face][0], pos[face][1], pos[face][2], pos[face][3]);
            FreeImage_FlipVertical(faceDataBM[face]);

            if (!faceDataBM[face]) {
                LOGE << "Texture: cube texture separation failed at nr: " << face;
            }

            m_texData.faceData[face] = FreeImage_GetBits(faceDataBM[face]);
        }

        m_texData.width  = m_texData.width / 4;
        m_texData.height = m_texData.height / 3;
    }

    auto texId = procTextureData();

    if (!m_keepBitmap) {
        FreeImage_Unload(pBitmap);
    }

    return texId;
}
#endif

#ifdef __EMSCRIPTEN__
GLuint Texture::loadSDL(GLenum _textTarget, int nrMipMaps) {
    // unsigned char* bits(0);
    GLenum        colorFormat;
    GLenum        inColorFormat;
    GLboolean     generateMips = false;  // maybe makes no sense to turn it off -> slower
    uint          width(0), height(0), BPP(0);
    uint          mimapLevels;
    SDL_Surface **faceDataBM;

    generateMips = false;
    mimapLevels  = 0;

    SDL_Surface *surface;  // Gives us the information to make the texture

    // if ( SDL_Init(SDL_INIT_VIDEO) != 0 ) {
    //    printf("Unable to initialize SDL: %s\n", SDL_GetError());
    // }

    int flags   = IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF;
    int initted = IMG_Init(flags);

    if (!(surface = IMG_Load(filename.c_str())))
        printf("SDL could not load %s: %s\n", filename.c_str(), SDL_GetError());
#ifndef NO_PRELOADED
    int   w, h;
    char *data = emscripten_get_preloaded_image_data(filename.c_str(), &w, &h);
    width      = w;
    height     = h;
#endif
    SDL_PixelFormat *sdlFormat = surface->m_format;
    BPP                        = sdlFormat->BitsPerPixel;
    width                      = surface->w;
    height                     = surface->h;

    // flip the image
    SDL_Surface *flipped = flip_surface(surface, FLIP_VERTICAL);
    m_texData.bits       = static_cast<unsigned char *>(flipped->pixels);

    if (sdlFormat->Amask) {
        colorFormat        = GL_RGB;
        inColorFormat      = GL_BGR;
        m_texData.m_nrChan = 3;
    } else {
        colorFormat        = GL_RGBA;
        inColorFormat      = GL_BGRA;
        m_texData.m_nrChan = 4;
    }

    m_texData.width  = width;
    m_texData.height = height;
    m_texData.tex_t  = width / m_texData.width;
    m_texData.tex_u  = height / m_texData.height;
    m_texData.target = _textTarget;  // asuming 2d pictures
    m_texData.format = BPP == 32 ? GL_RGBA : BPP == 24 ? GL_BGR : BPP == 8 ? GL_R8 : 0;
    m_texData.type   = GL_UNSIGNED_BYTE;
    // die desktop formate wie GL_RGBA8 funktionieren hier nicht...
    m_texData.internalFormat = BPP == 32 ? GL_RGBA : BPP == 24 ? GL_RGB : BPP == 8 ? GL_DEPTH_COMPONENT : 0;
    m_texData.textureID      = 0;  // init id

    // if the texture is a cube map, cut the input file according to a standard
    // cubemap separation
    if (m_texData.target == GL_TEXTURE_CUBE_MAP) {
        int stepX     = m_texData.width / 4;
        int stepY     = m_texData.height / 3;
        int pos[6][4] = {
            {stepX * 2, stepY, stepX * 3, stepY * 2},  // 0: positive-x
            {0, stepY, stepX, stepY * 2},              // 1: negative-x
            {stepX, 0, stepX * 2, stepY},              // 3: negative-y
            {stepX, stepY * 2, stepX * 2, stepY * 3},  // 2: positive-y
            {stepX, stepY, stepX * 2, stepY * 2},      // 4: positive-z
            {stepX * 3, stepY, stepX * 4, stepY * 2}   // 5: negative-z
        };

        m_texData.faceData = new unsigned char *[6];
        faceDataBM         = new SDL_Surface *[6];
        Uint32 rmask, gmask, bmask, amask;

        // SDL interprets each pixel as a 32-bit number, so our masks must
        // depend on the endianness (byte order) of the machine
#if SDL_BYTORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
#else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
#endif
        SDL_Rect dstRect;
        SDL_Rect srcRect;
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = stepX;
        dstRect.h = stepY;

        for (auto face = 0; face < 6; face++) {
            srcRect.x = pos[face][0];
            srcRect.y = pos[face][1];  // SDL takes the upper left corner and
                                       // not the lower left
            srcRect.w = stepX;
            srcRect.h = stepY;

            faceDataBM[face] = SDL_CreateRGBSurface(0, stepX, stepY, BPP, rmask, gmask, bmask, amask);

            SDL_BlitSurface(surface, &srcRect, faceDataBM[face], &dstRect);

            if (faceDataBM[face] == NULL) LOGE << "Texture: cube texture separation failed at nr: " << face;

            SDL_Surface *flipped = flip_surface(faceDataBM[face], FLIP_VERTICAL);
            faceDataBM[face]     = flipped;

            m_texData.faceData[face] = static_cast<unsigned char *>(faceDataBM[face]->pixels);
        }

        m_texData.width  = m_texData.width / 4;
        m_texData.height = m_texData.height / 3;
    }

    GLuint texId = procTextureData();

    SDL_FreeSurface(surface);
    if (m_texData.target == GL_TEXTURE_CUBE_MAP)
        for (auto face = 0; face < 6; face++) SDL_FreeSurface(faceDataBM[face]);

    // IMG_Quit(); // wird ignoriert...
    // SDL_Quit(); // wird auch ignoriert

    return texId;
}
#endif

GLuint Texture::procTextureData() {
    // if this somehow one of these failed (they shouldn't), return failure
    if (m_texData.width == 0 || m_texData.height == 0) {
        LOGE << "Texture Error: could not read image.";
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glActiveTexture(GL_TEXTURE0);

    // generate an OpenGL texture ID for this texture
    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more than one, but for now, just one

    // bind to the new texture ID
    glBindTexture(m_texData.target, m_texData.textureID);

    // Specify the data for the texture
    switch (m_texData.target) {
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
        case GL_TEXTURE_1D:
            glTexStorage1D(m_texData.target, m_mipmapLevels, m_texData.internalFormat, m_texData.width);
            glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m_texData.width, m_texData.format, m_texData.pixelType,
                            m_texData.bits);
            break;
#endif
        case GL_TEXTURE_CUBE_MAP:
            // Now that storage is allocated for the texture object, we can place the texture data into its texel array.
            for (GLuint face = 0; face < 6; face++) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, m_texData.internalFormat, m_texData.width,
                             m_texData.height, 0, m_texData.format, m_texData.pixelType, m_texData.faceData[face]);
            }

#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
            break;
        default:
#ifndef __EMSCRIPTEN__
#ifdef ARA_USE_GLES31
            if (m_texData.format == GL_BGR || m_texData.format == GL_BGRA) {
                GLint const Swizzle[] = {GL_BLUE, GL_GREEN, GL_RED, GL_ALPHA};
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, Swizzle[0]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, Swizzle[1]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, Swizzle[2]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, Swizzle[3]);
            }
#endif
            glTexStorage2D(m_texData.target, m_mipmapLevels, m_texData.internalFormat, m_texData.width, m_texData.height);
            glTexSubImage2D(m_texData.target, 0, 0, 0, m_texData.width, m_texData.height, m_texData.format,
                            m_texData.pixelType, m_texData.bits);
#else
            glTexImage2D(m_texData.target,
                         mimapLevels,  // nr of mipmap levels
                         m_texData.internalFormat, m_texData.width, m_texData.height, 0, m_texData.format,
                         m_texData.type, m_texData.bits);
#endif
            break;
    }

    // mipmaps
    if (m_mipmapLevels > 1) {
        glTexParameterf(m_texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(m_texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(m_texData.target);
    } else {
        // set linear filtering
        glTexParameterf(m_texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(m_texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (
#if !defined(ARA_USE_GLES31)
        m_texData.target == GL_TEXTURE_RECTANGLE ||
#endif
        m_texData.target == GL_TEXTURE_CUBE_MAP) {
        // GL_TEXTURE_RECTANGLE can´t repeat
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    } else {
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

#if !defined(ARA_USE_GLES31)
    glm::vec4 colBlack = glm::vec4(0.f, 0.f, 0.f, 0.f);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &colBlack[0]);
#endif
    glBindTexture(m_texData.target, 0);
    m_texData.bAllocated = true;

    return m_texData.textureID;
}

void Texture::genTexture(GLuint& id) {
    if (id) {
        glDeleteTextures(1, &id);
    }

    glGenTextures(1, &id);  // could be more than one, but for now, just one
    glBindTexture(m_texData.target, id);
}

#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
GLuint Texture::allocate1D(uint w, GLenum internalGlDataType, GLenum extGlDataType, GLenum pixelType) {
    m_texData.width          = w;
    m_texData.tex_t          = static_cast<float>(w) / static_cast<float>(m_texData.width);
    m_texData.target         = GL_TEXTURE_1D;
    m_texData.internalFormat = internalGlDataType;
    m_texData.format         = extGlDataType;
    m_texData.pixelType      = pixelType;

    if (m_texData.textureID) {
        glDeleteTextures(1, (GLuint *)&m_texData.textureID);
    }

    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more than one, but for now, just one
    glBindTexture(m_texData.target, (GLuint)m_texData.textureID);

    // define immutable storage space. best practise since opengl hereby stops
    // tracking certain features use levels = 1
    glTexStorage1D(m_texData.target, 1, m_texData.internalFormat, m_texData.width);

    int nrChans = m_texData.internalFormat == GL_RGBA8  ? 4
                  : m_texData.internalFormat == GL_RGB8 ? 3
                  : m_texData.internalFormat == GL_RG8  ? 2
                  : m_texData.internalFormat == GL_R8   ? 1 : 1;

    vector<float> nullImg(m_texData.width * nrChans);

    // Specify the data for the texture
    glTexSubImage1D(m_texData.target, 0, 0, m_texData.width, m_texData.format, m_texData.pixelType, &nullImg[0]);

    glTexParameterf(m_texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);

    m_texData.width      = w;
    m_texData.bAllocated = true;

    return m_texData.textureID;
}
#endif

GLuint Texture::allocate3D(const uint w, const uint h, const uint d, const GLenum internalGlDataType,
                            const GLenum extGlDataType, const GLenum textTarget, const GLenum pixelType) {
    m_texData.width  = w;
    m_texData.height = h;
    m_texData.depth  = d;
    m_texData.slices = static_cast<GLsizei>(d);

    m_texData.internalFormat = internalGlDataType;
    m_texData.format         = extGlDataType;
    m_texData.target         = textTarget;
    m_texData.pixelType      = pixelType;
    m_texData.mipLevels      = 1;

    vector<uint64_t> nullImg(m_texData.width * m_texData.height * m_texData.depth * 4);

    genTexture(m_texData.textureID);

    // define immutable storage space. best practise since opengl hereby stops
    // tracking certain features
    glTexStorage3D(m_texData.target, m_texData.mipLevels, m_texData.internalFormat, m_texData.width, m_texData.height,
                   m_texData.depth);

    glTexSubImage3D(m_texData.target, 0, 0, 0, 0, m_texData.width, m_texData.height, m_texData.depth, m_texData.format,
                    m_texData.pixelType, static_cast<void *>(&nullImg[0]));

    glTexParameterf(m_texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(m_texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    m_texData.bAllocated = true;
    return m_texData.textureID;
}

GLuint Texture::allocate2D(uint w, uint h, GLenum internalGlDataType, GLenum extGlDataType, GLenum textTarget,
                           GLenum pixelType, uint samples, GLboolean fixedsamplelocations) {
    m_texData.width  = w;
    m_texData.height = h;

    m_texData.tex_t                = static_cast<float>(m_texData.width) / static_cast<float>(m_texData.width);
    m_texData.tex_u                = static_cast<float>(m_texData.height) / static_cast<float>(m_texData.height);
    m_texData.target               = textTarget;
    m_texData.internalFormat       = internalGlDataType;
    m_texData.format               = extGlDataType;
    m_texData.pixelType            = pixelType;
    m_texData.samples              = samples;
    m_texData.fixedsamplelocations = fixedsamplelocations;

    if (m_texData.textureID) {
        glDeleteTextures(1, (GLuint *)&m_texData.textureID);
    }

    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more than one, but for now, just one
    glBindTexture(m_texData.target, (GLuint)m_texData.textureID);

#ifndef __EMSCRIPTEN__
    if (m_texData.target != GL_TEXTURE_2D_MULTISAMPLE)
        glTexStorage2D(m_texData.target, 1, m_texData.internalFormat, m_texData.width, m_texData.height);
    else
        glTexStorage2DMultisample(m_texData.target, m_texData.samples, m_texData.internalFormat, m_texData.width,
                                  m_texData.height, GL_FALSE);
#endif
    vector<float> nullImg(m_texData.width * m_texData.height * 4);

    // Specify the data for the texture
    switch (m_texData.target) {
#ifndef __EMSCRIPTEN__
#ifndef ARA_USE_GLES31
        case GL_TEXTURE_1D:
            glTexSubImage1D(m_texData.target,  // target
                            0,                 // mipmap level
                            0,                 // x and y offset
                            m_texData.width,   // width and height
                            m_texData.format, m_texData.pixelType, &nullImg[0]);
            break;
#endif
        case GL_TEXTURE_2D:
            glTexSubImage2D(m_texData.target,  // target
                            0,                 // mipmap level
                            0, 0,              // x and y offset
                            m_texData.width,   // width and height
                            m_texData.height, m_texData.format, m_texData.pixelType, &nullImg[0]);
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
            /*	glTexImage2DMultisample(
                    m_texData.target,
                    m_texData.samples,
                    m_texData.internalFormat,
                    m_texData.width,
                    m_texData.height,
                    m_texData.fixedsamplelocations);*/
            break;
#ifndef ARA_USE_GLES31
        case GL_TEXTURE_RECTANGLE:
            glTexSubImage2D(m_texData.target,  // target
                            0,                 // mipmap level
                            0, 0,              // x and y offset
                            m_texData.width,   // width and height
                            m_texData.height, m_texData.format, m_texData.pixelType, &nullImg[0]);
            break;
#endif
#else
        case GL_TEXTURE_2D:
            glTexImage2D(m_texData.target,          // target
                         0,                         // mipmap level
                         m_texData.internalFormat,  // x and y offset
                         m_texData.width,           // width and height
                         m_texData.height, 0, m_texData.format, m_texData.pixelType, &nullImg[0]);
            break;
        case GL_TEXTURE_RECTANGLE:
            glTexImage2D(m_texData.target,          // target
                         0,                         // mipmap level
                         m_texData.internalFormat,  // x and y offset
                         m_texData.width,           // width and height
                         m_texData.height, 0, m_texData.format, m_texData.pixelType, &nullImg[0]);
            break;
#endif
        default: break;
    }

    if (m_texData.target != GL_TEXTURE_2D_MULTISAMPLE) {
        glTexParameterf(m_texData.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(m_texData.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#ifndef ARA_USE_GLES31
    glm::vec4 colBlack = glm::vec4(0.f, 0.f, 0.f, 0.f);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &colBlack[0]);
#endif

    m_texData.width      = w;
    m_texData.height     = h;
    m_texData.bAllocated = true;

    return m_texData.textureID;
}

GLuint Texture::gen(GLenum target) {
    m_texData.target = target;

    if (m_texData.textureID) {
        glDeleteTextures(1, &m_texData.textureID);
    }

    glGenTextures(1, &m_texData.textureID);  // could be more than one, but for now, just one
    glBindTexture(m_texData.target, m_texData.textureID);

    return (GLuint)m_texData.textureID;
}

void Texture::upload(const void *dataPtr) const {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    // bind to the new texture ID
    glBindTexture(m_texData.target, m_texData.textureID);

    // Specify the data for the texture
    switch (m_texData.target) {
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
        case GL_TEXTURE_1D:
            glTexSubImage1D(GL_TEXTURE_1D,    // target
                            0,                // mipmap level
                            0,                // xoffset
                            m_texData.width,  // width
                            m_texData.format, m_texData.pixelType, dataPtr);
            break;
#endif
        default:
#ifndef __EMSCRIPTEN__
            glTexSubImage2D(m_texData.target,  // target
                            0,                 // mipmap level
                            0, 0,              // x and y offset
                            m_texData.width,   // width and height
                            m_texData.height, m_texData.format, m_texData.pixelType, dataPtr);
#else
            glTexImage2D(m_texData.target,
                         mimapLevels,  // nr of mipmap levels
                         m_texData.internalFormat, m_texData.width, m_texData.height, 0, m_texData.format,
                         m_texData.type, m_texData.bits);
#endif
            break;
    }

    glBindTexture(m_texData.target, 0);
}

void Texture::upload(const void *dataPtr, int width, int height, int depth, int xOffs, int yOffs, int zOffs, GLenum uplFormat, GLenum uplPixType) const {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    // bind to the new texture ID
    glBindTexture(m_texData.target, m_texData.textureID);

    // Specify the data for the texture
    switch (m_texData.target) {
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
        case GL_TEXTURE_1D:
            glTexSubImage1D(GL_TEXTURE_1D,  // target
                            0,              // mipmap level
                            xOffs,          // xoffset
                            width,          // width
                            uplFormat ? uplFormat : m_texData.format, uplPixType ? uplPixType : m_texData.pixelType,
                            dataPtr);
            break;
        case GL_TEXTURE_3D:
            glTexSubImage3D(GL_TEXTURE_3D,  // target
                            0,              // mipmap level
                            xOffs, yOffs, zOffs, width, height, depth, uplFormat ? uplFormat : m_texData.format,
                            uplPixType ? uplPixType : m_texData.pixelType, dataPtr);
            break;
#endif
        default:
#ifndef __EMSCRIPTEN__
            glTexSubImage2D(m_texData.target,  // target
                            0,                 // mipmap level
                            xOffs, yOffs,      // x and y offset
                            width,             // width and height
                            height, uplFormat ? uplFormat : m_texData.format,
                            uplPixType ? uplPixType : m_texData.pixelType, dataPtr);
#else
            glTexImage2D(m_texData.target,
                         mimapLevels,  // nr of mipmap levels
                         m_texData.internalFormat, m_texData.width, m_texData.height, 0, m_texData.format,
                         m_texData.type, m_texData.bits);
#endif
            break;
    }

    glBindTexture(m_texData.target, 0);
}

void Texture::setSamplerFiltering(int a_tfMagnification, int a_tfMinification) {
    // Set magnification filter
    if (a_tfMagnification == TEXTURE_FILTER_MAG_NEAREST)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    else if (a_tfMagnification == TEXTURE_FILTER_MAG_BILINEAR)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set minification filter
    if (a_tfMinification == TEXTURE_FILTER_MIN_NEAREST)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    else if (a_tfMinification == TEXTURE_FILTER_MIN_BILINEAR)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    else if (a_tfMinification == TEXTURE_FILTER_MIN_NEAREST_MIPMAP)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    else if (a_tfMinification == TEXTURE_FILTER_MIN_BILINEAR_MIPMAP)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    else if (a_tfMinification == TEXTURE_FILTER_MIN_TRILINEAR)
        glSamplerParameteri(m_samplerID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    m_tfMinification  = a_tfMinification;
    m_tfMagnification = a_tfMagnification;
}

void Texture::setFiltering(GLenum magFilter, GLenum minFilter) const {
    if (m_texData.target != GL_TEXTURE_2D_MULTISAMPLE) {
        GLint val;
        switch (m_texData.target) {
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
            case GL_TEXTURE_1D: glGetIntegerv(GL_TEXTURE_BINDING_1D, &val); break;
            case GL_TEXTURE_1D_ARRAY: glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY, &val); break;
            case GL_TEXTURE_3D: glGetIntegerv(GL_TEXTURE_BINDING_3D, &val); break;
#endif
            case GL_TEXTURE_2D: glGetIntegerv(GL_TEXTURE_BINDING_2D, &val); break;
            case GL_TEXTURE_2D_ARRAY: glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &val); break;
            default: break;
        }
        glBindTexture(m_texData.target, (GLuint)m_texData.textureID);
        glTexParameteri(m_texData.target, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(m_texData.target, GL_TEXTURE_MIN_FILTER, minFilter);
        glBindTexture(m_texData.target, val);
    }
}

void Texture::setWraping(GLfloat wrap) const {
    // Set magnification filter
    glBindTexture(m_texData.target, m_texData.textureID);

    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, wrap);
    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, wrap);
}

void Texture::bind() const {
    glBindTexture(m_texData.target, m_texData.textureID);
}

void Texture::bind(GLuint texUnit) const {
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(m_texData.target, m_texData.textureID);
}

void Texture::bind(GLuint su, GLuint si, GLuint tu) {
    m_samplerUnit = static_cast<GLint>(su);
    glActiveTexture(GL_TEXTURE0 + tu);
    glBindTexture(m_texData.target, m_texData.textureID);
    glBindSampler(m_samplerUnit, si);
}

void Texture::unbind() const {
    glBindTexture(m_texData.target, 0);
}

void Texture::releaseTexture() {
    glDeleteTextures(1, &m_texData.textureID);
    m_texData.textureID = 0;
#ifdef ARA_USE_FREEIMAGE
    if (m_keepBitmap) {
        FreeImage_Unload(m_texData.pBitmap);
    }
#endif
}

void Texture::generateSampler() {
    glGenSamplers(1, &m_samplerID);
    glBindSampler(m_samplerUnit, m_samplerID);
}

void Texture::getGlFormatAndType(GLenum glInternalFormat, GLenum &glFormat, GLenum &type) {
    switch (glInternalFormat) {
        case GL_R8:
            glFormat = GL_RED;
            type     = GL_UNSIGNED_BYTE;
            break;
        case GL_RGBA:
#ifndef TARGET_OPENGLES
        case GL_RGBA8:
#endif
            glFormat = GL_RGBA;
            type     = GL_UNSIGNED_BYTE;
            break;
        case GL_RGB:
#ifndef TARGET_OPENGLES
        case GL_RGB8:
#endif
            glFormat = GL_RGB;
            type     = GL_UNSIGNED_BYTE;
            break;
            //            case GL_LUMINANCE:
            // #ifndef TARGET_OPENGLES
            //            case GL_LUMINANCE8:
            // #endif
            //                m_inpPixFmt = GL_LUMINANCE;
            //                type = GL_UNSIGNED_BYTE;
            //                break;

#ifndef ARA_USE_GLES31
            // 16-bit unsigned short formats
        case GL_RGBA16:
            glFormat = GL_RGBA;
            type     = GL_UNSIGNED_SHORT;
            break;
        case GL_RGB16:
            glFormat = GL_RGB;
            type     = GL_UNSIGNED_SHORT;
            break;
            //            case GL_LUMINANCE16:
            //                m_inpPixFmt = GL_LUMINANCE;
            //                type = GL_UNSIGNED_SHORT;
            //                break;

            // 32-bit float formats
        case GL_RGBA32F:
            glFormat = GL_RGBA;
            type     = GL_FLOAT;
            break;
        case GL_RGB32F:
            glFormat = GL_RGB;
            type     = GL_FLOAT;
            break;
        case GL_RG32F:
            glFormat = GL_RG;
            type     = GL_FLOAT;
            break;
        case GL_R32F:
            glFormat = GL_RED;
            type     = GL_FLOAT;
            break;
            //            case GL_LUMINANCE32F_ARB:
            //                m_inpPixFmt = GL_LUMINANCE;
            //                type = GL_FLOAT;
            //                break;

            // 16-bit float formats
        case GL_RGBA16F:
            glFormat = GL_RGBA;
            type     = GL_FLOAT;
            break;
        case GL_RGB16F:
            glFormat = GL_RGB;
            type     = GL_FLOAT;
            break;
        case GL_RG16F:
            glFormat = GL_RG;
            type     = GL_FLOAT;
            break;
        case GL_R16F:
            glFormat = GL_RED;
            type     = GL_FLOAT;
            break;
//            case GL_LUMINANCE16F_ARB:
//                m_inpPixFmt = GL_LUMINANCE;
//                type = GL_FLOAT;
//                break;
#endif

            // used by prepareBitmapTexture(), not supported by ofPixels
            //            case GL_LUMINANCE_ALPHA:
            // #ifndef TARGET_OPENGLES
            //            case GL_LUMINANCE8_ALPHA8:
            // #endif
            //                m_inpPixFmt = GL_LUMINANCE_ALPHA;
            //                type = GL_UNSIGNED_BYTE;
            //                break;

        default:
            glFormat = glInternalFormat;
            type     = GL_UNSIGNED_BYTE;
            break;
    }
}

vec2 Texture::getCoordFromPercent(float xPct, float yPct) const {
    vec2 temp{};

    if (!m_texData.bAllocated) {
        return temp;
    }

#ifndef ARA_USE_GLES31
    if (m_texData.target == GL_TEXTURE_RECTANGLE) {
        temp.x = xPct * m_texData.width;
        temp.y = yPct * m_texData.height;
    } else {
#endif
        xPct *= m_texData.tex_t;
        yPct *= m_texData.tex_u;
        temp.x = xPct;
        temp.y = yPct;
#ifndef ARA_USE_GLES31
    }
#endif

        return temp;
}

#ifdef __EMSCRIPTEN__
Uint32 Texture::get_pixel32(SDL_Surface *surface, int x, int y) {
    // Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;

    // Get the requested pixel
    return pixels[(y * surface->w) + x];
}

void Texture::put_pixel32(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    // Convert the pixels to 32 bit
    Uint32 *pixels = (Uint32 *)surface->pixels;

    // Set the pixel
    pixels[(y * surface->w) + x] = pixel;
}

SDL_Surface *Texture::flip_surface(SDL_Surface *surface, int flags) {
    // Pointer to the soon to be flipped surface
    SDL_Surface *flipped = NULL;

    // If the image is color keyed
    if (surface->flags & SDL_SRCCOLORKEY) {
        flipped = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel,
                                       surface->format->Rmask, surface->format->Gmask, surface->format->Bmask, 0);
    }  // Otherwise
    else {
        flipped = SDL_CreateRGBSurface(SDL_SWSURFACE, surface->w, surface->h, surface->format->BitsPerPixel,
                                       surface->format->Rmask, surface->format->Gmask, surface->format->Bmask,
                                       surface->format->Amask);
    }

    // If the surface must be locked
    if (SDL_MUSTLOCK(surface)) {
        // Lock the surface
        SDL_LockSurface(surface);
    }

    // Go through columns
    for (int x = 0, rx = flipped->w - 1; x < flipped->w; x++, rx--) {
        // Go through rows
        for (int y = 0, ry = flipped->h - 1; y < flipped->h; y++, ry--) {
            // Get pixel
            Uint32 pixel = get_pixel32(surface, x, y);
            // Copy pixel
            if ((flags & FLIP_VERTICAL) && (flags & FLIP_HORIZONTAL)) {
                put_pixel32(flipped, rx, ry, pixel);
            } else if (flags & FLIP_HORIZONTAL) {
                put_pixel32(flipped, rx, y, pixel);
            } else if (flags & FLIP_VERTICAL) {
                put_pixel32(flipped, x, ry, pixel);
            }
        }
    }

    // Unlock surface
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    // Copy color key
    if (surface->flags & SDL_SRCCOLORKEY) {
        SDL_SetColorKey(flipped, SDL_RLEACCEL | SDL_SRCCOLORKEY, surface->format->Amask);
    }

    // Return flipped surface
    return flipped;
}
#endif

#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
void Texture::saveTexToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, GLenum internalFormat,
                              GLint texNr) {
    GLenum    format;
    GLenum    type;
    FIBITMAP *bitmap = nullptr;

    glBindTexture(GL_TEXTURE_2D, texNr);
    getGlFormatAndType(internalFormat, format, type);

    switch (type) {
        case GL_UNSIGNED_SHORT: {
            if (format == GL_RED) {
                bitmap = FreeImage_AllocateT(FIT_UINT16, w, h); break;
            } else {
                LOGE << "Texture::saveTexToFile2D Error: unknown format";
                break;
            }

            if (bitmap) {
                auto bits = (GLubyte *)FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
                glesGetTexImage(texNr, GL_TEXTURE_2D, format, GL_UNSIGNED_SHORT, w, h, bits);
#else
                glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
#endif
            } else {
                printf(
                    "Texture::saveTexToFile2D Error: could not allocate bitmap "
                    "\n");
            }
            break;
        }
        case GL_UNSIGNED_BYTE: {
            int nrChan = 3;

            switch (format) {
                case GL_RED: nrChan = 1; break;
                case GL_RG: nrChan = 2; break;
                case GL_RGB: nrChan = 3; break;
                case GL_RGBA: nrChan = 4; break;
                default: printf("Texture::saveTexToFile2D Error: unknown format \n"); break;
            }

            bitmap = FreeImage_Allocate(w, h, nrChan * 8);
            if (bitmap) {
                BYTE *bits = (BYTE *)FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
                glesGetTexImage(texNr, GL_TEXTURE_2D, format, GL_UNSIGNED_BYTE, w, h, bits);
#else
                glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
#endif
            } else {
                printf(
                    "Texture::saveTexToFile2D Error: could not allocate bitmap "
                    "\n");
            }
            break;
        }
        case GL_FLOAT: {
            switch (internalFormat) {
                case GL_R32F: bitmap = FreeImage_AllocateT(FIT_FLOAT, w, h); break;
                case GL_RGB16F: bitmap = FreeImage_AllocateT(FIT_RGB16, w, h); break;
                case GL_RGBA16F: bitmap = FreeImage_AllocateT(FIT_RGBA16, w, h); break;
                case GL_RGB32F: bitmap = FreeImage_AllocateT(FIT_RGBF, w, h); break;
                case GL_RGBA32F: bitmap = FreeImage_AllocateT(FIT_RGBAF, w, h); break;
                default: LOGE << "Texture::saveTexToFile2D Error: unknown format"; break;
            }

            if (bitmap) {
                BYTE *bits = (BYTE *)FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
                glesGetTexImage(texNr, GL_TEXTURE_2D, format, GL_FLOAT, w, h, bits);
#else
                glGetTexImage(GL_TEXTURE_2D, 0, format, type, bits);
#endif
            } else {
                LOGE << "Texture::saveTexToFile2D Error: could not allocate "
                        "bitmap";
            }
            break;
        }
        default: LOGE << "Texture::saveTexToFile2D Error: Unknown pixel format"; break;
    }

    if (!FreeImage_Save(filetype, bitmap, filename)) {
        LOGE << "Texture::saveTexToFile2D Error: FreeImage_Save failed";
    } else {
        FreeImage_Unload(bitmap);
    }
}

void Texture::saveBufToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan, uint8_t *buf, bool vFlip) {
    FreeImage_SetOutputMessage(Texture::FreeImageErrorHandler);

    // Determine image color type and pitch (row size in bytes)
    int bpp = nrChan * 8; // bits per pixel
    int pitch = w * nrChan; // bytes per row

    // FreeImage expects BGR(A) format, not RGB(A), so we might need to swap channels manually if necessary
    auto bitmap = FreeImage_ConvertFromRawBits(
            buf,  // FreeImage uses BYTE*, which is uint8_t*
            w,
            h,
            pitch,
            bpp,
            FI_RGBA_RED_MASK,   // These masks are correct for RGBA/BGRA formats
            FI_RGBA_GREEN_MASK,
            FI_RGBA_BLUE_MASK,
            true                // top-down row order (true if buffer has top-down order)
    );

    if (vFlip) {
        FreeImage_FlipVertical(bitmap);
    }

    if (!bitmap) {
        return;
    }

    if (filetype == FIF_JPEG && nrChan == 4) {
        auto bgrImage = FreeImage_ConvertTo24Bits(bitmap);
        if (!bgrImage) {
            return;
        }
        FreeImage_Unload(bitmap);
        bitmap = bgrImage;
    }

    FreeImage_Save(filetype, bitmap, filename);
    FreeImage_Unload(bitmap);
}

#endif

#ifdef ARA_USE_FREEIMAGE
void Texture::FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char* message) {
    // Handle the error message here (e.g., print to console, log to file, display to user)
    LOGE << "*** FreeImage Error ***";
    if (fif != FIF_UNKNOWN) {
        LOGE << "Format: " << FreeImage_GetFormatFromFIF(fif);
    }
    LOGE <<  "Message:" << message;
}

void Texture::saveFrontBuffer(const std::string &filename, FREE_IMAGE_FORMAT fif, uint32_t w, uint32_t h, uint32_t nrChan) {
    std::vector<uint8_t> bitmap(w * h * nrChan);
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, bitmap.data());  // synchronous, blocking command, no swap() needed
    FreeImage::Save(filename, fif, w, h, nrChan, bitmap.data());
}
#endif

Texture::~Texture() {
    glDeleteTextures(1, &m_texData.textureID);
#ifdef ARA_USE_FREEIMAGE
    if (m_keepBitmap) {
        FreeImage_Unload(m_texData.pBitmap);
    }

// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
    FreeImage_DeInitialise();
#endif
#endif
}

}  // namespace ara
