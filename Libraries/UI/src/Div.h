//
// Created by user on 05.10.2020.
//

#pragma once

#include "UINode.h"

namespace ara {

class Div : public UINode {
public:
    Div();

    explicit Div(const std::string& styleClass);
    ~Div() override = default;

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
