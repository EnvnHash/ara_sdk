//
//  SkyBox.h
//  Tav_App
//
//  Created by Sven Hahne on 26/11/21.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <GeoPrimitives/Sphere.h>
#include <SceneNodes/SceneNode.h>

namespace ara {

class SNSkyBox : public SceneNode {
public:
    explicit SNSkyBox(sceneData* sd = nullptr);

    void init();
    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;

private:
    std::unique_ptr<Quad>   m_normQuad;
    std::unique_ptr<Sphere> m_sphere;
    float                   m_angle  = 0.f;
    GLuint                  m_VAOId  = 0;
    GLuint                  m_posBuf = 0;
    int                     m_numberVertices = 0;
};
}  // namespace ara
