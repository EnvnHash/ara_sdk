//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once
#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {
class ShadowMapArray : public ShadowMap {
public:
    ShadowMapArray(CameraSet* _cs, int _scrWidth, int _scrHeight, uint _initNrLights);
    ~ShadowMapArray() = default;

    void rebuildShader(uint _nrLights);
    void rebuildFbo(uint _nrLights);
    void setShadowTexPar(GLenum type);
    void begin();
    void end();
    void clear();
    void setNrLights(uint _nrLights);
    void setScreenSize(uint _width, uint _height);
    void bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs);

private:
    GLint               max_shader_invoc;
    GLint               maxNumberLayers;
    std::vector<GLuint> depthTexViews;
    ShaderCollector*    shCol;
    // sceneData*					scd;
    uint nrLights;
};
}  // namespace ara
