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
    explicit CarrouselSlide(const std::string& styleClass);

    void setOnShowFunc(const std::function<void()>& f) { m_onShow = f; }

private:
    std::function<void()>   m_onShow;
};

class Carrousel : public UIStack, public Div {
public:
    Carrousel();
    explicit Carrousel(const std::string& styleClass);

    void initFixedChildren();
    void mouseUp(hidData* data) override;
    void mouseDrag(hidData* data) override;

    CarrouselSlide* add();
    CarrouselSlide* add(const std::string& styleclass);
    void postAdd(CarrouselSlide* sl);

    void rotate(float pos) const;
    bool isRotating();
    bool isCurrent(CarrouselSlide* sl);
    void show(int32_t);
    void show(const std::string& name) override {};
    void showSelector(bool val) const;

private:
    Div*                            m_content = nullptr;
    Div*                            m_selector = nullptr;
    AnimVal<float>                  m_blend;
    std::vector<CarrouselSlide*>    m_slides;
    bool                            m_getDragDir = true;
    int32_t                         m_currentIdx = -1;
    int32_t                         m_moveToIdx = -1;
};
}
