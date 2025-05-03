//
//  ObjectSelector.h
//
//  Created by Sven Hahne on 04/9/17
//

#pragma once

#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {

class FBO;
class SNGizmo;
class VAO;

class SPObjectSelector : public ShaderProto {
public:
    enum class objSelState : int { idle = 0, translating, rotating, scaling };

    SPObjectSelector(sceneData* sd);

    void buildFbo(uint nrCameras);
    void initDepthGenShader(uint nrCameras);
    void initDepthReadShader();
    bool zoomSelectedObject(trackballPreset* camPreset, bool toObjCenter = false, bool adjRotation = false);
    void zoomToBounds(trackballPreset* camPreset, const glm::vec3& bbMin, const glm::vec3& bbMax, const glm::vec3& transOffs,
                      bool toObjCenter = false, bool adjRotation = false);

    void calcGizmoAxisNDC();
    void calcMouseMoveVec(float x, float y, float* mouseMovedSign);
    void calcIntersectPlane(float x, float y, bool skipPlaneSorting);
    void calcOffsVecTransAndScale(glm::vec3* offsetVec, const float* mouseMovedSign);
    void mouseDownLeft(float x, float y, SceneNode* sceneTree) override;
    void mouseUpLeft(SceneNode* sceneTree) override;
    void mouseUpRight(SceneNode* sceneTree) override;
    void keyDown(hidData* data) override;
    void keyUp(hidData* data) override;

    glm::vec4 unProjectMouse(const glm::vec4& inPoint, glm::vec3 camPos, const glm::vec3& planeOrig, const glm::vec3& planeNormal);
    void      mouseMove(float x, float y) override;
    void      selectObj(int objId, bool onMouseUp);
    void      addGizmo(transMode gMode);

    void sendPar(CameraSet* cs, double time, SceneNode* node, SceneNode* parent, renderPass pass,
                 uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;

    void                        clear(renderPass pass) override;
    void                        clearDepth() const;
    Shaders*                    getShader(renderPass pass, uint loopNr = 0) override { return s_shader; }
    [[nodiscard]] float         getDepthAtScreen(float x, float y, float near, float far) const;
    void                        setScreenSize(uint width, uint height) override;
    void                        setTransMode(transMode trMode);
    void                        deselect();
    [[nodiscard]] SceneNode*    getGizmoNode() const;
    [[nodiscard]] SceneNode*    getLastSceneTree() const { return m_lastSceneTree; }
    [[nodiscard]] SceneNode*    getSelectedNode() const { return m_selectedNode; }
    [[nodiscard]] SceneNode*    getSelectedObjectNode() const { return m_selectedObjectNode; }
    glm::vec4&                  getGizmoMoveVec2D() { return m_gizmoMoveVec2D; }
    transMode                   getTransMode() override { return m_actTransMode; }
    objSelState                 getState() { return m_state; }
    [[nodiscard]] uint64_t      getGizmoselected() const { return m_gizmoselected; }
    cfState                     getCfState() { return m_cfState; }
    [[nodiscard]] bool          gizmoWasDragged() const { return m_gizmoDragged; }

    void setGizmoNodes(std::vector<SNGizmo*>* gizmos) { m_gizmos = gizmos; }
    void setLastSceneTree(SceneNode* inSceneTree) { m_lastSceneTree = inSceneTree; }
    void setNrCams(int nrCams) override {
        s_nrCams = nrCams;
        initDepthGenShader(nrCams);
        buildFbo(nrCams);
    }
    void setAddGizmoCb(std::function<void(transMode)> f) { m_addGizmoCb = std::move(f); }
    void setGNormCenter(glm::vec4&& v) { g_norm_center = v; }
    void setGNormEnd(glm::vec4&& v) { g_norm_end = v; }
    void setCfState(cfState s) { m_cfState = s; }

private:
    CameraSet* m_cs = nullptr;

    std::unique_ptr<FBO> m_fbo;
    std::unique_ptr<FBO> m_resFbo;
    SceneNode*           m_selectedNode       = nullptr;
    SceneNode*           m_selectedObjectNode = nullptr;
    SceneNode*           m_lastSceneTree      = nullptr;
    SceneNode*           m_lastGizmoTree      = nullptr;
    sceneData*           m_lastSceneData      = nullptr;

    Shaders*             m_read_shader = nullptr;
    std::unique_ptr<VAO> m_point;

    std::function<void(transMode)> m_addGizmoCb;

    GLfloat m_readVal = 0.f;

    bool m_doesInters            = false;
    bool m_needUnbindFbo         = false;
    bool m_mouseDragged          = false;
    bool m_gizmoDragged          = false;
    bool m_skipMoveAnalogToMouse = false;
    bool m_addGizmoSelectsAxis   = true;

    float m_aspect                  = 0.f;
    float m_rayDist                 = 0.f;
    float m_planeNormalLength       = 0.f;
    float m_gizmoDragAxisLength     = 0.f;
    float m_offsetSign              = 0.f;
    float m_mouseDragToleranceRot   = 0.001f;
    float m_mouseDragToleranceTrans = 0.001f;

    int m_nrCameras = 1;

    uint64_t    m_gizmoselected = 0;
    transMode   m_actTransMode  = transMode::translate;
    objSelState m_state         = objSelState::idle;

    glm::vec2 m_mouseDown{0.f};
    glm::vec2 m_mouseDownNorm{0.f};
    glm::vec2 m_mouseMoved2D{0.f};
    glm::vec3 m_mouseDownObjTransVec{0.f};
    glm::vec3 m_mouseDownObjScaleVec{0.f};
    glm::quat m_mouseDownObjRot{};

    glm::vec3 m_gizmoMouseDownTransVec{0.f};
    glm::vec3 m_gizmoMouseDownScaleVec{0.f};
    glm::quat m_gizmoMouseDownRot{};
    glm::mat4 m_gizmoMouseDownModelMat = glm::mat4(1.f);
    glm::vec3 m_newTransVec{0.f};
    glm::vec3 t_planeOrig{0.f};
    glm::vec3 t_planeNormal{0.f};
    glm::vec3 m_planeNormal{0.f};
    glm::vec3 m_planeWithGizmoAxisMostOrtho{0.f};
    glm::vec3 m_tempBoxSize{0.f};
    glm::vec4 g_norm_center{0.f};
    glm::vec4 g_norm_end{0.f};
    glm::vec4 m_rotMouseVec{0.f};
    glm::vec4 t_unProjMouseVec{0.f};
    glm::vec4 g_norm_center_mod{0.f};
    glm::vec4 g_norm_axis_mod{0.f};
    glm::vec3 g_center_offset3D{0.f};
    glm::vec4 m_gizmoDstTrans2D{0.f};
    glm::vec4 m_gizmoMoveVec2D{0.f};
    glm::vec4 m_gizmoCenter2D{0.f};
    glm::vec4 m_gizmoAxis2D{0.f};
    glm::vec4 m_gizmoDst2D{0.f};

    glm::mat4 m_gizmo_camModelMat = glm::mat4(1.f);
    glm::mat4 m_gizmo_projViewMat = glm::mat4(1.f);
    glm::mat4 m_invVP             = glm::mat4(1.f);

    glm::quat m_rotToXAxis{};
    glm::quat m_invRotToXAxis{};

    std::map<float, glm::vec3> m_sortedNormals;
    std::vector<SNGizmo*>*     m_gizmos = nullptr;

    cfState m_cfState = cfState::normal;
};

}  // namespace ara
