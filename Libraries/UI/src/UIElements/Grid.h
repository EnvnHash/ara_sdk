//
// Created by user on 05.10.2020.
//

#pragma once

#include "UIElements/Div.h"

namespace ara {

class Grid : public Div {
public:
    Grid();
    ~Grid() override = default;

    void init() override;
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    bool drawFunc(const uint32_t& objId);
    void setNumOfSeparations(float sepX, float sepY) { m_numOfSeparations = glm::vec2(sepX, sepY); }

private:
    Shaders *m_gridShdr  = nullptr;
    glm::vec2 m_numOfSeparations{4.f, 4.f};
};

}  // namespace ara
