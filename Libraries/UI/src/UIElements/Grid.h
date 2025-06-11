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

private:
    Shaders *m_gridShdr  = nullptr;
};

}  // namespace ara
