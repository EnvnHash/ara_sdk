//
// Created by user on 30.09.2020.
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
    gp.width        = static_cast<int>(actDeskWidth * 0.3f);
    gp.height       = static_cast<int>(static_cast<float>(gp.width) * texAspect);
    gp.shiftX       = (static_cast<int>(actDeskWidth) - gp.width) / 2;
    gp.shiftY       = (static_cast<int>(actDeskHeight) - gp.height) / 2;
    gp.resizeable   = false;
    m_win.create(gp);

    glViewportIndexedf(0, 0.f, 0.f, static_cast<float>(gp.width), static_cast<float>(gp.height));
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_shCol  = std::make_unique<ShaderCollector>();
    m_quad   = std::make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f});
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