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
    ~CrossHair() override = default;

    void init() override;
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    bool drawFunc(const uint32_t& objId);

private:
    Shaders* m_crossHairShdr = nullptr;
};

}  // namespace ara
