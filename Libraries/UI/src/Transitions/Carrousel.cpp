//
// Created by sven on 04-04-25.
//

#include "Carrousel.h"

namespace ara {

CarrouselSlide::CarrouselSlide() : Div() {
    setName(getTypeName<CarrouselSlide>());
}

CarrouselSlide::CarrouselSlide(std::string &&styleClass) : Div() {
    setName(getTypeName<CarrouselSlide>());
    addStyleClass(std::move(styleClass));
}

Carrousel::Carrousel() : Div() {
    setName(getTypeName<Carrousel>());
    initFixedChildren();
}

Carrousel::Carrousel(std::string &&styleClass)  {
    setName(getTypeName<Carrousel>());
    addStyleClass(std::move(styleClass));
    initFixedChildren();
}

void Carrousel::initFixedChildren() {
    m_transTime = 0.5;

    m_content = addChild<Div>();
    m_content->setName("CarouselContent");
    m_content->setAlign(ara::align::center, ara::valign::center);

    m_selector = addChild<Div>();
    m_selector->setBorderColor(0.f, 0.f, 0.f, 0.8f);
    m_selector->setBorderWidth(1);
    m_selector->setBorderRadius(6);
    m_selector->setAlign(align::center, valign::center);
}

void Carrousel::mouseDrag(hidData* data) {
    if (m_blend.stopped() && m_getDragDir && std::abs(data->movedPix.x) > 50.f) {
        m_moveToIdx = std::min(m_currentIdx + (data->movedPix.x < 1 ? 1 : -1), static_cast<int32_t>(m_slides.size()) -1);

        if (m_moveToIdx != m_currentIdx) {
            show(m_moveToIdx);
            getSharedRes()->requestRedraw = true;
        }

        m_getDragDir = false;
    }
}

void Carrousel::mouseUp(hidData* data) {
    m_getDragDir = true;
}

CarrouselSlide* Carrousel::add() {
    m_slides.emplace_back(m_content->addChild<CarrouselSlide>());
    postAdd(m_slides.back());
    return m_slides.back();
}

CarrouselSlide* Carrousel::add(std::string&& styleclass) {
    m_slides.emplace_back(m_content->addChild<CarrouselSlide>(std::move(styleclass)));
    postAdd(m_slides.back());
    return m_slides.back();
}

void Carrousel::postAdd(CarrouselSlide* sl) {
    sl->setAlign(ara::align::center, ara::valign::center);
    sl->setBorderRadius(20);
    sl->setBorderWidth(10);
    rotate(0.f);
    if (m_currentIdx == -1) {
        m_currentIdx = 0;
    }
}

void Carrousel::show(int32_t toIdx) {
    m_blend.start(static_cast<float>(m_currentIdx) / static_cast<float>(m_slides.size() -1),
                  static_cast<float>(toIdx) / static_cast<float>(m_slides.size() -1),
                  m_transTime,
                  false,
                  [this](float v) { rotate(v); });

    m_blend.setEndFunc([this]{
        m_currentIdx = m_moveToIdx;
    });

    addGlCb("carrousel_rot", [this]{
        m_blend.update();
        getSharedRes()->requestRedraw = true;
        return m_blend.stopped();
    });
}

void Carrousel::showSelector(bool val) {
    m_selector->setVisibility(val);
    m_selector->excludeFromObjMap(true);
}

void Carrousel::rotate(float pos) {
    int i = 0;

    float relSlideWidth = 1.f / static_cast<float>(m_slides.size());
    m_selector->setWidth(relSlideWidth);

    for (auto& slid : m_slides) {
        slid->setSize(relSlideWidth, 1.f);
        slid->setX((static_cast<float>(i) - (static_cast<float>(m_slides.size() -1) * pos)) * relSlideWidth);
        i++;
    }
}

bool Carrousel::isRotating() {
    return !m_blend.stopped();
}

bool Carrousel::isCurrent(CarrouselSlide* sl) {
    auto r = std::find(m_slides.begin(), m_slides.end(), sl);
    if (r != m_slides.end()) {
        return static_cast<int>(std::distance(m_slides.begin(), r)) == m_currentIdx;
    }
    return false;
}

}