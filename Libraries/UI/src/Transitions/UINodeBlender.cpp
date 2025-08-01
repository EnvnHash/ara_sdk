//
// Created by sven on 05-03-25.
//

#include "Transitions/UINodeBlender.h"
#include <UISharedRes.h>

namespace ara {

UINodeBlender::UINodeBlender() {
    m_node[type::front] = nullptr;
    m_node[type::back] = nullptr;
}

UINode* UINodeBlender::set(UINodeBlender::type t, UINode* node) {
    if (m_root) {
        m_node[t] = node;
        if (t == type::back) {
            m_node[t]->setAlpha(0.f);
        }
        return m_node[t];
    }
    return nullptr;
}

void UINodeBlender::blend(float val) {
    if (m_node[type::front]) {
        m_node[type::front]->setAlpha(1.f - val);
        if (val > 0.f) {
            if (!m_node[type::front]->isVisible()) {
                m_node[type::front]->setVisibility(true);
            }
        }
    }
    if (m_node[type::back]) {
        m_node[type::back]->setAlpha(val);
        if (val > 0.f) {
            if (!m_node[type::back]->isVisible()) {
                m_node[type::back]->setVisibility(true);
            }
        }
    }
}

void UINodeBlender::transition(transType tt, double dur) {
    if (!m_root) {
        return;
    }

    m_transType = tt;

    if (tt == transType::frontToBack) {
        if (dur == 0.0) {
            if (m_node[type::front]) {
                m_node[type::front]->setAlpha(0.f);
                m_node[type::front]->setVisibility(false);
            }

            if (m_node[type::back]) {
                m_node[type::back]->setAlpha(1.f);
                m_node[type::back]->setVisibility(true);
            }

            m_root->getSharedRes()->reqRedraw();

            if (m_blendPos.getEndFunc()) {
                // node may not be initialized at this point
                if (!m_node[type::back]->isInited()) {
                    m_node[type::back]->getSharedRes()->reqRedraw();
                    m_node[type::back]->addGlCb("retryUINodeBlenderFrontToBack", [this]{
                        m_blendPos.getEndFunc()();
                        return true;
                    });
                } else {
                    m_blendPos.getEndFunc()();
                }
            }
        } else {
            m_blendPos.start(0.f, 1.f, dur, false, [&](const float& v){
                blend(v);
                if (v == 1.f) {
                    m_node[type::front]->setVisibility(false);
                }
            });

            m_root->addGlCb("nodeBlend", [this] {
                m_blendPos.update();
                m_root->getSharedRes()->reqRedraw();
                return m_blendPos.stopped();
            });
        }
    }
}

void UINodeBlender::swap() {
    if (m_node[type::front] && m_node[type::back]) {
        auto tmp = m_node[type::front];
        m_node[type::front] = m_node[type::back];
        m_node[type::back] = tmp;
    } else if (!m_node[type::front] && m_node[type::back]) {
        m_node[type::front] = m_node[type::back];
    }
}

UINode* UINodeBlender::get(type t) {
    return m_node[t];
}

}