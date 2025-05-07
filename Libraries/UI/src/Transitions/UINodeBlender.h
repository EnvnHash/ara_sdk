//
// Created by sven on 05-03-25.
//

#pragma once

#include <UIElements/UINodeBase/UINode.h>
#include <AnimVal.h>

namespace ara {

class UINodeBlender {
public:
    enum class type : int { front=0, back };
    enum class transType : int { frontToBack =0 };

    UINodeBlender();

    template <class T>
    UINode* set(type t) {
        if (m_root) {
            m_nodes[t] = m_root->addChild<T>();
            if (t == type::back) {
                m_nodes[t]->setAlpha(0.f);
            }
            return m_nodes[t];
        }
        return nullptr;
    }

    UINode* set(type t, UINode* node);
    void    swap();
    void    blend(float val);
    void    transition(transType tt, double dur);

    void setRoot(ara::UINode* node) { m_root = node; }
    void setDelay(float val) { m_blendPos.setDelay(val); }
    void setEndFunc(const std::function<void()>& endFunc) { m_blendPos.setEndFunc(endFunc); }

    UINode*     get(type t);
    bool        stopped() { return m_blendPos.stopped(); }
    transType   getTransType() const { return m_transType; }

private:
    std::unordered_map<type, ara::UINode*>  m_nodes;
    UINode*                                 m_root = nullptr;
    AnimVal<float>                          m_blendPos{};
    transType                               m_transType{};
};

}
