//
// Created by user on 21.01.2022.
//

#pragma once

#include "DrawManager.h"

namespace ara {

class DrawManagerGizmo : public DrawManager {
public:
    DrawManagerGizmo(GLBase* glbase) : DrawManager(glbase) {}

    void     draw() override;
    Shaders* getShader(DrawSet& ds) override;

    void setOptTex(GLuint tex) { m_optTex = tex; }

private:
    GLuint m_optTex = 0;
};

}  // namespace ara
