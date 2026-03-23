//
//
// Created by user on 03.03.2021.
//

#pragma once

#include "Button/Button.h"
#include "Button/ImageButton.h"
#include "UIElements/PropSlider.h"
#include "UIElements/Slider.h"

namespace ara {

class ZoomView : public Div {
public:
    ZoomView();
    ~ZoomView() override = default;

    void init() override;

    template <typename T, typename... Args>
    requires (sizeof...(Args) != 1 || (!std::is_same_v<Args, UINodePars> && ...))
    T& push(Args&& ... args) {
        checkForWorkingArea();
        return m_content->push<T>(args...);
    }

    template<typename T>
    T& push(const UINodePars& arg) {
        checkForWorkingArea();
        return m_content->push<T>(arg);
    }

    [[nodiscard]] UINode* getWorkingArea() const { return m_workingArea; }
    [[nodiscard]] UINode* getContent() const { return m_content; }
    [[nodiscard]] float   getInitZoomPropVal() const { return m_initZoomPropVal; }
    [[nodiscard]] Property<float>& getZoomProp() { return m_zoomProp; }

    void hideContent() const;
    void resetZoom();
    void keepContentWithinBoundaries(bool val);
    void scaleAndCenterContent();

    void initContent(std::function<void(UINode*)> f) { m_initContFunc = std::move(f); }
    void addChangeCb(const std::function<void()>& f) { m_onChangedCb.emplace_back(f); }
    void showSlider(const bool val) { m_showSlider = val; }
    void showResetButton(const bool val) { m_showResetButton = val; }
    void setZoomRange(const float mi, const float ma) { m_zoomProp.setMinMax(mi, ma); }
    void setZoom(const float val) { m_zoomProp = val; }
    void setCenterAndScaleOnReset(const bool val) { m_centerAndScaleOnReset = val; }

private:
    void setZoomPropChangeCb();
    void checkForWorkingArea();

    void keyDown(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseWheel(hidData& data) override;
    void scaleGest(hidData& data) override;

    void dragContent(hidData& data) const;
    void addWorkingArea();
    void addZoomSlider();
    void addResetButton();

    // UI Elements
    UINode*         m_workingArea = nullptr;
    UINode*         m_content     = nullptr;
    PropSlider*     m_zoomSlider  = nullptr;
    Div*            m_bottomMenu  = nullptr;
    Button*         m_resetZoom   = nullptr;
    Property<float> m_zoomProp{100.f, 10.f, 600.f, 1.f};

    glm::mat4  m_identMat = glm::mat4(1.f);
    glm::vec2  m_mouseDownPos{};
    glm::vec3  m_mouseDownViewTrans{};

    int   m_bottMenHeight   = 25;
    float m_initZoomPropVal = 100.f;
    float m_resetButtWidth  = 0.15f;
    float m_centerAndScaleMargin  = 0.95f;
    bool  m_zoomUseWheel    = true;
    bool  m_showSlider      = true;
    bool  m_showResetButton = true;
    bool  m_centerAndScaleOnReset = false;

    std::function<void()>            m_updtCb;
    std::function<void(UINode*)>     m_initContFunc;
    std::list<std::function<void()>> m_onChangedCb;
};

}  // namespace ara
