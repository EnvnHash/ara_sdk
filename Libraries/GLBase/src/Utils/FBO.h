/**
 * \brief FBO, C++ Framebuffer Object Wrapper (supporting all formats)
 *
 */

#pragma once

#include "glb_common/glb_common.h"

#ifdef ARA_USE_FREEIMAGE
#ifndef __EMSCRIPTEN__
#include <FreeImage.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <emscripten/emscripten.h>
#endif
#endif

namespace ara {

class GLBase;
class Shaders;
class Texture;
class Quad;

class FBO {
public:
    FBO();
    FBO(GLBase *glbase, int width, int height, int _mipMapLevels = 0);
    FBO(GLBase *glbase, int width, int height, GLenum type, GLenum target, bool depthBuf, int nrAttachments,
        int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered);
    FBO(GLBase *glbase, int width, int height, int depth, GLenum _type, GLenum _target, bool _depthBuf,
        int _nrAttachments, int _mipMapLevels, int _nrSamples, GLenum _wrapMode, bool _layered);

    virtual ~FBO();

    void remove();
    void fromShared(FBO *sharedFbo);
    void fromSharedExtractLayer(FBO *sharedFbo, int layer);
    void fromSharedSelectAttachment(FBO *sharedFbo, int attNr);
    void initFromShared(FBO *sharedFbo);
    void fromTexMan(Texture *texMan);
    void init();
    void allocColorTexture();
    void allocDepthTexture();
    void attachTextures(bool doCheckFbo);
    void bind(bool saveStates = true);
    void unbind(bool doRestoreStates = true);
    void blit(uint scrWidth, uint scrHeight, GLenum interp = GL_NEAREST) const;
    void blitTo(FBO *dst) const;
    void resize(uint _width, uint _height, uint _depth = 0, bool checkStates = true);
    void clearAlpha(float alpha, float col = 0.f) const;
    void clearToAlpha(float alpha);
    void clearToColor(float r, float g, float b, float a);
    void clearToColor(float r, float g, float b, float a, size_t bufIndx) const;
    void clearWhite() const;
    void clear();
    void clearDepth() const;
    bool saveToFile(const std::filesystem::path &filename, size_t attachNr, GLenum intFormat);
    void deleteColorTextures();
    void deleteDepthTextures();
    void download(void *ptr, GLenum intFormat, GLenum extFormat = 0);

    void genFbo() {
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    }

    void deleteFbo() {
        glDeleteFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLuint getColorImg() {
        if (!m_textures.empty() && m_nrAttachments > 0 && m_type != GL_DEPTH24_STENCIL8)
            return m_textures[0];
        else
            return 0;
    }

    GLuint getColorImg(int index) {
        if ((int)m_textures.size() > index && m_nrAttachments > index && m_type != GL_DEPTH24_STENCIL8)
            return m_textures[index];
        else
            return 0;
    }

    [[nodiscard]] GLuint getDepthImg() const {
        if (m_hasDepthBuf)
            return m_depthBuf;
        else
            return 0;
    }
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
    void setMinFilter(GLenum _type);
    void setMagFilter(GLenum _type);
    void setMinFilter(GLenum _type, int attNr);
    void setMagFilter(GLenum _type, int attNr);
    void set3DLayer(int attachment, int offset);
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

    void getActStates() {
        // if there is any FBO bound, remember it now, and rebind it later again
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_lastBoundFbo);
#ifndef ARA_USE_GLES31
        glGetBooleanv(GL_MULTISAMPLE, &m_lastMultiSample);
#endif
        for (GLuint i = 0; i < m_maxNrDrawBuffers; i++) glGetIntegerv(GL_DRAW_BUFFER0 + i, &m_lastDrawBuffers[i]);
    }

    void restoreStates() {
        // rebind last s_fbo
        glBindFramebuffer(GL_FRAMEBUFFER, m_lastBoundFbo);

        if (m_lastBoundFbo != 0) {
            m_restAttachments.clear();

            m_restNrValidBuffers = 0;
            for (GLuint i = 0; i < m_maxNrDrawBuffers; i++) {
                if (m_lastDrawBuffers[i]) {
                    m_restAttachments.emplace_back(GL_COLOR_ATTACHMENT0 + i);
                    m_restNrValidBuffers++;
                }
            }
            glDrawBuffers(m_restNrValidBuffers, &m_restAttachments[0]);
        }

#ifndef ARA_USE_GLES31
        if (!m_lastMultiSample)
            glDisable(GL_MULTISAMPLE);
        else
            glEnable(GL_MULTISAMPLE);
#endif
    }

private:
    bool m_hasDepthBuf;
    bool m_layered;
    bool m_inited = false;
    // bool m_hasBeenInited = false;
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
    GLuint  m_tex_depth    = 0;
    GLfloat m_f_tex_width  = 0;
    GLfloat m_f_tex_height = 0;
    GLfloat m_f_tex_depth  = 0;

    std::array<GLint, 4> m_csVp{};

    std::vector<GLuint> m_textures;
    GLuint              m_depthBuf      = 0;
    GLenum              m_type          = 0;
    GLenum              m_extType       = 0;
    GLenum              m_pixType       = 0;
    GLenum              m_depthType     = 0;
    GLenum              m_target        = 0;
    GLenum              m_wrapMode      = 0;
    GLenum              m_magFilterType = 0;
    GLenum              m_minFilterType = 0;

    std::unique_ptr<Quad> m_quad;

    GLBase          *m_glbase        = nullptr;
    ShaderCollector *m_shCol         = nullptr;
    Shaders         *m_colShader     = nullptr;
    Shaders         *m_toAlphaShader = nullptr;
    Shaders         *m_clearShader   = nullptr;

    int m_mipMapLevels  = 0;
    int m_nrAttachments = 0;
    int m_nrSamples     = 0;
    int m_sharedLayer   = -1;

    std::vector<GLenum> m_bufModes;
    glm::vec4           m_clearCol{0.f};

    std::mutex               *m_sharedDrawMtx = nullptr;
    FBO                      *m_sharedFbo     = nullptr;
    static inline const float m_transparent[] = {0, 0, 0, 0};
};
}  // namespace ara
