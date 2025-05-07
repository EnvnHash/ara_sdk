/**
 * \brief FBO, C++ Framebuffer Object Wrapper (supporting all formats)
 *
 */

#pragma once

#include "GlbCommon/GlbCommon.h"
#include <GeoPrimitives/Quad.h>

struct FIBITMAP;

namespace ara {

class GLBase;
class Shaders;
class Texture;
class ShaderCollector;
class Quad;

class FBO {
public:
    FBO() = default;
    FBO(const FboInitParams&);
    virtual ~FBO();

    void remove();
    void fromShared(FBO *sharedFbo);
    void fromSharedExtractLayer(FBO *sharedFbo, int layer);
    void fromSharedSelectAttachment(FBO *sharedFbo, int attNr);
    void initFromShared(FBO *sharedFbo);
    void fromTexMan(const Texture *texMan);
    void init();
    void allocColorTexture();
    void allocDepthTexture();
    void attachTextures(bool doCheckFbo) const;
    void bind(bool saveStates = true);
    void unbind(bool doRestoreStates = true);
    void blit(uint scrWidth, uint scrHeight, GLenum interp = GL_NEAREST) const;
    void blitTo(const FBO *dst) const;
    void resize(float width, float height, float depth = 0, bool checkStates = true);
    void resize(uint width, uint height, uint depth = 0, bool checkStates = true);
    void clearAlpha(float alpha, float col = 0.f) const;
    void clearToAlpha(float alpha) const;
    void clearToColor(float r, float g, float b, float a) const;
    void clearToColor(float r, float g, float b, float a, size_t bufIndx) const;
    void clearWhite() const;
    void clear() const;
    void clearDepth() const;
    bool saveToFile(const std::filesystem::path &filename, size_t attachNr, GLenum intFormat);
    void deleteColorTextures() const;
    void deleteDepthTextures() const;
    void download(void *ptr, GLenum intFormat, GLenum extFormat = 0) const;
    void getTexImage(FIBITMAP* fibitmap, GLuint texId, GLenum saveTarget, GLenum format) const;
    void genFbo();
    void deleteFbo() const;
    GLuint getColorImg() const;
    GLuint getColorImg(int index) const;

    [[nodiscard]] GLuint       getDepthImg() const { return  m_hasDepthBuf ? m_depthBuf : 0; }
    [[nodiscard]] GLuint       getWidth() const { return m_tex_width; }
    [[nodiscard]] GLuint       getHeight() const { return m_tex_height; }
    [[nodiscard]] GLuint       getDepth() const { return m_tex_depth; }
    [[nodiscard]] GLuint       getFbo() const { return m_fbo; }
    [[nodiscard]] GLenum       getType() const { return m_type; }
    [[nodiscard]] GLenum       getExtType() const { return m_extType; }
    [[nodiscard]] GLenum       getPixelType() const { return m_pixType; }
    GLenum                    *getTypePtr() { return &m_type; }
    [[nodiscard]] GLenum       getTarget() const { return m_target; }
    [[nodiscard]] bool         getHasDepthBuffer() const { return m_hasDepthBuf; }
    [[nodiscard]] bool         getDepthType() const { return m_hasDepthBuf; }
    [[nodiscard]] bool         getIsLayered() const { return m_layered; }
    [[nodiscard]] int          getNrAttachments() const { return m_nrAttachments; }
    [[nodiscard]] int          getMipMapLevels() const { return m_mipMapLevels; }
    [[nodiscard]] int          getNrSamples() const { return m_nrSamples; }
    [[nodiscard]] GLenum       getWrapMode() const { return m_wrapMode; }
    [[nodiscard]] GLenum       getMagFilterType() const { return m_magFilterType; }
    [[nodiscard]] GLenum       getMinFilterType() const { return m_minFilterType; }
    std::vector<GLuint>       *getTextures() { return &m_textures; }
    std::vector<GLenum>       *getBufModes() { return &m_bufModes; }
    [[nodiscard]] bool         isInited() const { return m_inited; }
    [[nodiscard]] bool         isMultiSample() const { return m_isMultiSample; }
    static void                printFramebufferLimits();
    [[nodiscard]] uint         getBitCount() const { return ::ara::getBitCount(m_type); }
    [[nodiscard]] uint         getNrChan() const { return getNrColChans(m_type); }
    [[nodiscard]] int          getSharedLayer() const { return m_sharedLayer; }
    std::mutex                *getSharedDrawMtx() { return m_sharedDrawMtx; }
    [[nodiscard]] unsigned int getColDataSize() const { return getWidth() * getHeight() * getNrColChans(m_type); }
    void                      *getContext() { return m_enterCtx; }

    void assignTex(int attachmentNr, GLuint tex) const;
    void setMinFilter(GLint type);
    void setMagFilter(GLint type);
    void setMinFilter(GLint type, int attNr) const;
    void setMagFilter(GLint type, int attNr) const;
    void set3DLayer(int attachment, int offset) const;
    void setMipMapLevels(int nrLevels) { m_mipMapLevels = nrLevels; }
    void setNrAttachments(int nrAtt) { m_nrAttachments = nrAtt; }
    void setWidth(int width) { m_tex_width = width; }
    void setHeight(int height) { m_tex_height = height; }
    void setDepth(int depth) { m_tex_depth = depth; }
    void setType(GLenum typeIn) { m_type = typeIn; }
    void setNrSamples(int nrSamples) { m_nrSamples = nrSamples; }
    void setTarget(GLenum target) { m_target = target; }
    void setHasDepthBuffer(bool val) { m_hasDepthBuf = val; }
    void setWrapMode(GLenum mode) { m_wrapMode = mode; }
    void setLayered(bool val) { m_layered = val; }
    void setSharedDrawMtx(std::mutex *mtx) { m_sharedDrawMtx = mtx; }
    void setGlbase(GLBase *glbase) { m_glbase = glbase; }
    static void checkFbo();
    void getActStates();
    void restoreStates();

private:
    bool m_hasDepthBuf = false;
    bool m_layered = false;
    bool m_inited = false;
    bool m_isShared      = false;
    bool m_isMultiSample = false;
    bool m_hasTexViews   = false;

    void *m_enterCtx         = nullptr;
    void *m_nativeDeviceHndl = nullptr;

    GLuint               m_fbo              = 0;
    GLuint               m_maxNrDrawBuffers = 2;
    std::array<GLint, 2> m_lastDrawBuffers  = {0};
    std::array<float, 4> m_clearColAr{};
    std::vector<GLenum>  m_restAttachments;
    GLboolean            m_lastMultiSample    = false;
    int                  m_lastBoundFbo       = 0;
    int                  m_lastDrawBuffer     = 0;
    int                  m_restNrValidBuffers = 0;

    GLuint  m_tex_width    = 0;
    GLuint  m_tex_height   = 0;
    GLuint  m_tex_depth    = 1;
    GLfloat m_f_tex_width  = 0;
    GLfloat m_f_tex_height = 0;
    GLfloat m_f_tex_depth  = 0;

    std::array<GLint, 4> m_csVp{};

    std::vector<GLuint> m_textures;
    GLuint              m_depthBuf      = 0;
    GLenum              m_type          = GL_RGBA8;
    GLenum              m_extType       = GL_RGBA;
    GLenum              m_pixType       = GL_UNSIGNED_BYTE;
    GLenum              m_depthType     = 0;
    GLenum              m_target        = 0;
    GLenum              m_wrapMode      = GL_REPEAT;
    GLenum              m_magFilterType = GL_LINEAR;
    GLenum              m_minFilterType = GL_LINEAR;

    std::unique_ptr<Quad> m_quad;

    GLBase          *m_glbase        = nullptr;
    ShaderCollector *m_shCol         = nullptr;
    Shaders         *m_colShader     = nullptr;
    Shaders         *m_toAlphaShader = nullptr;
    Shaders         *m_clearShader   = nullptr;

    int m_mipMapLevels  = 1;
    int m_nrAttachments = 1;
    int m_nrSamples     = 1;
    int m_sharedLayer   = -1;

    std::vector<GLenum> m_bufModes;
    glm::vec4           m_clearCol{0.f};

    std::mutex               *m_sharedDrawMtx = nullptr;
    FBO                      *m_sharedFbo     = nullptr;
    static inline const float m_transparent[] = {0, 0, 0, 0};
};
}  // namespace ara
