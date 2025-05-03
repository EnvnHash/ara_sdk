//
//  CameraSet.h
//
//  Created by Sven Hahne on 16.08.17
//

#pragma once

#include "Shaders/ShaderPrototype/ShaderProtoFact.h"
#include "Utils/TrackBallCam.h"

namespace ara {

class FBO;
class Quad;
class ShaderProto;
class SNGizmo;

class CameraSet {
public:
    explicit CameraSet(sceneData* sc);
    virtual ~CameraSet() = default;

    virtual void onKey(int key, int scancode, int action, int mods)                                               = 0;
    virtual void clearScreen(renderPass pass)                                                                     = 0;
    virtual void clearDepth()                                                                                     = 0;
    virtual void clearFbo()                                                                                       = 0;
    virtual void render(SceneNode* scene, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) = 0;
    virtual void renderTree(SceneNode* scene, double time, double dt, uint ctxNr, renderPass pass);
    virtual void iterateNode(SceneNode* scene, double time, double dt, uint ctxNr, renderPass pass, bool calcMatrixStack);

    virtual void renderFbos(float* extDrawMatr)                  = 0;
    virtual void postRender(renderPass pass, float* extDrawMatr) = 0;
    virtual void mask();

    virtual void setViewport(uint x, uint y, uint width, uint height, bool resizeProto) = 0;

    virtual ShaderProto* addShaderProto(const std::string& protoName, const std::list<renderPass>& protoPasses);
    virtual ShaderProto* getShaderProto(const std::string& protoName);
    void removeShaderProto(const std::string& protoName, const std::list<renderPass>& protoPasses);
    virtual std::map<std::string, std::unique_ptr<ShaderProto>>* getShaderProtos() { return &s_shaderProto; }
    virtual ShaderProto*  getProtoForPass(renderPass pass, SceneNode* node);

    virtual void                                                   setInteractCam(TrackBallCam* cam);
    virtual std::vector<std::pair<TrackBallCam*, void*>>::iterator addCamera(TrackBallCam* camDef, void* name);
    virtual void                                                   removeCamera(void* name);
    virtual void                                                   buildCamMatrixArrays();
    virtual void                                                   swapCameras(TrackBallCam* cam1, TrackBallCam* cam2);

    [[nodiscard]] int                             getNrCameras() const { return (int)s_cam.size(); }
    std::vector<std::pair<TrackBallCam*, void*>>* getCameras() { return &s_cam; }
    TrackBallCam*                                 getInteractCam() { return s_interactCam; }
    TrackBallCam*                                 getLayer(size_t idx) { return (s_cam.size() > idx) ?  s_cam[idx].first : nullptr; }

    glm::vec2*      getActFboSize() { return &s_actFboSize; }
    glm::vec4*      getViewport() { return &s_viewport; }
    glm::mat4       getMVP() { return s_interactCam ? s_interactCam->getMVP() : glm::mat4(1.f); }
    glm::mat4       getModelMatr() { return s_interactCam->getModelMatr(); }
    glm::mat4       getViewMatr() { return s_interactCam->getViewMatr(); }
    glm::mat4       getProjectionMatr() { return s_interactCam->getProjectionMatr(); }
    glm::mat3       getNormalMatr() { return s_interactCam->getNormalMatr(); }
    GLfloat*        getModelMatrPtr() { return s_interactCam->getModelMatrPtr(); }
    GLfloat*        getSetModelMatrPtr() { return &s_modelMatrixList[0][0][0]; }
    GLfloat*        getViewMatrPtr() { return s_interactCam->getViewMatrPtr(); }
    GLfloat*        getSetViewMatrPtr() { return &s_viewMatrixList[0][0][0]; }
    GLfloat*        getProjectionMatrPtr() { return s_interactCam->getProjMatrPtr(); }
    GLfloat*        getSetProjectionMatrPtr() { return &s_projectionMatrixList[0][0][0]; }
    GLfloat*        getSetFloorSwitches() { return &s_floorSwitches[0]; }
    int*            getSetFishEyeSwitches() { return &s_fishEyeSwitches[0]; }
    GLfloat*        getSetFishEyeParams() { return &s_fishEyeParam[0][0]; }
    GLfloat*        getMVPPtr() { return s_interactCam->getMVPPtr(); }
    GLfloat*        getNormalMatrPtr() { return s_interactCam->getNormMatrPtr(); }
    glm::vec3       getCamPos() { return s_interactCam->getCamPos(); }
    glm::vec3       getLookAtPoint() { return s_interactCam->getCamLookAt(); }
    glm::vec3       getViewerVec() { return s_interactCam->getViewerVec(); }
    float           getNear() { return s_interactCam->getNear(); }
    float           getFar() { return s_interactCam->getFar(); }
    float           getFov() { return s_interactCam->getFov(); }
    virtual FBO*    getFbo() { return nullptr; }
    virtual GLBase* getGLBase() { return m_glbase; }

    virtual void setMask(glm::vec3 scale, glm::vec3 trans);
    void         addUpdtCb(const std::function<void()>& cb, void* name) { s_updtCb[name].emplace_back(cb); }

    std::map<std::string, std::unique_ptr<ShaderProto>> s_shaderProto;
    std::vector<std::pair<TrackBallCam*, void*>>        s_cam;
    int                                                 s_selectObj   = 0;
    SNGizmo*                                            s_activeGizmo = nullptr;

protected:
    bool s_calcMatrixStack = false;

    glm::vec4  s_viewport{};
    glm::ivec4 s_iViewport{};

    float s_fScrWidth{};
    float s_fScrHeight{};

    GLBase*                                       m_glbase = nullptr;
    std::vector<std::unique_ptr<TrackBallCam>>    s_intern_cam;
    TrackBallCam*                                 s_interactCam = nullptr;
    Quad*                                         s_quad = nullptr;
    std::unique_ptr<Quad>                         s_maskQuad;
    Shaders*                                      s_clearShader = nullptr;
    ShaderCollector*                              s_shCol = nullptr;
    std::map<renderPass, std::list<ShaderProto*>> s_renderPassShProto;

    glm::vec2 s_actFboSize{};
    glm::mat4 s_sumMat{};

    std::vector<glm::vec4> s_clearColors;
    std::list<glm::mat4*>  s_matrixStack;
    std::vector<float>     s_floorSwitches;
    std::vector<int>       s_fishEyeSwitches;
    std::vector<glm::mat4> s_modelMatrixList;
    std::vector<glm::mat4> s_viewMatrixList;
    std::vector<glm::mat4> s_projectionMatrixList;
    std::vector<glm::vec4> s_fishEyeParam;

    std::map<void*, std::list<std::function<void()>>> s_updtCb;
    ShaderProtoFact                                   m_spf;
    sceneData*                                        s_sd = nullptr;
};
}  // namespace ara
