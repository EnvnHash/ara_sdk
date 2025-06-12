#include <Asset/ResNode.h>
#include <UIElements/ScrollView.h>
#include <UIWindow.h>
#include <string_utils.h>

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
    m_content = UINode::addChild<Div>({ .name = "content" });
}

void ScrollView::init() {
    // padding may come from code or from stylesheet. in the case of stylesheets, this value won't be available at this
    // moment, so check if there's a padding entry for this element in the stylesheets and do read it directly,
    // otherwise just read the set entry
    if (getStyleResNode() && getStyleResNode()->has("padding")) {
        auto pv = getStyleResNode()->splitNodeValue("padding");
        if (pv.size() == 1 && !is_number(pv[0])) {
            if (auto n = getStyleResNode()->getRoot()->findNode(pv[0])) {
                pv = n->splitValue(',');
            }
        }
        m_origPadding = vec4{pv.f(0), pv.f(1, pv.f(0)), pv.f(2, pv.f(0)), pv.f(3, pv.f(0))};
    } else {
        m_origPadding = m_padding;
    }

    // the scroll view size should always stay the same, independent of the scrollbars visibility to achieve this padding
    // is applied when the bars become visible
    m_HSB = UINode::addChild<UIHScrollBar>(UINodePars{
        .size = ivec2(-m_scrollBarSize, m_scrollBarSize),
        .fgColor = m_sharedRes->colors->at(uiColors::sepLine),
        .bgColor = m_sharedRes->colors->at(uiColors::background),
        .alignX = align::left,
        .alignY = valign::bottom,
        .visible = false,
        .excludeFromParentViewTrans = true,
        .excludeFromScissoring = true,
        .excludeFromPadding = true,
        .excludeFromOutOfBorderCheck = true
    });

    m_VSB = UINode::addChild<UIVScrollBar>(UINodePars{
        .size = ivec2{m_scrollBarSize, -m_scrollBarSize},
        .fgColor = m_sharedRes->colors->at(uiColors::sepLine),
        .bgColor = m_sharedRes->colors->at(uiColors::background),
        .alignX = align::right,
        .alignY = valign::top,
        .visible = false,
        .excludeFromParentViewTrans = true,
        .excludeFromScissoring = true,
        .excludeFromPadding = true,
        .excludeFromOutOfBorderCheck = true
    });

    // little div to cover the corner in case both scrollbars are showing
    m_corner = UINode::addChild<Div>({
        .size = ivec2{m_scrollBarSize, m_scrollBarSize},
        .bgColor = m_sharedRes->colors->at(uiColors::background),
        .name = "UIScrollBarCorner",
        .alignX = align::right,
        .alignY = valign::bottom,
        .visible = false,
        .excludeFromParentViewTrans = true,
        .excludeFromScissoring = true,
        .excludeFromPadding = true,
        .excludeFromOutOfBorderCheck = true
    });
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

    if (m_HSB && m_HSB->isVisible() != m_needH) {
        m_HSB->setVisibility(m_needH);
    }

    if (m_VSB && m_VSB->isVisible() != m_needV) {
        m_VSB->setVisibility(m_needV);
    }

    if (m_corner && m_corner->isVisible() != (m_needH && m_needV)) {
        m_corner->setVisibility(m_needH && m_needV);
    }

    // to have the scroll view keep it's size independent of the scrollbars visibility, we apply a right-/bottom-padding
    // by the size of the scrollbar to have the content area representing the visible part. This keeps the following
    // calculations simple
    m_newPadd = vec2(m_needV ? static_cast<float>(m_scrollBarSize) + m_origPadding.z : m_origPadding.z,
                     m_needH ? static_cast<float>(m_scrollBarSize) + m_origPadding.w : m_origPadding.w);

    // check if we need to update the matrices
    if (m_newPadd.x != m_padding.z || m_newPadd.y != m_padding.w) {
        if (m_needH && m_needV) {
            if (m_HSB) {
                m_HSB->setWidth(-m_scrollBarSize);
            }
            if (m_VSB) {
                m_VSB->setHeight(-m_scrollBarSize);
            }
        } else {
            if (m_HSB) {
                m_HSB->setWidth(1.f);
            }
            if (m_VSB) {
                m_VSB->setHeight(1.f);
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

void ScrollView::mouseWheel(hidData& data) {
    if (m_VSB && m_VSB->isVisible()) {
        setScrollOffset(m_offs.x, data.degrees * 100 + getContentTransTransl().y);
        if (m_scrollCb) {
            m_scrollCb();
        }
    }
}

#ifdef __ANDROID__
void ScrollView::mouseDrag(hidData& data) {
    if (m_VSB && m_VSB->isVisible()) {
        if (data.dragStart) {
            m_dragInitOffs = getContentTransTransl();
        }
        setScrollOffset(m_offs.x, data.movedPix.y + m_dragInitOffs.y);
        if (m_scrollCb) m_scrollCb();
    }
}
#endif

/** absolute offset in pixel */
void ScrollView::setScrollOffset(float offsX, float offsY) {
    m_maxOffs   = -(getBBSize() - getSize());
    m_newOffs.x = offsX;
    m_newOffs.y = offsY;
    m_offs      = glm::min(glm::max(m_newOffs, m_maxOffs), {});

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

}  // namespace ara