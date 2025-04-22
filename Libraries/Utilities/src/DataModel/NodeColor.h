//
// Created by sven on 04-03-25.
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
