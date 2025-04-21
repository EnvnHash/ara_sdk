//
// Created by user on 05.10.2020.
//

#pragma once

#include "UINode.h"

namespace ara {

class Div : public UINode {
public:
    Div();
    Div(std::string *styleClass);
    Div(std::string &&styleClass);
    virtual ~Div() = default;

    bool draw(uint32_t *objId) override;
    bool drawIndirect(uint32_t *objId) override;
    void updateDrawData() override;
    void pushVaoUpdtOffsets() override;

protected:
    static inline std::string m_objIdName = "objId";
    glm::vec2                 m_uvSize{1.f};
    glm::vec2                 m_divRefSize{1.f};
};

}  // namespace ara
