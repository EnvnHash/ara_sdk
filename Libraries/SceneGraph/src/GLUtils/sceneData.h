#pragma once

#include <GLBase.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>

#include "glsg_common/glsg_common.h"

namespace ara {

class sceneData {
public:
    sceneData() = default;
    ~sceneData() {}

    // inline static sceneData* inst() { if (!m_inst) m_inst = new sceneData();
    // return m_inst; }
    // static void deleteInst() { if (m_inst) { delete m_inst; m_inst = nullptr;
    // } }
    // GLSG_BIND static sceneData* m_inst;

    void *getCtxName() {
#ifdef _WIN32
        return (void *)wglGetCurrentContext();
#elif __linux__
#ifndef ARA_USE_GLES31
        return (void *)glXGetCurrentContext();
#else
        return nullptr;
#endif
#elif __APPLE__
        return (void *)CGLGetCurrentContext();
#endif
    }

    Quad *getStdQuad() {
        auto ctxName = getCtxName();
        if (!stdQuad[ctxName]) {
            stdQuad[ctxName] = std::make_unique<Quad>(QuadInitParams{});
        }
        return stdQuad[ctxName].get();
    }

    Quad *getStdHFlipQuad() {
        auto ctxName = getCtxName();
        if (!stdHFlipQuad[ctxName]) {
            stdHFlipQuad[ctxName] = std::make_unique<Quad>(QuadInitParams{.color = {0.f, 0.f, 0.f, 0.f}, .flipHori = true});
        }
        return stdHFlipQuad[ctxName].get();
    }

    Quad *getStdHalfQuad() {
        auto ctxName = getCtxName();
        if (!stdHalfQuad[ctxName]) {
            stdHalfQuad[ctxName] = std::make_unique<Quad>(QuadInitParams{ {-0.5f, -0.5f}, {1.f, 1.f} });
        }
        return stdHalfQuad[ctxName].get();
    }

    bool                                     debug              = false;
    bool                                     aiLogStreamSet     = false;
    bool                                     forceTestPicOutput = false;
    int                                     *texNrs             = nullptr;
    GLuint                                  *tex                = nullptr;
    GLuint                                  *textures           = nullptr;
    glm::vec2                                contentScale;
    glm::vec3                                camPos{0.f, 0.f, 1.f};
    glm::vec3                                camLookAt{0.f, 0.f, 0.f};
    glm::vec3                                camUpVec{0.f, 1.f, 0.f};
    glm::vec3                               *roomDim = nullptr;
    glm::vec4                               *colors  = nullptr;
    glm::vec4                                winViewport;
    std::string                              setupName;
    std::string                              dataPath;
    uint                                     nrTextures;
    void                                    *aImport       = nullptr;
    void                                    *boundBoxer    = nullptr;
    void                                    *contentCamSet = nullptr;
    void                                    *ft_fonts      = nullptr;
    void                                    *winMan        = nullptr;
    void                                    *uiWindow      = nullptr;
    void                                    *scene3D       = nullptr;
    void                                    *netCameras    = nullptr;
    std::mutex                              *drawMtx       = nullptr;
    std::function<void()>                    deselectAll;
    std::map<renderPass, std::atomic<bool>> *reqRenderPasses;
    GLBase                                  *glbase = nullptr;

private:
    std::map<void *, std::unique_ptr<Quad>> stdQuad;
    std::map<void *, std::unique_ptr<Quad>> stdHalfQuad;
    std::map<void *, std::unique_ptr<Quad>> stdHFlipQuad;
};

}  // namespace ara
