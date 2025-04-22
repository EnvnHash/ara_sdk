
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

#include <DataModel/Node.h>
#include <glm/include/glm/vec4.hpp>
#include <glm/include/glm/gtc/type_ptr.hpp>

namespace ara::node {

class Color : public ara::Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(m_color, m_format)

    enum class format : int { rgb=0, rgbf, rgba, rgbaf, hsl, hsla };
    Color();

private:
    static bool hexColor2rgba(float *rgba, const char *str);  // rgba=float[4] in [0..1]
    static bool hsla2rgba(float *rgba, float h, float s, float l, float a);  // rgba=float[4] in [0..1], h in [0..359],
    // s and l in [0..1], a in [0..1]
    static unsigned hex2dec(int ch) {
        return (ch >= '0' && ch <= '9') ? ch - '0'
                                        : (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10
                                                                   : (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10
                                                                                              : 0;
    }

    void    setColor(float c0, float c1, float c2) { m_color[0] = c0; m_color[1] = c1; m_color[2] = c2; }
    void    setColor(float c0, float c1, float c2, float c3) { m_color[0] = c0; m_color[1] = c1; m_color[2] = c2; m_color[3] = c3; }
    void    setFormat(Color::format& fmt) { m_format = m_colorFunc[static_cast<int>(fmt)]; }

    float       *getColor4fv() { return m_color.data(); }
    glm::vec4   getColorVec4() const { return glm::make_vec4(m_color.data()); }

    inline static std::vector<std::string> m_colorFunc = {"rgb", "rgbf", "rgba", "rgbaf", "hsl", "hsla"};

    std::string             m_format;
    std::array<float, 4>    m_color{};    ///> can be any format
    std::array<float, 4>    m_rgba{};     ///> in float 0-1
};

}
