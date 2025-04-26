/*
 * Vertex.h
 *
 *  Created on: 19.09.2016
 *      Author: sven
 */

#pragma once

#include "glb_common/glb_common.h"

namespace ara {

struct Vertex {
    Vertex() = default;
    Vertex(glm::vec3 const &position, glm::vec3 const &normal, glm::vec2 const &texcoord, glm::vec4 const &color)
        : position(glm::vec4(position, 1.0f)), normal(glm::vec4(normal, 0.0f)),
          texcoord(glm::vec4(texcoord, 0.0f, 0.0f)), color(glm::vec4(1.0f)) {}

    glm::vec4 position{};
    glm::vec4 normal{};
    glm::vec4 texcoord{};
    glm::vec4 color{};
};

}  // namespace ara
