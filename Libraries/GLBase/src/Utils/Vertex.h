//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#pragma once

#include "GlbCommon/GlbCommon.h"

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
