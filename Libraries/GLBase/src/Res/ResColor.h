//
// Created by user on 04.02.2021.
//

#pragma once

#include <glm/gtc/type_ptr.hpp>

#include "Res/ResNode.h"

namespace ara {

class ResColor : public ResNode {
public:
    ResColor(std::string name, GLBase *glbase);
    bool        OnProcess() override;
    static bool isClass(ResNode *snode);
    bool        OnSourceResUpdate(bool deleted, ResNode *unode) override;

    static bool hexColor2rgba(float      *rgba, const char *str);  // rgba=float[4] in [0..1]
    static bool hsla2rgba(float *rgba, float h, float s, float l, float a);  // rgba=float[4] in [0..1], h in [0..359],
                                     // s and l in [0..1], a in [0..1]
    static unsigned hex2dec(int ch) {
        return (ch >= '0' && ch <= '9')   ? ch - '0'
               : (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10
               : (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10
                                          : 0;
    }

    float                                 *getColor4fv() { return rgba; }
    glm::vec4                              getColorvec4() { return glm::make_vec4(rgba); }
    inline static std::vector<std::string> colorFunc = {"rgb", "rgbf", "rgba", "rgbaf", "hsl", "hsla"};
    float                                  rgba[4]{};
};

}  // namespace ara
