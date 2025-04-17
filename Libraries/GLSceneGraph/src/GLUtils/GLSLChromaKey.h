//
// Created by sven on 06-10-22.
//

#pragma once

#include <GLUtils/FastBlurMem.h>

#include "GLUtils/GLSLChromaKeyPar.h"
#include "GLUtils/GLSLHistogram.h"

namespace ara {

class GLSLChromaKey {
public:
    bool init(GLBase* glbase, Property<GLSLChromaKeyPar*>* par);
    void initChromaShdr();
    void initChromaMaskShdr();
    GLuint proc(GLuint tex);
    void pickMask2Color(glm::vec2& pos);
    void setPar(Property<GLSLChromaKeyPar*>* par) { m_par = par; }
    void setGLBase(GLBase* glbase) { m_glbase = glbase; }

    void readHistoVal();

    static glm::vec2 RGBAToCC(glm::vec3& col) {
        float y = 0.299f * col.r + 0.587f * col.g + 0.114f * col.b;
        return glm::vec2{(col.b - y) * 0.565, (col.r - y) * 0.713};
    }

    template <typename T>
    void onChanged(Property<T>* p, std::function<void(std::any)> f) {
        m_onValChangedCb[p] = std::make_shared<std::function<void(std::any)>>(f);
        if (p) p->onPreChange(m_onValChangedCb[p]);
    }

    bool isInited() { return m_inited; }

private:
    std::unordered_map<void*, std::shared_ptr<std::function<void(std::any)>>> m_onValChangedCb;

    Property<GLSLChromaKeyPar*>*   m_par            = nullptr;
    std::string                    m_emptyStr       = std::string{0};
    GLBase*                        m_glbase         = nullptr;
    Shaders*                       m_chromaShdr     = nullptr;
    Shaders*                       m_chromaMaskShdr = nullptr;
    Shaders*                       m_stdTex         = nullptr;
    std::unique_ptr<Quad>          m_quad;
    std::unique_ptr<Quad>          m_hFlipQuad;
    std::unique_ptr<FBO>           m_rgbFbo;
    std::unique_ptr<FBO>           m_maskFbo;
    std::unique_ptr<FBO>           m_procFbo;
    std::unique_ptr<GLSLHistogram> m_histo;
    std::unique_ptr<FastBlurMem>   m_fastBlurMem;

    std::array<Property<float>*, 3> m_keyCol  = {nullptr, nullptr, nullptr};
    std::array<Property<float>*, 3> m_keyCol2 = {nullptr, nullptr, nullptr};
    std::vector<uint8_t>            m_downBuf;

    std::function<void(std::any)> m_keyColUpdtFunc;
    std::function<void(std::any)> m_keyCol2UpdtFunc;

    glm::vec4 m_keyCC{0.f};
    glm::vec3 m_keyColTmp{0.f};
    glm::vec3 m_keyColTmp2{0.f};
    glm::vec2 m_pickMask2Color{0.f};

    bool m_inited           = false;
    int  m_nrBlurIt         = 1;
    int  m_chromaInRGBSpace = 0;
};

}  // namespace ara
