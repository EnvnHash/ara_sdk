//
//  SkyBoxBlend.h
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <glsg_common/glsg_common.h>

namespace ara {

class Sphere;
class Texture;
class Shaders;
class CameraSet;
class sceneData;
class TFO;

class SkyBoxBlend {
public:
    explicit SkyBoxBlend(const std::string& textureFile, unsigned _nrCams = 1, sceneData* sd = nullptr);

    void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* tfo = nullptr) const;
    void setMatr(glm::mat4* inMatr) { m_modMatr = inMatr; }

    void remove() const;

private:
    GLBase*                     m_glbase     = nullptr;
    std::unique_ptr<Sphere>     m_sphere;
    std::unique_ptr<Texture>    m_cubeTex;
    Shaders*                    m_perlinShader = nullptr;
    Shaders*                    m_sbShader     = nullptr;
    Shaders*                    m_testShader   = nullptr;
    std::string                 m_vShader;
    std::string                 m_gShader;
    std::string                 m_fShader;
    std::string                 m_vShaderSrc;
    GLuint                      m_texUnit = 0;
    glm::mat4                   m_pvm{};
    float                       m_angle   = 0.f;
    glm::mat4*                  m_modMatr = nullptr;
};
}  // namespace ara
