//
// Created by user on 18.11.2020.
//

#pragma once

#ifdef ARA_USE_GLFW

#include "WindowManagement/GLFWWindow.h"

namespace ara {
using GLWindow  = GLFWWindow;
using GLContext = GLFWwindow *;
}  // namespace ara
#elif ARA_USE_EGL
#include "WindowManagement/EGLWindow.h"
namespace ara {
using GLWindow  = EGLWindow;
using GLContext = EGLContext;
}  // namespace ara
#else
namespace ara {
using GLWindow  = void;
using GLContext = void *;
}  // namespace ara
#endif