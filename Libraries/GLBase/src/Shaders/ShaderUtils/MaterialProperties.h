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

#include "Shaders/ShaderUtils/ShaderProperties.h"

namespace ara {
class MaterialProperties : public ShaderProperties {
public:
    MaterialProperties();

    void setupStd();

    glm::vec4 getAmbient();
    glm::vec4 getDiffuse();
    glm::vec4 getEmissive();
    glm::vec4 getSpecular();

    float getShininess() { return parameters["shininess"].fVal; }
    float getStrength() { return parameters["strength"].fVal; }
    void  setAmbient(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("ambient", r, g, b, a); }
    void  setDiffuse(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("diffuse", r, g, b, a); }
    void  setEmissive(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("emissive", r, g, b, a); }
    void  setSpecular(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("specular", r, g, b, a); }
    void  setShininess(float val) { setFloat("shininess", val); }
    void  setStrength(float val) { setFloat("strength", val); }
};
}  // namespace ara
