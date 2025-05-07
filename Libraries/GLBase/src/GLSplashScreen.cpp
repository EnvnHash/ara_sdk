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

#ifdef ARA_USE_GLFW

#include "GLSplashScreen.h"

#ifdef _WIN32

namespace ara {

void GLSplashScreen::open() {
    DWORD          DispNum       = 0;
    DISPLAY_DEVICE DisplayDevice = {0};
    DEVMODEA       defaultMode{};
    float          actDeskWidth;
    float          actDeskHeight;

    // initialize DisplayDevice
    DisplayDevice.cb = sizeof(DisplayDevice);

    // get all display devices
    while (EnumDisplayDevices(nullptr, DispNum, &DisplayDevice, 0)) {
        defaultMode.dmSize = sizeof(DEVMODE);
        if (!EnumDisplaySettingsA((LPSTR)DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, &defaultMode))
            OutputDebugStringA("Store default failed\n");

        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP && !defaultMode.dmPosition.x &&
            !defaultMode.dmPosition.y) {
            actDeskWidth  = static_cast<float>(defaultMode.dmPelsWidth);
            actDeskHeight = static_cast<float>(defaultMode.dmPelsHeight);
        }

        ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
        DisplayDevice.cb = sizeof(DisplayDevice);
        DispNum++;
    }  // end while for all display devices

    float texAspect = 300.f / 800.f;

    glWinPar gp;
    gp.createHidden = false;
    gp.decorated    = false;
    gp.floating     = true;
    gp.doInit       = true;
    gp.transparent  = false;
    gp.size.x        = static_cast<int>(actDeskWidth * 0.3f);
    gp.size.y       = static_cast<int>(static_cast<float>(gp.size.x) * texAspect);
    gp.shift.x       = (static_cast<int>(actDeskWidth) - gp.size.x) / 2;
    gp.shift.y       = (static_cast<int>(actDeskHeight) - gp.size.y) / 2;
    gp.resizeable   = false;
    m_win.create(gp);

    glViewportIndexedf(0, 0.f, 0.f, static_cast<float>(gp.size.x), static_cast<float>(gp.size.y));
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_shCol  = std::make_unique<ShaderCollector>();
    m_quad   = std::make_unique<Quad>(QuadInitParams{});
    m_stdTex = m_shCol->getStdTex();
    m_tex.loadTexture2D("data/precision_splash.jpg", 1);

    m_stdTex->begin();
    m_stdTex->setIdentMatrix4fv("m_pvm");
    m_stdTex->setUniform1i("tex", 0);
    m_tex.bind(0);
    m_quad->draw();

    m_win.swap();

    wglMakeCurrent(nullptr, nullptr);
}

void GLSplashScreen::close() {
    m_win.makeCurrent();
    m_quad.reset();
    m_shCol.reset();
    m_tex.releaseTexture();
    m_win.close();
    m_win.destroy();
}
}  // namespace ara

#endif
#endif