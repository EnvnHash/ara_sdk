// Created by user on 30.09.2020.
//

#pragma once

#ifdef ARA_USE_GLFW

#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"
#include "Utils/Texture.h"
#include "WindowManagement/GLWindow.h"

namespace ara {

class GLSplashScreen {
public:
    void open();
    void close();

private:
    GLWindow                         m_win{};
    std::unique_ptr<Quad>            m_quad;
    std::unique_ptr<ShaderCollector> m_shCol;
    Shaders                         *m_stdTex{};
    Texture                          m_tex;
};

}  // namespace ara

#endif