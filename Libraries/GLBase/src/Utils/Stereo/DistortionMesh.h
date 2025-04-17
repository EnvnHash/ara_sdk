/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Meshes/Mesh.h"
#include "Utils/Stereo/PolynomialRadialDistortion.h"

namespace ara {

class DistortionMesh {
public:
    DistortionMesh(const PolynomialRadialDistortion &distortion,
                   // Units of the following parameters are tan-angle units.
                   float screen_width, float screen_height, float x_eye_offset_screen, float y_eye_offset_screen,
                   float texture_width, float texture_height, float x_eye_offset_texture, float y_eye_offset_texture);

    virtual ~DistortionMesh() = default;

    Mesh     &getMesh() { return m_mesh; }
    glm::vec2 m_min{0.f};
    glm::vec2 m_max{0.f};

private:
    static constexpr int   kResolution = 40;
    Mesh                   m_mesh;
    std::vector<GLuint>    m_index_data;
    std::vector<glm::vec2> m_vertex_data;
    std::vector<glm::vec2> m_uvs_data;
};

}  // namespace ara
