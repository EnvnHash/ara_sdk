//
// Created by sven on 01-04-25.
//

#pragma once

#include <UIElements/UINodeBase/UINode.h>

namespace ara {

class UIStack {
public:
    virtual ~UIStack() = default;

    template <class T>
    T* add(const std::string& name, const std::optional<UINodePars>& nodePars = std::nullopt) {
        if (nodePars.has_value()) {
            m_nodes[name] = &m_rootNode->push<T>(nodePars.value());
        } else {
            m_nodes[name] = &m_rootNode->push<T>();
        }
        m_nodes[name]->setVisibility(false);
        return dynamic_cast<T*>(m_nodes[name]);
    }

    template <class T>
    T* add(const std::string& name, const std::function<void()>& f,
           const std::optional<UINodePars>& nodePars = std::nullopt) {
        auto newNode = add<T>(name, nodePars);
        m_onShowFunctions[name] = f;
        return newNode;
    }

    virtual void show(const std::string& name) = 0;
    virtual void setRootNode(UINode* node);

    UINode* get(const std::string& name);
    void setTransitionTime(const double val) { m_transTime = val; }

protected:
    UINode*                                     m_rootNode = nullptr;
    std::unordered_map<std::string, UINode*>    m_nodes;
    double                                      m_transTime = 0.0;

    std::unordered_map<std::string, std::function<void()>>  m_onShowFunctions;
};

}
