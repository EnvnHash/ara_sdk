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


#include "MaterialProperties.h"

using namespace glm;

namespace ara {
MaterialProperties::MaterialProperties() : ShaderProperties() {
    parameters["ambient"]      = UniformType();
    parameters["ambient"].type = GL_FLOAT_VEC4;

    parameters["diffuse"]      = UniformType();
    parameters["diffuse"].type = GL_FLOAT_VEC4;

    parameters["emissive"]      = UniformType();
    parameters["emissive"].type = GL_FLOAT_VEC4;

    parameters["specular"]      = UniformType();
    parameters["specular"].type = GL_FLOAT_VEC4;

    parameters["shininess"]      = UniformType();
    parameters["shininess"].type = GL_FLOAT;

    parameters["strength"]      = UniformType();
    parameters["strength"].type = GL_FLOAT;

    setupStd();
}

void MaterialProperties::setupStd() {
    setAmbient(0.1f, 0.1f, 0.1f, 1.f);
    setDiffuse(1.f, 1.f, 1.f, 1.f);
    setEmissive(0.f, 0.f, 0.f, 0.f);
    setSpecular(0.f, 0.f, 0.f, 0.f);
    setShininess(70.0f);
    setStrength(0.5f);

    if (m_useUbBlock) update();
}

glm::vec4 MaterialProperties::getAmbient() {
    return {parameters["ambient"].f4Val[0], parameters["ambient"].f4Val[1], parameters["ambient"].f4Val[2],
            parameters["ambient"].f4Val[3]};
}

glm::vec4 MaterialProperties::getDiffuse() {
    return {parameters["diffuse"].f4Val[0], parameters["diffuse"].f4Val[1], parameters["diffuse"].f4Val[2],
            parameters["diffuse"].f4Val[3]};
}

glm::vec4 MaterialProperties::getEmissive() {
    return {parameters["emissive"].f4Val[0], parameters["emissive"].f4Val[1], parameters["emissive"].f4Val[2],
            parameters["emissive"].f4Val[3]};
}

glm::vec4 MaterialProperties::getSpecular() {
    return {parameters["specular"].f4Val[0], parameters["specular"].f4Val[1], parameters["specular"].f4Val[2],
            parameters["specular"].f4Val[3]};
}

}  // namespace ara
