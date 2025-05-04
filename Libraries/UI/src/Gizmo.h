#pragma once

#include <Image.h>
#include <Utils/TrackBallCam.h>
#include <GLUtils/sceneData.h>
#include <SceneNodes/SceneNode.h>

namespace ara {

class AssetColor;
class GizmoAxisLabel;
class DrawManagerGizmo;
class SNGizmo;

class Gizmo : public Image {
public:
    Gizmo();
    explicit Gizmo(const std::string& styleClass);
    ~Gizmo() override { delete m_crossVao; };

    void init() override;

    Shaders *initLineShdr();
    Shaders *initLineShdr2();
    Shaders *initTexDepthShdr();
    Shaders *initObjMapTexShdr();

    void setModelCam(TrackBallCam *cam);
    void cbUpdt(TrackBallCam *cam, TbModData &data, bool mapByMouseRot);

    bool draw(uint32_t *objId) override;
    bool drawIndirect(uint32_t *objId) override;
    bool drawToFbo(uint32_t *objId);
    bool drawToFbo2(uint32_t *objId);

    glm::vec3 getAxisEnd(glm::mat4 &pvm, float yVal);

    void mouseDrag(hidData *data) override;
    void drag(hidData *data);
    void mouseUp(hidData *data) override;
    void mouseDownRight(hidData *data) override;
    void mouseUpRight(hidData *data) override;
    void mouseDown(hidData *data) override;
    void mouseMove(hidData *data) override;
    void excludeLabelsFromStyles(bool val);

    void keyDown(hidData *data) override {
        m_cam.keyDown(data->key, data->shiftPressed, data->altPressed, data->ctrlPressed);
    }

    void keyUp(hidData *data) override {
        m_cam.keyUp(data->key, data->shiftPressed, data->altPressed, data->ctrlPressed);
    }

    std::vector<GizmoAxisLabel *> *getAxisLabels() { return &m_axisLabels; }
    TrackBallCam                  *getCamera() { return &m_cam; }
    TrackBallCam                  *getModelCam() { return m_modelCam; }
    glm::vec2    getGizmoRelMousePos(glm::vec2 &mousePos) { return (mousePos - getPos()) / getSize(); }
    UISharedRes *getAuxSharedRes() { return &m_auxSharedRes; }

    hidData  *m_lastMouseHoverData = nullptr;
    glm::vec2 m_rotScale           = glm::vec2(1.f);
    bool      m_updtDrawData       = false;
    bool      m_camChanged         = true;

protected:
    TrackBallCam                        m_cam;
    TrackBallCam                       *m_modelCam = nullptr;
    Texture                            *tex        = nullptr;
    std::unique_ptr<DrawManagerGizmo>   m_drawMan;
    std::array<std::unique_ptr<FBO>, 2> m_fbo;
    Shaders                            *m_stdTex = nullptr;
    // Shaders*                               m_objMapTexShdr=nullptr;
    Shaders    *m_DrawShader = nullptr;
    SceneNode   m_rootNode;
    SNGizmo    *m_gizmoSN  = nullptr;
    VAO        *m_crossVao = nullptr;
    FBO        *m_sceneFbo = nullptr;
    UISharedRes m_auxSharedRes;
    AssetColor   *m_bkColor = nullptr;
    sceneData   m_sd;

    UINode                        m_auxUIRoot;
    GizmoAxisLabel               *m_axisLabelX    = nullptr;
    GizmoAxisLabel               *m_axisLabelY    = nullptr;
    GizmoAxisLabel               *m_axisLabelZ    = nullptr;
    GizmoAxisLabel               *m_axisLabelXNeg = nullptr;
    GizmoAxisLabel               *m_axisLabelYNeg = nullptr;
    GizmoAxisLabel               *m_axisLabelZNeg = nullptr;
    std::vector<GizmoAxisLabel *> m_axisLabels;
    std::list<GizmoAxisLabel *>   m_zSortedAxisLabels;
    std::list<GizmoAxisLabel *>   m_newZSortedAxisLabels;

    std::vector<glm::vec4> m_crossPos;
    std::vector<glm::vec4> m_crossColor;
    std::vector<glm::vec4> m_crossAux0;

    glm::mat4  m_pvm          = glm::mat4(1.f);
    glm::mat4  m_auxOrtho     = glm::mat4(1.f);
    glm::vec4  m_auxViewport  = glm::vec4{0.f};
    glm::vec2  m_dragStartPos = glm::vec2{0.f};
    glm::vec3  m_axlPos{0.f};
    glm::ivec2 m_axisLabelSize = glm::ivec2(20, 20);

    bool m_resetExcludeFromStyles = false;
    bool m_rightPressed           = false;  // additional check for avoind errors with hid
                                            // blocking during camera animation
    bool m_leftPressed = false;             // additional check for avoind errors with hid
                                            // blocking during camera animation

    int axIndx = 0;

    uint32_t m_tempObjId   = 0;
    uint32_t m_dfTempObjId = 0;
    uint32_t m_dfObjId     = 0;

    StopWatch m_watch;
};

}  // namespace ara
