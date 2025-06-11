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

    template <class T>
    T* setContent() {
        if (!ui_content) {
            remove_child(ui_content);
        }

        if (ui_workingArea) {
            ui_content = ui_workingArea->addChild<T>();
        } else {
            ui_content = addChild<T>();
        }

        return static_cast<T *>(ui_content);
    }

    void setDisplaySize(int w, int h);
    void keyDown(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;
    void mouseWheel(hidData& data) override;

    [[nodiscard]] UINode* getWorkingArea() const { return ui_workingArea; }
    [[nodiscard]] UINode* getContent() const { return ui_content; }
    [[nodiscard]] float   getInitZoomPropVal() const { return m_initZoomPropVal; }

    void    hideContent() const {
        if (ui_content) {
            ui_content->setVisibility(false);
        }
    }
    void initContent(std::function<void(UINode*)> f) { m_initContFunc = std::move(f); }
    void addChangeCb(const std::function<void()>& f) { m_onChangedCb.emplace_back(f); }

private:
    // UI Elements
    UINode*         ui_workingArea = nullptr;
    UINode*         ui_content     = nullptr;
    PropSlider*     ui_zoomSlider  = nullptr;
    Div*            ui_bottomMenu  = nullptr;
    ImageButton*    ui_resetZoom   = nullptr;
    Property<float> m_zoomProp{100.f, 10.f, 600.f, 1.f};

    glm::mat4  m_identMat = glm::mat4(1.f);
    glm::ivec2 m_displaySize{0};
    glm::vec2  s_mouseDownPos{};
    glm::vec3  m_mouseDownViewTrans{};

    int   m_bottMenHeight   = 25;
    float m_initZoomPropVal = 100.f;
    bool  m_zoomUseWheel    = true;

    std::function<void()>            m_updtCb;
    std::function<void(UINode*)>     m_initContFunc;
    std::list<std::function<void()>> m_onChangedCb;
};

}  // namespace ara
