//
//  SkyBoxBlend.h
//  Tav_App
//
//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <CameraSets/CameraSet.h>

#include "GeoPrimitives/Sphere.h"
#include "SceneNodes/SceneNode.h"
#include "Shaders/Shaders.h"
#include "Utils/TFO.h"
#include "Utils/Texture.h"

namespace ara {
class SkyBoxBlend {
public:
    SkyBoxBlend(std::string textureFile, unsigned _nrCams = 1, sceneData* sd = nullptr);
    SkyBoxBlend(const char* textureFile, unsigned _nrCams = 1, sceneData* sd = nullptr);
    virtual ~SkyBoxBlend();

    void init(const char* textureFile, unsigned _nrCams, sceneData* sd = nullptr);
    void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* tfo = nullptr);
    void setMatr(glm::mat4* inMatr) { modMatr = inMatr; }

    void remove();

private:
    GLBase*     m_glbase     = nullptr;
    Sphere*     sphere       = nullptr;
    Texture*    cubeTex      = nullptr;
    Shaders*    perlinShader = nullptr;
    Shaders*    sbShader     = nullptr;
    Shaders*    testShader   = nullptr;
    std::string vShader;
    std::string gShader;
    std::string fShader;
    std::string vShaderSrc;
    GLuint      texUnit = 0;
    glm::mat4   pvm;
    float       angle   = 0.f;
    glm::mat4*  modMatr = nullptr;
};
}  // namespace ara
