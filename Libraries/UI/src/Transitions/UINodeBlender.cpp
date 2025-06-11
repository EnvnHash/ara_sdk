//
// Created by sven on 05-03-25.
//

#include "Transitions/UINodeBlender.h"
#include <UISharedRes.h>

namespace ara {

UINodeBlender::UINodeBlender() {
    m_nodes[type::front] = nullptr;
    m_nodes[type::back] = nullptr;
}

UINode* UINodeBlender::set(UINodeBlender::type t, UINode* node) {
    if (m_root) {
        m_nodes[t] = node;
        if (t == type::back) {
            m_nodes[t]->setAlpha(0.f);
        }
        return m_nodes[t];
    }
    return nullptr;
}

void UINodeBlender::blend(float val) {
    if (m_nodes[type::front]) {
        m_nodes[type::front]->setAlpha(1.f - val);
        if (val > 0.f) {
            if (!m_nodes[type::front]->isVisible()) {
                m_nodes[type::front]->setVisibility(true);
            }
        }
    }
    if (m_nodes[type::back]) {
        m_nodes[type::back]->setAlpha(val);
        if (val > 0.f) {
            if (!m_nodes[type::back]->isVisible()) {
                m_nodes[type::back]->setVisibility(true);
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
            if (m_nodes[type::front]) {
                m_nodes[type::front]->setAlpha(0.f);
                m_nodes[type::front]->setVisibility(false);
            }

            if (m_nodes[type::back]) {
                m_nodes[type::back]->setAlpha(1.f);
                m_nodes[type::back]->setVisibility(true);
            }

            m_root->getSharedRes()->reqRedraw();

            if (m_blendPos.getEndFunc()) {
                // node may not be initialized at this point
                if (!m_nodes[type::back]->isInited()) {
                    m_nodes[type::back]->getSharedRes()->reqRedraw();
                    m_nodes[type::back]->addGlCb("retryUINodeBlenderFrontToBack", [this]{
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
                    m_nodes[type::front]->setVisibility(false);
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
    if (m_nodes[type::front] && m_nodes[type::back]) {
        auto tmp = m_nodes[type::front];
        m_nodes[type::front] = m_nodes[type::back];
        m_nodes[type::back] = tmp;
    } else if (!m_nodes[type::front] && m_nodes[type::back]) {
        m_nodes[type::front] = m_nodes[type::back];
    }
}

UINode* UINodeBlender::get(type t) {
    return m_nodes[t];
}

}