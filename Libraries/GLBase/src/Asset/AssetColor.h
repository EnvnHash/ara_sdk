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

#include <glm/gtc/type_ptr.hpp>

#include "Asset/ResNode.h"

namespace ara {

class AssetColor : public ResNode {
public:
    AssetColor(std::string name, GLBase *glbase);
    void        onProcess() override;
    static bool isClass(const ResNode *snode);
    bool        onSourceResUpdate(bool deleted, ResNode *unode) override;

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
    [[nodiscard]] glm::vec4                getColorvec4() const { return glm::make_vec4(rgba); }
    inline static std::vector<std::string> colorFunc = {"rgb", "rgbf", "rgba", "rgbaf", "hsl", "hsla"};
    float                                  rgba[4]{};
};

}  // namespace ara
