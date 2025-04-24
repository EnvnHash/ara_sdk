#include <Asset/ResSrcFile.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetManager.h>
#include <string_utils.h>

#include "UIApplication.h"

using namespace glm;
using namespace std;

namespace ara {

UINode::UINode() : m_parentMat(&m_identMat) {
    setName(getTypeName<UINode>());
}

void UINode::setX(int x, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXInt  = x;
        m_posXType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(x) + "px", st);
}

void UINode::setX(float x, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXFloat = x;
        m_posXType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(x * 100) + "%", st);
}

void UINode::setY(int y, state st) {
    if (st == state::m_state || st == m_state) {
        m_posYInt  = y;
        m_posYType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("y", std::to_string(y) + "px", st);
}

void UINode::setY(float y, state st) {
    if (st == state::m_state || st == m_state) {
        m_posYFloat = y;
        m_posYType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("y", std::to_string(y * 100) + "%", st);
}

void UINode::setPos(int posX, int posY, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXInt  = posX;
        m_posXType = unitType::Pixels;
        m_posYInt  = posY;
        m_posYType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(posX) + "px", st);
    setStyleInitVal("y", std::to_string(posY) + "px", st);
}

void UINode::setPos(int posX, float posY, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXInt   = posX;
        m_posXType  = unitType::Pixels;
        m_posYFloat = posY;
        m_posYType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(posX) + "px", st);
    setStyleInitVal("y", std::to_string(posY * 100) + "%", st);
}

void UINode::setPos(float posX, int posY, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXFloat = posX;
        m_posXType  = unitType::Percent;
        m_posYInt   = posY;
        m_posYType  = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(posX * 100) + "%", st);
    setStyleInitVal("y", std::to_string(posY) + "px", st);
}

void UINode::setPos(float posX, float posY, state st) {
    if (st == state::m_state || st == m_state) {
        m_posXFloat = posX;
        m_posXType  = unitType::Percent;
        m_posYFloat = posY;
        m_posYType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("x", std::to_string(posX * 100) + "%", st);
    setStyleInitVal("y", std::to_string(posY * 100) + "%", st);
}

void UINode::setWidth(int width, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthInt  = width;
        m_widthType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width) + "px", st);
}

void UINode::setWidth(float width, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthFloat = width;
        m_widthType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width * 100) + "%", st);
}

void UINode::setHeight(int height, state st) {
    if (st == state::m_state || st == m_state) {
        m_heightInt  = height;
        m_heightType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("height", std::to_string(height) + "px", st);
}

void UINode::setHeight(float height, state st) {
    if (st == state::m_state || st == m_state) {
        m_heightFloat = height;
        m_heightType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("height", std::to_string(height * 100) + "%", st);
}

void UINode::setSize(int width, int height, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthInt   = width;
        m_widthType  = unitType::Pixels;
        m_heightInt  = height;
        m_heightType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width) + "px", st);
    setStyleInitVal("height", std::to_string(height) + "px", st);
}

void UINode::setSize(int width, float height, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthInt    = width;
        m_widthType   = unitType::Pixels;
        m_heightFloat = height;
        m_heightType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width) + "px", st);
    setStyleInitVal("height", std::to_string(height * 100) + "%", st);
}

void UINode::setSize(float width, int height, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthFloat = width;
        m_widthType  = unitType::Percent;
        m_heightInt  = height;
        m_heightType = unitType::Pixels;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width * 100) + "%", st);
    setStyleInitVal("height", std::to_string(height) + "px", st);
}

void UINode::setSize(float width, float height, state st) {
    if (st == state::m_state || st == m_state) {
        m_widthFloat  = width;
        m_widthType   = unitType::Percent;
        m_heightFloat = height;
        m_heightType  = unitType::Percent;
        setChanged(true);
    }
    setStyleInitVal("width", std::to_string(width * 100) + "%", st);
    setStyleInitVal("height", std::to_string(height * 100) + "%", st);
}

void UINode::setAlignX(align type, state st) {
    if (st == state::m_state || st == m_state) {
        m_alignX = type;
        m_pivX   = (type == align::left ? pivotX::left : (type == align::right ? pivotX::right : pivotX::center));
        setChanged(true);
    }
    setStyleInitVal("align", type == align::left ? "left" : (type == align::right ? "right" : "center"), st);
}

void UINode::setAlignY(valign type, state st) {
    if (st == state::m_state || st == m_state) {
        m_alignY = type;
        m_pivY   = (type == valign::top ? pivotY::top : (type == valign::bottom ? pivotY::bottom : pivotY::center));
        setChanged(true);
    }
    setStyleInitVal("v-align", type == valign::top ? "top" : (type == valign::bottom ? "bottom" : "center"), st);
}

void UINode::setAlign(align alignX, valign alignY, state st) {
    setAlignX(alignX, st);
    setAlignY(alignY, st);
}

void UINode::setPadding(float left, float top, float right, float bot, state st) {
    if (st == state::m_state || st == m_state) {
        m_padding.x  = left;
        m_padding.y  = top;
        m_padding.z  = right;
        m_padding.w  = bot;
        m_geoChanged = true;
    }
    setStyleInitVal(
        "padding",
        std::to_string(left) + "," + std::to_string(top) + "," + std::to_string(right) + "," + std::to_string(bot), st);
}

void UINode::setPadding(glm::vec4& val, state st) {
    if (st == state::m_state || st == m_state) {
        m_padding    = val;
        m_geoChanged = true;
    }
    setStyleInitVal("padding",
                    std::to_string(val[0]) + "," + std::to_string(val[1]) + "," + std::to_string(val[2]) + "," +
                        std::to_string(val[3]), st);
}

void UINode::setBorderColor(float r, float g, float b, float a, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderColor      = glm::vec4(r, g, b, a);
        m_drawParamChanged = true;
    }
    setStyleInitVal("border-color",
                    "rgba(" + std::to_string(int(r * 255)) + "," + std::to_string(int(g * 255)) + "," +
                        std::to_string(int(b * 255)) + "," + std::to_string(int(a * 255)) + ")",
                    st);
}

void UINode::setBorderColor(glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderColor      = col;
        m_drawParamChanged = true;
    }
    setStyleInitVal("border-color",
                    "rgba(" + std::to_string(int(col.r * 255)) + "," + std::to_string(int(col.g * 255)) + "," +
                        std::to_string(int(col.b * 255)) + "," + std::to_string(int(col.a * 255)) + ")",
                    st);
}

void UINode::setColor(float r, float g, float b, float a, state st) {
    if (st == state::m_state || st == m_state) {
        m_color.r          = r;
        m_color.g          = g;
        m_color.b          = b;
        m_color.a          = a;
        m_drawParamChanged = true;
    }
    setStyleInitVal("color",
                    "rgba(" + std::to_string(int(r * 255)) + "," + std::to_string(int(g * 255)) + "," +
                        std::to_string(int(b * 255)) + "," + std::to_string(int(a * 255)) + ")",
                    st);
}

void UINode::setColor(glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_color            = col;
        m_drawParamChanged = true;
    }
    setStyleInitVal("color",
                    "rgba(" + std::to_string(int(col.r * 255)) + "," + std::to_string(int(col.g * 255)) + "," +
                        std::to_string(int(col.b * 255)) + "," + std::to_string(int(col.a * 255)) + ")",
                    st);
}

void UINode::setBackgroundColor(float r, float g, float b, float a, state st) {
    if (st == state::m_state || st == m_state) {
        m_bgColor.r        = r;
        m_bgColor.g        = g;
        m_bgColor.b        = b;
        m_bgColor.a        = a;
        m_drawParamChanged = true;
    }
    setStyleInitVal("bkcolor",
                    "rgba(" + std::to_string(int(r * 255)) + "," + std::to_string(int(g * 255)) + "," +
                        std::to_string(int(b * 255)) + "," + std::to_string(int(a * 255)) + ")",
                    st);
}

void UINode::setBackgroundColor(glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_bgColor          = col;
        m_drawParamChanged = true;
    }
    setStyleInitVal("bkcolor",
                    "rgba(" + std::to_string(int(col.r * 255)) + "," + std::to_string(int(col.g * 255)) + "," +
                        std::to_string(int(col.b * 255)) + "," + std::to_string(int(col.a * 255)) + ")",
                    st);
}

mat4* UINode::getContentMat(bool excludedFromParentContentTrans, bool excludedFromPadding) {
    return excludedFromPadding ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                               : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);
}

mat4* UINode::getFlatContentMat(bool excludedFromParentContentTrans, bool excludedFromPadding) {
    auto mat = excludedFromPadding ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                                : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);

    // don't use the dynamic m_parentMat ptr since here it doesn't contain the
    // information of all other parentMats use instead the local copy, which is
    // safe to do, since this is only called after this node's updateMatrix()
    m_flatContentTransMat = m_parentMatLocCpy * *mat;

    return &m_flatContentTransMat;
}

glm::vec2& UINode::getParentNodeRelPos() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the node's position relative the parent node's upper left
    // corner in pixels
    if (m_parent) {
        vec2 wp              = getWinPos();
        m_parentNodeRelPos.x = wp.x - m_parent->getWinPos().x;
        m_parentNodeRelPos.y = wp.y - m_parent->getWinPos().y;
    }

    return m_parentNodeRelPos;
}

glm::vec2& UINode::getWinPos() {
    if (m_geoChanged) {
        updateMatrix();
    }

    if (m_parentMat) {
        m_parentTransPos = vec2(*m_parentMat * vec4(m_pos, 0.f, 1.f));
        m_winRelPos      = m_parentTransPos;
    } else {
        m_winRelPos = m_pos;
    }

    return m_winRelPos;
}

glm::vec2& UINode::getWinRelSize() {
    if (m_geoChanged) {
        updateMatrix();
    }

    if (m_parentMat) {
        for (int i = 0; i < 2; i++) m_winRelSize[i] = m_parentMatLocCpy[i][i] * m_size[i];
    } else {
        m_winRelSize = m_size;
    }

    return m_winRelSize;
}

glm::vec2& UINode::getContWinPos() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // this node's content's left/top corner in relation to the window's
    // top/left corner
    m_contWinPos.x = m_parentContVp.x + m_pos.x + m_padding.x + (float)m_borderWidth;
    m_contWinPos.y = m_parentContVp.y + m_pos.y + m_padding.y + (float)m_borderWidth;

    return m_contWinPos;
}

glm::vec2& UINode::getContentSize() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the size of the node's content area
    m_contentSize.x = m_size.x - (m_padding.x + m_padding.z + (float)m_borderWidth * 2.f);
    m_contentSize.y = m_size.y - (m_padding.y + m_padding.w + (float)m_borderWidth * 2.f);

    return m_contentSize;
}

glm::vec2& UINode::getContentOffset() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the size of the node's content area
    m_contentOffset.x = m_padding.x + (float)m_borderWidth;
    m_contentOffset.y = m_padding.y + (float)m_borderWidth;

    return m_contentOffset;
}

glm::vec2& UINode::getBorderWidthRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderWidthRel[i] = (float)m_borderWidth / m_size[i] / m_parentContScale[i];
        } else {
            m_borderWidthRel[i] = 0.f;
        }
    }

    return m_borderWidthRel;
}

glm::vec2& UINode::getBorderRadiusRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderRadiusRel[i] = (float)m_borderRadius / m_size[i] / m_parentContScale[i];
        } else {
            m_borderRadiusRel[i] = 0.f;
        }
    }

    return m_borderRadiusRel;
}

glm::vec2& UINode::getBorderAliasRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderAliasRel[i] = (float)m_borderAlias / m_size[i] / m_parentContScale[i];
        } else {
            m_borderAliasRel[i] = 0.f;
        }
    }

    return m_borderAliasRel;
}

void UINode::calcNormMat() {
    p[0] = *m_parentMat * vec4(m_pos.x, m_pos.y, 0.f, 1.f);
    p[1] = *m_parentMat * vec4(m_pos.x + m_size.x, m_pos.y + m_size.y, 0.f, 1.f);

    for (int i = 0; i < 2; i++) {
        pN[i]   = vec2(p[i]) / vec2(m_viewPort.z, m_viewPort.w);
        pN[i].y = 1.f - pN[i].y;
        pN[i]   = pN[i] * 2.f - 1.f;
    }

    // to render a normalized quad lb (-1|-1) rt (1|1)
    m_normMat = translate(vec3(pN[1] + (pN[0] - pN[1]) * 0.5f, 0.f)) * scale(vec3(glm::abs(pN[1] - pN[0]) * 0.5f, 1.f));
}

void UINode::setViewport(float x, float y, float width, float height) {
    m_viewPort.x = x;
    m_viewPort.y = y;
    m_viewPort.z = width;
    m_viewPort.w = height;
    m_geoChanged = true;

    for (auto& it : m_children) {
        it->setViewport(x, y, width, height);
    }
}

void UINode::setContentTransScale(float x, float y) {
    setChanged(true);  // force children to update
    m_contentTransScale.x          = x;
    m_contentTransScale.y          = y;
    m_contentTransScale.z          = 1.f;
    m_contentTransScaleFixAspect.x = x;
    m_contentTransScaleFixAspect.y = y;
    m_contentTransScaleFixAspect.z = 1.f;
    calcContentTransMat();
}

void UINode::setContentTransTransl(float x, float y) {
    m_contentTransMatTransl.x = x;
    m_contentTransMatTransl.y = y;
    setChanged(true);  // force children to update
}

void UINode::setContentRotation(float angle, float ax, float ay, float az) {
    setChanged(true);  // force children to update

    m_contentTransRotate.w = angle;
    m_contentTransRotate.x = ax;
    m_contentTransRotate.y = ay;
    m_contentTransRotate.z = az;

    m_hasContRot = true;
    m_contRot    = rotate(m_contentTransRotate.w, vec3(m_contentTransRotate));
}

void UINode::setSelected(bool val, bool forceStyleUpdt) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    setState(val ? state::selected : state::none);

    if (forceStyleUpdt){
        applyStyle();
    }

    if (m_selectedCb) {
        m_selectedCb(val);
    }
}

void UINode::setDisabled(bool val, bool forceStyleUpdt) {
    setState(val ? state::disabled : state::none);

    // clear m_lastState -> otherwise will cause unwanted style changed on
    // mouseout
    if (!val) {
        m_lastState = state::none;
    }

    if (forceStyleUpdt) {
        applyStyle();
    }

    if (m_selectedCb) {
        m_selectedCb(val);
    }
}

void UINode::setHighlighted(bool val, bool forceStyleUpdt) {
    setState(val ? state::highlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINode::setDisabledHighlighted(bool val, bool forceStyleUpdt) {
    setState(val ? state::disabledHighlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINode::setDisabledSelected(bool val, bool forceStyleUpdt) {
    setState(val ? state::disabledSelected : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINode::updateMatrix() {
    if (!m_geoChanged || m_updating) {
        return;
    }

    m_updating     = true;  // avoid infinite recursive call
    m_parentContVp = m_viewPort;

    // get the parent's content's viewport in pixels. in case of the root node,
    // this will stay the window's viewport
    if (m_parent) {
        // create a local copy of the parent's matrix
        m_parentMat       = m_parent->getFlatContentMat(m_excludeFromParentContentTrans, m_excludeFromPadding);
        m_parentMatLocCpy = *m_parentMat;

        m_parentContScale.x = (*m_parentMat)[0][0];
        m_parentContScale.y = (*m_parentMat)[1][1];

        if (m_excludeFromPadding) {
            // TODO: check if the parent's content translation, is respected here
            m_parentContVp = m_parent->getNodeViewport();
        } else {
            m_parentContVp.x = (*m_parentMat)[3][0];
            m_parentContVp.y = (*m_parentMat)[3][1];
            m_parentContVp.z = m_parent->getContentSize().x;
            m_parentContVp.w = m_parent->getContentSize().y;
        }

        m_absoluteAlpha = m_parent->getAlpha() * m_alpha;
    }

    // set position and size for further calculations
    m_pos.x       = (float)m_posXInt;
    m_pos.y       = (float)m_posYInt;
    m_init_size.x = (float)m_widthInt;
    m_init_size.y = (float)m_heightInt;

    // if there is a negative integer width or height, convert it to size - val
    if (m_widthType == unitType::Pixels && m_widthInt < 0) {
        m_init_size.x = m_parentContVp.z + (float)m_widthInt;
    }

    if (m_heightType == unitType::Pixels && m_heightInt < 0) {
        m_init_size.y = m_parentContVp.w + (float)m_heightInt;
    }

    // if there are relative position or size coordinates, convert them to
    // pixels
    if (m_posXType == unitType::Percent) {
        m_pos.x = m_posXFloat * m_parentContVp.z;
    }
    if (m_posYType == unitType::Percent) {
        m_pos.y = m_posYFloat * m_parentContVp.w;
    }
    if (m_widthType == unitType::Percent) {
        m_init_size.x = m_widthFloat * m_parentContVp.z;
    }
    if (m_heightType == unitType::Percent) {
        m_init_size.y = m_heightFloat * m_parentContVp.w;
    }

    // get the node's aspect ratio
    m_aspect = m_init_size.x / m_init_size.y;

    m_work_size.x = m_init_size.x;
    m_work_size.y = m_init_size.y;

    // if there is a request aspect process it
    if (m_fixAspect > 0.f) {
        // correct size, always shrink
        if (m_fixAspect < m_aspect) {
            m_work_size.x = m_init_size.y * m_fixAspect;
            m_work_size.y = m_init_size.y;
        } else {
            m_work_size.x = m_init_size.x;
            m_work_size.y = m_init_size.x / m_fixAspect;
        }
    }

    // process x pivot and alignment
    switch (m_alignX) {
        case align::right: m_pos.x += m_parentContVp.z; break;
        case align::center: m_pos.x += m_parentContVp.z * 0.5f; break;
        default: break;
    }

    switch (m_pivX) {
        case pivotX::left: break;
        case pivotX::right: m_pos.x -= m_work_size.x; break;
        case pivotX::center: m_pos.x -= m_work_size.x * 0.5f; break;
    }

    // process x pivot and alignment
    switch (m_alignY) {
        case valign::bottom: m_pos.y += m_parentContVp.w; break;
        case valign::center: m_pos.y += m_parentContVp.w * 0.5f; break;
        default: break;
    }

    switch (m_pivY) {
        case pivotY::top: break;
        case pivotY::bottom: m_pos.y -= m_work_size.y; break;
        case pivotY::center: m_pos.y -= m_work_size.y * 0.5f; break;
    }

    // store the size of this node (including its padding and border)
    m_size = m_work_size;

    // parent relative content matrix for the node's children
    m_nodeMat[3][0] = m_pos.x + (float)m_borderWidth;
    m_nodeMat[3][1] = m_pos.y + (float)m_borderWidth;

    m_contentMat[3][0] = m_nodeMat[3][0] + m_padding.x;
    m_contentMat[3][1] = m_nodeMat[3][1] + m_padding.y;

    // calculate the content transformation matrix
    calcContentTransMat();

    m_geoChanged = false;
    m_updating   = false;

    if (!m_drawImmediate) {
        m_drawParamChanged = true;
    }

    if (m_changeCb) {
        m_changeCb();
    }
}

void UINode::calcContentTransMat() {
    if (m_contTransMatCentered) {
        // for scaling, center the content on the origin, scale and uncenter
        // again
        m_contentTransMatRel = m_contRot;

        m_contentTransMatRel[3][0] += m_contentTransMatTransl.x + m_size.x * -0.5f;
        m_contentTransMatRel[3][1] += m_contentTransMatTransl.y + m_size.y * -0.5f;
        m_contentTransMatRel[3][2] += m_contentTransMatTransl.z;

        m_contentTransMatRel = scale(m_contentTransScaleFixAspect) * m_contentTransMatRel;

        m_contentTransMatRel[3][0] += m_size.x * 0.5f;
        m_contentTransMatRel[3][1] += m_size.y * 0.5f;

    } else {
        // this is expensive
        if (m_hasContRot) {
            m_contentTransMatRel = translate(m_contentTransMatTransl) * m_contRot * scale(m_contentTransScale);
        } else {
            m_contentTransMatRel = translate(m_contentTransMatTransl) * scale(m_contentTransScale);
        }
    }

    // since we don't need the content relative offset anymore
    // (m_contentTransMat is defined by the vec3 translation, scale, rotation)
    // use it again
    m_nodeTransMat    = m_nodeMat * m_contentTransMatRel;
    m_contentTransMat = m_contentMat * m_contentTransMatRel;

    m_nodePosMat[3][0] = m_pos.x;
    m_nodePosMat[3][1] = m_pos.y;

    // apply the windows orthographic matrix, this matrix will be used for
    // rendering
    if (m_orthoMat) {
        // this is expensive
        m_mvp   = *m_orthoMat * m_parentMatLocCpy * m_nodePosMat;
        m_mvpHw = m_mvp * glm::scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
    }
}

void UINode::setDrawFlag() {
    if (m_sharedRes) {
        m_sharedRes->setDrawFlag();
    }
}

UINode* UINode::getNode(const std::string& name) {
    UINode* fn = nullptr;
    getNodeIt(this, &fn, name);
    return fn;
}

UINode* UINode::getRoot() {
    UINode* out = this;
    while (out->getParent()) out = out->getParent();
    return out;
}

void UINode::setPivotX(pivotX pX) {
    m_pivX       = pX;
    m_geoChanged = true;
}

void UINode::setPivotY(pivotY pY) {
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINode::setPivot(pivotX pX, pivotY pY) {
    m_pivX       = pX;
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINode::setPadding(float val, state st) {
    if (st == state::m_state || st == m_state) {
        m_padding    = glm::vec4(val, val, val, val);
        m_geoChanged = true;
    }
    setStyleInitVal("padding", std::to_string(val), st);
}

void UINode::setBorderWidth(uint32_t val, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderWidth = val;
        m_geoChanged  = true;
    }
    setStyleInitVal("border-width", std::to_string(val), st);
}

void UINode::setBorderRadius(uint32_t val, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderRadius = val;
        m_geoChanged   = true;
    }
    setStyleInitVal("border-radius", std::to_string(val), st);
}

void UINode::setChanged(bool val) {
    m_geoChanged = val;
    for (auto& it : m_children) {
        it->setChanged(val);
    }
}

void UINode::setFixAspect(float val) {
    m_fixAspect = val;
    setChanged(true);
}

glm::vec2 UINode::getOrigPos() {
    return {m_posXType == unitType::Pixels ? (float)m_posXInt : m_posXFloat,
            m_posYType == unitType::Pixels ? (float)m_posYInt : m_posYFloat};
}

glm::vec2& UINode::getPos() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_pos;
}

glm::vec2& UINode::getSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size;
}

glm::vec2& UINode::getNodeSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size;
}

float UINode::getNodeWidth() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size.x;
}

float UINode::getNodeHeight() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size.y;
}

glm::vec2 UINode::getNodeRelSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    m_relSize.x = m_size.x / m_parentContVp.z;
    m_relSize.y = m_size.y / m_parentContVp.w;
    return m_relSize;
}

glm::vec4 UINode::getNodeViewportGL() {
    getWinPos();
    return {m_winRelPos.x, m_viewPort.w - (m_winRelPos.y + m_size.y), m_size.x, m_size.y};
}

glm::vec4 UINode::getContentViewport() {
    getContWinPos();
    getContentSize();
    return {m_contWinPos.x, m_contWinPos.y, m_contentSize.x, m_contentSize.y};
}

glm::vec4 UINode::getNodeViewport() {
    getWinPos();
    getNodeSize();
    return {m_winRelPos.x, m_winRelPos.y, m_size.x, m_size.y};
}

float* UINode::getMVPMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvp[0][0];
}

glm::mat4* UINode::getMvp() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvp;
}

glm::mat4* UINode::getHwMvp() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvpHw;
}

float* UINode::getWinRelMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_winRelMat[0][0];
}

glm::mat4* UINode::getWinRelMat() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_winRelMat;
}

glm::mat4* UINode::getNormMat() {
    if (m_geoChanged) {
        updateMatrix();
    }
    calcNormMat();
    return &m_normMat;
}

float* UINode::getNormMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    calcNormMat();
    return &m_normMat[0][0];
}

glm::vec2& UINode::getParentContentScale() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_parentContScale;
}

void  UINode::setState(state st) {
    if (m_state != st) {
        m_lastState = m_state;
    }
    m_state = st;
}

void UINode::runOnMainThread(const std::function<bool()>& func, bool forcePush) {
    if (m_glbase) {
        m_glbase->runOnMainThread(func, forcePush);
    }
}

void UINode::addGlCbSync(const std::function<bool()>& func) const {
    if (m_glbase) {
        m_glbase->addGlCbSync(func);
    }
}

// styles - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void UINode::addStyleClass(std::string&& styleClass) {
    std::string sc = styleClass;  // done this way, since on Windows a DLL has an own memory
                                  // heap and at the end of this function styleClass would be
                                  // deleted causing a crash

    if (m_styleTree.empty() || (m_styleTree.size() == 1 && m_styleTree.front().find("__") != std::string::npos)) {
        m_baseStyleClass = sc;
    }

    m_styleClassInited = false;

    // in case there was a previous (not default) style definition, delete it
    if (!m_styleTree.empty()) {
        auto it = find_if(m_styleTree.begin(), m_styleTree.end(), [sc](const std::string& st) { return st == sc; });
        if (it == m_styleTree.end()) m_styleTree.push_back(sc);
    } else {
        m_styleTree.emplace_back(sc);
    }
}

void UINode::clearStyles() {
    // clear style tree (text definitions)
    m_styleTree.clear();

    // clear lambada functions
    for (int i = 0; i < (int)state::count; i++) {
        m_setStyleFunc[(state)i].clear();
    }

    loadStyleDefaults();  // calls the derived classes loadStyleDefaults
    rebuildCustomStyle();

    m_styleClassInited = false;
}

void UINode::updateStyleIt(ResNode* node, state st, std::string& styleClass) {
    AssetColor* color;

    auto x = node->findNumericNode("x");
    if (get<ResNode*>(x)) {
        if (get<unitType>(x) == unitType::Percent) {
            float val                        = stof(get<string>(x)) * 0.01f;
            m_setStyleFunc[st][styleInit::x] = [this, val, st]() { setX(val, st); };
        } else {
            int val                          = stoi(get<string>(x));
            m_setStyleFunc[st][styleInit::x] = [this, val, st]() { setX(val, st); };
        }
    }

    auto y = node->findNumericNode("y");
    if (get<ResNode*>(y)) {
        if (get<unitType>(y) == unitType::Percent) {
            float val                        = stof(get<string>(y)) * 0.01f;
            m_setStyleFunc[st][styleInit::y] = [this, val, st]() { setY(val, st); };
        } else {
            int val                          = stoi(get<string>(y));
            m_setStyleFunc[st][styleInit::y] = [this, val, st]() { setY(val, st); };
        }
    }

    auto width = node->findNumericNode("width");
    if (get<ResNode*>(width)) {
        if (get<unitType>(width) == unitType::Percent) {
            float val                            = stof(get<string>(width)) * 0.01f;
            m_setStyleFunc[st][styleInit::width] = [this, val, st]() { setWidth(val, st); };
        } else {
            int val                              = stoi(get<string>(width));
            m_setStyleFunc[st][styleInit::width] = [this, val, st]() { setWidth(val, st); };
        }
    }

    auto height = node->findNumericNode("height");
    if (get<ResNode*>(height)) {
        if (get<unitType>(height) == unitType::Percent) {
            float val                             = stof(get<string>(height)) * 0.01f;
            m_setStyleFunc[st][styleInit::height] = [this, val, st]() { setHeight(val, st); };
        } else {
            int val                               = stoi(get<string>(height));
            m_setStyleFunc[st][styleInit::height] = [this, val, st]() { setHeight(val, st); };
        }
    }

    auto align = node->findNode("align");
    if (align) {
        if (align->m_value == "center") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::center, st); };
        } else if (align->m_value == "left") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::left, st); };
        } else if (align->m_value == "right") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::right, st); };
        }
    }

    auto v_align = node->findNode("v-align");
    if (v_align) {
        if (v_align->m_value == "center")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::center, st); };
        else if (v_align->m_value == "top")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::top, st); };
        else if (v_align->m_value == "bottom")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::bottom, st); };
    }

    if ((color = node->findNode<AssetColor>("color")) != nullptr) {
        vec4 col                             = color->getColorvec4();
        m_setStyleFunc[st][styleInit::color] = [this, st, col]() { setColor(col.r, col.g, col.b, col.a, st); };
    }

    if ((color = node->findNode<AssetColor>("bkcolor")) != nullptr) {
        vec4 col                               = color->getColorvec4();
        m_setStyleFunc[st][styleInit::bkcolor] = [this, st, col]() {
            setBackgroundColor(col.r, col.g, col.b, col.a, st);
        };
    }

    if ((color = node->findNode<AssetColor>("border-color")) != nullptr) {
        vec4 col                                = color->getColorvec4();
        m_setStyleFunc[st][styleInit::brdColor] = [this, st, col]() { setBorderColor(col.r, col.g, col.b, col.a, st); };
    }

    auto borderWidth = node->findNumericNode("border-width");
    if (get<ResNode*>(borderWidth) && get<unitType>(borderWidth) == unitType::Pixels) {
        int val                                 = stoi(get<string>(borderWidth));
        m_setStyleFunc[st][styleInit::brdWidth] = [this, val, st]() { setBorderWidth(val, st); };
    }

    auto borderRad = node->findNumericNode("border-radius");
    if (get<ResNode*>(borderRad) && get<unitType>(borderRad) == unitType::Pixels) {
        int val                                  = stoi(get<string>(borderRad));
        m_setStyleFunc[st][styleInit::brdRadius] = [this, val, st]() { setBorderRadius(val, st); };
    }

    if (node->has("padding")) {
        ParVec pv = node->splitNodeValue("padding");

        // in case this is a reference iterate again
        if (pv.size() == 1 && !is_number(pv[0])) {
            auto n = node->getRoot()->findNode(pv[0]);
            if (n) pv = n->splitValue(',');
        }

        vec4 pd{pv.f(0), pv.f(1, pv.f(0)), pv.f(2, pv.f(0)), pv.f(3, pv.f(0))};
        m_setStyleFunc[st][styleInit::padding] = [this, st, pd]() {
            setPadding(pd.x, pd.y, pd.z, pd.w, st);
        };  // this will avoid exceptions if padding value has less than 4
            // values,
        // default is first value, so one can make it padding:10 for example and
        // will become 10,10,10,10
    }

    auto vis = node->findNode("visible");
    if (vis) {
        bool val                               = vis->m_value == "true";
        m_setStyleFunc[st][styleInit::visible] = [this, val, st]() { setVisibility(val, st); };
    }
}

void UINode::updateStyle() {
    m_styleChanged     = false;
    m_styleClassInited = true;

    if (m_excludeFromStyles){
        return;
    }

    // reset style functions
    for (auto& it : m_setStyleFunc) {
        it.second.clear();
    }

    loadStyleDefaults();

    for (auto& it : m_styleTree) {
        auto resNode = (getCustomDefName() == it) ? m_customStyleNode.get() : m_sharedRes->res->findNode(it);
        if (!resNode) {
            continue;
        }

        // get styles for state::none
        updateStyleIt(resNode, state::none, it);

        // if there are subdefinitions and the corresponding flags are set,
        // return those definitions
        if (!resNode->m_node.empty()) {
            ResNode* auxResNode = nullptr;
            if ((auxResNode = resNode->findNode("selected"))) {
                updateStyleIt(auxResNode, state::selected, it);
            }
            if ((auxResNode = resNode->findNode("highlighted"))) {
                updateStyleIt(auxResNode, state::highlighted, it);
            }
            if ((auxResNode = resNode->findNode("disabled"))) {
                updateStyleIt(auxResNode, state::disabled, it);
            }
            if ((auxResNode = resNode->findNode("disabledSelected"))) {
                updateStyleIt(auxResNode, state::disabledSelected, it);
            }
            if ((auxResNode = resNode->findNode("disabledHighlighted"))) {
                updateStyleIt(auxResNode, state::disabledHighlighted, it);
            }
        }
    }

    // execute style functions for the none state to set default values
    auto tmpState = m_state;

    // need to change m_state to none temporarily in order to allow setters take effect
    if (m_state == state::selected) {
        setSelected(false, true);  // call the method instead of setting the variable directly, in order to respect
    }
    // modifications of this method in derived classes, force a style update

    // set state to none and update styles
    m_state = m_lastState = state::none;
    for (auto& it : m_setStyleFunc[state::none]) {
        it.second();
    }

    // change state back
    m_state = tmpState;

    // in case the actual state is different from none, call those functions to
    // update the styles
    if (m_state != state::none) {
        if (m_state == state::selected) {
            setSelected(true, true);
        } else {
            for (auto& it : m_setStyleFunc[m_state]) {
                it.second();
            }
        }
    }

    // updateDrawData(); // updateStyle is only called in updateMatrIt which
    // causes m_drawParamChanged = true, which causes updateDrawData()

    setChanged(true);  // recursive true
}

void UINode::rebuildCustomStyle() {
    if (m_styleCustDefs.empty()) return;

    // rebuild the stylesheet
    m_custDefStyleSheet.clear();
    for (auto& stateDef : m_styleCustDefs) {
        switch (stateDef.first) {
            case state::selected:
                m_custDefStyleSheet += "selected { \n";
                m_styleChanged = true;
                break;
            case state::highlighted:
                m_custDefStyleSheet += "highlighted { \n";
                m_styleChanged = true;
                break;
            case state::disabled:
                m_custDefStyleSheet += "disabled { \n";
                m_styleChanged = true;
                break;
            case state::disabledSelected:
                m_custDefStyleSheet += "disabledSelected { \n";
                m_styleChanged = true;
                break;
            case state::disabledHighlighted:
                m_custDefStyleSheet += "disabledHighlighted { \n";
                m_styleChanged = true;
                break;
            default: break;
        }

        if (!stateDef.second.empty())
            for (auto& it : stateDef.second) {
                if (it.first == "text") {
                    m_custDefStyleSheet += "\t" + it.first + ":\"" + it.second + "\"\n";
                } else {
                    m_custDefStyleSheet += "\t" + it.first + ":" + it.second + "\n";
                }
            }

        if (stateDef.first == state::selected || stateDef.first == state::highlighted ||
            stateDef.first == state::disabled || stateDef.first == state::disabledHighlighted ||
            stateDef.first == state::disabledSelected) {
            m_custDefStyleSheet += "} \n";
        }
    }

    // convert the collected style definitions (strings) into a local ResNode
    if (!m_custDefStyleSheet.empty()) {
        SrcFile              s(m_glbase);
        std::vector<uint8_t> vp(m_custDefStyleSheet.begin(), m_custDefStyleSheet.end());
        ResNode::Ptr         n = std::make_unique<ResNode>(getCustomDefName(), m_glbase);

        if (s.process(n.get(), vp)) {
            n->preprocess();
            n->process();

            if (n->errList.empty() && n->load() && n->errList.empty()) {
                m_customStyleNode = std::move(n);
            }
        }

        if (m_styleTree.empty() || !m_styleTree.empty() && m_styleTree.front() != getCustomDefName()) {
            m_styleTree.push_front(getCustomDefName());  // must be the first entry
        }

        m_styleChanged = true;
    }
}

void UINode::loadStyleDefaults() {
    m_setStyleFunc[state::none][styleInit::x]         = [this]() { setX((int)0); };
    m_setStyleFunc[state::none][styleInit::y]         = [this]() { setY((int)0); };
    m_setStyleFunc[state::none][styleInit::width]     = [this]() { setWidth(1.f); };
    m_setStyleFunc[state::none][styleInit::height]    = [this]() { setHeight(1.f); };
    m_setStyleFunc[state::none][styleInit::align]     = [this]() { setAlignX(align::left); };
    m_setStyleFunc[state::none][styleInit::valign]    = [this]() { setAlignY(valign::top); };
    m_setStyleFunc[state::none][styleInit::color]     = [this]() { setColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::bkcolor]   = [this]() { setBackgroundColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdColor]  = [this]() { setBorderColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdWidth]  = [this]() { setBorderWidth(0); };
    m_setStyleFunc[state::none][styleInit::brdRadius] = [this]() { setBorderRadius(0); };
    m_setStyleFunc[state::none][styleInit::padding]   = [this]() { setPadding(0, 0, 0, 0); };
    m_setStyleFunc[state::none][styleInit::visible]   = [this]() { setVisibility(true); };
}

void UINode::setSharedRes(UISharedRes* shared) {
    if (shared) {
        m_shCol     = shared->shCol;
        m_objSel    = shared->objSel;
        m_quad      = shared->quad;
        m_sharedRes = shared;
        m_orthoMat  = shared->orthoMat;
        m_shdr      = m_shCol->getUIColBorder();
        m_glbase    = shared->glbase;
    }

    setChangeCb([this] { onResize(); });
}

void UINode::setVisibility(bool val, state st) {
    if (st == state::m_state || st == m_state) {
        m_visible = val;
        if (!m_drawImmediate) {
            reqUpdtTree();
        }

        // object ids won't be updated when not visible - set them all to zero immediately
        if (!val) {
            itrNodes(this, [](UINode* nd) {
                nd->setId(0);
            });
        }
    }

    setStyleInitVal("visible", val ? "true" : "false", st);
}

void UINode::applyStyle() {
    if (!m_excludeFromStyles) {
        // call all style definitions for the highlighted state if there are any
        for (auto& it : m_setStyleFunc[m_state]) {
            it.second();
        }

        m_drawParamChanged = true;

        if (m_uniBlock.isInited()) {
            m_uniBlock.update();
        }

        if (!m_setStyleFunc[m_state].empty()) {
            m_sharedRes->requestRedraw = true;
            setDrawFlag();
        }
    }
}

// Drawing - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void UINode::drawAsRoot(uint32_t* objId) {
#ifdef ARA_DEBUG
    postGLError();
#endif

    m_scissorStack.stack.clear();
    m_scissorStack.stack.emplace_back(0, 0, m_viewPort.z * getPixRatio(), m_viewPort.w * getPixRatio());

    // first update all matrices and calculate the children's bounding boxes
    updtMatrIt(&m_scissorStack);

    // now iterate again through the scenegraph and draw

    // scissor stack must be in hardware pixel coordinates, 0|0 is bottom left
    glScissor(0, 0, (GLsizei)(m_viewPort.z * getPixRatio()), (GLsizei)(m_viewPort.w * getPixRatio()));

    if (m_reqTreeChanged) {
        m_treeChanged    = true;
        m_reqTreeChanged = false;
    }

    if (m_treeChanged && m_sharedRes && m_sharedRes->drawMan) {
        m_sharedRes->drawMan->clear();
        m_sharedRes->drawMan->addSet();
    }

    bool skip = true;
    drawIt(m_scissorStack, objId, m_treeChanged, &skip);

    if (m_treeChanged) {
        m_sharedRes->drawMan->rebuildVaos();
        m_treeChanged = false;
    } else {
        m_sharedRes->drawMan->update();
    }
}

void UINode::updtMatrIt(scissorStack* ss) {
    if (!m_visible && !m_forceInit) {
        return;
    }

    // make sure the node is inited
    if (!m_inited) {
        if (m_parent) {
            // check if there is a correct viewport in the child, if this is not the case, try to set it
            if (m_parent->isViewportValid()) {
                setViewport(m_parent->getViewport());
            }

            // in case a UINode has been added in a constructor but needs the sharedRes, pass it down from the parent here
            if (!getSharedRes() && m_parent->getSharedRes()) {
                setSharedRes(m_parent->getSharedRes());
            }
        }

        // be sure init always done in default state
        setState(state::none);  // sync states

        init();  // the Node may create children in its init function which might depend on other node's size
        // so to be sure the calculation won't turn out wrong, since the
        // depending on nodes might change after init set the node to changed again

        rebuildCustomStyle();

        // restore selected state, if set before init. Also needed to sync states if set like this in UINode derivative
        if (m_lastState != state::none) {
            if (m_lastState == state::selected) {
                setSelected(true);  // sync states
            } else {
                m_state = m_lastState;
            }
        }

        m_inited = true;
    }

    if (m_inited && getWindow() && (getWindow()->resChanged() || m_styleChanged || !m_styleClassInited)) {
        updateStyle();

        m_drawParamChanged = true;

        // in case the window was resize before the res.txt was changed, the styledefaults loaded will still in respect
        // of the initial window size, so if this is called from a res update "rescale" the window
        // TODO: default styles should be updated on resize
        if (m_sharedRes && m_sharedRes->win) {
            auto win = static_cast<UIWindow*>(m_sharedRes->win);
            if (win->resChanged()) {
                win->window_size_callback(win->getSize().x, win->getSize().y);
                m_sharedRes->requestRedraw = true;
            }
        }
    }

    if (ss && !ss->stack.empty() && m_parent) {
        // if the child is excluded from the parents scissoring, go one more step back in the scissor stack
        m_sc = *(ss->stack.end() - (m_excludeFromParentScissoring && m_parent->getScissorChildren() ? 2 : 1));

        if (!m_drawImmediate) {
            m_scIndDraw = m_sc;
            m_scIndDraw /= getPixRatio();  // convert to virtual pixels
            m_scIndDraw.y = m_viewPort[3] - m_scIndDraw.y - m_scIndDraw.w;  // glScissor Origin is bottom left, change to top, left
        }
    }

    updateMatrix();  // calculate the nodes' matrices, must be done for drawing and child bounding box calculation

    // push a viewport to the scissor stack if the Node has the scissorchildren flag set std::round() because straight
    // forward casting to int will produce wrong results...
    if (ss && m_ScissorChildren && m_sc.z != 0.f && m_sc.w != 0.f) {
        // here we get the top left corner, but glScissor needs lower left corner scissor area can't exceed actual scissor bounds
        auto t_nodeViewPortGL = getNodeViewportGL();

        m_scissVp.x = std::max(std::floor(std::round(t_nodeViewPortGL.x + (float)getBorderWidth())) * getPixRatio(), m_sc.x);
        m_scissVp.y = std::max(std::floor(std::round(t_nodeViewPortGL.y + (float)getBorderWidth())) * getPixRatio(), m_sc.y);
        m_scissVp.z = std::floor(std::round(t_nodeViewPortGL.z - getBorderWidth() * 2)) * getPixRatio();
        m_scissVp.w = std::floor(std::round(t_nodeViewPortGL.w - getBorderWidth() * 2)) * getPixRatio();

        ss->stack.emplace_back(
            m_scissVp.x, m_scissVp.y,
            m_scissVp.x + m_scissVp.z > m_sc.z ? std::min(m_scissVp.x + m_scissVp.z, m_sc.x + m_sc.z) - m_scissVp.x
                                               : m_scissVp.z,
            m_scissVp.y + m_scissVp.w > m_sc.w ? std::min(m_scissVp.y + m_scissVp.w, m_sc.y + m_sc.w) - m_scissVp.y
                                               : m_scissVp.w);
    }

    // if we are drawing in indirect mode and an UINode has switched from out-of-bounds to in-bounds or visa-versa,
    // the indirect drawing tree has to be updated
    m_oob = isOutOfParentBounds();
    if (m_isOutOfParentBounds != m_oob) {
        reqUpdtTree();
    }
    m_isOutOfParentBounds = m_oob;

    // calculate a bounding box around all children, in node relative coordinates
    if (m_children.empty()) {
        memset(&m_childBoundBox[0], 0, sizeof(float) * 4);
    } else {
        m_childBoundBox.x = std::numeric_limits<float>::max();
        m_childBoundBox.y = std::numeric_limits<float>::max();
        m_childBoundBox.z = std::numeric_limits<float>::min();
        m_childBoundBox.w = std::numeric_limits<float>::min();
    }

    // continue iterating through the children
    for (auto& it : m_children) {
        it->updtMatrIt(ss);

        // keep track of the children's boundingBox
        if (it->isVisible()) {
            it->rec_ChildrenBoundBox(m_childBoundBox);
        }
    }

    // if the node was pushing a viewport to the scissor stack, remove it here again
    if (ss && m_ScissorChildren && !ss->stack.empty()) {
        ss->stack.pop_back();
    }
}

void UINode::drawIt(scissorStack& ss, uint32_t* objId, bool treeChanged, bool* skip) {
    // draw to the screen and the object map
    if (!m_visible || !m_inited || m_isOutOfParentBounds || m_referenceDrawing) {
        return;
    }

    if (!(*skip)) {
        if (!glm::all(glm::equal(m_sc, ss.active))) {
            ss.active = m_sc;
            if (m_drawImmediate) {
                glScissor((GLint)m_sc.x, (GLint)m_sc.y, (GLint)m_sc.z, (GLint)m_sc.w);
            }
        }

        if (m_objIdMin != *objId && (m_drawImmediate || (!m_drawImmediate && treeChanged))) {
            m_objIdMin = m_objIdMax = *objId;
            m_drawParamChanged      = true;
        }

        // matrices may be changed within a draw loop, so be sure everything is up-to-date
        if (m_inited && m_geoChanged) {
            updateMatrix();
        }

        // if the node is being drawn indirectly, check if the DivData needs to be updated
        if (m_drawParamChanged) {
            updateDrawData();

            // if drawing indirectly with an unchanged tree, update the DrawSet at the position of this specific UINode
            if (!m_drawImmediate && !treeChanged) {
                pushVaoUpdtOffsets();
            }

            m_drawParamChanged = false;
        }

        if ((m_drawImmediate && draw(objId)) || (!m_drawImmediate && treeChanged && drawIndirect(objId))) {
            ++(*objId);  // increase objId
        }

    } else {
        (*skip) = false;
    }

    for (auto& it : m_children) {
        it->drawIt(ss, objId, treeChanged, skip);
    }

    // Note: although the recursive call to drawIt is not the last call, difference in performance is not relevant
}

void UINode::limitDrawVaoToBounds(vector<DivVaoData>::iterator& dIt, vec2& size, vec2& uvDiff, vec4& scIndDraw, vec4& vp) {
    dIt->pos.y *= -1.f;

    bool limit = glm::compAdd(scIndDraw) > 0.f;
    for (int i = 0; i < 2; i++) {
        // convert to pixels and limit to NDC bounds
        dIt->pos[i] = (dIt->pos[i] * 0.5f + 0.5f) * vp[2 + i];

        uvDiff[i] = dIt->pos[i];  // save initial pixel position for later uv correction

        // limit same as glScissor TODO: this causes problems with border-radius, review
        if (limit) {
             dIt->pos[i] = std::min(std::max(scIndDraw[i], dIt->pos[i]), scIndDraw[i] + scIndDraw[i + 2]);
        }

        // calculate relative change for offsetting texture coordinates accordingly
        uvDiff[i] = std::abs(uvDiff[i] - dIt->pos[i]) / size[i];

        // convert back to normalized coordinates
        dIt->pos[i] = (dIt->pos[i] / vp[2 + i]) * 2.f - 1.f;
    }

    dIt->pos.y *= -1.f;
}

void UINode::limitTexCoordsToBounds(float* tc, int stdQuadVertInd, const vec2& tvSize, const glm::vec2& uvDiff) {
    // limit left side of texture view
    if (stdQuadVertInd == 0 || stdQuadVertInd == 2) {
        tc[0] += tvSize.x * uvDiff.x;
    }

    // limit right side of texture view
    if (stdQuadVertInd == 1 || stdQuadVertInd == 3) {
        tc[0] -= tvSize.x * uvDiff.x;
    }

    // limit bottom side of texture view
    if (stdQuadVertInd == 0 || stdQuadVertInd == 1) {
        tc[1] += tvSize.y * uvDiff.y;
    }

    // limit top side of texture view
    if (stdQuadVertInd == 2 || stdQuadVertInd == 3) {
        tc[1] -= tvSize.y * uvDiff.y;
    }
}

void UINode::reqUpdtTree() {
    if (!m_drawImmediate && m_sharedRes && m_sharedRes->win) {
        auto win = static_cast<UIWindow*>(m_sharedRes->win);
        if (win->getRootNode() && win->getRootNode()->getRoot()) {
            win->getRootNode()->getRoot()->reqTreeChanged(true);
        }
    }
}

// HID - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool UINode::isInBounds(glm::vec2& pos) {
    if (!m_visible || m_referenceDrawing) {
        return false;
    }

    // inBounds calculation must respect the parent content-transformation matrices and children bounds
    getWinPos();
    getWinRelSize();

    for (int32_t i = 0; i < 2; i++) {
        m_objItLT[i] = m_winRelPos[i] + std::min(0.f, m_childBoundBox[i]);
        m_objItRB[i] = m_objItLT[i] + std::max(m_winRelSize[i], m_childBoundBox[i + 2] - m_childBoundBox[i]);
    }

    return glm::all(glm::greaterThanEqual(pos, m_objItLT)) && glm::all(glm::lessThanEqual(pos, m_objItRB));
}

bool UINode::contains(UINode* outer, UINode* node) {
    if (!node || !outer) {
        return false;
    }

    glm::vec2 t_contLT{};
    glm::vec2 t_contRB{};

    for (int32_t i = 0; i < 2; i++) {
        t_contLT[i] = node->getWinPos()[i] + std::min(0.f, node->getChildrenBoundBox()[i]);
        t_contRB[i] = m_objItLT[i] + std::max(node->getWinRelSize()[i],
                                              node->getChildrenBoundBox()[i + 2] - node->getChildrenBoundBox()[i]);
    }

    return outer->isInBounds(t_contLT) && outer->isInBounds(t_contLT);
}

bool UINode::objPosIt(ObjPosIt& opi) {
    bool inBounds = (*opi.it)->isInBounds(opi.pos);

    // optional callback for additional ui elements there are not part of the regular tree
    bool foundOot = false;
    if ((*opi.it)->m_outOfTreeObjId) {
        foundOot = (*opi.it)->m_outOfTreeObjId(opi);
        if (foundOot) {
            return false;
        }
    }

    // if in bounds and not excluded from the object map and
    if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && opi.foundTreeLevel <= opi.treeLevel) {
        opi.foundNode      = opi.it->get();
        opi.foundId        = (*opi.it)->getId();
        opi.foundTreeLevel = opi.treeLevel;
        // call hid interaction method on node, can optionally stop iteration
    }

    // if within bounds, not excluded from objMap and no more children -> found it!!
    if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && (*opi.it)->m_children.empty()) {
        return false;
    } else if (inBounds && !(*opi.it)->isExcludedFromObjMap() && (*opi.it)->isVisible() && !(*opi.it)->m_children.empty()) {
        // if within bounds and more children -> step down
        opi.parents.emplace_back(opi.it);
        opi.list = &(*opi.it)->m_children;
        opi.it   = (*opi.it)->m_children.end() - 1;
        opi.treeLevel++;
    } else {
        // try to step towards front
        if (opi.it != opi.list->begin()) {
            --opi.it;
        } else {
            // if this is the front element, try to go one level up
            if (opi.parents.empty() || !(*opi.parents.back())->getParent()) {
                return false;
            }

            opi.list = &(*opi.parents.back())->getParent()->m_children;
            opi.it   = opi.parents.back();
            opi.parents.pop_back();
            opi.treeLevel--;

            // try to step towards front
            if (opi.it == opi.list->begin()) {
                return false;
            }

            --opi.it;
        }
    }
    return objPosIt(opi);
}

void UINode::hidIt(hidData* data, hidEvent evt, std::list<UINode*>::iterator it, std::list<UINode*>& tree) {
    data->hit = (*it)->containsObjectId(data->objId)
               && (*it)->getState() != state::disabled
               && (*it)->getState() != state::disabledSelected
               && (*it)->getState() != state::disabledHighlighted
               && !(*it)->isHIDBlocked();

    if (data->hit && evt == hidEvent::MouseDownLeft) {
        data->actIcon = (*it)->ui_MouseIcon;

        // proc input focus
        if ((*it)->getWindow()) {
            (*it)->getWindow()->procInputFocus();
        }
    }

    // calculate the mouse position relative to this node
    data->mousePosNodeRel = data->mousePos - (*it)->getWinPos();  // virtual pixels

    switch (evt) {
        case hidEvent::MouseDownLeft:
            (*it)->mouseDown(data);
            break;
        case hidEvent::MouseUpLeft:
            (*it)->mouseUp(data);
            break;
        case hidEvent::MouseDownRight:
            (*it)->mouseDownRight(data);
            break;
        case hidEvent::MouseUpRight:
            (*it)->mouseUpRight(data);
            break;
        case hidEvent::MouseMove:
            (*it)->mouseMove(data);
            break;
        case hidEvent::MouseDrag:
            (*it)->mouseDrag(data);
            break;
        case hidEvent::MouseWheel:
            (*it)->mouseWheel(data);
            break;
        default: break;
    }

    // process callbacks
    for (auto& cbIt : (*it)->getMouseHidCb(evt)) {
        if (!cbIt.second || (cbIt.second && (data->hit || evt == hidEvent::MouseDrag))) {
            cbIt.first(data);
            if (data->breakCbIt) {
                return;
            }
        }
    }

    // if hit, store that node and procInput focus if left Mouse button was clicked
    if (data->consumed) {
        data->hitNode[evt] = *it;
        return;
    }

    // go up one hierarchy, if we are at the end of the list, stop
    it++;
    if (it == tree.end()) {
        return;
    }

    UINode::hidIt(data, evt, it, tree);
}

void UINode::keyDownIt(hidData* data) {
    if (!m_blockHID && m_state != state::disabled && m_state != state::disabledSelected &&
        m_state != state::disabledHighlighted) {
        keyDown(data);
    }

    for (auto& it : m_children) {
        it->keyDownIt(data);
    }
}

void UINode::onCharIt(hidData* data) {
    if (!m_blockHID && m_state != state::disabled && m_state != state::disabledSelected &&
        m_state != state::disabledHighlighted) {
        onChar(data);
    }

    for (auto& it : m_children) {
        it->onCharIt(data);
    }
}

void UINode::mouseIn(hidData* data) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    if (!m_excludeFromStyles) {
        // set state only in case there are styledefinitions for it. If this is
        // not the case, we assume that this Node should ignore this state
        // setState(state::highlighted, true);
        if (!m_setStyleFunc[state::highlighted].empty()) {
            setState(state::highlighted);
        }

        // call all style definitions for the highlighted state if there are any
        for (auto& it : m_setStyleFunc[state::highlighted]) {
            it.second();
        }

        if (!m_setStyleFunc[state::highlighted].empty()) {
            m_sharedRes->requestRedraw = true;
        }
    }

    for (auto& it : m_mouseInCb) it.second(data);
}

void UINode::mouseOut(hidData* data) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    state lastState  = m_lastState;
    bool  changeBack = !m_setStyleFunc[state::highlighted].empty() && m_state != state::selected;

    // if the last state has no highlighted style definitions, the state didn't
    // change, so no need to change it back
    if (changeBack) {
        setState(m_lastState);
    }

    // change styles back to last state if necessary
    if (!m_excludeFromStyles && changeBack) {
        for (auto& it : m_setStyleFunc[lastState]) {
            it.second();
        }

        // takes about 90 microseconds for 16 lambdas in debug or 10
        // microseconds in release
        if (changeBack) {
            m_sharedRes->requestRedraw = true;
        }
    }

    for (auto& it : m_mouseOutCb) {
        it.second(data);
    }
}

void UINode::addMouseHidCb(hidEvent evt, const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[evt].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseClickCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDownLeft].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseClickRightCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDownRight].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseUpCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseUpLeft].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseUpRightCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseUpRight].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseDragCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDrag].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseMoveCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseMove].emplace_back(std::make_pair(func, onHit));
}

void UINode::addMouseWheelCb(const std::function<void(hidData*)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseWheel].emplace_back(std::make_pair(func, onHit));
}

void UINode::clearMouseCb(hidEvent evt) {
    m_mouseHidCb[evt].clear();
}

void UINode::addMouseInCb(std::function<void(hidData*)> func, state st) {
    m_mouseInCb[st] = std::move(func);
}

void UINode::addMouseOutCb(std::function<void(hidData*)> func, state st) {
    m_mouseOutCb[st] = std::move(func);
}

UINode* UINode::addChild(std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::addChild failed, child empty!";
        return nullptr;
    }
    m_children.emplace_back(std::move(child));
    reqTreeChanged(true);
    init_child(m_children.back().get(), this);
    return m_children.back().get();
}

UINode* UINode::insertChild(int position, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertChild failed, child empty!";
        return nullptr;
    }
    auto it = m_children.insert(m_children.begin() + position, std::move(child));
    reqTreeChanged(true);
    init_child(it->get(), this);
    return it->get();
}

UINode* UINode::insertAfter(const std::string& name, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertAfter failed, child empty!";
        return nullptr;
    }

    auto r = std::find_if(m_children.begin(), m_children.end(), [&name](auto& it){
        return name == it->getName();
    });

    if (r != m_children.end() && static_cast<int>(std::distance(m_children.begin(), r) +1) <= static_cast<int>(m_children.size())) {
        auto it = m_children.insert(std::next(r, 1), std::move(child));
        reqTreeChanged(true);
        init_child(it->get(), this);
        return it->get();
    } else {
        return nullptr;
    }
}

UINode* UINode::insertChild(const std::string& name, std::unique_ptr<UINode>&& child) {
    if (!child) {
        LOGE << "UINode::insertAfter failed, child empty!";
        return nullptr;
    }

    auto r = std::find_if(m_children.begin(), m_children.end(), [&name](auto& it){
        return name == it->getName();
    });

    if (r != m_children.end()) {
        auto it = m_children.insert(r, std::move(child));
        reqTreeChanged(true);
        init_child(it->get(), this);
        return it->get();
    } else {
        return nullptr;
    }
}

void UINode::moveChildTo(int position, UINode* node) {
    if (!node->getParent()) {
        return;
    }

    auto nodeIt = std::find_if(node->getParent()->getChildren().begin(), node->getParent()->getChildren().end(),
                               [node](const std::unique_ptr<UINode>& it) { return node == it.get(); });
    if (nodeIt == node->getParent()->getChildren().end()) {
        return;
    }

    m_children.insert(m_children.begin() + position, std::make_move_iterator(node->getParent()->getChildren().begin()),
                      std::make_move_iterator(node->getParent()->getChildren().begin() + 1));

    node->getParent()->getChildren().erase(nodeIt, nodeIt + 1);

    reqTreeChanged(true);
    (*nodeIt)->setParent(this);
}

void UINode::remove_child(UINode* node) {
    // be sure the node to delete is really a child of this node
    auto it = std::find_if(m_children.begin(), m_children.end(),
                           [node](const unique_ptr<UINode>& n) { return n.get() == node; });

    if (it != m_children.end()) {
        (*it)->removeFocus();
        m_children.erase(it);
    }

    reqTreeChanged(true);
}

bool UINode::removeFocus() {
    if (getWindow() && getWindow()->getInputFocusNode())
        if (getWindow()->getInputFocusNode() == this) {
            this->onLostFocus();
            return true;
        }

    // must be done recursively
    for (auto& it : m_children) it->removeFocus();

    return false;
}

void UINode::clearChildren() {
    removeFocus();

    reqTreeChanged(true);
    m_children.clear();
}

void UINode::init_child(UINode* child, UINode* parent) {
    // check if viewport is initialised
    if (isViewportValid()) {
        child->setViewport(getViewport());
    }

    child->setSharedRes(parent->getSharedRes());
    child->setParent(parent);
}

void UINode::addGlCb(const std::string& identifier, const std::function<bool()>& f) {
    if (m_sharedRes) {
        ((UIWindow*)m_sharedRes->win)->addGlCb(this, identifier, f);
    }
}

void UINode::eraseCb(const std::string& identifier) {
    ((UIWindow*)m_sharedRes->win)->eraseGlCb(this, identifier);
}

bool UINode::hasCb(const std::string& identifier) {
    return ((UIWindow*)m_sharedRes->win)->hasCb(this, identifier);
}

uint32_t UINode::getMinChildId(uint32_t minId) {
    auto nextMinId = m_objIdMin ? std::min(m_objIdMin, minId) : minId;

    for (auto& child : m_children) {
        if (child->getMinId() < nextMinId) {
            nextMinId = child->getMinId();
        }
        nextMinId = child->getMinChildId(nextMinId);
    }

    return nextMinId;
}

uint32_t UINode::getMaxChildId(uint32_t maxId) {
    auto nextMaxId = std::max(m_objIdMax, maxId);

    for (auto& child : m_children) {
        if (child->getMaxId() > nextMaxId) {
            nextMaxId = child->getMaxId();
        }
        nextMaxId = child->getMaxChildId(nextMaxId);
    }

    return nextMaxId;
}

UINode* UINode::getNodeById(uint32_t searchID) {
    for (auto& child : m_children) {
        if (child->getId() == searchID) {
            return child.get();
        } else {
            auto cN = child->getNodeById(searchID);
            if (cN) {
                return cN;
            }
        }
    }
    return nullptr;
}

/** \brief scale the content of this View. On zoom the actual visible center stays the same **/
void UINode::setZoomNormMat(float val) {
    setContentTransScale(val, val);
    setChanged(true);
}

/** \brief scale the content of this View, center onto actual mouse coordinates (must be in window relative pixels) **/
void UINode::setZoomWithCenter(float val, glm::vec2& actMousePos) {
    // convert absolut window relative mousePos to node relative mouse pos
    auto t_vec2 = vec2(glm::inverse(m_contentTransMatRel) * vec4(actMousePos - getWinPos(), 0.f, 1.f));

    setContentTransScale(val, val);
    calcContentTransMat();

    auto newAbsMousePos = vec2(m_contentTransMatRel * vec4(t_vec2, 0.f, 1.f)) + getWinPos();
    auto newMouseOffs   = newAbsMousePos - actMousePos;
    newMouseOffs /= vec2(m_contentTransScale);

    bool lastContTransMode = m_contTransMatCentered;
    m_contTransMatCentered = false;

    setContentTransTransl(m_contentTransMatTransl.x - newMouseOffs.x, m_contentTransMatTransl.y - newMouseOffs.y);

    m_contTransMatCentered = lastContTransMode;
}

void UINode::dump() {
    LOG << "----------- UINode Tree: -----------";
    int depth = 0;
    dumpIt(this, &depth, true);
}

void UINode::dumpIt(UINode* node, int* depth, bool dumpLocalTree) {
    string pr;
    string state;

    switch (node->getState()) {
        case state::none: state = "none"; break;
        case state::selected: state = "selected"; break;
        case state::disabled: state = "disabled"; break;
        case state::disabledSelected: state = "disabledSelected"; break;
        case state::disabledHighlighted: state = "disabledHighlighted"; break;
        case state::m_state: state = "m_state"; break;
        case state::highlighted: state = "highlighted"; break;
        default: break;
    }

    for (auto i = 0; i < *depth; i++) pr += "\t";
    LOG << pr << "[" << node->m_name.c_str() << "], \tvisible: " << node->isVisible() << ", \tstate: " << state
        << (node->isVisible()
                ? " \tid: [ min: " + std::to_string(node->getMinId()) + " max: " + std::to_string(node->getMaxId()) +
                      (node->isExcludedFromObjMap() ? " EXCLUDED"
                       : !node->isVisible()         ? " INVISIBLE"
                                                    : "") +
                      " ]"
                : "")
        << ", \tpos:" << glm::to_string(node->m_pos) << ", \tsize:" << glm::to_string(node->m_size);

    (*depth)++;
    for (auto& it : node->m_children) {
        dumpIt(it.get(), depth, dumpLocalTree);
    }
    (*depth)--;
}

uint32_t UINode::getSubNodeCount() {
    uint32_t count = 0;
    getSubNodeCountIt(this, &count);
    return count;
}

void UINode::getSubNodeCountIt(UINode* node, uint32_t* count) {
    for (auto& it : node->m_children) {
        (*count)++;
        getSubNodeCountIt(it.get(), count);
    }
}

UIWindow* UINode::getWindow() {
    return m_sharedRes ? static_cast<UIWindow*>(m_sharedRes->win) : nullptr;
}

UIApplication* UINode::getApp() {
    return (m_sharedRes && m_sharedRes->win) ? static_cast<UIWindow*>(m_sharedRes->win)->getApplicationHandle()
                                             : nullptr;
}

float UINode::getPixRatio() {
    return m_sharedRes ? static_cast<UIWindow*>(m_sharedRes->win)->getPixelRatio() : 1.f;
}

void UINode::setHIDBlocked(bool val) {
    m_blockHID = val;
    for (auto& it : m_children) {
        if (it) {
            it->setHIDBlocked(val);
        }
    }
}

void UINode::onLostFocus() {
    if (m_onLostFocusCb) {
        m_onLostFocusCb();
    }
}

void UINode::onGotFocus() {
    if (m_onFocusedCb) {
        m_onFocusedCb();
    }
}

// ------------------------------- RESOURCES -----------------------------------------------

ResNode* UINode::getStyleResNode() {
    return m_sharedRes->res->findNode(m_baseStyleClass);
}

// ---------------------------------- UTIL -------------------------------------------------

void UINode::util_FillRect(ivec2 pos, ivec2 size, vec4 col, Shaders* shdr, Quad* quad) {
    if (shdr == nullptr) {
        shdr = m_shdr;
    }

    if (quad == nullptr) {
        quad = m_quad;
    }

    if (shdr == nullptr || quad == nullptr || size.x <= 0 || size.y <= 0) {
        return;
    }

    auto m = *m_orthoMat * m_parentMatLocCpy * translate(vec3(static_cast<float>(pos.x) + getPos().x, static_cast<float>(pos.y) + getPos().y, 0.f));

    shdr->begin();
    shdr->setUniformMatrix4fv("m_pvm", &m[0][0]);
    shdr->setUniform2f("size", static_cast<float>(size.x), static_cast<float>(size.y));
    shdr->setUniform4f("color", col.r, col.g, col.b, col.a);
    shdr->setUniform2f("borderWidth", 0.f, 0.f);
    shdr->setUniform2f("borderRadius", 0.f, 0.f);

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UINode::util_FillRect(ivec2 pos, ivec2 size, float* color, Shaders* shdr, Quad* quad) {
    return util_FillRect(pos, size, {color[0], color[1], color[2], color[3]}, shdr, quad);
}

void UINode::util_FillRect(glm::ivec4& r, float* color, Shaders* shdr, Quad* quad) {
    return util_FillRect({r.x, r.y}, {r.z, r.w}, {color[0], color[1], color[2], color[3]}, shdr, quad);
}

// node argument refers to a child node
bool UINode::rec_ChildrenBoundBox(glm::vec4& ref) {
    if (!m_visible || m_excludeFromParentContentTrans) {
        return false;
    }

    m_bb.x = !m_children.empty() ? m_pos.x + m_childBoundBox.x : m_pos.x;
    m_bb.y = !m_children.empty() ? m_pos.y + m_childBoundBox.y : m_pos.y;
    m_bb.z = !m_children.empty() ? m_pos.x + std::max<float>(m_size.x, m_childBoundBox.z) : m_pos.x + m_size.x;
    m_bb.w = !m_children.empty() ? m_pos.y + std::max<float>(m_size.y, m_childBoundBox.w) : m_pos.y + m_size.y;

    ref.x = m_bb.x < ref.x ? m_bb.x : ref.x;
    ref.y = m_bb.y < ref.y ? m_bb.y : ref.y;
    ref.z = m_bb.z > ref.z ? m_bb.z : ref.z;
    ref.w = m_bb.w > ref.w ? m_bb.w : ref.w;

    return true;
}

UINode::~UINode() {
    // check if the actual is selected as inputFocusNode, if this is the case,
    // set it to nullptr
    if (m_sharedRes) {
        if (!m_drawImmediate && m_sharedRes->drawMan) {
            m_sharedRes->drawMan->removeUINode(this);
        }

        if (m_sharedRes->win) {
            auto win = static_cast<UIWindow*>(m_sharedRes->win);
            win->removeGlCbs(this);  // remove all callbacks issued by this Node
            win->onNodeRemove(this);
        }
    }
}

}  // namespace ara
