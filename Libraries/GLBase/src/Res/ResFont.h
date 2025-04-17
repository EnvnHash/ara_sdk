#pragma once

#include "Res/ResNode.h"

namespace ara {

class Instance;
class GLBase;

class ResFont : public ResNode {
public:
    enum { none = 0, bold = 1, italic = 2 };

    int         m_Size  = 0;
    unsigned    m_Flags = none;
    std::string m_FontPath;

    ResFont(std::string name, GLBase *glbase) : ResNode(std::move(name), glbase) {}

    bool OnProcess() override;
    bool OnResourceChange(bool deleted, const std::string &res_fpath) override;

    bool        OnLoad() override { return true; }
    bool        isOK() override { return true; }
    static bool isClass(ResNode *snode) { return snode->getFlag("font") != nullptr; }
};

}  // namespace ara