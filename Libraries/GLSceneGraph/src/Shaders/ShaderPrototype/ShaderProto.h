//
//  ShaderProto.h
//
//  Created by Sven Hahne on 14.08.14.
//

#pragma once

#include "Lights/Light.h"
#include "SceneNodes/SceneNode.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"
#include "Utils/UniformBlock.h"

namespace ara {
class ShaderProto {
public:
    struct isThisLight {
        Light* m_value;
        isThisLight(Light* value) : m_value(value) {}
        bool operator()(const Light* cls) const { return (cls == m_value); }
    };

    ShaderProto(sceneData* sd);
    virtual ~ShaderProto() = default;

    virtual void sendPar(CameraSet* cp, double time, SceneNode* node, SceneNode* parent, renderPass pass,
                         uint loopNr = 0)                                = 0;
    virtual bool begin(CameraSet* cp, renderPass _pass, uint loopNr = 0) = 0;
    virtual bool end(renderPass pass, uint loopNr = 0)                   = 0;
    virtual void postRender(renderPass pass) {}

    virtual std::string getNodeDataUb(uint32_t nrCameras);
    virtual std::string getUbPar(uint32_t nrCameras);

    virtual void addLight(Light* light);
    virtual void removeLight(Light* light);
    virtual void calcLights(CameraSet* cp) {}
    virtual void eraseShadowMap(uint nr) {}
    virtual void clear(renderPass pass) = 0;

    virtual Shaders* getShader(renderPass pass, uint loopInd = 0) = 0;
    virtual void     setNrCams(int nrCams) { s_nrCams = nrCams; }
    virtual void     setScreenSize(uint width, uint height) {}
    virtual void      mouseDownLeft(float x, float y, SceneNode* sceneTree) {}
    virtual void      mouseUpLeft(SceneNode* sceneTree) {}
    virtual void      mouseUpRight(SceneNode* sceneTree) {}
    virtual void      mouseMove(float x, float y) {}
    virtual void      keyDown(hidData* data) {}
    virtual void      keyUp(hidData* data) {}
    virtual transMode getTransMode() { return transMode(0); }

    Shaders*         s_shader = nullptr;
    ShaderCollector* s_shCol  = nullptr;
    UniformBlock     s_ub;
    GLBase*          s_glbase = nullptr;

    int s_nrCams     = 0;
    int s_camLimit   = 0;
    int s_skipForInd = 0;

    uint s_nrLights  = 0;
    uint s_scrWidth  = 0;
    uint s_scrHeight = 0;
    uint s_nrPasses  = 1;

    bool s_reqCalcLights       = false;
    bool s_useUniformBlock     = false;
    bool s_usesNodeMaterialPar = false;

    float s_ambientBrightness = 0.f;
    float s_maxSceneLightDens = 0.f;
    float s_highLight         = 0.f;
    int   s_showProjBright    = 0;

    std::vector<Light*> s_lights;

    std::string s_name;
    sceneData*  s_sd = nullptr;
};
}  // namespace ara
