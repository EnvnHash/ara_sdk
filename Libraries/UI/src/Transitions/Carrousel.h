//
// Created by sven on 04-04-25.
//

#pragma once

#include <AnimVal.h>
#include <Transitions/UIStack.h>
#include <UIElements/Div.h>

namespace ara {

class CarrouselSlide : public Div {
public:
    CarrouselSlide();
    void setOnShowFunc(const std::function<void()>& f) { m_onShow = f; }

private:
    std::function<void()>   m_onShow;
};

enum class CarrouselMode : int { fitAllOnScreen = 0, fitOneSlideOnScreen };

class Carrousel : public UIStack, public Div {
public:
    Carrousel();

    void initFixedChildren();
    void mouseUp(hidData& data) override;
    void mouseDrag(hidData& data) override;

    CarrouselSlide* add();
    CarrouselSlide* add(const UINodePars& pars);
    void postAdd(CarrouselSlide* sl);

    void rotate(float pos);
    void rotateAllOnScreen(float pos);
    void rotateFitOneOnScreen(float pos);

    bool isRotating();
    bool isCurrent(CarrouselSlide* sl);
    glm::ivec2 getAbsSlideSize();

    void show(int32_t);
    void show(const std::string& name) override {};
    void showSelector(bool val) const;
    void setMode(CarrouselMode m) { m_carMode = m; }

private:
    Div*                            m_content = nullptr;
    Div*                            m_selector = nullptr;
    AnimVal<float>                  m_blend;
    std::vector<CarrouselSlide*>    m_slides;
    bool                            m_getDragDir = true;
    int32_t                         m_currentIdx = -1;
    int32_t                         m_moveToIdx = -1;
    float                           m_dragStartPos = 0.f;
    float                           m_dragSlidePos = 0.f;
    float                           m_mouseDragThresh = 60.f;
    CarrouselMode                   m_carMode = CarrouselMode::fitAllOnScreen;
    glm::ivec2                      m_inset {100, 10};
};
}
