//
// Created by sven on 04-04-25.
//

#include <Transitions/Carrousel.h>
#include <UISharedRes.h>

using namespace glm;

namespace ara {

CarrouselSlide::CarrouselSlide() : Div() {
    setName(getTypeName<CarrouselSlide>());
}

Carrousel::Carrousel() : Div() {
    setName(getTypeName<Carrousel>());
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

void Carrousel::mouseDrag(hidData& data) {
    if (data.dragStart) {
        m_dragStartPos = static_cast<float>(m_currentIdx) / static_cast<float>(m_slides.size() -1);
    }

    if (m_carMode == CarrouselMode::fitAllOnScreen) {
        if (m_blend.stopped() && m_getDragDir && std::abs(data.movedPix.x) > m_mouseDragThresh) {
            m_moveToIdx = std::min(m_currentIdx + (data.movedPix.x < 1 ? 1 : -1), static_cast<int32_t>(m_slides.size()) -1);
            if (m_moveToIdx != m_currentIdx) {
                show(m_moveToIdx);
                getSharedRes()->reqRedraw();
            }

            m_getDragDir = false;
        }
    } else if (m_carMode == CarrouselMode::fitOneSlideOnScreen) {
        auto absSlideSize = getAbsSlideSize();
        m_dragSlidePos = m_dragStartPos - data.movedPix.x / static_cast<float>(absSlideSize.x * (m_slides.size() -1));
        rotateFitOneOnScreen(m_dragSlidePos);
        m_sharedRes->requestRedraw = true;
    }
}

void Carrousel::mouseUp(hidData& data) {
    m_getDragDir = true;
    if (std::abs(data.movedPix.x) > m_mouseDragThresh && m_carMode == CarrouselMode::fitOneSlideOnScreen) {
        auto nextIdx = std::min(static_cast<int>(m_slides.size() - 1),
                                std::max(0, static_cast<int>(m_dragSlidePos * static_cast<float>(m_slides.size() -1))
                                        + (data.movedPix.x > 0.f ? 0 : 1) ));
        show(nextIdx);
    }
}

CarrouselSlide* Carrousel::add() {
    m_slides.emplace_back(m_content->addChild<CarrouselSlide>());
    postAdd(m_slides.back());
    return m_slides.back();
}

CarrouselSlide* Carrousel::add(const UINodePars& pars) {
    m_slides.emplace_back(m_content->addChild<CarrouselSlide>(pars));
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
    m_moveToIdx = toIdx;

    m_blend.start(m_dragSlidePos,
                  static_cast<float>(toIdx) / static_cast<float>(m_slides.size() -1),
                  m_transTime,
                  false,
                  [this](float v) { rotate(v); });

    m_blend.setEndFunc([this]{
        m_currentIdx = m_moveToIdx;
        m_dragSlidePos = static_cast<float>(m_currentIdx) / static_cast<float>(m_slides.size() -1);
    });

    addGlCb("carrousel_rot", [this]{
        m_blend.update();
        getSharedRes()->reqRedraw();
        return m_blend.stopped();
    });
}

void Carrousel::showSelector(bool val) const {
    m_selector->setVisibility(val);
    m_selector->excludeFromObjMap(true);
}

void Carrousel::rotate(float pos) {
    std::unordered_map<CarrouselMode, std::function<void(float)>> moveFuncMap{
        { CarrouselMode::fitAllOnScreen, [&](float p){ rotateAllOnScreen(p); } },
        { CarrouselMode::fitOneSlideOnScreen, [&](float p){ rotateFitOneOnScreen(p); } }
    };
    moveFuncMap[m_carMode](pos);
    m_dragStartPos = pos;
}

void Carrousel::rotateAllOnScreen(float pos) {
    int i = 0;
    float relSlideWidth = 1.f / static_cast<float>(m_slides.size());
    m_selector->setWidth(relSlideWidth);

    for (const auto& slid : m_slides) {
        slid->setSize(relSlideWidth, 1.f);
        slid->setX((static_cast<float>(i) - (static_cast<float>(m_slides.size() -1) * pos)) * relSlideWidth);
        i++;
    }
}

void Carrousel::rotateFitOneOnScreen(float pos) {
    int i = 0;
    auto absSlideSize = getAbsSlideSize();
    m_selector->setSize(absSlideSize.x, absSlideSize.y);
    auto relSlideSize = vec2(absSlideSize) / m_contentSize;

    for (const auto& slid : m_slides) {
        slid->setSize(relSlideSize.x, relSlideSize.y);
        float slidOffs = static_cast<float>(i) * relSlideSize.x;
        slid->setX(slidOffs - pos * relSlideSize.x * static_cast<float>(m_slides.size() -1));
        i++;
    }
}

bool Carrousel::isRotating() {
    return !m_blend.stopped();
}

bool Carrousel::isCurrent(CarrouselSlide* sl) {
    auto r = std::ranges::find(m_slides, sl);
    if (r != m_slides.end()) {
        return static_cast<int>(std::distance(m_slides.begin(), r)) == m_currentIdx;
    }
    return false;
}

glm::ivec2 Carrousel::getAbsSlideSize() {
    auto cs = getContentSize();
    return ivec2(cs) - m_inset;
}

}