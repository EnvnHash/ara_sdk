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

ZoomView::ZoomView() : Div() { setName(getTypeName<ZoomView>()); }

void ZoomView::init() {
    Div::init();

    setScissorChildren(true);

    ui_workingArea = addChild<Div>();
    ui_workingArea->setHeight(-m_bottMenHeight);
    ui_workingArea->setZoomNormMat(m_zoomProp() * 0.01f);
    ui_workingArea->setContentTransCentered(true);
    ui_workingArea->setCanReceiveDrag(true);
    ui_workingArea->addMouseDragCb(
        [this](hidData& data) {
            // translate the working area view
            if (data.mousePressed && !data.altPressed && !data.shiftPressed) {
                auto moved    = vec2(data.mousePos) - s_mouseDownPos;
                auto resTrans = static_cast<vec2>(m_mouseDownViewTrans) + moved / static_cast<vec2>(ui_workingArea->getContentTransScale());
                ui_workingArea->setContentTransTransl(resTrans.x, resTrans.y);
                setDrawFlag();
            }
        },
        true);

    //--------------------------------------------------------------------------------------

    ui_bottomMenu = addChild<Div>();
    ui_bottomMenu->setHeight(m_bottMenHeight);
    ui_bottomMenu->setAlignY(valign::bottom);

    //--------------------------------------------------------------------------------------

    ui_zoomSlider = ui_bottomMenu->addChild<PropSlider>();
    ui_zoomSlider->setProp(&m_zoomProp);
    onChanged<float>(&m_zoomProp, [this](const std::any& val) {
        if (ui_workingArea) {
            if (m_zoomUseWheel) {
                ui_workingArea->setZoomWithCenter(std::any_cast<float>(val) * 0.01f, getWindow()->getActMousePos());
            } else {
                ui_workingArea->setZoomNormMat(std::any_cast<float>(val) * 0.01f);
            }
        }

        for (const auto& it : m_onChangedCb) {
            it();
        }
    });

    ui_zoomSlider->addStyleClass("zoom_view.zoom_slider");
    ui_zoomSlider->setLabel("Zoom");
    ui_zoomSlider->setProp(&m_zoomProp);
    ui_zoomSlider->setPrecision(1);
    ui_zoomSlider->setValueChgCb([this] { m_zoomUseWheel = false; });
    ui_zoomSlider->setOnLostFocusCb([this] { getWindow()->setInputFocusNode(this, false); });

    ui_resetZoom = ui_bottomMenu->addChild<ImageButton>();
    ui_resetZoom->setBorderColor(0.8f, 0.8f, 0.8f, 1.f);
    m_initZoomPropVal = m_zoomProp();
    ui_resetZoom->setClickedCb([this] {
        ui_workingArea->setContentTransTransl(0.f, 0.f);
        ui_workingArea->setContentTransScale(m_initZoomPropVal * 0.01f, m_initZoomPropVal * 0.01f);
        m_zoomProp = m_initZoomPropVal;
        setDrawFlag();
    });

    // check if a display size has been set before init() was called
    if (m_displaySize.x != 0.f && m_displaySize.y != 0.f) setDisplaySize(m_displaySize.x, m_displaySize.y);

    if (m_initContFunc) m_initContFunc(ui_workingArea);
}

void ZoomView::setDisplaySize(int w, int h) {
    m_displaySize.x = w;
    m_displaySize.y = h;

    if (ui_content) {
        ui_content->setFixAspect(static_cast<float>(w) / static_cast<float>(h));
        ui_content->updateMatrix();
    }
}

void ZoomView::keyDown(hidData& data) {
    if (!data.key) return;
}

void ZoomView::mouseDown(hidData& data) {
    // working area movement
    if (ui_workingArea) m_mouseDownViewTrans = ui_workingArea->getContentTransTransl();
    s_mouseDownPos = (vec2)data.mousePos;
}

void ZoomView::mouseUp(hidData& data) {
    /*
        if (!ui_workingArea) return;
        bool isInWorkingArea = data.mousePos.x >
       ui_workingArea->getNodeViewport().x
                               && data.mousePos.y >
       ui_workingArea->getNodeViewport().y
                               && data.mousePos.x <
       (ui_workingArea->getNodeViewport().x +
       ui_workingArea->getNodeViewport().z)
                               && data.mousePos.y <
       (ui_workingArea->getNodeViewport().y +
       ui_workingArea->getNodeViewport().w);
    */
}

void ZoomView::mouseWheel(hidData& data) {
    if (getWindow()->isMousePressed()) return;

    if (m_zoomProp) {
        float newVal   = m_zoomProp() * 0.01f * (1.f + data.degrees * (data.ctrlPressed ? 0.01f : 0.1f));
        m_zoomUseWheel = true;
        m_zoomProp.setClamp(newVal * 100.f);
    }

    data.consumed = true;
}

}  // namespace ara