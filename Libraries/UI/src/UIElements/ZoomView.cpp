//
// Created by user on 03.03.2021.
//

#include <DataModel/PropertyItemUi.h>
#include "ZoomView.h"
#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace fs = std::filesystem;

namespace ara {

ZoomView::ZoomView() : Div() {
    setName(getTypeName<ZoomView>());
}

void ZoomView::init() {
    Div::init();

    setScissorChildren(true);
    addWorkingArea();

    if (m_showSlider || m_showResetButton) {
        m_bottomMenu = UINode::addChild<Div>();
        m_bottomMenu->setHeight(m_bottMenHeight);
        m_bottomMenu->setAlignY(valign::bottom);
    }

    setZoomPropChangeCb();
    m_initZoomPropVal = m_zoomProp();

    if (m_showSlider) {
        addZoomSlider();
    }
    if (m_showResetButton) {
        addResetButton();
    }

    if (m_initContFunc) {
        m_initContFunc(m_workingArea);
    }
}

void ZoomView::setZoomPropChangeCb() {
    onChanged<float>(m_zoomProp, [this](const std::any& val) {
        if (m_workingArea && m_zoomUseWheel) {
            m_workingArea->setZoomWithCenter(std::any_cast<float>(val) * 0.01f, getWindow()->getActMousePos());
        } else if (m_workingArea) {
            m_workingArea->setZoomNormMat(std::any_cast<float>(val) * 0.01f);
        }

        for (const auto& it : m_onChangedCb) {
            it();
        }

        getSharedRes()->reqRedraw();
    });
    m_zoomProp.callOnPreChange(m_zoomProp()); // make sure any value set before init() is applied
}

void ZoomView::addWorkingArea() {
    if (m_workingArea) {
        return;
    }

    m_workingArea = UINode::addChild<Div>({ .name = "ZoomViewWorkingArea"});
    if (m_showSlider || m_showResetButton) {
        m_workingArea->setHeight(-m_bottMenHeight);
    }
    m_workingArea->setZoomNormMat(m_zoomProp() * 0.01f);
    m_workingArea->setContentTransCentered(true);
    m_workingArea->setCanReceiveDrag(true);
    m_workingArea->addMouseDragCb([this](hidData& data) {
        dragContent(data);
    }, true);

    m_content = m_workingArea->addChild<Div>({ .name = "ZoomViewContent" });
}

void ZoomView::dragContent(hidData& data) {
    // translate the working area view
    if (data.mousePressed && !data.altPressed && !data.shiftPressed) {
        auto moved    = vec2(data.mousePos) - m_mouseDownPos;
        auto resTrans = static_cast<vec2>(m_mouseDownViewTrans) + moved / static_cast<vec2>(m_workingArea->getContentTransScale());
        m_workingArea->setContentTransTransl(resTrans.x, resTrans.y);
        setDrawFlag();
    }
}

void ZoomView::addZoomSlider() {
    m_zoomSlider = m_bottomMenu->addChild<PropSlider>({
        .bgColor = vec4{0.2f, 0.2f, 0.2f, 1.f},
        .name = "ZoomViewZoomSlider"
    });
    m_zoomSlider->setSize(1.f - (m_showResetButton ? m_resetButtWidth + 0.01f : 0.f), m_bottMenHeight);
    m_zoomSlider->setProp(m_zoomProp);
    m_zoomSlider->setLabel("Zoom");
    m_zoomSlider->setPrecision(1);
    m_zoomSlider->setValueChgCb([this] { m_zoomUseWheel = false; });
    m_zoomSlider->setOnLostFocusCb([this] { getWindow()->setInputFocusNode(this, false); });
}

void ZoomView::addResetButton() {
    m_resetZoom = m_bottomMenu->addChild<Button>({
        .size = vec2{ m_resetButtWidth, 1.f },
        .bgColor = vec4{0.2f, 0.2f, 0.2f, 1.f},
        .align = align::right,
        .borderWidth = 1,
        .borderColor = vec4{0.8f, 0.8f, 0.8f, 1.f},
    });
    m_resetZoom->setText("Reset");

    m_resetZoom->setClickedCb([this] {
        m_workingArea->setContentTransTransl(0.f, 0.f);
        m_workingArea->setContentTransScale(m_initZoomPropVal * 0.01f, m_initZoomPropVal * 0.01f);
        m_zoomProp = m_initZoomPropVal;
        setDrawFlag();
    });
}

void ZoomView::checkForWorkingArea() {
    if (!m_workingArea) {
        addWorkingArea();
    }
}

void ZoomView::hideContent() const {
    if (m_content) {
        m_content->setVisibility(false);
    }
}

void ZoomView::keyDown(hidData& data) {
}

void ZoomView::mouseDown(hidData& data) {
    // working area movement
    if (m_workingArea) {
        m_mouseDownViewTrans = m_workingArea->getContentTransTransl();
    }
    m_mouseDownPos = data.mousePos;
}

void ZoomView::mouseWheel(hidData& data) {
#ifndef __ANDROID__
    if (getWindow()->isMousePressed()) {
        return;
    }
#endif
    if (m_zoomProp) {
        float newVal   = m_zoomProp() * 0.01f * (1.f + data.degrees * (data.ctrlPressed ? 0.01f : 0.1f));
        m_zoomUseWheel = true;
        m_zoomProp.setClamp(newVal * 100.f);
    }

    data.consumed = true;
    getSharedRes()->reqRedraw();
}

void ZoomView::scaleGest(hidData& data) {
    if (data.scaleFact != 0.f && m_zoomProp) {
        float newVal   = m_zoomProp() * data.scaleFact;
        m_zoomProp.setClamp(newVal);
        data.consumed = true;
        getSharedRes()->reqRedraw();
    }
}

void ZoomView::keepContentWithinBoundaries(bool val) {
    checkForWorkingArea();
    m_workingArea->limitContentTrans(val);
}

}  // namespace ara