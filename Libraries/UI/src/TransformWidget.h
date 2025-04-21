//
// Created by user on 04.11.2021.
//

#pragma once

#include <SceneNodes/SceneNode.h>
#include <Button/Button.h>
#include <Button/ImageButton.h>
#include <Div.h>
#include <Label.h>
#include <Utils/TrackBallCam.h>

namespace ara {

class TransformWidget : public Div {
public:
    enum class transAlign : int { objectRelative = 0, worldRelative };

    TransformWidget();
    explicit TransformWidget(std::string&& styleClass);
    virtual ~TransformWidget();

    void init() override;
    void translate(int j);
    void rotate(int j);
    void setTransMode(transMode m, bool updtColors = false);
    void setPlane(twPlane p);
    int  getColorIdx(int buttIdx);
    void setPlaneSwitcherState(transMode m, twPlane p);
    void setDisabled(bool val, bool forceStyleUpdt = false) override;
    void setVisibility(bool val);
    void highLight(bool val);
    void keyDown(hidData* data) override;
    void keyUp(hidData* data) override;
    void checkCamFlags();
    void checkPlaneSwitchIdx();

    void       setTransAlign(transAlign t) { m_tRel = t; }
    void       setModelNode(SceneNode* node) { m_modelNode = node; }
    SceneNode* getModelNode() { return m_modelNode; }
    twPlane    getPlaneType() { return m_transPlane; }
    transMode  getTransMode() { return m_transMode; }
    void       setEnableCb(std::function<void()> f) { m_enableCb = std::move(f); }
    void       setCoarseFine(cfState st) { m_cfState = st; }
    void       setKeyRotStep(float fine, float normal, float coarse) {
        m_rotAmt[0] = fine;
        m_rotAmt[1] = normal;
        m_rotAmt[2] = coarse;
    }
    void setKeyTransStep(float fine, float normal, float coarse) {
        m_transAmt[0] = fine;
        m_transAmt[1] = normal;
        m_transAmt[2] = coarse;
    }
    void setPlaneSwitchCb(std::function<void(transMode, twPlane)> f) { m_planeSwCb = std::move(f); }
    void setBlockStateChange(bool val) { m_blockStateChange = val; }

private:
    transMode                    m_transMode         = transMode::translate;
    SceneNode*                   m_modelNode         = nullptr;
    std::pair<int, ImageButton*> m_resetButHighlight = std::make_pair(-1, nullptr);
    twPlane                      m_transPlane        = twPlane::xy;
    ImageButton*                 m_centInd           = nullptr;
    Div*                         m_buttCont          = nullptr;
    Div*                         m_planeSwitchCont   = nullptr;
    cfState                      m_cfState           = cfState::normal;
    transAlign                   m_tRel              = transAlign::objectRelative;

    bool m_mousePressed     = false;
    bool m_firstDown        = false;
    bool m_disHighlighted   = false;
    bool m_mouseInBounds    = false;
    bool m_blockStateChange = false;

    glm::mat4 m_inRm;
    glm::mat4 m_modRotMat;

    std::array<float, 3> m_rotAmt   = {0.1f, 1.f, 10.f};
    std::array<float, 3> m_transAmt = {0.01f, 0.1f, 1.f};
    std::array<std::string, 4> m_arrowName  = {"left", "right", "top", "bottom"};
    std::array<glm::vec4, 3>   m_colors     = {glm::vec4{1.f, 0.3f, 0.3f, 1.f}, glm::vec4{0.3f, 1.f, 0.3f, 1.f},
                                               glm::vec4{0.3f, 0.3f, 1.f, 1.f}};
    std::array<glm::vec4, 3>   m_colorsHigh = {glm::vec4{1.f, 0.6f, 0.6f, 1.f}, glm::vec4{0.6f, 1.f, 0.6f, 1.f},
                                               glm::vec4{0.6f, 0.6f, 1.f, 1.f}};
    std::array<glm::vec4, 3>   m_colorsDis  = {glm::vec4{0.6f, 0.5f, 0.5f, 1.f}, glm::vec4{0.5f, 0.6f, 0.5f, 1.f},
                                               glm::vec4{0.5f, 0.5f, 0.6f, 1.f}};
    std::array<std::array<ImageButton*, 4>, 2> m_arrowButts    = {0};
    std::array<std::array<Label*, 4>, 2>       m_arrowLabels   = {0};
    std::array<std::array<ImageButton*, 5>, 2> m_planeSwitcher = {0};
    std::array<Label*, 2>                      m_psLabels      = {0};

    std::array<std::pair<transMode, twPlane>, 6> m_psSwitchMap = {
        std::pair{transMode::translate, twPlane::xy}, std::pair{transMode::translate, twPlane::xz},
        std::pair{transMode::translate, twPlane::yz}, std::pair{transMode::rotate, twPlane::yz},
        std::pair{transMode::rotate, twPlane::xz},    std::pair{transMode::rotate, twPlane::xy}};
    int m_psSwitchMapIdx = 0;

    std::chrono::time_point<std::chrono::system_clock> m_downTp;

    std::function<void()>                   m_enableCb;
    std::function<void(transMode, twPlane)> m_planeSwCb;
};

}  // namespace ara
