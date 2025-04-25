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

#include <glb_common/glb_common.h>

namespace ara {

class GLStateManager {
public:
    GLStateManager() = default;

    void set(glFun f, std::deque<GLenum> &&arg) {
        // if function not yet used, or state different, store the value
        auto it     = enumStates.find(f);
        bool exists = it != enumStates.end();
        if (!exists || (exists && arg != it->second)) {
            enumStates[f] = arg;
            procEnum(f, enumStates[f]);
        }
    }

    static void procEnum(glFun f, std::deque<GLenum> &arg) {
        switch (f) {
            case glFun::Enable: glEnable(arg[0]); break;
            case glFun::Disable: glDisable(arg[0]); break;
            case glFun::BlendEquation: glBlendEquation(arg[0]); break;
            case glFun::BlendFunc: glBlendFunc(arg[0], arg[1]); break;
#ifndef ARA_USE_EGL
            case glFun::PolygonMode: glPolygonMode(arg[0], arg[1]); break;
#endif
            default: break;
        }
    }

private:
    std::unordered_map<glFun, std::deque<GLenum>> enumStates;
};

}  // namespace ara