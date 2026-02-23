//
// Created by sven on 19-02-26.
//

#pragma once

#include <UIElements/Div.h>

namespace ara {

class TreeCollapsible : public Div {
public:
    TreeCollapsible();

    void setNode(Node* node);
    void setFontHeight(int32_t v) { m_fontHeight = v; }
    void setySpacing(int32_t y) { m_ySpacing = y; }

private:
    void rebuild();
    void rebuildUiElements();
    void rebuildIt(Node* nd, size_t tabIdx, int32_t& yOffs);
    void rebuildCollapseState(Node* nd);

    Node*   m_tree = nullptr;
    bool    m_reqRebuild = false;
    int32_t m_fontHeight = 18;
    int32_t m_ySpacing = 7;
    std::unordered_map<std::string, bool> m_collapseState;
};

}
