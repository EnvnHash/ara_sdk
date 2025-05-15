//
// Created by user on 5/5/25.
//

#include "UIElements/UINodeBase/UINodeGeom.h"
#include "UIElements/UINodeBase/UINode.h"
#include "UIWindow.h"
#include <GLBase.h>
#include <UISharedRes.h>

using namespace std;
using namespace glm;

namespace ara {

UINodeGeom::UINodeGeom() : m_parentMat(&m_identMat) {
}

bool UINodeGeom::contains(UINodeGeom* outer, UINodeGeom* node) {
    if (!node || !outer) {
        return false;
    }

    vec2 contLT{};
    vec2 contRB{};

    for (int32_t i = 0; i < 2; i++) {
        contLT[i] = node->getWinPos()[i] + std::min(0.f, node->getChildrenBoundBox()[i]);
        contRB[i] = contLT[i] + std::max(node->getWinRelSize()[i],
                                         node->getChildrenBoundBox()[i + 2] - node->getChildrenBoundBox()[i]);
    }

    return outer->isInBounds(contLT) && outer->isInBounds(contLT);
}

void UINodeGeom::calcContentTransMat() {
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
        m_mvpHw = m_mvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
    }
}

void UINodeGeom::calcNormMat() {
    p[0] = *m_parentMat * vec4(m_pos.x, m_pos.y, 0.f, 1.f);
    p[1] = *m_parentMat * vec4(m_pos.x + m_size.x, m_pos.y + m_size.y, 0.f, 1.f);

    for (int i = 0; i < 2; i++) {
        pN[i]   = vec2(p[i]) / vec2(m_viewPort.z, m_viewPort.w);
        pN[i].y = 1.f - pN[i].y;
        pN[i]   = pN[i] * 2.f - 1.f;
    }

    // to render a normalized quad lb (-1|-1) rt (1|1)
    m_normMat = translate(vec3(pN[1] + (pN[0] - pN[1]) * 0.5f, 0.f)) * scale(vec3(abs(pN[1] - pN[0]) * 0.5f, 1.f));
}

void UINodeGeom::setAlignX(align type, state st) {
    if (st == state::m_state || st == m_state) {
        m_alignX = type;
        m_pivX   = (type == align::left ? pivotX::left : (type == align::right ? pivotX::right : pivotX::center));
        setChanged(true);
    }
    setStyleInitVal("align", type == align::left ? "left" : (type == align::right ? "right" : "center"), st);
}

void UINodeGeom::setAlignY(valign type, state st) {
    if (st == state::m_state || st == m_state) {
        m_alignY = type;
        m_pivY   = (type == valign::top ? pivotY::top : (type == valign::bottom ? pivotY::bottom : pivotY::center));
        setChanged(true);
    }
    setStyleInitVal("v-align", type == valign::top ? "top" : (type == valign::bottom ? "bottom" : "center"), st);
}

void UINodeGeom::setAlign(align alignX, valign alignY, state st) {
    setAlignX(alignX, st);
    setAlignY(alignY, st);
}

void UINodeGeom::setPadding(float left, float top, float right, float bot, state st) {
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

void UINodeGeom::setPadding(vec4& val, state st) {
    if (st == state::m_state || st == m_state) {
        m_padding    = val;
        m_geoChanged = true;
    }
    setStyleInitVal("padding",
                    std::to_string(val[0]) + "," + std::to_string(val[1]) + "," + std::to_string(val[2]) + "," +
                    std::to_string(val[3]), st);
}

void UINodeGeom::setPivotX(pivotX pX) {
    m_pivX       = pX;
    m_geoChanged = true;
}

void UINodeGeom::setPivotY(pivotY pY) {
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINodeGeom::setPivot(pivotX pX, pivotY pY) {
    m_pivX       = pX;
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINodeGeom::setPadding(float val, state st) {
    if (st == state::m_state || st == m_state) {
        m_padding    = vec4(val, val, val, val);
        m_geoChanged = true;
    }
    setStyleInitVal("padding", std::to_string(val), st);
}

void UINodeGeom::setBorderWidth(uint32_t val, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderWidth = val;
        m_geoChanged  = true;
    }
    setStyleInitVal("border-width", std::to_string(val), st);
}

void UINodeGeom::setBorderRadius(uint32_t val, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderRadius = val;
        m_geoChanged   = true;
    }
    setStyleInitVal("border-radius", std::to_string(val), st);
}

void UINodeGeom::setFixAspect(float val) {
    m_fixAspect = val;
    setChanged(true);
}

void UINodeGeom::setViewport(float x, float y, float width, float height) {
    m_viewPort.x = x;
    m_viewPort.y = y;
    m_viewPort.z = width;
    m_viewPort.w = height;
    m_geoChanged = true;
}

void UINodeGeom::setContentTransScale(float x, float y) {
    setChanged(true);  // force children to update
    m_contentTransScale.x          = x;
    m_contentTransScale.y          = y;
    m_contentTransScale.z          = 1.f;
    m_contentTransScaleFixAspect.x = x;
    m_contentTransScaleFixAspect.y = y;
    m_contentTransScaleFixAspect.z = 1.f;
    calcContentTransMat();
}

void UINodeGeom::setContentTransTransl(float x, float y) {
    m_contentTransMatTransl.x = x;
    m_contentTransMatTransl.y = y;
    setChanged(true);  // force children to update
}

void UINodeGeom::setContentRotation(float angle, float ax, float ay, float az) {
    setChanged(true);  // force children to update

    m_contentTransRotate.w = angle;
    m_contentTransRotate.x = ax;
    m_contentTransRotate.y = ay;
    m_contentTransRotate.z = az;

    m_hasContRot = true;
    m_contRot    = rotate(m_contentTransRotate.w, vec3(m_contentTransRotate));
}

void UINodeGeom::setSharedRes(UISharedRes* shared) {
    if (shared) {
        m_sharedRes = shared;
        m_orthoMat  = shared->orthoMat;
    }
}

/** \brief scale the content of this View. On zoom the actual visible center stays the same **/
void UINodeGeom::setZoomNormMat(float val) {
    setContentTransScale(val, val);
    setChanged(true);
}

/** \brief scale the content of this View, center onto actual mouse coordinates (must be in window relative pixels) **/
void UINodeGeom::setZoomWithCenter(float val, vec2& actMousePos) {
    // convert absolut window relative mousePos to node relative mouse pos
    auto t_vec2 = vec2(inverse(m_contentTransMatRel) * vec4(actMousePos - getWinPos(), 0.f, 1.f));

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

vec2 UINodeGeom::getOrigPos() {
    return {m_posXType == unitType::Pixels ? static_cast<float>(m_posXInt) : m_posXFloat,
            m_posYType == unitType::Pixels ? static_cast<float>(m_posYInt) : m_posYFloat};
}

vec2& UINodeGeom::getPos() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_pos;
}

vec2& UINodeGeom::getSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size;
}

vec2& UINodeGeom::getNodeSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size;
}

float UINodeGeom::getNodeWidth() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size.x;
}

float UINodeGeom::getNodeHeight() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_size.y;
}

vec2 UINodeGeom::getNodeRelSize() {
    if (m_geoChanged) {
        updateMatrix();
    }
    m_relSize.x = m_size.x / m_parentContVp.z;
    m_relSize.y = m_size.y / m_parentContVp.w;
    return m_relSize;
}

vec4 UINodeGeom::getNodeViewportGL() {
    getWinPos();
    return {m_winRelPos.x, m_viewPort.w - (m_winRelPos.y + m_size.y), m_size.x, m_size.y};
}

vec4 UINodeGeom::getContentViewport() {
    getContWinPos();
    getContentSize();
    return {m_contWinPos.x, m_contWinPos.y, m_contentSize.x, m_contentSize.y};
}

vec4 UINodeGeom::getNodeViewport() {
    getWinPos();
    getNodeSize();
    return {m_winRelPos.x, m_winRelPos.y, m_size.x, m_size.y};
}

float* UINodeGeom::getMVPMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvp[0][0];
}

mat4* UINodeGeom::getMvp() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvp;
}

mat4* UINodeGeom::getHwMvp() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_mvpHw;
}

float* UINodeGeom::getWinRelMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_winRelMat[0][0];
}

mat4* UINodeGeom::getWinRelMat() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return &m_winRelMat;
}

mat4* UINodeGeom::getNormMat() {
    if (m_geoChanged) {
        updateMatrix();
    }
    calcNormMat();
    return &m_normMat;
}

float* UINodeGeom::getNormMatPtr() {
    if (m_geoChanged) {
        updateMatrix();
    }
    calcNormMat();
    return &m_normMat[0][0];
}

vec2& UINodeGeom::getParentContentScale() {
    if (m_geoChanged) {
        updateMatrix();
    }
    return m_parentContScale;
}

mat4* UINodeGeom::getContentMat(bool excludedFromParentContentTrans, bool excludedFromPadding) {
    return excludedFromPadding ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                               : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);
}

mat4* UINodeGeom::getFlatContentMat(bool excludedFromParentContentTrans, bool excludedFromPadding) {
    auto mat = excludedFromPadding ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                                   : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);

    // don't use the dynamic m_parentMat ptr since here it doesn't contain the
    // information of all other parentMats use instead the local copy, which is
    // safe to do, since this is only called after this node's updateMatrix()
    m_flatContentTransMat = m_parentMatLocCpy * *mat;

    return &m_flatContentTransMat;
}

vec2& UINodeGeom::getParentNodeRelPos() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the node's position relative to the parent node's upper left corner in pixels
    if (getParent()) {
        auto parent           = dynamic_cast<UINodeGeom*>(getParent());
        auto wp              = getWinPos();
        m_parentNodeRelPos.x = wp.x - parent->getWinPos().x;
        m_parentNodeRelPos.y = wp.y - parent->getWinPos().y;
    }

    return m_parentNodeRelPos;
}

vec2& UINodeGeom::getWinPos() {
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

vec2& UINodeGeom::getWinRelSize() {
    if (m_geoChanged) {
        updateMatrix();
    }

    if (m_parentMat) {
        for (int i = 0; i < 2; i++) {
            m_winRelSize[i] = m_parentMatLocCpy[i][i] * m_size[i];
        }
    } else {
        m_winRelSize = m_size;
    }

    return m_winRelSize;
}

vec2& UINodeGeom::getContWinPos() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // this node's content's left/top corner in relation to the window's top/left corner
    m_contWinPos.x = m_parentContVp.x + m_pos.x + m_padding.x + static_cast<float>(m_borderWidth);
    m_contWinPos.y = m_parentContVp.y + m_pos.y + m_padding.y + static_cast<float>(m_borderWidth);

    return m_contWinPos;
}

vec2& UINodeGeom::getContentSize() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the size of the node's content area
    m_contentSize.x = m_size.x - (m_padding.x + m_padding.z + static_cast<float>(m_borderWidth) * 2.f);
    m_contentSize.y = m_size.y - (m_padding.y + m_padding.w + static_cast<float>(m_borderWidth) * 2.f);

    return m_contentSize;
}

vec2& UINodeGeom::getContentOffset() {
    if (m_geoChanged) {
        updateMatrix();
    }

    // calculate the size of the node's content area
    m_contentOffset.x = m_padding.x + static_cast<float>(m_borderWidth);
    m_contentOffset.y = m_padding.y + static_cast<float>(m_borderWidth);

    return m_contentOffset;
}

vec2& UINodeGeom::getBorderWidthRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderWidthRel[i] = static_cast<float>(m_borderWidth) / m_size[i] / m_parentContScale[i];
        } else {
            m_borderWidthRel[i] = 0.f;
        }
    }

    return m_borderWidthRel;
}

vec2& UINodeGeom::getBorderRadiusRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderRadiusRel[i] = static_cast<float>(m_borderRadius) / m_size[i] / m_parentContScale[i];
        } else {
            m_borderRadiusRel[i] = 0.f;
        }
    }

    return m_borderRadiusRel;
}

vec2& UINodeGeom::getBorderAliasRel() {
    if (m_geoChanged) {
        updateMatrix();
    }

    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderAliasRel[i] = static_cast<float>(m_borderAlias) / m_size[i] / m_parentContScale[i];
        } else {
            m_borderAliasRel[i] = 0.f;
        }
    }

    return m_borderAliasRel;
}

float UINodeGeom::getPixRatio() const {
    return m_sharedRes ? static_cast<UIWindow*>(m_sharedRes->win)->getPixelRatio() : 1.f;
}

bool UINodeGeom::isInBounds(vec2& pos) {
    // inBounds calculation must respect the parent content-transformation matrices and children bounds
    getWinPos();
    getWinRelSize();

    vec2 objItLT{};
    vec2 objItRB{};

    for (int32_t i = 0; i < 2; i++) {
        objItLT[i] = m_winRelPos[i] + std::min(0.f, m_childBoundBox[i]);
        objItRB[i] = objItLT[i] + std::max(m_winRelSize[i], m_childBoundBox[i + 2] - m_childBoundBox[i]);
    }

    return all(greaterThanEqual(pos, objItLT)) && all(lessThanEqual(pos, objItRB));
}

bool UINodeGeom::isOutOfParentBounds() {
    if (!getParent() || m_skipBoundCheck) {
        return false;
    }
    auto parent = dynamic_cast<UINodeGeom*>(getParent());
    return glm::any(greaterThan(m_parentNodeRelPos, parent->m_size)) ||
           glm::any(lessThan(m_parentNodeRelPos + m_size, {}));
}



}