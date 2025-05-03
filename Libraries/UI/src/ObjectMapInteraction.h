#pragma once

#include <GLBase.h>
#include <Shaders/ShaderUtils/ShaderBuffer.h>
#include <Utils/FBO.h>

#include "Shaders/ShaderCollector.h"
#include "StopWatch.h"

namespace ara {

class ObjectMapInteraction {
public:
    ObjectMapInteraction(ShaderCollector* _shCol, uint32_t width, uint32_t height, FBO* extFbo = nullptr,
                         bool isMultisample = false);
    virtual ~ObjectMapInteraction() = default;

    void initObjIdShdr();
    void initExtFboObjIdShdr();
    void initExtFboObjIdTexAlphaShdr();
    void initNormExtFboObjIdShdr();
    void initMSTextureReadShdr();

    uint32_t drawElement(const std::function<void(Shaders*, uint32_t*)>& func, glm::vec4* scissor,
                         bool disable_depth = true) { return 0; }  // marco.g : disable_depth implemented to handle 3D objects
                                                      // correctly, if set to false no operation to GL_DEPTH_TEST
                                                      // will be performed     //           note : the FBO has
                                                      // been also altered to it can handle depth (20200708)

    void resize(uint32_t width, uint32_t height, float pixRatio) {} // placeholder
    FBO* getObjIdFbo() { return nullptr; }
    void bindObjIdFbo() {}
    void unbindObjIdFbo() {}

    // must be normalized coordinates [0-1]
    // this is called from procHID(), no FBO is bound. x and y must be in hardware pixels
    uint32_t getObjAtPos(float x, float y) { return 0; }

    bool      usesExtFbo() { return m_extFbo; }
    float     getMaxObjId() const { return m_maxNrGuiObjects - 1.f; };
    uint32_t* getIdCtr() { return &m_idCtr; }
    FBO*      getFbo() { return m_extFbo ? m_extFbo : m_objIDFBO; }
    FBO*      getDownFbo() { return m_downFbo; }
    Shaders*  getNormShdr() { return m_normObjIdShdr; }

    void reset() { m_idCtr = 1; }

private:
    FBO* m_objIDFBO    = nullptr;
    FBO* m_extFbo      = nullptr;
    FBO* m_downFbo     = nullptr;
    FBO* m_down1PixFbo = nullptr;

    ShaderCollector* m_shCol             = nullptr;
    Shaders*         m_objIdShdr         = nullptr;
    Shaders*         m_objIdTexAlphaShdr = nullptr;
    Shaders*         m_normObjIdShdr     = nullptr;
    Shaders*         m_msFboReadShdr     = nullptr;

    StopWatch m_watch;

    GLuint* m_pbos    = nullptr;
    GLuint  m_nullVao = 0;
    GLubyte m_pixels[4];

    glm::ivec2 m_guiFboSize{0};
    glm::ivec2 m_mousePosPix{0};
    uint32_t   m_idCtr           = 0;
    uint32_t   m_nrPboBufs       = 0;
    uint32_t   m_pboIndex        = 0;
    uint32_t   m_downId          = 0;
    float      m_maxNrGuiObjects = 16777216.f;  // "only" 2^24 due to limitation in
                                                // opengl fragDepth processing
    float m_pixRatio  = 1.f;
    bool  m_multiSamp = false;
    bool  m_useSSBO   = false;

    GLubyte* m_ptr = nullptr;

    ShaderBuffer<GLuint>* m_sb;
    std::array<GLuint, 1> m_int;
};

}  // namespace ara
