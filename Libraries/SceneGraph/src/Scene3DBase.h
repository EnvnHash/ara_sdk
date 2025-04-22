/**
 * \brief Scene3DBase, Entry point for everything that needs to be displayed in
 * 3D
 *
 */

#pragma once

#include <Assimp/AssimpImport.h>
#include <CameraSets/CameraSet.h>
#include <GLUtils/SSAO.h>
#include <GLUtils/sceneData.h>
#include <Lights/LICamera.h>
#include <SceneNodes/SNGridFloor.h>
#include <SceneNodes/SNGridFloorAxes.h>
#include <SceneNodes/SNSkyBox.h>
#include <Shaders/ShaderPrototype/SPObjectSelector.h>
#include <StopWatch.h>
#include <Div.h>
#include <glb_common/glb_common.h>

#include "SceneNodes/SNWorldAxes.h"

namespace ara {

class BoundingBoxer;

class Scene3DBase : public Div {
public:
    Scene3DBase();
    virtual ~Scene3DBase();

    void         init() override;
    virtual bool initScene();
    virtual void initSceneData();
    virtual void initBoundingBoxer();
    virtual void initSceneTree();
    virtual void initGizmos();
    virtual void initSsao();
    virtual void initCamSet() {}
    virtual void initShaderProtos();
    virtual void enableSkyBox(bool val);
    virtual void loadBasicSceneModels();
    virtual void loadTypo();

    bool draw(uint32_t* objId) override;
    bool drawIndirect(uint32_t* objId) override;
    bool drawFunc(uint32_t* objId);

    void updateMatrix() override;

    void keyDown(hidData* data) override;
    void keyUp(hidData* data) override;
    void moveObjectByArrowKeys(const hidData* data);
    void mouseDown(hidData* data) override;
    void mouseUp(hidData* data) override;
    void mouseDownRight(hidData* data) override;
    void mouseUpRight(hidData* data) override;
    void mouseDrag(hidData* data) override;
    void mouseWheel(hidData* data) override;
    void resetMousePos();
    void setViewport(float x, float y, float width, float height) override;
    void setViewport(glm::vec4* viewport) override;
    void updateScene3DBaseViewport(float x, float y, float width, float height);

    virtual void addLightObj(SPObjectSelector* objSel, const std::string& _type);
    virtual bool addCamToSet(LICamera* netCam);
    virtual void removeCamFromSet(LICamera* netCam);
    virtual void addCameraViewRelative(SPObjectSelector* objSel);
    virtual void swapCameras(TrackBallCam* cam1, TrackBallCam* cam2);
    virtual void addShaderProto(const std::string* shdrName);
    void         setBasePlane(basePlane bp);
    void freeGLResources();

    template <class T>
    void initCamSet() {
        // init the cameraSet
        m_sceneRenderCam = std::make_unique<T>(&s_sd);
        if (!m_sceneRenderCam) {
            return;
        }
        if (m_sceneRenderCam->getLayer(0)) {
            m_sceneRenderCam->getLayer(0)->setClearColor(0.f, 0.f, 0.f, 0.f);
            m_sceneRenderCam->getLayer(0)->setFloorSwitch(1.f);
            m_sceneRenderCam->getLayer(0)->setTrackBallMode(TrackBallCam::mode::orbitNoRoll);
        }
        m_sceneRenderCam->setViewport(0, 0, (uint)s_sd.winViewport.z, (uint)s_sd.winViewport.w, false);

        m_camSet.emplace_back(m_sceneRenderCam.get());

        // trackball preset
        if (m_sceneRenderCam->getInteractCam()) {
            m_sceneRenderCam->getInteractCam()->setTrackBallTrans(
                vec3(0.f, 0.8f, 3.f));  // absolute position in world coordinates of the virtual trackball camera
            m_sceneRenderCam->getInteractCam()->setTrackBallRot(
                vec3(-0.3f, 0.f, 0.f));  // absolute position in world coordinates of the virtual trackball camera
            m_sceneRenderCam->getInteractCam()->updateFromExternal(true);
        }

        // share the cameraset globally
        s_sd.contentCamSet = (void*)&m_camSet;
        updateMatrix();
    }

    // void                 setViewport(uint _x, uint _y, uint _width, uint _height);
    virtual void         setFloorVisibility(bool val);
    virtual void         setWorldAxesVisibility(bool val, float ndcSize);
    virtual void         setWorldAxesVisibility(bool val);
    virtual SceneNode*   getModelSN(std::string&& name);
    virtual SceneNode*   getModelSN(std::string* name);
    virtual ShaderProto* getShaderProto(uint ind, std::string&& name);
    virtual void         setAddGizmoCb(std::function<void(transMode)> f);
    virtual void         addPassiveGizmo(SceneNode* node, float objRelativeSize = 0.3f, float worldScale = 1.f);
    virtual void         removePassiveGizmo(SceneNode* node);
    virtual void         scaleGizmos(float gizmoScale);
    virtual void         selectObj(int objId);
    virtual void         deselectAll();
    virtual void         setCfState(cfState cf);

    SPObjectSelector*           getObjectSelector() { return m_objSel; }
    std::vector<CameraSet*>*    getCamSet() { return &m_camSet; }
    SceneNode*                  getGizmoTree() const { return gizmoTree; }
    std::vector<SNGizmo*>*      getGizmos() { return &m_gizmos; }
    SceneNode*                  getSceneTree() const { return m_sceneTree; }
    std::vector<LICamera*>*     getNetCameras() { return &m_netCameras; }
    CameraSet*                  getSceneCam() { return m_sceneRenderCam ? m_sceneRenderCam.get() : nullptr; }
    TrackBallCam*               getSceneCamDef() { return m_sceneCam; }
    SceneNode*                  getGridFloor() { return m_gridFloor; }
    bool                        isFloorVisible() { return m_gridFloor && m_gridFloor->isVisible(); }
    float*                      getKeyTransStep() { return m_keyTransStep; }
    float*                      getKeyRotStep() { return m_keyRotStep; }
    basePlane                   getBasePlane() { return m_basePlane; }
    bool                        getWorldAxesVisibility() const { return m_worldAxisVisibility; }
    float                       getWorldAxesNdcSize() const { return m_worldAxisNdcSize; }
    glm::vec2&                  getMouseRotScale() { return m_mouseRotScale; }
    float                       getMouseRotExp() const { return m_mouseRotExp; }
    cfState                     getCfState() { return m_cfState; }
    sceneData*                  getSceneData() { return &s_sd; }

    void setSceneTrackBallMode(TrackBallCam::mode mode) {
        if (m_sceneCam) {
            m_sceneCam->setTrackBallMode(mode);
        }
    }
    void setKeyTransStep(float fine, float normal, float coarse) {
        m_keyTransStep[0] = fine;
        m_keyTransStep[1] = normal;
        m_keyTransStep[2] = coarse;
    }
    void setKeyRotStep(float fine, float normal, float coarse) {
        m_keyRotStep[0] = fine;
        m_keyRotStep[1] = normal;
        m_keyRotStep[2] = coarse;
    }
    void setDeselectAllCb(std::function<void()> func) { m_deselectAllCb = std::move(func); }
    void setDataPath(const std::string* _dataPath) { s_sd.dataPath = *_dataPath; }
    void setAllProcSteps() {
        for (auto i = 0; i < GLSG_NUM_RENDER_PASSES; i++) {
            m_renderPasses[(renderPass)i]    = true;
            m_reqRenderPasses[(renderPass)i] = true;
        }
    }
    void setDrawProcSteps() {
        m_reqRenderPasses[GLSG_SCENE_PASS]      = true;
        m_reqRenderPasses[GLSG_SHADOW_MAP_PASS] = true;
        m_reqRenderPasses[GLSG_OBJECT_MAP_PASS] = true;
    }
    void setMouseRotScale(float x, float y) {
        m_mouseRotScale.x = x;
        m_mouseRotScale.y = y;
    }
    void setMouseRotExp(float exp) {
        if (m_sceneCam) {
            m_sceneCam->setMouseRotExp(exp);
        }
        m_mouseRotExp = exp;
    }
    void         setGizmoVisibility(bool val) { m_drawGizmoFbo = val; }
    void         setExtDrawMatr(std::function<float*()> f) { m_extDrawMatrCb = std::move(f); }
    void         setLoadBasicSceneModels(bool val) { m_loadBasicSceneModels = false; }
    virtual void setPermRedraw(bool val);

    void addInitCb(std::function<void()> func) { m_initCb.emplace_back(std::move(func)); }

    std::unique_ptr<SceneNode> m_rootNode;
    SceneNode*                 m_sceneTree     = nullptr;
    SceneNode*                 m_sceneTreeCont = nullptr;
    SceneNode*                 gizmoTree       = nullptr;
    StopWatch                  watch;
    StopWatch                  cbWatch;

    std::unique_ptr<SceneNode> spotLightSN;
    std::unique_ptr<SceneNode> projectorSN;
    std::unique_ptr<SceneNode> netCamSN;

    std::condition_variable                                             stopped;
    std::map<std::string, std::list<std::function<void(Scene3DBase*)>>> m_preSceneShdrProtoPar;
    std::map<renderPass, std::atomic<bool>>                             m_reqRenderPasses;

protected:
    ShaderCollector*               m_shCol = nullptr;
    std::unique_ptr<BoundingBoxer> m_boundBoxer;
    std::unique_ptr<Quad>          m_normQuad;

    Shaders* m_colShdr = nullptr;

    // camera setups for rendering
    std::vector<CameraSet*>    m_camSet;
    std::unique_ptr<CameraSet> m_sceneRenderCam;
    TrackBallCam*              m_sceneCam = nullptr;
    std::unique_ptr<FBO>       m_gizmoFbo;
    std::vector<SNGizmo*>      m_gizmos;
    SNGridFloor*               m_gridFloor        = nullptr;
    SNGridFloor*               m_gridFloorPre     = nullptr;
    SNGridFloorAxes*           m_gridFloorAxesPre = nullptr;
    SNGridFloorAxes*           m_gridFloorAxes    = nullptr;
    SNWorldAxes*               m_worldAxesPre     = nullptr;
    SNWorldAxes*               m_worldAxes        = nullptr;
    SNGizmo*                   m_sceneGizmo       = nullptr;
    SNGizmo*                   m_sceneGizmoPre    = nullptr;
    SNSkyBox*                  m_skyBox           = nullptr;

    // projector, spots, etc
    std::vector<LICamera*>        m_netCameras;
    std::unique_ptr<SSAO>         m_ssao;
    std::unique_ptr<TypoGlyphMap> m_typo;
    std::map<renderPass, bool>    m_renderPasses;

    glm::vec3   m_roomDim{0.f};
    glm::vec2   s_mousePos{0.f};
    glm::vec2   m_mouseRotScale{1.5f, 1.5f};
    glm::vec2   m_dragStartPos{0.f};
    glm::vec4   m_typocol{1.f, 0.f, 0.f, 1.f};
    glm::vec4   m_nodeVpGL{0.f};
    std::string m_fpsStr;

    bool m_inited               = false;
    bool m_useSsao              = false;
    bool m_drawFps              = false;
    bool m_startTimeSet         = false;
    bool m_forceMouseUp         = false;
    bool m_leftClick            = false;
    bool m_worldAxisVisibility  = true;
    bool m_drawGizmoFbo         = true;
    bool m_leftClickViewDrag    = false;
    bool m_isMouseTranslating   = false;
    bool m_loadBasicSceneModels = false;
    bool m_permRedraw           = false;
    bool m_scenePostRender      = true;
    bool m_enableSkybox         = false;
    bool m_enableGrid           = false;

    int m_fontSize{};

    double m_intTime  = 0.0;  // in seconds
    double m_lastTime = 0;
    double dt         = 0.0;

    float m_keyTransStep[3]{0.08f, 0.5f, 2.f};
    float m_keyRotStep[3]{0.005f, 0.05f, 0.5f};
    float m_worldAxisNdcSize = 0.2f;
    float m_mouseRotExp      = 1.f;

    uint32_t m_tempObjId = 0;
    uint32_t m_dfObjId   = 0;

    std::function<void()>            m_deselectAllCb;
    std::function<void(transMode)>   m_addGizmoCb;
    std::list<std::function<void()>> m_initCb;
    std::function<float*()>          m_extDrawMatrCb;

    SPObjectSelector* m_objSel = nullptr;

    std::chrono::time_point<std::chrono::system_clock> m_startTime;
    std::chrono::time_point<std::chrono::system_clock> m_timeNow;
    cfState                                            m_cfState = cfState::normal;

    AnimVal<float> m_testAnim;

    basePlane m_basePlane = basePlane::xz;
    sceneData s_sd;

#ifdef ARA_USE_ASSIMP
    std::array<std::unique_ptr<AssimpImport>, 3> m_aImport;
#endif
};

}  // namespace ara
