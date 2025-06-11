//
// Created by sven on 05-07-22.
//

#ifdef __ANDROID__

#include "Android/UIAppAndroidJNI.h"


#include <UIElements/UINodeBase/UINode.h>
#include "UIApplication.h"

namespace ara {

UIAppAndroidJNI::UIAppAndroidJNI(/*AAssetManager* asset_manager*/) : UIApplicationBase() {
    m_threadedWindowRendering = false;
    // can't initialize the application here, since there is no gl context yet
}

void UIAppAndroidJNI::setInternalDataPath(std::string path) {
    m_internalPath = std::move(path);
}

void UIAppAndroidJNI::setExternalDataPath(const std::string& path) {
    LOG << "UIAppAndroidJNI::setExternalDataPath " << path;
}

void UIAppAndroidJNI::setDisplayDensity(float density, float w, float h, float xdpi, float ydpi) {
    m_cmd_data.density        = density;
    m_cmd_data.xdpi           = xdpi;
    m_cmd_data.ydpi           = ydpi;
    m_cmd_data.width_meters   = (w / xdpi) * kMetersPerInch;
    m_cmd_data.height_meters  = (h / ydpi) * kMetersPerInch;
    m_glbase.g_androidDensity = density;
}

void UIAppAndroidJNI::OnStart() {
    for (auto &it : m_appStateCbs[android_app_cmd::onStart]) {
        it(&m_cmd_data);
    }
}

void UIAppAndroidJNI::OnPause() {
    for (auto &it : m_appStateCbs[android_app_cmd::onPause]) {
        it(&m_cmd_data);
    }
}

void UIAppAndroidJNI::OnResume(JNIEnv *env, void *context, void *activity) {
    m_cmd_data.activity = activity;
    // m_cmd_data.context = env->NewGlobalRef((jobject)context);
    m_cmd_data.context = context;
    m_cmd_data.env     = env;

    for (auto &it : m_appStateCbs[android_app_cmd::onResume]) {
        it(&m_cmd_data);
    }
}

void UIAppAndroidJNI::OnSurfaceCreated(JNIEnv *env) {
    m_cmd_data.env = env;
    init(nullptr);
    for (auto &it : m_appStateCbs[android_app_cmd::onSurfaceCreated]) {
        it(&m_cmd_data);
    }
}

void UIAppAndroidJNI::OnDisplayGeometryChanged(int display_rotation, int width, int height) {
    m_cmd_data.width            = width;
    m_cmd_data.height           = height;
    m_cmd_data.display_rotation = display_rotation;
    m_cmd_data.vWidth           = static_cast<int>(static_cast<float>(width) / m_cmd_data.density + 0.5f);
    m_cmd_data.vHeight          = static_cast<int>(static_cast<float>(height) / m_cmd_data.density);

    if (m_mainWindow) {
        m_mainWindow->osSetViewport(0, 0, m_cmd_data.vWidth, m_cmd_data.vHeight);
    }

    for (auto &it : m_appStateCbs[android_app_cmd::onDisplayGeometryChanged]) {
        it(&m_cmd_data);
    }
}

void UIAppAndroidJNI::OnDrawFrame() {
    // draw frame
    if (m_mainWindow && m_mainWindow->getSharedRes()) {
        m_mainWindow->getSharedRes()->setDrawFlag();
        m_mainWindow->draw(0, 0, 0);

        m_mainWindow->getSharedRes()->reqRedraw();

        // proc force redraw if requested
        if (m_mainWindow->getSharedRes()->requestRedraw) {
            m_mainWindow->getSharedRes()->requestRedraw = false;
        }
    }
}

void UIAppAndroidJNI::OnTouched(float x, float y) {
    m_cmd_data.x = x;
    m_cmd_data.y = y;

    for (auto &it : m_appStateCbs[android_app_cmd::onTouched]) {
        it(&m_cmd_data);
    }
}

}  // namespace ara

#endif