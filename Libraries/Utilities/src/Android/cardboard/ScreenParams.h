//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#pragma once

#include <util_common.h>

namespace ara::cardboard {

struct DisplayMetrics {
    float xdpi;
    float ydpi;
};

class ScreenParams {
public:
    void           init(JNIEnv* env, jobject context);
    void           LoadJNIResources(JNIEnv* env);
    DisplayMetrics getDisplayMetrics();
    // glm::vec2 getScreenSizeInMeters(int width_pixels, int height_pixels);

private:
    jclass  m_screen_params_utils_class  = nullptr;
    jclass  m_screen_pixel_density_class = nullptr;
    JNIEnv* m_env                        = nullptr;
    jobject m_context                    = nullptr;

    static inline float kMetersPerInch = 0.0254f;
};

}  // namespace ara::cardboard

#endif