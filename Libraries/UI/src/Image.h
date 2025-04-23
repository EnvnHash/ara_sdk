//
// Created by user on 13.01.2021.
//

#pragma once

#include <Utils/PBO.h>
#include <Utils/Texture.h>
#include <Utils/PingPongFbo.h>

#include "Div.h"

namespace ara {

class AssetImageBase;

class Image : public Div {
public:
    Image();
    Image(std::string &&styleClass);
    Image(const std::string& file, int mipMapLevel, bool keep_aspect, align ax = align::center, valign ay = valign::center);
    ~Image() override = default;

    void            init() override;
    virtual void    initDefaults();
    void            loadStyleDefaults() override;
    void            updateStyleIt(ResNode *node, state st, std::string &styleClass) override;
    virtual void    setImgFlag(ResNode *node, state st);
    virtual void    setImgAlign(ResNode *node, state st);
    virtual void    setImgScale(ResNode *node, state st);
    virtual void    setImgBase(AssetImageBase *imgBase);
    void            updateDrawData() override;
    void            initUnitBlock();
    bool            draw(uint32_t *objId) override;
    bool            drawIndirect(uint32_t *objId) override;
    void            pushTexture(DrawSet *ds);
    void            pushVaoUpdtOffsets() override;
    virtual void    loadImg();
    virtual void    reload();
    virtual bool    setTexId(GLuint inTexId, int width, int height, int bitCount);
    virtual void    setFillToNodeSize(bool val, state st = state::m_state);
    virtual void    setObjUsesTexAlpha(bool val);
    void            clearDs() override;
    bool            isInBounds(glm::vec2& pos) override;

    virtual void    setImg(const std::string& file, int mipMapLevel = 1) ;
    unsigned        setImgFlags(unsigned flags);
    void            setImgScale(float scale);

    virtual void    setSizeToAspect(bool val) { m_sizeToAspect = val; }
    virtual void    selectSection(int idx) { m_sectIndex = idx; }
    void            setSizeChangeCb(std::function<void(int, int)> f) { m_sizeChangeCb = std::move(f); }
    void            setSrcBlendFunc(GLenum bf) { m_srcBlendFunc = bf; }
    void            setDstBlendFunc(GLenum bf) { m_dstBlendFunc = bf; }
    void            setSrcBlendAlphaFunc(GLenum bf) { m_srcBlendAlphaFunc = bf; }
    void            setDstBlendAlphaFunc(GLenum bf) { m_dstBlendAlphaFunc = bf; }
    void            setSepBlendFunc(bool val) { m_sepBlendFunc = val; }
    void            setLod(float val);
    void            setSectionSize(const glm::ivec2& sz) { m_secSize = sz; }
    void            setSectionSep(const glm::ivec2& sp) { m_secSep = sp; }
    void            setSectionPos(const glm::ivec2& pos) { m_secPos = pos; }
    void            setTextureSize(const glm::ivec2& tsz) { m_texSize = tsz; }
    void            setZOffsPos(float z) { m_offsZPos = z; }

    AssetImageBase *getImgBase() { return m_imgBase; }
    Texture        *getTexture() { return tex; }
    GLuint          getTexID();
    unsigned        getImgFlags() const { return m_imgFlags; }
    GLuint          getExtTexId() const { return m_texId; }
    glm::ivec2      getExtTexSize() const { return {m_extTexWidth, m_extTexHeight}; }
    int             getExtTexBitCount() const { return m_extTexBitCount; }
    bool            isLoaded() { return m_loaded; }
    int            *getImgBasePos() { return m_ppos; }
    int             getSectIdx() { return m_sectIndex; }
    PBO            *getUplPbo() { return &m_uplPbo; }

    void            resizeUplPbo(int w, int h, GLenum format) { m_uplPbo.resize(w, h, format); }
    PingPongFbo    *getUplFbo();
    void            initUplPbo(int w, int h, GLenum format);
    void            initUplFbo(int width, int height, GLenum type, GLenum target, bool depthBuf, int nrAttachments,
                               int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered);
    void            rebuildUplFbo(int width, int height, GLenum type, GLenum target, bool depthBuf, int nrAttachments,
                                  int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered);

    unsigned     m_imgFlags    = 0;
    float        m_imgScale    = 1;
    unsigned     m_imgAlign[2] = {1, 1};  // center,vcenter
    IndDrawBlock m_imgDB;
    float        m_texUnit = -1.f;

protected:
    Texture                     *tex       = nullptr;
    Shaders                     *m_texShdr = nullptr;
    std::unique_ptr<PingPongFbo> m_uplFbo;
    PBO                          m_uplPbo;
    UniformBlock                 m_texUniBlock;
    AssetImageBase              *m_imgBase = nullptr;

    bool m_loaded          = false;
    bool m_useTexId        = false;
    bool m_sizeToAspect    = false;
    bool m_sepBlendFunc    = false;
    bool m_objUsesTexAlpha = false;
    bool m_mpInBounds      = false;

    int m_mipMapLevel        = 8;
    int m_sectIndex          = 0;
    int m_extTexWidth        = 0;
    int m_extTexHeight       = 0;
    int m_extTexBitCount     = 0;
    int m_extObjTexObjIdOffs = 0;

    float m_texAspect = 1.f;
    float m_offsZPos  = 0.f;
    float m_lastObjId = 0.f;
    float m_lod       = 0.f;

    std::string m_imageFile;
    GLuint      m_texId             = 0;
    GLuint      m_indDrawTexId      = 0;
    GLuint      m_lastIndDrawTexId  = 0;
    GLuint      m_extObjTexId       = 0;
    GLuint      m_depthTexId        = 0;
    GLenum      m_srcBlendFunc      = GL_SRC_ALPHA;
    GLenum      m_dstBlendFunc      = GL_ONE_MINUS_SRC_ALPHA;
    GLenum      m_srcBlendAlphaFunc = GL_SRC_ALPHA;
    GLenum      m_dstBlendAlphaFunc = GL_ONE_MINUS_SRC_ALPHA;

    glm::mat4   m_pvm               = glm::mat4(1.f);

    glm::ivec2  m_secPos{0};
    glm::ivec2  m_secSize{0};
    glm::ivec2  m_secSep{0};
    glm::ivec2  m_texSize{0};
    glm::vec2   m_nSize{0.f};
    glm::vec2   m_nPos{0.f};
    glm::vec2   m_tuvSize{0.f};
    glm::vec2   m_uvSize{0.f};
    glm::vec2   m_hidMp{0.f};
    int         m_ppos[2]{0};

    std::function<void(int, int)> m_sizeChangeCb;
    std::vector<GLenum>           m_attachments = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

    glm::vec2 v{0.f}, uv{0.f}, tuv{0.f}, ts{0.f}, tso{0.f};
};

}  // namespace ara
