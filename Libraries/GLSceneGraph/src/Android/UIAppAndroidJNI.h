//
// Created by sven on 05-07-22.
//

#pragma once

#include <jni.h>

#include "Android/UIAppAndroidCommon.h"
#include "UIApplicationBase.h"

namespace ara {

class UIAppAndroidJNI : public UIApplicationBase {
public:
    UIAppAndroidJNI(/*AAssetManager* asset_manager*/);

    virtual void init(std::function<void()> func) = 0;

    void setInternalDataPath(std::string path);
    void setExternalDataPath(std::string path);
    void setDisplayDensity(float density, float w, float h, float xdpi, float ydpi);

    /// OnStart is called on the UI thread from the Activity's onStart method.
    void OnStart();

    /// OnPause is called on the UI thread from the Activity's onPause method.
    void OnPause();

    /// OnResume is called on the UI thread from the Activity's onResume method.
    void OnResume(JNIEnv* env, void* context, void* activity);

    /// OnSurfaceCreated is called on the OpenGL thread when GLSurfaceView is
    /// created.
    void OnSurfaceCreated(JNIEnv* env);

    /** \brief OnDisplayGeometryChanged is called on the OpenGL thread when the
     * render surface size or display rotation changes.
     *
     * @param display_rotation: current display rotation.
     * @param width: width of the changed surface view.
     * @param height: height of the changed surface view.
     * */
    void OnDisplayGeometryChanged(int display_rotation, int width, int height);

    /// OnDrawFrame is called on the OpenGL thread to render the next frame.
    void OnDrawFrame();

    /** \brief OnTouched is called on the OpenGL thread after the user touches
     * the screen.
     * @param x: x position on the screen (pixels).
     * @param y: y position on the screen (pixels).
     */
    void OnTouched(float x, float y);
    void set_requested_screen_orientation(ANativeActivity* activity, int an_orientation);

    float getDisplayDensity() { return m_glbase.g_androidDensity; }

private:
    int m_width            = 1;
    int m_height           = 1;
    int m_display_rotation = 0;

public:
    std::unordered_map<android_app_cmd, std::list<std::function<void(android_cmd_data*)>>> m_appStateCbs;
    android_cmd_data                                                                       m_cmd_data;
    AAssetManager*                                                                         m_asset_manager = nullptr;
};

}  // namespace ara