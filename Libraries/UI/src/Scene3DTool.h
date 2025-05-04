//
// Created by Sven Hahne on 25.10.2021.
//

#pragma once

#include <Scene3D.h>
#include <Gizmo.h>
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
    ~Scene3DTool();

    void init() override;
    void connectUINodes();
    void startMouseDrag(TrackBallCam* cam, hidData* data);
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

    float*               getKeyTransStep() { return m_keyTransStep; }
    float*               getKeyRotStep() { return m_keyRotStep; }
    Scene3D<CsPerspFbo>* getScene3D() { return m_scene3D; }
    Gizmo*               getGizmoWidget() { return m_axesGizmo; }
    TransformWidget*     getTransWidget() { return m_transWidget; }
    Div*                 getToolBar() { return m_toolBar; }
    Div*                 getScene3DCont() { return m_sceneContainer; }
    TrackBallCam*        getSceneCam() { return m_scene3D ? m_scene3D->getSceneCamDef() : nullptr; }
    Div*                 getRightSideCont() { return m_rightSide; }
    Div*                 getSceneContainer() { return m_sceneContainer; }
    Image*               getSceneBackImg() { return m_sceneBackImg; }
    transMode            getTransMode() {
        if (m_scene3D && m_scene3D->getObjectSelector()) {
            return m_toolState == toolState::objToolBlocked ? transMode::none
                                                            : m_scene3D->getObjectSelector()->getTransMode();
        } else {
            return transMode::none;
        }
    }

    /** for resetZoom option, view will aligned to view to be behind
     * m_aimingNode looking towards m_focusNode, using the basePlane upvector */
    void setFocusNode(SceneNode* node) { m_focusNode = node; }
    void setAimingNode(SceneNode* node) { m_aimingNode = node; }
    /** shortcut for setFixAspect on sceneContainer */
    void setSceneAspect(float aspect) {
        m_sceneAspect = aspect;
        if (m_sceneContainer) m_sceneContainer->setFixAspect(aspect);
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
    glm::vec3 m_bb[2];
    glm::vec3 m_modelCenter{0.f};
    glm::vec3 m_modelCamVec{0.f};
    glm::vec3 m_stdCamVec{0.f, 0.f, -1.f};
    glm::vec3 m_stdUpVec{0.f, 1.f, 0.f};
    glm::vec3 m_upVec{0.f};
    glm::quat m_rot;
    glm::mat4 m_rzViewMat = glm::mat4(1.f);
    glm::vec3 m_newBb[2];

    float m_sceneAspect = 1.777f;
    float m_mouseRotExp = 1.f;
    float m_keyTransStep[3]{0.01f, 0.1f, 1.f};
    float m_keyRotStep[3]{0.1f, 1.f, 10.f};

    toolState m_toolState           = toolState::objToolBlocked;
    bool      m_preventObjToolClick = false;

    std::unordered_map<ToolBarIcon, ImageButton*> m_toolBarIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_objToolIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_objTransIcons;
    std::unordered_map<ToolBarIcon, ImageButton*> m_viewToolIcons;
};

}  // namespace ara
