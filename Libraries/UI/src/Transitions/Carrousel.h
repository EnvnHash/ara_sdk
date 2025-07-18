//
// Created by sven on 04-04-25.
//

#pragma once

#include <AnimVal.h>
#include <Transitions/UIStack.h>
#include <UIElements/Div.h>

namespace ara {

class ImageButton;

class CarrouselSlide : public Div {
public:
    CarrouselSlide();
    void setOnShowFunc(const std::function<void()>& f) { m_onShow = f; }

private:
    std::function<void()>   m_onShow;
};

enum class CarrouselMode : int { fitAllOnScreen = 0, fitOneSlideOnScreen, leftAlign };

class Carrousel : public UIStack, public Div {
public:
    Carrousel();

    void initFixedChildren();

    CarrouselSlide* add();
    CarrouselSlide* add(const UINodePars& pars);

    bool isRotating();
    bool isCurrent(CarrouselSlide* sl);
    Div* getSelector() const { return m_selector; }
    ImageButton* getLeftArrow() const { return m_arrows[0]; }
    ImageButton* getRightArrow() const { return m_arrows[1]; }

    void show(int32_t, bool animate = true);
    void show(const std::string& name) override {};
    void showSelector(bool val) const;
    void showArrows(bool val);
    void setMode(CarrouselMode m) { m_carMode = m; }
    void setSpacing(int32_t val) { m_spacing = val; }
    void setInset(int32_t val) { m_inset = val; }

private:
    void rotate(float pos);
    void rotateCentered(float pos);
    void rotateNoFit(float pos);
    float getSlideWidthSum(int32_t endIdx);
    float getMaxSlideWay();
    float getDragSlidePos();
    void slideToNextIdx(float movedPixX);
    float getPosFromSlideIdx(int32_t idx);
    int32_t getAbsSlideWidth();
    int32_t getZeroPos();

    void postAdd(CarrouselSlide* sl);

    void mouseUp(hidData& data) override;
    void mouseDrag(hidData& data) override;

    std::array<ImageButton*, 2>     m_arrows{};
    Div*                            m_content = nullptr;
    Div*                            m_selector = nullptr;
    AnimVal<float>                  m_blend;
    std::vector<CarrouselSlide*>    m_slides;
    bool                            m_getDragDir = true;
    int32_t                         m_currentIdx = -1;
    int32_t                         m_moveToIdx = -1;
    int32_t                         m_spacing = 0;
    float                           m_dragStartPos = 0.f;
    float                           m_dragSlidePos = 0.f;
    float                           m_mouseDragThresh = 10.f;
    int32_t                         m_zeroPos;
    CarrouselMode                   m_carMode = CarrouselMode::fitAllOnScreen;
    int32_t                         m_inset = 100;
    glm::ivec2                      m_arrowSize{ 19, 32 };
};
}
