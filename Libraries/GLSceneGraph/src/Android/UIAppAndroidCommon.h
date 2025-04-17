//
// Created by sven on 05-07-22.
//

#pragma once

#include <android/asset_manager.h>
#include <android/sensor.h>
#include <android_native_app_glue.h>

#include <functional>

namespace ara {

class android_app_state {
public:
    float   angle = 0.f;
    int32_t x     = 0;
    int32_t y     = 0;
};

enum class android_app_cmd : int {
    onPause = 0,
    onStart,
    onStop,
    onResume,
    onSurfaceCreated,
    onDisplayGeometryChanged,
    onTouched
};

class android_cmd_data {
public:
    JNIEnv*                  env              = nullptr;
    void*                    context          = nullptr;
    void*                    activity         = nullptr;
    int                      display_rotation = 0;
    int                      width            = 1;
    int                      height           = 1;
    int                      vWidth           = 1;
    int                      vHeight          = 1;
    int                      x                = 0;
    int                      y                = 0;
    float                    density          = 1.f;
    float                    xdpi             = 0.f;
    float                    ydpi             = 0.f;
    float                    width_meters     = 0.f;
    float                    height_meters    = 0.f;
    std::function<void(int)> oriCb;
    std::function<void()>    resetOri;
};

static constexpr float kMetersPerInch = 0.0254f;

}  // namespace ara