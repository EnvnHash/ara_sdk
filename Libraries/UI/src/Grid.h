//
// Created by user on 05.10.2020.
//

#pragma once

#include "Div.h"

namespace ara {

class Grid : public Div {
public:
    Grid();

    explicit Grid(std::string *styleClass);
    explicit Grid(std::string &&styleClass);

    virtual ~Grid() = default;

    void init() override;

    bool draw(uint32_t *objId) override;

    bool drawIndirect(uint32_t *objId) override;

    bool drawFunc(uint32_t *objId);

private:
    Shaders *m_gridShdr  = nullptr;
    uint32_t m_tempObjId = 0;
    uint32_t m_dfObjId   = 0;
};

}  // namespace ara
