//
// Created by user on 05.10.2020.
//

#pragma once

#include "Div.h"

namespace ara {

class CrossHair : public Div {
public:
    CrossHair();
    CrossHair(const std::string& styleClass);
    virtual ~CrossHair() = default;

    void init();
    bool draw(uint32_t* objId);
    bool drawIndirect(uint32_t* objId);
    bool drawFunc(uint32_t* objId);

private:
    Shaders* m_crossHairShdr = nullptr;
    uint32_t m_tempObjId     = 0;
    uint32_t m_dfObjId       = 0;
};

}  // namespace ara
