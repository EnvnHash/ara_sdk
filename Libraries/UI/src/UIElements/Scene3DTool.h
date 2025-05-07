//
// Created by Sven Hahne on 25.10.2021.
//

#pragma once

#include <UIElements/Scene3D.h>
#include <UIElements/Gizmo.h>
#include "Button/ImageButton.h"
#include <CameraSets/CsPerspFbo.h>
#include "TransformWidget.h"

namespace ara {

class Scene3DTool : public Div {
public:
    enum class ToolBarIcon : char {
        none = 0,
        select,
        translate,
        rotate,
        twWidget,
        pan,
        orbit,
        zoom,
        /*zoomIn, zoomOut,*/ zoomReset,
        coarseFine
    };
    enum class toolState : int { objToolBlocked = 0, allUnBlocked };

    Scene3DTool();
    explicit Scene3DTool(const std::string& styleClass);
    ~Scene3DTool() override;

    void init() override;
    void connectUINodes();
    void startMouseDrag(TrackBallCam* cam, hidData& data);
    void resetMousePos();
    void resetZoom();
    void objectToolClicked(ToolBarIcon idx);
    void setToolState(toolState st);
    void viewToolClicked(ToolBarIcon idx);
    void setMouseRotScale(float x, float y);
    void setMouseRotExp(float exp);
    void setModifyCoarseFine();
    void setTransWidget(SceneNode* node);
    void setKeyTransStep(float fine, float normal, float coarse);
    void setKeyRotStep(float fine, float normal, float coarse);
    void enableNetCamHighlight(bool val);

    float*  getKeyTransStep() { return m_keyTransStep.data(); }
    float*  getKeyRotStep() { return m_keyRotStep.data(); }

    [[nodiscard]] Scene3D<CsPerspFbo>* getScene3D() const { return m_scene3D; }
    [[nodiscard]] Gizmo*               getGizmoWidget() const { return m_axesGizmo; }
    [[nodiscard]] TransformWidget*     getTransWidget() const { return m_transWidget; }
    [[nodiscard]] Div*                 getToolBar() const { return m_toolBar; }
    [[nodiscard]] Div*                 getScene3DCont() const { return m_sceneContainer; }
    [[nodiscard]] TrackBallCam*        getSceneCam() const { return m_scene3D ? m_scene3D->getSceneCamDef() : nullptr; }
    [[nodiscard]] Div*                 getRightSideCont() const { return m_rightSide; }
    [[nodiscard]] Div*                 getSceneContainer() const { return m_sceneContainer; }
    [[nodiscard]] Image*               getSceneBackImg() const { return m_sceneBackImg; }
    [[nodiscard]] transMode            getTransMode() const {
        if (m_scene3D && m_scene3D->getObjectSelector()) {
            return m_toolState == toolState::objToolBlocked ? transMode::none
                                                            : m_scene3D->getObjectSelector()->getTransMode();
        }
        return transMode::none;
    }

    /** for resetZoom option, view will be aligned to view to be behind
     * m_aimingNode looking towards m_focusNode, using the basePlane upvector */
    void setFocusNode(SceneNode* node) { m_focusNode = node; }
    void setAimingNode(SceneNode* node) { m_aimingNode = node; }
    /** shortcut for setFixAspect on sceneContainer */
    void setSceneAspect(float aspect) {
        m_sceneAspect = aspect;
        if (m_sceneContainer) {
            m_sceneContainer->setFixAspect(aspect);
        }
    }

private:
    Div*                 m_toolBar        = nullptr;
    Div*                 m_sceneContainer = nullptr;
    Div*                 m_rightSide      = nullptr;
    Scene3D<CsPerspFbo>* m_scene3D        = nullptr;
    Gizmo*               m_axesGizmo      = nullptr;
    TransformWidget*     m_transWidget    = nullptr;
    SceneNode*           m_focusNode      = nullptr;
    SceneNode*           m_aimingNode     = nullptr;
    Image*               m_sceneBackImg   = nullptr;

    glm::vec2 m_dragStartPos{0.f};
    glm::vec2 m_mouseRotScale{1.5f, 1.5f};
    std::array<glm::vec3, 2> m_bb{};
    glm::vec3 m_modelCenter{0.f};
    glm::vec3 m_modelCamVec{0.f};
    glm::vec3 m_stdCamVec{0.f, 0.f, -1.f};
    glm::vec3 m_stdUpVec{0.f, 1.f, 0.f};
    glm::vec3 m_upVec{0.f};
    glm::quat m_rot{};
    glm::mat4 m_rzViewMat = glm::mat4(1.f);
    std::array<glm::vec3, 2> m_newBb{};

    float m_sceneAspect = 1.777f;
    float m_mouseRotExp = 1.f;
    std::array<float, 3> m_keyTransStep{0.01f, 0.1f, 1.f};
    std::array<float, 3> m_keyRotStep{0.1f, 1.f, 10.f};

    toolState m_toolState           = toolState::objToolBlocked;
    bool      m_preventObjToolClick = false;

    std::unordered_map<ToolBarIcon, ImageButton*> m_toolBarIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_objToolIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_objTransIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_viewToolIcons;
};

}  // namespace ara
