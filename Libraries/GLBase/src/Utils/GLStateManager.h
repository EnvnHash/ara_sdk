//
// Created by user on 20.01.2022.
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