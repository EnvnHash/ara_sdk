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

#include <GlbCommon/GlbCommon.h>

namespace ara {

enum class orientation : int {
    default_ori = 1,
    /// \brief Represents a screen rotated 180 degrees, also known as
    /// upside-down.
    upside_down = 2,
    /// \brief Represents a screen rotated 90 degrees clockwise.
    left_90 = 3,
    /// \brief Represents a screen rotated 90 degrees counter-clockwise.
    right_90 = 4,
    /// \brief Represents an unknown orientation.
    unknown = 5
};

class glWinPar {
public:
    bool doInit       = true;       // initialize glfw and get display info
    bool fullScreen   = false;
    bool decorated    = false;      // menu bar at the top of the window
    bool floating     = false;      // window always on top
    bool createHidden = false;
    bool openGlDebug  = false;      // set the GLFW_OPENGL_DEBUG_CONTEXT flag
    bool debug        = false;      // show debug info
    bool resizeable   = true;
    bool transparent  = false;
    bool hidInput     = true;
    bool hidExtern    = false;      // if false use HID management from GLFWWindow, if
                                    // true from GLFWWindowManager
    unsigned int nrSamples = 2;     // multisampling -> lower than 2 disables
                                    // GL_POLYGON_SMOOTH, GL_LINE_SMOOTH, etc.
    unsigned int bits      = 32;    // RGBA 8 bit
    glm::ivec2   shift{};           // offset relative to desktop origin
    int          monitorNr = 0;     // assign to specific monitor index, only relevant for fullscreen windows
    glm::ivec2   size           = {1,1};
    int          refreshRate    = 60;     // irrelevant for non-fullscreen windows
    bool         scaleToMonitor = false;  // specified whether the window content area should be resized based on the monitor
                                  // content scale of any monitor it is placed on. This includes the initial placement
                                  // when the window is created
    void *shareCont              = nullptr;
    bool  transparentFramebuffer = false;
    void *extWinHandle           = nullptr;
    void *glbase                 = nullptr;
    glm::vec2 contScale{1.f, 1.f};
    std::string title            = "GLBase";
};

typedef struct glVidMode {
    int width;
    int height;
    int redBits;
    int greenBits;
    int blueBits;
    int refreshRate;
} glVidMode;

class WindowCallbacks {
public:
    std::function<void(unsigned int)>       character;
    std::function<void(unsigned int, int)>  charmods;
    std::function<void()>                   close;
    std::function<void(double, double)>     cursorPos;
    std::function<void(bool)>               cursorEnter;
    std::function<void(int, const char **)> drop;
    std::function<void(int, int)>           fbsize;
    std::function<void(bool)>               focus;
    std::function<void(bool)>               iconify;
    std::function<void(int, int, int, int)> key;
    std::function<void()>                   refresh;
    std::function<void(bool)>               maximize;
    std::function<void(int, int, int)>      mouseButton;
    std::function<void(int, int)>           pos;
    std::function<void(float, float)>       scale;
    std::function<void(double, double)>     scroll;
    std::function<void(int, int)>           size;
};

}  // namespace ara
