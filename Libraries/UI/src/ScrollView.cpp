#include "ScrollView.h"

#include "Log.h"
#include "UIApplication.h"
#include "UIWindow.h"
#include "string_utils.h"

using namespace glm;
using namespace std;

namespace ara {

ScrollView::ScrollView() : Div() {
    setName(getTypeName<ScrollView>());
    setFocusAllowed(false);
    setScissorChildren(true);
#ifdef __ANDROID__
    setCanReceiveDrag(true);
#endif
    m_content = dynamic_cast<Div*>(UINode::addChild(make_unique<Div>()));
    m_content->setName("content");
    m_content->excludeFromObjMap(true);
}

ScrollView::ScrollView(std::string&& styleClass) : Div(std::move(styleClass)) {
    setName(getTypeName<ScrollView>());
    setFocusAllowed(false);
    setScissorChildren(true);
    m_content = dynamic_cast<Div*>(UINode::addChild(make_unique<Div>()));
    m_content->setName("content");
    m_content->excludeFromObjMap(true);
}

void ScrollView::init() {
    // padding may come from code or from stylesheet. in the case of stylesheets, this value won't be available at this
    // moment, so check if there's a padding entry for this element in the stylesheets and do read it directly,
    // otherwise just read the set entry
    if (getStyleResNode() && getStyleResNode()->has("padding")) {
        auto pv = getStyleResNode()->splitNodeValue("padding");
        if (pv.size() == 1 && !is_number(pv[0])) {
            auto n = getStyleResNode()->getRoot()->findNode(pv[0]);
            if (n) {
                pv = n->splitValue(',');
            }
        }

        m_origPadding = vec4{pv.f(0), pv.f(1, pv.f(0)), pv.f(2, pv.f(0)), pv.f(3, pv.f(0))};
    } else {
        m_origPadding = m_padding;
    }

    // the scroll view size should always stay the same, independent of the scrollbars visibility to achieve this padding
    // is applied when the bars become visible
    ui_HSB = UINode::addChild<UIHScrollBar>();
    ui_HSB->setAlign(align::left, valign::bottom);
    ui_HSB->setSize(-m_scrollBarSize, m_scrollBarSize);  // if both are needed, then take size, so they don't overlap
    ui_HSB->setColor(m_sharedRes->colors->at(uiColors::sepLine));
    ui_HSB->setBackgroundColor(m_sharedRes->colors->at(uiColors::background));
    ui_HSB->excludeFromParentViewTrans(true);
    ui_HSB->excludeFromScissoring(true);
    ui_HSB->excludeFromPadding(true);
    ui_HSB->excludeFromOutOfBorderCheck(true);
    ui_HSB->setVisibility(false);

    ui_VSB = UINode::addChild<UIVScrollBar>();
    ui_VSB->setAlign(align::right, valign::top);
    ui_VSB->setSize(m_scrollBarSize, -m_scrollBarSize);  // if both are needed, then take size, so they don't overlap
    ui_VSB->setColor(m_sharedRes->colors->at(uiColors::sepLine));
    ui_VSB->setBackgroundColor(m_sharedRes->colors->at(uiColors::background));
    ui_VSB->excludeFromParentViewTrans(true);
    ui_VSB->excludeFromScissoring(true);
    ui_VSB->excludeFromPadding(true);
    ui_VSB->excludeFromOutOfBorderCheck(true);
    ui_VSB->setVisibility(false);

    // little div to cover the corner in case both scrollbars are showing
    m_corner = UINode::addChild<Div>();
    m_corner->setName("UIScrollBarCorner");
    m_corner->setAlign(align::right, valign::bottom);
    m_corner->setSize(m_scrollBarSize, m_scrollBarSize);  // if both are needed, then take size so they don't overlap
    m_corner->excludeFromParentViewTrans(true);
    m_corner->excludeFromScissoring(true);
    m_corner->excludeFromPadding(true);
    m_corner->excludeFromOutOfBorderCheck(true);
    m_corner->setVisibility(false);
    m_corner->setBackgroundColor(m_sharedRes->colors->at(uiColors::background));
}

void ScrollView::updtMatrIt(scissorStack* ss) {
    UINode::updtMatrIt(ss);  // children's bounding box must be calculated before the rest is evaluated

    if (!m_inited || !m_content) {
        return;
    }

    if (m_adaptContentTrans) {
        m_maxOffs = getSize() - getBBSize();

        // in case the content is scrolled beyond the maxOffs, correct the content translation
        if (m_maxOffs.x > getContentTransTransl().x || m_maxOffs.y > getContentTransTransl().y) {
            setContentTransTransl(std::max(std::min(m_maxOffs.x, 0.f), getContentTransTransl().x),
                                  std::max(std::min(m_maxOffs.y, 0.f), getContentTransTransl().y));
        }

        m_adaptContentTrans = false;
    }

    // checkScrollBarVisibility
    m_bb = m_content->getChildrenBoundBox();  // is in absolute coordinates top/left x,y
                                              // right/bottom x,y => vec4(x,y,z,w)

    // if the children bound box is either wider or higher than the node's size, activate the scrollbars
    m_needV = !m_blockVerScroll && m_size.y < (m_bb.w - m_bb.y + m_origPadding.y + m_origPadding.w);
    m_needH = !m_blockHorScroll && m_size.x < (m_bb.z - m_bb.x + m_origPadding.x + m_origPadding.z);

    if (ui_HSB && ui_HSB->isVisible() != m_needH) {
        ui_HSB->setVisibility(m_needH);
    }

    if (ui_VSB && ui_VSB->isVisible() != m_needV) {
        ui_VSB->setVisibility(m_needV);
    }

    if (m_corner && m_corner->isVisible() != (m_needH && m_needV)) {
        m_corner->setVisibility(m_needH && m_needV);
    }

    // to have the scroll view keep it's size independent of the scrollbars visibility, we apply a right-/bottom-padding
    // by the size of the scrollbar to have the content area representing the visible part. This keeps the following
    // calculations simple
    m_newPadd = vec2(m_needV ? (float)m_scrollBarSize + m_origPadding.z : m_origPadding.z,
                     m_needH ? (float)m_scrollBarSize + m_origPadding.w : m_origPadding.w);

    // check if we need to update the matrices
    if (m_newPadd.x != m_padding.z || m_newPadd.y != m_padding.w) {
        if (m_needH && m_needV) {
            if (ui_HSB) {
                ui_HSB->setWidth(-m_scrollBarSize);
            }
            if (ui_VSB) {
                ui_VSB->setHeight(-m_scrollBarSize);
            }
        } else {
            if (ui_HSB) {
                ui_HSB->setWidth(1.f);
            }
            if (ui_VSB) {
                ui_VSB->setHeight(1.f);
            }
        }

        UINode::setPadding(m_origPadding.x, m_origPadding.y, m_newPadd.x, m_newPadd.y);

        // padding has changed, children's matrices must be updated again
        setChanged(true);  // mark as changed recursively
        UINode::updtMatrIt(ss);
    }
}

void ScrollView::clearContentChildren() const {
    if (m_content) {
        m_content->clearChildren();
    }
}

std::vector<std::unique_ptr<UINode>>* ScrollView::getContChildren() const {
    if (!m_content) {
        return nullptr;
    }
    return &m_content->getChildren();
}

void ScrollView::mouseWheel(hidData* data) {
    if (ui_VSB && ui_VSB->isVisible()) {
        setScrollOffset(m_offs.x, data->degrees * 100 + getContentTransTransl().y);
        if (m_scrollCb) {
            m_scrollCb();
        }
    }
}

#ifdef __ANDROID__
void ScrollView::mouseDrag(hidData* data) {
    if (ui_VSB && ui_VSB->isVisible()) {
        if (data->dragStart) {
            m_dragInitOffs = getContentTransTransl();
        }
        setScrollOffset(m_offs.x, data->movedPix.y + m_dragInitOffs.y);
        if (m_scrollCb) m_scrollCb();
    }
}
#endif

/** absolute offset in pixel */
void ScrollView::setScrollOffset(float offsX, float offsY) {
    m_maxOffs   = -(getBBSize() - getSize());
    m_newOffs.x = offsX;
    m_newOffs.y = offsY;
    m_offs      = glm::min(glm::max(m_newOffs, m_maxOffs), m_zeroVec);

    setContentTransTransl(m_offs.x, m_offs.y);
    setDrawFlag();
}

void ScrollView::setViewport(float x, float y, float width, float height) {
    if (width != 0.f && height != 0.f) {
        UINode::setViewport(x, y, width, height);
        m_adaptContentTrans = true;
    }
}

glm::vec2 ScrollView::getBBSize() {
    m_bbSize.x = m_childBoundBox.z - m_childBoundBox.x + m_padding.x + m_padding.z;
    m_bbSize.y = m_childBoundBox.w - m_childBoundBox.y + m_padding.y + m_padding.w;
    return m_bbSize;
}

void ScrollView::setPadding(float val) {
    m_origPadding = glm::vec4(val, val, val, val);
    UINode::setPadding(val, val, val, val);
}

void ScrollView::setPadding(float left, float top, float right, float bot) {
    m_origPadding = glm::vec4(left, top, right, bot);
    UINode::setPadding(left, top, right, bot);
}

void ScrollView::setPadding(glm::vec4& val) {
    m_origPadding = val;
    UINode::setPadding(val);
}

UINode* ScrollView::addChild(std::unique_ptr<UINode>&& child) {
    if (!m_content) {
        return nullptr;
    }
    return m_content->addChild(std::move(child));
}

}  // namespace ara