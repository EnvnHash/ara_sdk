//
// Created by sven on 21-04-25.
//

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

class DivVaoData {
public:
    glm::vec4 pos{0.f};
    glm::vec2 texCoord{0.f};
    glm::vec4 color{0.f};
    glm::vec4 aux0{0.f};  /// aux0
    glm::vec4 aux1{0.f};  /// aux1 (Div: x:borderWidth, y:borderHeight,
    /// z:borderRadiusX, w:borderRadiusY
    glm::vec4 aux2{0.f};  /// aux2
    glm::vec4 aux3{0.f};  /// aux3
};

class DrawSet;

class IndDrawBlock {
public:
    void stdInit() {
        vaoData.resize(4);
        indices.resize(6);
    }
    std::pair<std::vector<DivVaoData>*, uint32_t> getUpdtPair() { return {&vaoData, vaoOffset}; }

    std::vector<DivVaoData> vaoData;
    std::vector<GLuint>     indices;
    uint32_t                vaoOffset = 0;
    DrawSet*                drawSet   = nullptr;
};

}