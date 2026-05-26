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

void UINodeGeom::updateContentTransMat() {
    if (m_limitContentTrans) {
        const auto overflow = getContentTransOverflow(m_contentTransMatTransl.x, m_contentTransMatTransl.y);
        m_contentTransMatTransl.x += overflow.x;
        m_contentTransMatTransl.y += overflow.y;
    }

    calcContentTransMat(m_contentTransMatRel, m_contentTransMatTransl);

    m_contentTransMat = m_contentMat * m_contentTransMatRel;
    m_nodeTransMat    = m_nodeMat * m_contentTransMatRel;

    m_nodePosMat[3][0] = m_pos.x;
    m_nodePosMat[3][1] = m_pos.y;

    if (m_excludeSizeFromParentContentTrans) {
        const auto size = m_size;
        m_size /= m_parentContScale;
        m_nodePosMat[3][0] += (size.x - m_size.x) * 0.5f;
        m_nodePosMat[3][1] += (size.y - m_size.y) * 0.5f;
    }

    // apply the window orthographic matrix, this matrix will be used for rendering
    if (m_orthoMat) {
        m_mvp   = *m_orthoMat * m_parentMatLocCpy * m_nodePosMat; // this is expensive
        m_mvpHw = m_mvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
    }
}

void UINodeGeom::calcContentTransMat(mat4& mat, const vec3& trans) const {
    if (m_contTransMatCentered) {
        // for scaling, center the content to the origin, scale and move back to the initial position
        mat = m_contRot;

        mat[3][0] += trans.x + m_size.x * -0.5f;
        mat[3][1] += trans.y + m_size.y * -0.5f;
        mat[3][2] += trans.z;

        mat = scale(m_contentTransScaleFixAspect) * mat;

        mat[3][0] += m_size.x * 0.5f;
        mat[3][1] += m_size.y * 0.5f;

    } else {
        // this is expensive
        if (m_hasContRot) {
            mat = translate(trans) * m_contRot * scale(m_contentTransScale);
        } else {
            mat = translate(trans) * scale(m_contentTransScale);
        }
    }
}

void UINodeGeom::calcNormMat() {
    const std::array p{
        *m_parentMat * vec4(m_pos.x, m_pos.y, 0.f, 1.f),
        *m_parentMat * vec4(m_pos.x + m_size.x, m_pos.y + m_size.y, 0.f, 1.f)
    };

    std::array<vec2, 2> pN{};
    for (int i = 0; i < 2; i++) {
        pN[i]   = vec2(p[i]) / vec2(m_viewPort.z, m_viewPort.w);
        pN[i].y = 1.f - pN[i].y;
        pN[i]   = pN[i] * 2.f - 1.f;
    }

    // to render a normalized quad lb (-1|-1) rt (1|1)
    m_normMat = translate(vec3(pN[1] + (pN[0] - pN[1]) * 0.5f, 0.f)) * scale(vec3(abs(pN[1] - pN[0]) * 0.5f, 1.f));
}

void UINodeGeom::setAlignX(const align type, const state st) {
    if (st == state::m_state || st == m_state) {
        m_alignX = type;
        m_pivX   = (type == align::left ? pivotX::left : (type == align::right ? pivotX::right : pivotX::center));
        setChanged(true);
    }
    setStyleInitVal("align", type == align::left ? "left" : (type == align::right ? "right" : "center"), st);
}

void UINodeGeom::setAlignY(const valign type, const state st) {
    if (st == state::m_state || st == m_state) {
        m_alignY = type;
        m_pivY   = (type == valign::top ? pivotY::top : (type == valign::bottom ? pivotY::bottom : pivotY::center));
        setChanged(true);
    }
    setStyleInitVal("v-align", type == valign::top ? "top" : (type == valign::bottom ? "bottom" : "center"), st);
}

void UINodeGeom::setAlign(const align alignX, const valign alignY, const state st) {
    setAlignX(alignX, st);
    setAlignY(alignY, st);
}

void UINodeGeom::setPivotX(const pivotX pX) {
    m_pivX       = pX;
    m_geoChanged = true;
}

void UINodeGeom::setPivotY(const pivotY pY) {
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINodeGeom::setPivot(const pivotX pX, const pivotY pY) {
    m_pivX       = pX;
    m_pivY       = pY;
    m_geoChanged = true;
}

void UINodeGeom::setBorderWidth(const uint32_t val, const state st) {
    if (st == state::m_state || st == m_state) {
        m_borderWidth = val;
        m_geoChanged  = true;
    }
    setStyleInitVal("border-width", std::to_string(val), st);
}

void UINodeGeom::setBorderRadius(const uint32_t val, const state st) {
    if (st == state::m_state || st == m_state) {
        m_borderRadius = val;
        m_geoChanged   = true;
    }
    setStyleInitVal("border-radius", std::to_string(val), st);
}

void UINodeGeom::setFixAspect(const float val) {
    m_fixAspect = val;
    setChanged(true);
}

void UINodeGeom::setViewport(const float x, const float y, const float width, const float height) {
    m_viewPort.x = x;
    m_viewPort.y = y;
    m_viewPort.z = width;
    m_viewPort.w = height;
    m_geoChanged = true;
}

void UINodeGeom::setContentTransScale(const float x, const float y) {
    setChanged(true);  // force children to update
    m_contentTransScale.x          = x;
    m_contentTransScale.y          = y;
    m_contentTransScale.z          = 1.f;
    m_contentTransScaleFixAspect.x = x;
    m_contentTransScaleFixAspect.y = y;
    m_contentTransScaleFixAspect.z = 1.f;
    updateContentTransMat();
}

void UINodeGeom::setContentTransTransl(const float x, const float y) {
    m_contentTransMatTransl.x = x;
    m_contentTransMatTransl.y = y;
    setChanged(true);  // force children to update
}

vec2 UINodeGeom::getContentTransOverflow(const float x, const float y) {
    vec2 overflow{};
    const std::array quadCorners {
        vec4{ 0.f, 0.f, 0.f, 1.f },
        vec4{ m_size.x, m_size.y, 0.f, 1.f },
    };

    mat4 mat(1.f);
    calcContentTransMat(mat, vec3{x, y, m_contentTransMatTransl.z});

    for (int i=0; i<quadCorners.size(); ++i) {
        auto newCorn = mat * quadCorners[i];
        if (i == 0) {
            for (int j=0; j<2; ++j) {
                overflow[j] = std::min(-newCorn[j], overflow[j]);
            }
        } else {
            for (int j=0; j<2; ++j) {
                overflow[j] = std::max(m_size[j] - newCorn[j], overflow[j]);
            }
        }
    }

    overflow /= vec2(m_contTransMatCentered ? m_contentTransScaleFixAspect : m_hasContRot ? m_contentTransScale : vec2{1.f, 1.f});
    return overflow;
}

void UINodeGeom::setContentRotation(const float angle, const float ax, const float ay, const float az) {
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
void UINodeGeom::setZoomNormMat(const float val) {
    setContentTransScale(val, val);
    setChanged(true);
}

/** \brief scale the content of this View, center onto actual mouse coordinates (must be in window relative pixels) **/
    void UINodeGeom::setZoomWithCenter(const float val, vec2& actMousePos) {
    if (epsilonEqual(val, m_contentTransScale.x, 0.0001f)
        && epsilonEqual(val, m_contentTransScale.y, 0.0001f)) {
        return;
    }

    const vec2 winPos = getWinPos();

    // convert absolut window relative mousePos to node relative mouse pos
    const auto t_vec2 = vec2(inverse(m_contentTransMatRel) * vec4(actMousePos - winPos, 0.f, 1.f));

    setContentTransScale(val, val);
    updateContentTransMat();

    const auto newAbsMousePos = vec2(m_contentTransMatRel * vec4(t_vec2, 0.f, 1.f)) + winPos;
    auto newMouseOffs   = newAbsMousePos - actMousePos;
    newMouseOffs /= vec2(m_contentTransScale);

    const bool lastContTransMode = m_contTransMatCentered;
    m_contTransMatCentered = false;

    setContentTransTransl(m_contentTransMatTransl.x - newMouseOffs.x, m_contentTransMatTransl.y - newMouseOffs.y);

    m_contTransMatCentered = lastContTransMode;
}

vec2 UINodeGeom::getOrigPos() {
    return {m_posXType == unitType::Pixels ? static_cast<float>(m_posXInt) : m_posXFloat,
            m_posYType == unitType::Pixels ? static_cast<float>(m_posYInt) : m_posYFloat};
}

void UINodeGeom::checkUpdateMatrix() {
    if (m_geoChanged) {
        updateMatrix();
    }
}

vec2& UINodeGeom::getPos() {
    checkUpdateMatrix();
    return m_pos;
}

vec2& UINodeGeom::getAlignedPos() {
    checkUpdateMatrix();
    return m_preAlignPos;
}

vec2& UINodeGeom::getSize() {
    checkUpdateMatrix();
    return m_size;
}

vec2& UINodeGeom::getNodeSize() {
    checkUpdateMatrix();
    return m_size;
}

float UINodeGeom::getNodeWidth() {
    checkUpdateMatrix();
    return m_size.x;
}

float UINodeGeom::getNodeHeight() {
    checkUpdateMatrix();
    return m_size.y;
}

vec2 UINodeGeom::getNodeRelSize() {
    checkUpdateMatrix();
    m_relSize.x = m_size.x / m_parentContVp.z;
    m_relSize.y = m_size.y / m_parentContVp.w;
    return m_relSize;
}

vec4 UINodeGeom::getNodeViewportGL() {
    getWinPos();

    const auto uiWin = static_cast<UIWindow*>(getSharedRes()->win);
    std::array qc{glm::vec4{0.f, 0.f, 0.f, 1.f}, glm::vec4{m_size.x, m_size.y, 0.f, 1.f}};
    for (auto & i : qc) {
        i = m_mvp * i;
        for (auto j=0; j<2; ++j) {
            i[j] = (i[j] * 0.5f + 0.5f) * uiWin->getSize()[j];
        }
    }
    return {qc[0].x, qc[1].y, qc[1].x - qc[0].x, qc[0].y - qc[1].y};
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
    checkUpdateMatrix();
    return &m_mvp[0][0];
}

mat4* UINodeGeom::getMvp() {
    checkUpdateMatrix();
    return &m_mvp;
}

mat4* UINodeGeom::getHwMvp() {
    checkUpdateMatrix();
    return &m_mvpHw;
}

float* UINodeGeom::getWinRelMatPtr() {
    checkUpdateMatrix();
    return &m_winRelMat[0][0];
}

mat4* UINodeGeom::getWinRelMat() {
    checkUpdateMatrix();
    return &m_winRelMat;
}

mat4* UINodeGeom::getNormMat() {
    checkUpdateMatrix();
    calcNormMat();
    return &m_normMat;
}

float* UINodeGeom::getNormMatPtr() {
    checkUpdateMatrix();
    calcNormMat();
    return &m_normMat[0][0];
}

vec2& UINodeGeom::getParentContentScale() {
    checkUpdateMatrix();
    return m_parentContScale;
}

mat4* UINodeGeom::getContentMat(const bool excludedFromParentContentTrans, const bool excludedFromPaddingAndBorder) {
    return excludedFromPaddingAndBorder ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                               : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);
}

mat4* UINodeGeom::getFlatContentMat(const bool excludedFromParentContentTrans, const bool excludedFromPaddingAndBorder) {
    const auto mat = excludedFromPaddingAndBorder ? (!excludedFromParentContentTrans ? &m_nodeTransMat : &m_nodeMat)
                                        : (!excludedFromParentContentTrans ? &m_contentTransMat : &m_contentMat);

    // don't use the dynamic m_parentMat ptr since here it doesn't contain the information of all other parentMats use
    // instead the local copy, which is safe to do, since this is only called after this node's updateMatrix()
    m_flatContentTransMat = m_parentMatLocCpy * *mat;
    return &m_flatContentTransMat;
}

vec2& UINodeGeom::getParentNodeRelPos() {
    checkUpdateMatrix();
    // calculate the node's position relative to the parent node's upper left corner in pixels
    if (getParent()) {
        const auto parent         = dynamic_cast<UINodeGeom*>(getParent());
        const auto wp        = getWinPos();
        m_parentNodeRelPos.x = wp.x - parent->getWinPos().x;
        m_parentNodeRelPos.y = wp.y - parent->getWinPos().y;
    }

    return m_parentNodeRelPos;
}

vec2& UINodeGeom::getWinPos() {
    checkUpdateMatrix();
    if (m_parentMat) {
        m_parentTransPos = vec2(*m_parentMat * vec4(m_pos, 0.f, 1.f));
        m_winRelPos      = m_parentTransPos;
    } else {
        m_winRelPos = m_pos;
    }

    return m_winRelPos;
}

vec2& UINodeGeom::getWinRelSize() {
    checkUpdateMatrix();
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
    checkUpdateMatrix();
    // this node's content's left/top corner in relation to the window's top/left corner
    m_contWinPos.x = m_parentContVp.x + m_pos.x + m_padding.x + static_cast<float>(m_borderWidth);
    m_contWinPos.y = m_parentContVp.y + m_pos.y + m_padding.y + static_cast<float>(m_borderWidth);

    return m_contWinPos;
}

vec2& UINodeGeom::getContentSize() {
    checkUpdateMatrix();
    // calculate the size of the node's content area
    m_contentSize.x = m_size.x - (m_padding.x + m_padding.z + static_cast<float>(m_borderWidth) * 2.f);
    m_contentSize.y = m_size.y - (m_padding.y + m_padding.w + static_cast<float>(m_borderWidth) * 2.f);

    return m_contentSize;
}

vec2& UINodeGeom::getContentOffset() {
    checkUpdateMatrix();
    // calculate the size of the node's content area
    m_contentOffset.x = m_padding.x + static_cast<float>(m_borderWidth);
    m_contentOffset.y = m_padding.y + static_cast<float>(m_borderWidth);

    return m_contentOffset;
}

vec2& UINodeGeom::getBorderWidthRel() {
    checkUpdateMatrix();
    for (int i = 0; i < 2; i++) {
        if (m_size[i] != 0.f && m_parentContScale[i] != 0.f) {
            m_borderWidthRel[i] = static_cast<float>(m_borderWidth) / m_size[i] / (m_staticBorderSize ? m_parentContScale[i] : 1.f); // borderWidth is zoom independent
        } else {
            m_borderWidthRel[i] = 0.f;
        }
    }

    return m_borderWidthRel;
}

vec2& UINodeGeom::getBorderRadiusRel() {
    checkUpdateMatrix();
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
    checkUpdateMatrix();
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

    const auto contentTransform = m_parentMatLocCpy * m_nodePosMat;
    auto absChildBbLT = contentTransform * vec4{ m_childBoundBox.x, m_childBoundBox.y, 0.f, 1.f };
    auto absChildBbRB = contentTransform * vec4{ m_childBoundBox.z, m_childBoundBox.w, 0.f, 1.f };

    vec2 objItLT{}, objItRB{};
    for (int32_t i = 0; i < 2; i++) {
        objItLT[i] = std::min(m_winRelPos[i], absChildBbLT[i]);
        objItRB[i] = std::max(m_winRelPos[i] + m_winRelSize[i], absChildBbRB[i]);
    }

    if (!m_excludeFromParentScissoring
        && (m_scIndDraw.z != 0.f || m_scIndDraw.w != 0.f || m_scIndDraw.x != 0.f || m_scIndDraw.y != 0.f)) {
        objItRB = glm::min(objItRB, vec2{m_scIndDraw.x + m_scIndDraw.z, m_scIndDraw.y + m_scIndDraw.w});
        objItLT = glm::max(objItLT, vec2{m_scIndDraw.x, m_scIndDraw.y});
    }

    return all(greaterThanEqual(pos, objItLT)) && all(lessThanEqual(pos, objItRB));
}

bool UINodeGeom::isOutOfParentBounds() {
    if (!getParent() || m_skipBoundCheck) {
        return false;
    }
    const auto parent = dynamic_cast<UINodeGeom*>(getParent());
    return glm::any(greaterThan(m_parentNodeRelPos, parent->m_size)) ||
           glm::any(lessThan(m_parentNodeRelPos + m_size, {}));
}

}