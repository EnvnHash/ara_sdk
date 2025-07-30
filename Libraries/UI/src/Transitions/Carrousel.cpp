//
// Created by sven on 04-04-25.
//

#include <Transitions/Carrousel.h>
#include <UIElements/Button/ImageButton.h>
#include <UISharedRes.h>
#include <UIWindow.h>

using namespace glm;

namespace ara {

CarrouselSlide::CarrouselSlide() {
    setName(getTypeName<CarrouselSlide>());
    setScissorChildren(true);
}

Carrousel::Carrousel() {
    setName(getTypeName<Carrousel>());
    initFixedChildren();
    setScissorChildren(true);
}

void Carrousel::initFixedChildren() {
    m_transTime = 0.3;

    m_content = addChild<Div>({
        .name = "CarouselContent",
        .align = align::center,
        .valign = valign::center,
    });
    m_content->setScissorChildren(true);

    m_selector = addChild<Div>({
        .align = align::center,
        .valign = valign::center,
        .borderWidth = 1,
        .borderColor = vec4{0.f, 0.f, 0.f, 0.8f},
    });

    m_arrows[0] = addChild<ImageButton>({
        .name = "CarrouselArrowLeft",
        .align = align::left,
        .valign = valign::center
    });
    m_arrows[0]->setImg("Icons/icon-arrow-left.png");

    m_arrows[1] = addChild<ImageButton>({
        .name = "CarrouselArrowRight",
        .align = align::right,
        .valign = valign::center
    });
    m_arrows[1]->setImg("Icons/icon-arrow-right.png");

    for (int i=0; i<m_arrows.size(); ++i) {
        m_arrows[i]->setSize(m_arrowSize.x, 1.f);
        m_arrows[i]->setClickedCb([this, i]{
            show(std::max(0, m_currentIdx + (i == 0 ? -1 : 1)), true);
        });
    }

    showArrows(false);
}

void Carrousel::mouseDrag(hidData& data) {
    if (data.dragStart) {
        m_dragStartPos = m_dragSlidePos;
    }

    float offs = -data.movedPix.x / getMaxSlideWay();
    m_dragSlidePos = std::min(1.f, std::max(0.f, m_dragStartPos + offs));

    if (m_carMode == CarrouselMode::fitAllOnScreen) {
        rotateCentered(m_dragSlidePos);
    } else if (m_carMode == CarrouselMode::leftAlign) {
        rotateNoFit(m_dragSlidePos);
    } else if (m_carMode == CarrouselMode::fitOneSlideOnScreen) {
        auto absSlideWidth = getAbsSlideWidth();
        m_dragSlidePos = m_dragStartPos - data.movedPix.x / static_cast<float>(absSlideWidth * (m_slides.size() -1));
        rotateCentered(m_dragSlidePos);
    }

    m_sharedRes->requestRedraw = true;
}

void Carrousel::mouseUp(hidData& data) {
    m_getDragDir = true;
    if (std::abs(data.movedPix.x) > m_mouseDragThresh){
        slideToNextIdx(data.movedPix.x);
    }
}

void Carrousel::postAdd(CarrouselSlide* sl) {
    // in case slides are formatted with styles, they need to be applied here, before layouting children
    uint32_t objId = 0;
    UINode::drawAsRoot(objId);

    rotate(0.f);
    if (m_currentIdx == -1) {
        m_currentIdx = 0;
    }
}

void Carrousel::show(int32_t toIdx, bool animate) {
    m_moveToIdx = toIdx;
    if (animate && m_blend.stopped()) {
        m_blend.start(m_dragSlidePos, getPosFromSlideIdx(toIdx), m_transTime, false, [this](float v) { rotate(v); });
        m_blend.setEndFunc([this]{
            m_currentIdx = m_moveToIdx;
            m_dragSlidePos = getDragSlidePos();
        });

        addGlCb("carrousel_rot", [this]{
            m_blend.update();
            getSharedRes()->reqRedraw();
            return m_blend.stopped();
        });
    } else {
        m_currentIdx = m_moveToIdx;
        m_dragSlidePos = getDragSlidePos();
        rotate(m_dragSlidePos);
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

void Carrousel::showSelector(bool val) const {
    m_selector->setVisibility(val);
    m_selector->excludeFromObjMap(true);
}

void Carrousel::showArrows(bool val)  {
    for (auto &it : m_arrows) {
        it->setVisibility(val);
    }
    if (val) {
        m_content->setWidth(-(m_arrowSize.x * 2 + static_cast<int32_t>(m_padding.x + m_padding.z)));
    }
}

void Carrousel::rotate(float pos) {
    std::unordered_map<CarrouselMode, std::function<void(float)>> moveFuncMap{
        { CarrouselMode::fitAllOnScreen, [&](float p){ rotateCentered(p); } },
        { CarrouselMode::fitOneSlideOnScreen, [&](float p){ rotateCentered(p); } },
        { CarrouselMode::leftAlign, [&](float p){ rotateNoFit(p); } }
    };
    auto limitPos = std::max(0.f, std::min(1.f, pos));
    moveFuncMap[m_carMode](limitPos);
    m_dragStartPos = limitPos;
}

void Carrousel::rotateCentered(float pos) {
    int i = 0;
    auto slideWidth = getAbsSlideWidth();
    auto slidePlusSpace = slideWidth + m_spacing;
    m_zeroPos = getZeroPos();
    m_selector->setWidth(slideWidth);

    for (const auto& slid : m_slides) {
        if (static_cast<int32_t>(slid->getSize().x) != slideWidth) {
            slid->setSize(slideWidth, 1.f);
        }
        auto slidOffs = i++ * slidePlusSpace;
        auto rotOffs = static_cast<int32_t>(pos * static_cast<float>(m_slides.size() -1) * static_cast<float>(slidePlusSpace) +0.5f);
        slid->setX(slidOffs + m_zeroPos - rotOffs);
    }
}

void Carrousel::rotateNoFit(float pos) {
    auto xPos = 0;
    for (const auto slid : m_slides) {
        slid->setAlignX(align::left);
        slid->setX(xPos - static_cast<int32_t>(pos * getMaxSlideWay()));
        xPos += static_cast<int32_t>(slid->getNodeWidth());
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

float Carrousel::getSlideWidthSum(int32_t endIdx) {
    return static_cast<float>(std::accumulate(m_slides.begin(), std::next(m_slides.begin(), endIdx), 0,
                              [&](const auto& a, const auto& b) {
                                    return a + static_cast<int32_t>(b->getNodeWidth());
                              }));
}

float Carrousel::getMaxSlideWay() {
    if (m_carMode == CarrouselMode::leftAlign) {
        return std::max(0.f, getSlideWidthSum(static_cast<int32_t>(m_slides.size())) - m_content->getSize().x);
    } else if (m_carMode == CarrouselMode::fitAllOnScreen) {
        return getSlideWidthSum(static_cast<int32_t>(m_slides.size()));
    }
    return 0.f;
}

float Carrousel::getDragSlidePos() {
    if (m_carMode == CarrouselMode::leftAlign) {
        return getSlideWidthSum(m_currentIdx) / getMaxSlideWay();
    } else {
        return static_cast<float>(m_currentIdx) / static_cast<float>(m_slides.size() -1);
    }
}

void Carrousel::slideToNextIdx(float movedPixX) {
    int32_t nextIdx = 0;
    if (m_carMode != CarrouselMode::leftAlign) {
        nextIdx = std::clamp(static_cast<int>(m_dragSlidePos * static_cast<float>(m_slides.size() - 1))
                                              + (movedPixX > 0.f ? 0 : 1),
                             0, static_cast<int>(m_slides.size() - 1));
    } else {
        auto currentDrawWayInPix = static_cast<int>(m_dragSlidePos * getMaxSlideWay());
        int32_t accuSlideSum = 0;
        auto it = m_slides.begin();
        accuSlideSum += static_cast<int32_t>((*it)->getSize().x);
        while (accuSlideSum < currentDrawWayInPix && ++it < m_slides.end()) {
            accuSlideSum += static_cast<int32_t>((*it)->getSize().x) + m_padding.x;
        }

        nextIdx = static_cast<int32_t>(std::distance(m_slides.begin(), it));
        nextIdx = std::clamp(nextIdx + (movedPixX > 0.f ? 0 : 1), 0, static_cast<int>(m_slides.size() - 1));
    }

    show(nextIdx);
}

float Carrousel::getPosFromSlideIdx(int32_t idx) {
    if (m_carMode == CarrouselMode::leftAlign) {
        return getSlideWidthSum(idx) / getMaxSlideWay();
    } else {
        return static_cast<float>(idx) / static_cast<float>(m_slides.size() -1);
    }
}

int32_t Carrousel::getAbsSlideWidth() {
    if (m_carMode == CarrouselMode::fitOneSlideOnScreen){
        return static_cast<int32_t>(getContentSize().x) - m_inset;
    } else if (m_carMode == CarrouselMode::fitAllOnScreen) {
        return static_cast<int32_t>(static_cast<float>(getContentSize().x - m_spacing * static_cast<float>(m_slides.size() -1)) / static_cast<float>(m_slides.size()));
    }
    return {};
}

int32_t Carrousel::getZeroPos() {
    if (m_carMode == CarrouselMode::fitAllOnScreen) {
        return static_cast<int32_t>(m_slides.size() / 2) * (getAbsSlideWidth() + m_spacing);
    } else if (m_carMode == CarrouselMode::fitOneSlideOnScreen) {
        return m_inset/2;
    }
    return 0;
}

}