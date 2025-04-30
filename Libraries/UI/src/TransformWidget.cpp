//
// Created by user on 04.11.2021.
//

#include "TransformWidget.h"

#include <UIWindow.h>

#include <magic_enum/magic_enum.hpp>

#include "Lights/LICamera.h"
#include "Lights/LIProjector.h"

using namespace std;
using namespace glm;
using namespace magic_enum;

namespace ara {

TransformWidget::TransformWidget() : Div() {
    m_canReceiveDrag = true;
    setName(getTypeName<TransformWidget>());
    setFocusAllowed(false);
    setScissorChildren(false);
    m_excludeFromParentScissoring = true;
}

TransformWidget::TransformWidget(std::string &&styleClass) : Div(std::move(styleClass)) {
    m_canReceiveDrag = true;
    setName(getTypeName<TransformWidget>());
    setFocusAllowed(false);
    setScissorChildren(false);
    m_excludeFromParentScissoring = true;
}

void TransformWidget::init() {
    m_buttCont = addChild<Div>(getStyleClass() + ".buttCont");
    m_buttCont->addMouseClickCb([this](hidData *data) { getWindow()->setInputFocusNode(this); });
    m_buttCont->setScissorChildren(false);

    m_centInd = m_buttCont->addChild<ImageButton>(getStyleClass() + ".centerIndicator");
    m_centInd->setIsToggle(true);
    m_centInd->setDisabled(true);

    // m_planeSwitchCont = addChild<Div>(getStyleClass()+".planeSwitchCont");

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            m_arrowButts[i][j] = m_buttCont->addChild<ImageButton>(
                getStyleClass() + ((i == 0 ? ".butt_trans_" : ".butt_rot_") + m_arrowName[j]));
            m_arrowButts[i][j]->setName(((i == 0 ? "TW_butt_trans_" : "TW_butt_rot_") + m_arrowName[j]));
            m_arrowButts[i][j]->init();
            m_arrowButts[i][j]->setCanReceiveDrag(false);
            m_arrowButts[i][j]->setObjUsesTexAlpha(true);
            m_arrowButts[i][j]->addMouseUpCb([this](hidData *data) { m_mousePressed = false; });

            m_arrowButts[i][j]->addMouseClickCb([this, i, j](hidData *data) {
                m_mousePressed = true;
                m_firstDown    = true;
                m_downTp       = chrono::system_clock::now();
                getWindow()->setInputFocusNode(this);

                // click + hold
                getWindow()->addGlCb(this, "ch", [this, i, j] {
                    if (!m_modelNode) return true;

                    auto   now  = chrono::system_clock::now();
                    double diff = std::chrono::duration<double, std::milli>(now - m_downTp).count();

                    if (m_firstDown || diff > 500.0) {
                        if (i == 0)
                            translate(j);
                        else
                            rotate(j);
                    }

                    m_firstDown = false;

                    if (m_mousePressed) {
                        setDrawFlag();
                        getSharedRes()->requestRedraw = true;
                    }

                    return !m_mousePressed;
                });

                setDrawFlag();
            });
        }

        for (int j = 0; j < 4; j++) {
            m_arrowLabels[i][j] = m_buttCont->addChild<Label>(
                getStyleClass() + ((i == 0 ? ".lbl_trans_" : ".lbl_rot_") + m_arrowName[j]));
            m_arrowLabels[i][j]->setName(((i == 0 ? "AR_lbl_trans_" : "AR_lbl_rot_") + m_arrowName[j]));
            m_arrowLabels[i][j]->excludeFromObjMap(true);
        }

        // m_psLabels[i] = m_planeSwitchCont->addChild<Label>(getStyleClass() +
        // ".ps_lbl_" + (i==0 ? "translate" : "rotate"));

        for (int j = 0; j < (int)twPlane::count; j++) {
            m_planeSwitcher[i][j] = m_buttCont->addChild<ImageButton>(
                getStyleClass() + ".ps_" + (i == 0 ? "translate" : "rotate") + "_" + string(enum_name((twPlane)j)));

            m_planeSwitcher[i][j]->setIsToggle(true);
            m_planeSwitcher[i][j]->setLod(0.f);
            m_planeSwitcher[i][j]->setObjUsesTexAlpha(true);
            m_planeSwitcher[i][j]->addMouseClickCb([this, i, j](hidData *data) {
                setPlaneSwitcherState(i == 0 ? transMode::translate : transMode::rotate, (twPlane)j);
                getWindow()->setInputFocusNode(this);
            });
        }

        m_planeSwitcher[i][(int)twPlane::count] =
            m_buttCont->addChild<ImageButton>(getStyleClass() + ".ps_" + (i == 0 ? "translate" : "rotate") + "_top");
        m_planeSwitcher[i][(int)twPlane::count]->setObjUsesTexAlpha(true);
        m_planeSwitcher[i][(int)twPlane::count]->setLod(1.f);

        m_planeSwitcher[i][(int)twPlane::count + 1] =
            m_buttCont->addChild<ImageButton>(getStyleClass() + ".ps_" + (i == 0 ? "translate" : "rotate") + "_bott");
        m_planeSwitcher[i][(int)twPlane::count + 1]->setObjUsesTexAlpha(true);
        m_planeSwitcher[i][(int)twPlane::count + 1]->setLod(1.f);
    }

    getWindow()->addGlobalMouseMoveCb(this, [this](hidData *data) {
        if (!getWindow()->isCursorVisible() || !m_visible) return;

        m_mouseInBounds = false;

        if ((uint32_t)data->objId >= m_objIdMin && (uint32_t)data->objId <= getMaxChildId() &&
            m_state == state::disabled && !m_blockHID) {
            if (!m_disHighlighted) highLight(true);

            m_mouseInBounds = true;
        }

        if (!m_mouseInBounds && m_disHighlighted && m_state == state::disabled) highLight(false);
    });

    getWindow()->addGlobalMouseDownLeftCb(this, [this](hidData *data) {
        if (!getWindow()->isCursorVisible() || !m_visible) return;

        if ((uint32_t)data->objId >= m_objIdMin && (uint32_t)data->objId <= getMaxChildId() &&
            m_state == state::disabled && m_visible && !m_blockHID) {
            setDisabled(false, true);
        }
    });

    getWindow()->addGlCb(this, "pl", [this] {
        setPlaneSwitcherState(transMode::translate, twPlane::xy);
        return true;
    });
}

void TransformWidget::translate(int j) {
    if (!m_modelNode) return;

    vec3  offs;
    float amt = m_transAmt[(int)m_cfState];
    switch (m_transPlane) {
        case twPlane::xy:
            offs = vec3{j == 0 ? amt : (j == 1 ? -amt : 0.f), j == 2 ? -amt : (j == 3 ? amt : 0.f), 0.f};
            break;
        case twPlane::xz:
            offs = vec3{j == 0 ? amt : (j == 1 ? -amt : 0.f), 0.f, j == 2 ? -amt : (j == 3 ? amt : 0.f)};
            break;
        case twPlane::yz:
            offs = vec3{0.f, j == 2 ? -amt : (j == 3 ? amt : 0.f), j == 0 ? amt : (j == 1 ? -amt : 0.f)};
            break;
        default: break;
    }

    switch (m_tRel) {
        case transAlign::objectRelative:
            m_modelNode->translate(m_modelNode->getTransVec() + vec3(m_modelNode->getRotMat() * vec4(offs, 1.f)));
            break;
        case transAlign::worldRelative: m_modelNode->translate(m_modelNode->getTransVec() + offs); break;
    }

    checkCamFlags();
}

void TransformWidget::rotate(int j) {
    if (!m_modelNode) {
        return;
    }

    m_inRm    = m_modelNode->getRotMat();
    auto amt = radians(m_rotAmt[static_cast<int>(m_cfState)]);

    switch (m_transPlane) {
        case twPlane::xy:
            m_modRotMat = eulerAngleXYZ(j == 2 ? -amt : (j == 3 ? amt : 0.f), j == 0 ? -amt : (j == 1 ? amt : 0.f), 0.f);
            break;
        case twPlane::xz:
            m_modRotMat = eulerAngleXYZ(j == 2 ? -amt : (j == 3 ? amt : 0.f), 0.f, j == 0 ? -amt : (j == 1 ? amt : 0.f));
            break;
        case twPlane::yz:
            m_modRotMat = eulerAngleXYZ(0.f, j == 0 ? -amt : (j == 1 ? amt : 0.f), j == 2 ? amt : (j == 3 ? -amt : 0.f));
            break;
        default: break;
    }

    switch (m_tRel) {
        case transAlign::objectRelative: m_modRotMat = m_inRm * m_modRotMat; break;
        case transAlign::worldRelative: m_modRotMat = m_modRotMat * m_inRm; break;
    }

    m_modelNode->rotate(m_modRotMat);

    checkCamFlags();
}

void TransformWidget::keyDown(hidData *data) {
    int dir = -1;
    if (data->key == GLSG_KEY_DOWN)
        dir = 3;
    else if (data->key == GLSG_KEY_UP)
        dir = 2;
    else if (data->key == GLSG_KEY_LEFT)
        dir = 0;
    else if (data->key == GLSG_KEY_RIGHT)
        dir = 1;

    if (dir > -1) {
        if (m_transMode == transMode::translate)
            translate(dir);
        else
            rotate(dir);

        if (m_arrowButts[m_transMode == transMode::translate ? 0 : 1][dir]) {
            m_resetButHighlight.first  = getColorIdx(dir);
            m_resetButHighlight.second = m_arrowButts[m_transMode == transMode::translate ? 0 : 1][dir];
            if (m_state != state::disabled)
                m_resetButHighlight.second->setColor(m_colorsHigh[m_resetButHighlight.first], state::none, false);
        }
    }

    // advance plane selection
    if (data->key == GLSG_KEY_TAB) {
        if (!data->shiftPressed)
            m_psSwitchMapIdx = (m_psSwitchMapIdx + 1) % 6;
        else
            m_psSwitchMapIdx = (m_psSwitchMapIdx + 5) % 6;

        setPlaneSwitcherState(m_psSwitchMap[m_psSwitchMapIdx].first, m_psSwitchMap[m_psSwitchMapIdx].second);
    }

    setDrawFlag();
}

void TransformWidget::keyUp(hidData *data) {
    if (m_resetButHighlight.second) {
        if (m_state != state::disabled)
            m_resetButHighlight.second->setColor(m_colors[m_resetButHighlight.first], state::none, false);

        m_resetButHighlight.second = nullptr;
        setDrawFlag();
    }
}

void TransformWidget::setPlaneSwitcherState(transMode m, twPlane p) {
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < (int)twPlane::count; j++) {
            if (m_planeSwitcher[i][j]) m_planeSwitcher[i][j]->setSelected(false, true);
        }

    if (m_planeSwitcher[m == transMode::translate ? 0 : 1][(int)p])
        m_planeSwitcher[m == transMode::translate ? 0 : 1][(int)p]->setSelected(true, true);

    setTransMode(m);
    setPlane(p);
}

void TransformWidget::setTransMode(transMode m, bool updtColors) {
    m_transMode = m;

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 4; j++) {
            if (m_arrowButts[i][j])
                m_arrowButts[i][j]->setVisibility((i == 0 ? transMode::translate : transMode::rotate) == m);

            if (m_arrowLabels[i][j])
                m_arrowLabels[i][j]->setVisibility((i == 0 ? transMode::translate : transMode::rotate) == m);
        }

    // switch the indicator button in the center
    if (m_centInd) {
        if (m == transMode::rotate) {
            m_centInd->setDisabledSelected(true, true);
        } else {
            m_centInd->setDisabled(true, true);
        }
    }

    if (updtColors) setPlane(m_transPlane);

    checkPlaneSwitchIdx();
}

void TransformWidget::setPlane(twPlane p) {
    m_transPlane = p;

    for (int i = 0; i < 4; i++) {
        string txt;
        switch (m_transPlane) {
            case twPlane::xy:
                txt = m_transMode == transMode::translate ? array<string, 4>{"X", "-X", "-Y", "Y"}[i]
                                                          : array<string, 4>{"Y", "-Y", "-X", "X"}[i];
                break;
            case twPlane::xz:
                txt = m_transMode == transMode::translate ? array<string, 4>{"X", "-X", "-Z", "Z"}[i]
                                                          : array<string, 4>{"Z", "-Z", "-X", "X"}[i];
                break;
            case twPlane::yz:
                txt = m_transMode == transMode::translate ? array<string, 4>{"Z", "-Z", "-Y", "Y"}[i]
                                                          : array<string, 4>{"Y", "-Y", "-Z", "Z"}[i];
                break;
            default: break;
        }

        int idx = getColorIdx(i);
        int ti  = m_transMode == transMode::translate ? 0 : 1;

        if (m_arrowButts[ti][i]) {
            m_arrowButts[ti][i]->setColor(m_colors[idx], state::none, false);
            m_arrowButts[ti][i]->setColor(m_colorsHigh[idx], state::highlighted, false);
            m_arrowButts[ti][i]->setColor(m_colorsDis[idx], state::disabled, false);
            m_arrowButts[ti][i]->setColor(m_colorsHigh[idx], state::disabledHighlighted);
        }

        if (m_arrowLabels[ti][i]) m_arrowLabels[ti][i]->setText(txt);
    }

    if (m_planeSwCb) m_planeSwCb(m_transMode, m_transPlane);

    checkPlaneSwitchIdx();
}

int TransformWidget::getColorIdx(int buttIdx) {
    int idx = buttIdx;

    switch (m_transPlane) {
        case twPlane::xy: idx = m_transMode == transMode::translate ? (buttIdx / 2) : 1 - (buttIdx / 2); break;
        case twPlane::xz: idx = m_transMode == transMode::translate ? (buttIdx / 2) * 2 : 2 - (buttIdx / 2) * 2; break;
        case twPlane::yz: idx = m_transMode == transMode::translate ? 2 - (buttIdx / 2) : (buttIdx / 2) + 1;
        default: break;
    }

    return idx;
}

void TransformWidget::setDisabled(bool val, bool forceStyleUpdt) {
    if (m_blockStateChange) return;

    m_disHighlighted = false;

    UINode::setDisabled(val, forceStyleUpdt);

    for (auto &it : m_children) it->setDisabled(val, forceStyleUpdt);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            if (m_arrowButts[i][j]) m_arrowButts[i][j]->setDisabled(val, forceStyleUpdt);
            if (m_arrowLabels[i][j]) m_arrowLabels[i][j]->setDisabled(val, forceStyleUpdt);
        }

        for (int j = 0; j < (int)twPlane::count; j++)
            if (m_planeSwitcher[i][j]) m_planeSwitcher[i][j]->setDisabled(val, forceStyleUpdt);

        if (m_psLabels[i]) m_psLabels[i]->setDisabled(val, forceStyleUpdt);
    }

    // set the corresponing planeSwitcher button the "selected"
    if (!val) setPlaneSwitcherState(m_transMode, m_transPlane);

    if (!val && m_enableCb) m_enableCb();
}

void TransformWidget::highLight(bool val) {
    if (m_blockStateChange) return;

    if (!val) {
        setDisabled(true, true);
        return;
    }

    m_disHighlighted = val;

    if (m_buttCont) m_buttCont->setDisabledHighlighted(val, true);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) {
            if (m_arrowButts[i][j]) m_arrowButts[i][j]->setDisabledHighlighted(val, true);

            if (m_arrowLabels[i][j]) m_arrowLabels[i][j]->setDisabledHighlighted(val, true);
        }

        for (int j = 0; j < (int)twPlane::count; j++)
            if (m_planeSwitcher[i][j]) m_planeSwitcher[i][j]->setDisabledHighlighted(val, true);

        if (m_psLabels[i]) m_psLabels[i]->setDisabledHighlighted(val, true);
    }
}

void TransformWidget::setVisibility(bool val) {
    UINode::setVisibility(val);
    setTransMode(m_transMode);
}

void TransformWidget::checkCamFlags() {
    // in case the modelnode contains a cameraDef, force its projection matrix
    // to be updated
    if (m_modelNode->getName() == getTypeName<LICamera>() || m_modelNode->getName() == getTypeName<LIProjector>()) {
        ((LICamera *)m_modelNode)->getCamDef()->setForceUpdtProjMat(true);
        ((LICamera *)m_modelNode)->getCamDef()->setForceUpdtCb(true);
    }
}

void TransformWidget::checkPlaneSwitchIdx() {
    for (int i = 0; i < 6; i++)
        if (m_psSwitchMap[i].first == m_transMode && m_psSwitchMap[i].second == m_transPlane) {
            m_psSwitchMapIdx = i;
            break;
        }
}

TransformWidget::~TransformWidget() {
    if (m_sharedRes && m_sharedRes->win) {
        auto uiWin = (UIWindow *)m_sharedRes->win;
        uiWin->removeGlobalMouseMoveCb(this);
        uiWin->removeGlobalMouseDownLeftCb(this);
    }
}

}  // namespace ara