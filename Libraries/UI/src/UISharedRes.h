//
// Created by sven on 7/20/20.
//

#pragma once

#include "glsg_common/glsg_common.h"

namespace ara {

enum class uiColors : int { background = 0, darkBackground, font, sepLine, highlight, black, blue, darkBlue, white };

class AssetManager;
class DrawManager;
class GLFWWindow;
class ObjectMapInteraction;
class ShaderCollector;
class Quad;
class WindowBase;

class scissorStack {
public:
    std::vector<glm::vec4> stack;
    glm::vec4              active = glm::vec4{0.f};
};

class UISharedRes {
public:
    void*            win   = nullptr;
    ShaderCollector* shCol = nullptr;
#ifdef ARA_USE_GLFW
    GLFWWindow* winHandle = nullptr;
#else
    void* winHandle = nullptr;
#endif
    ObjectMapInteraction*                                      objSel   = nullptr;
    Quad*                                                      quad     = nullptr;
    Quad*                                                      normQuad = nullptr;
    glm::mat4*                                                 orthoMat = nullptr;
    std::filesystem::path                                      dataPath;
    std::map<winProcStep, ProcStep>*                           procSteps     = nullptr;
    WindowBase*                                                scene         = nullptr;
    bool                                                       requestRedraw = false;
    std::unordered_map<uiColors, glm::vec4>*                   colors;
    glm::ivec2                                                 gridSize;
    float                                                      padding;
    glm::ivec2*                                                minWinSize = nullptr;
    AssetManager*                                              res        = nullptr;
    GLuint*                                                    nullVao    = nullptr;
    DrawManager*                                               drawMan    = nullptr;
    GLBase*                                                    glbase     = nullptr;

    void setDrawFlag(bool val = true) {
        if (procSteps) {
            procSteps->at(Draw).active = true;
        }
    }
};
}  // namespace ara
