//
// Created by sven on 01-04-25.
//

#pragma once

#include <UINode.h>
#include <Transitions/UINodeBlender.h>

namespace ara {

class UIStack {
public:
    template  <class T>
    T* add(const std::string& name) {
        m_nodes[name] = m_rootNode->addChild<T>();
        m_nodes[name]->setVisibility(false);
        return dynamic_cast<T*>(m_nodes[name]);
    }

    template <class T>
    T* add(const std::string& name, const std::function<void()>& f) {
        m_onShowFunctions[name] = f;
        return add<T>(name);
    }

    virtual void show(const std::string& name) = 0;
    virtual void setRootNode(UINode* node);

    UINode* get(const std::string& name);
    void setTransitionTime(double val) { m_transTime = val; }

protected:
    UINode*                                     m_rootNode;
    std::unordered_map<std::string, UINode*>    m_nodes;
    double                                      m_transTime = 0.0;

    std::unordered_map<std::string, std::function<void()>>  m_onShowFunctions;
};

}
