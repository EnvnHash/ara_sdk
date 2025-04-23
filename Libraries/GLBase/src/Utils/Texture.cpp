//
// Generic Texture Manager
// For use with OpenGL 3.2+ and the FreeImage library
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

namespace ara {

Texture::Texture() {
#if (defined(FREEIMAGE_LIB) || defined(__ANDROID__)) && defined(ARA_USE_FREEIMAGE)
    FreeImage_Initialise();
#endif
}

Texture::Texture(GLBase *glbase) : m_glbase(glbase) {
    // glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    // call this ONLY when linking with FreeImage as a static library
#if (defined(FREEIMAGE_LIB) || defined(__ANDROID__)) && defined(ARA_USE_FREEIMAGE)
    FreeImage_Initialise();
#endif
}

#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
FIBITMAP *Texture::ImageLoader(const char *path, int flag) {
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    fif = FreeImage_GetFileType(path, flag);

    if (fif == FIF_UNKNOWN) {
        // no signature ?
        // try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilename(path);
    }
    // check that the plugin has reading capabilities ...
    if ((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        // ok, let's load the file
        return FreeImage_Load(fif, path, flag);
    } else {
        LOGE << "Texture::Error unknown format";
        return nullptr;
    }
}
#endif

GLuint Texture::loadTextureRect(std::string filename, bool flipH) {
    m_filename = std::move(filename);
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
            FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(reinterpret_cast<FIMEMORY *>(vp.data()), static_cast<int>(vp.size()));
            if (fif == FIF_UNKNOWN) {
                LOGE << "Unknown image file format";
            } else {
                FIMEMORY* mem = FreeImage_OpenMemory(vp.data(), static_cast<DWORD>(vp.size()));
                if (mem == nullptr) {
                    LOGE << "Failed to open image memory";
                } else {
                    FIBITMAP* pBitmap = FreeImage_LoadFromMemory(fif, mem, 0);
                    if (pBitmap == nullptr) {
                        LOGE << "Failed to load image from memory";
                        FreeImage_CloseMemory(mem);
                    } else {
                        sz[0] = static_cast<float>(FreeImage_GetWidth(pBitmap));
                        sz[1] = static_cast<float>(FreeImage_GetHeight(pBitmap));
                        FreeImage_Unload(pBitmap);
                        FreeImage_CloseMemory(mem);
                    }
                }
            }
        }
#elif !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    if (fs::exists(fs::path(filename))) {
        m_filename        = filename;
        FIBITMAP *pBitmap = ImageLoader(filename.c_str(), 0);
        sz[0] = static_cast<float>(FreeImage_GetWidth(pBitmap));
        sz[1] = static_cast<float>(FreeImage_GetHeight(pBitmap));
        FreeImage_Unload(pBitmap);
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

GLuint Texture::loadFromFile(const std::string &filename, GLenum _textTarget, int nrMipMaps, bool flipH) {
/*#ifdef ARA_USE_CMRC
    auto fs = cmrc::ara::get_filesystem();

    if (fs.exists(filename)) {
        auto file = fs.open(filename);
        if (!file.size()) {
            return 0;
        }

        LOG << "Texture::loadFromFile " << filename;

        size_t size = file.size();
        if (size > 0) {
            vector<uint8_t> vp(size);
            std::copy(file.begin(), file.end(), vp.begin());
            return loadFromMemPtr(vp.data(), size, _textTarget, nrMipMaps, flipH);
        }
*/
#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
    if (std::filesystem::exists(std::filesystem::path(filename))) {
        m_filename        = filename;
        LOG << "Texture::loadFromFile " << filename;
        FIBITMAP *pBitmap = ImageLoader(filename.c_str(), 0);
        return loadFromFib(pBitmap, _textTarget, nrMipMaps, flipH);
#else
    if (fs::exists(fs::path(filename))) {
        return loadFromSDL(_textTarget, nrMipMaps);
#endif
    } else {
        LOGE << "Texture::loadFromFile Error: file (" << filename << ") does not exist!!!";
    }

    return 0;
}

GLuint Texture::loadFromMemPtr(void *ptr, size_t size, GLenum _textTarget, int nrMipMaps, bool flipH) {
#ifdef ARA_USE_FREEIMAGE
    if (!ptr || !size) {
        return 0;
    }
    FreeImg_MemHandler mh(ptr, size);
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);

    if ((mh.memPos < mh.memSize) &&
        ((m_texData.pBitmap = FreeImage_LoadFromHandle(fif, mh.io(), (fi_handle)&mh, 0)) == nullptr)) {
        LOGE << "Texture::loadFromMemPtr failed";
        return 0;
    }

    return loadFromFib(m_texData.pBitmap, _textTarget, nrMipMaps, flipH);
#else
    return 0;
#endif
}

#if defined(ARA_USE_FREEIMAGE) && !defined(__EMSCRIPTEN__)
GLuint Texture::loadFromFib(FIBITMAP *pBitmap, GLenum _textTarget, int nrMipMaps, bool flipH) {
    GLboolean generateMips = std::min(nrMipMaps, m_glbase->maxTexMipMapLevels()) > 1;
    uint      width(0), height(0), BPP(0);

    if (flipH) {
        FreeImage_FlipVertical(pBitmap);
    }

#if !defined(ARA_USE_GLES31)
    if (_textTarget == GL_TEXTURE_RECTANGLE || _textTarget == GL_TEXTURE_CUBE_MAP) generateMips = false;
#else
    if (_textTarget == GL_TEXTURE_CUBE_MAP) generateMips = false;
#endif

    m_mipmapLevels = generateMips ? std::min(nrMipMaps, m_glbase->maxTexMipMapLevels()) : 1;

    // bits = FreeImage_GetBits(pBitmap);
    m_texData.bits = (GLubyte *)FreeImage_GetBits(pBitmap);

    width                           = FreeImage_GetWidth(pBitmap);
    height                          = FreeImage_GetHeight(pBitmap);
    BPP                             = FreeImage_GetBPP(pBitmap);
    FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(pBitmap);

    switch (colorType) {
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
            m_texData.nrChan         = 3;
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
    m_texData.target = _textTarget;  // assuming 2d pictures
    m_texData.textureID = 0;  // init id
    m_texData.bpp       = BPP;

    // mipmap levels may have to limited in order to be not smaller than 1
    if (generateMips) {
        m_mipmapLevels = std::min(m_mipmapLevels, (uint)std::log2(std::max(m_texData.width, m_texData.height)));
    }

    // if the texture is a cube map, cut the input file according to a standard cubemap separation
    if (m_texData.target == GL_TEXTURE_CUBE_MAP) {
        array<FIBITMAP *, 6> faceDataBM{nullptr};

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

        for (auto face = 0; face < 6; face++) {
            faceDataBM[face] = FreeImage_Copy(pBitmap, pos[face][0], pos[face][1], pos[face][2], pos[face][3]);
            FreeImage_FlipVertical(faceDataBM[face]);

            if (!faceDataBM[face]) LOGE << "Texture: cube texture separation failed at nr: " << face;

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

#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
GLuint Texture::allocate1D(uint w, GLenum internalGlDataType, GLenum extGlDataType, GLenum pixelType) {
    // m_texData.width = std::min<uint>(w, (uint)maxTexSize);
    m_texData.width          = w;
    m_texData.tex_t          = (float)w / (float)m_texData.width;
    m_texData.target         = GL_TEXTURE_1D;
    m_texData.internalFormat = internalGlDataType;
    m_texData.format         = extGlDataType;
    m_texData.pixelType      = pixelType;

    if (m_texData.textureID) {
        glDeleteTextures(1, (GLuint *)&m_texData.textureID);
    }

    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more then one, but for now, just one
    glBindTexture(m_texData.target, (GLuint)m_texData.textureID);

    // define immutable storage space. best practise since opengl hereby stops
    // tracking certain features use levels = 1
    glTexStorage1D(m_texData.target, 1, m_texData.internalFormat, m_texData.width);

    int nrChans = m_texData.internalFormat == GL_RGBA8  ? 4
                  : m_texData.internalFormat == GL_RGB8 ? 3
                  : m_texData.internalFormat == GL_RG8  ? 2
                  : m_texData.internalFormat == GL_R8   ? 1
                                                        : 1;

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

GLuint Texture::allocate3D(uint w, uint h, uint d, GLenum internalGlDataType, GLenum extGlDataType, GLenum textTarget,
                           GLenum pixelType) {
    m_texData.width  = w;
    m_texData.height = h;
    m_texData.depth  = d;
    m_texData.slices = d;

    m_texData.internalFormat = internalGlDataType;
    m_texData.format         = extGlDataType;
    m_texData.target         = textTarget;
    m_texData.pixelType      = pixelType;
    m_texData.mipLevels      = 1;

    vector<uint64_t> nullImg(m_texData.width * m_texData.height * m_texData.depth *
                             4);  // make an array with initial zero data, ...don't care about the
                                  // number of channel, just make it big enough

    if (m_texData.textureID) {
        glDeleteTextures(1, (GLuint *)&m_texData.textureID);
    }

    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more then one, but for now, just one
    glBindTexture(m_texData.target, (GLuint)m_texData.textureID);

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

    m_texData.tex_t                = (float)m_texData.width / (float)m_texData.width;
    m_texData.tex_u                = (float)m_texData.height / (float)m_texData.height;
    m_texData.target               = textTarget;
    m_texData.internalFormat       = internalGlDataType;
    m_texData.format               = extGlDataType;
    m_texData.pixelType            = pixelType;
    m_texData.samples              = samples;
    m_texData.fixedsamplelocations = fixedsamplelocations;

    if (m_texData.textureID) {
        glDeleteTextures(1, (GLuint *)&m_texData.textureID);
    }

    glGenTextures(1, (GLuint *)&m_texData.textureID);  // could be more then one, but for now, just one
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

    glGenTextures(1,
                  &m_texData.textureID);  // could be more then one, but for now, just one
    glBindTexture(m_texData.target, m_texData.textureID);

    return (GLuint)m_texData.textureID;
}

void Texture::upload(void *dataPtr) {
    int mimapLevels = 1;

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

void Texture::upload(void *dataPtr, int width, int height, int depth, int xOffs, int yOffs, int zOffs, GLenum uplFormat, GLenum uplPixType) {
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

void Texture::setWraping(GLenum _wrap) const {
    // Set magnification filter
    glBindTexture(m_texData.target, m_texData.textureID);

    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_S, (float)_wrap);
    glTexParameterf(m_texData.target, GL_TEXTURE_WRAP_T, (float)_wrap);
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

GLfloat *Texture::getCoordFromPercent(float xPct, float yPct) {
    auto *temp = new GLfloat[2];

    if (!m_texData.bAllocated) return temp;

#ifndef ARA_USE_GLES31
    if (m_texData.target == GL_TEXTURE_RECTANGLE) {
        temp[0] = xPct * m_texData.width;
        temp[1] = yPct * m_texData.height;
    } else {
#endif
        xPct *= m_texData.tex_t;
        yPct *= m_texData.tex_u;
        temp[0] = xPct;
        temp[1] = yPct;
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
void Texture::saveTexToFile2D(const char *_filename, FREE_IMAGE_FORMAT _filetype, int w, int h, GLenum _internalFormat,
                              GLint _texNr) {
    GLenum    format;
    GLenum    type;
    FIBITMAP *bitmap = nullptr;

    glBindTexture(GL_TEXTURE_2D, _texNr);
    getGlFormatAndType(_internalFormat, format, type);

    switch (type) {
        case GL_UNSIGNED_SHORT: {
            switch (format) {
                case GL_RED: bitmap = FreeImage_AllocateT(FIT_UINT16, w, h); break;
                default: printf("Texture::saveTexToFile2D Error: unknown format \n"); break;
            }

            if (bitmap) {
                GLubyte *bits = (GLubyte *)FreeImage_GetBits(bitmap);
#ifdef ARA_USE_GLES31
                glesGetTexImage(_texNr, GL_TEXTURE_2D, format, GL_UNSIGNED_SHORT, w, h, bits);
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
                glesGetTexImage(_texNr, GL_TEXTURE_2D, format, GL_UNSIGNED_BYTE, w, h, bits);
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
            switch (_internalFormat) {
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
                glesGetTexImage(_texNr, GL_TEXTURE_2D, format, GL_FLOAT, w, h, bits);
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

    if (!FreeImage_Save(_filetype, bitmap, _filename)) {
        LOGE << "Texture::saveTexToFile2D Error: FreeImage_Save failed";
    } else {
        FreeImage_Unload(bitmap);
    }
}
#endif

#if !defined(__EMSCRIPTEN__) && defined(ARA_USE_FREEIMAGE)
void Texture::saveBufToFile2D(const char *filename, FREE_IMAGE_FORMAT filetype, int w, int h, int nrChan,
                              uint8_t *buf) {
    if (m_saveBufCont && (FreeImage_GetWidth(m_saveBufCont) != w || FreeImage_GetHeight(m_saveBufCont) != h)) {
        FreeImage_Unload(m_saveBufCont);
        m_saveBufCont = nullptr;
    }

    if (!m_saveBufCont) m_saveBufCont = FreeImage_Allocate(w, h, nrChan * 8);

    if (m_saveBufCont) {
        std::copy_n(buf, (w * h * nrChan), FreeImage_GetBits(m_saveBufCont));
        if (!FreeImage_Save(filetype, m_saveBufCont, filename)) {
            LOGE << "Texture::saveTexToFile2D Error: FreeImage_Save failed";
        }
    }
}
#endif

#ifdef ARA_USE_FREEIMAGE
void Texture::saveFrontBuffer(const std::string &filename, int w, int h, int nrChan) {
    FIBITMAP         *bitmap   = FreeImage_Allocate(w, h, nrChan * 8);
    FREE_IMAGE_FORMAT filetype = FIF_PNG;

    if (bitmap) {
        auto bits = FreeImage_GetBits(bitmap);
        glReadBuffer(GL_FRONT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, bits);  // synchronous, blocking command, no swap() needed
    }

    if (!FreeImage_Save(filetype, bitmap, filename.c_str())) {
        LOGE << "Texture::saveTexToFile2D Error: FreeImage_Save failed";
    } else {
        FreeImage_Unload(bitmap);
    }
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
