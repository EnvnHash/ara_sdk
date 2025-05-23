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

#ifdef ARA_USE_GLFW
#include "WindowManagement/GLFWWindow.h"
#elif ARA_USE_EGL
#include "WindowManagement/EGLWindow.h"
#endif

namespace ara {
#ifdef ARA_USE_GLFW
using GLWindow  = GLFWWindow;
using GLContext = GLFWwindow *;
#elif ARA_USE_EGL
using GLWindow  = EGLWindow;
using GLContext = EGLContext;
#else
using GLWindow  = void;
using GLContext = void *;
#endif

using winCtxHidCb = std::variant<
    std::function<void(GLContext, unsigned int)>,
    std::function<void(GLContext, int, int, int, int)>,
    std::function<void(GLContext, int, int, int)>,
    std::function<void(GLContext, int, int)>,
    std::function<void(GLContext, int)>,
    std::function<void(GLContext, double, double)>,
    std::function<void(GLContext)>>;

}  // namespace ara