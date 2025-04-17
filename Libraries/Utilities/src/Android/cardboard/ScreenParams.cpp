//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#include "Android/cardboard/ScreenParams.h"

#include "Android/cardboard/jni_utils.h"

namespace ara::cardboard {

void ScreenParams::init(JNIEnv* env, jobject context) {
    m_context = context;
    m_env     = env;
    LoadJNIResources(env);
}

void ScreenParams::LoadJNIResources(JNIEnv* env) {
    m_screen_params_utils_class = reinterpret_cast<jclass>(
        env->NewGlobalRef(LoadJClass(env, "com/google/cardboard/sdk/screenparams/ScreenParamsUtils")));
    m_screen_pixel_density_class =
        reinterpret_cast<jclass>(env->NewGlobalRef(LoadJClass(env,
                                                              "com/google/cardboard/sdk/screenparams/"
                                                              "ScreenParamsUtils$ScreenPixelDensity")));
}

DisplayMetrics ScreenParams::getDisplayMetrics() {
    if (m_env) return {0.f, 0.f};

    const jmethodID get_screen_pixel_density_method =
        m_env->GetStaticMethodID(m_screen_params_utils_class, "getScreenPixelDensity",
                                 "(Landroid/content/Context;)Lcom/google/cardboard/sdk/screenparams/"
                                 "ScreenParamsUtils$ScreenPixelDensity;");

    const jobject screen_pixel_density =
        m_env->CallStaticObjectMethod(m_screen_params_utils_class, get_screen_pixel_density_method, m_context);
    const jfieldID xdpi_id = m_env->GetFieldID(m_screen_pixel_density_class, "xdpi", "F");
    const jfieldID ydpi_id = m_env->GetFieldID(m_screen_pixel_density_class, "ydpi", "F");

    const float xdpi = m_env->GetFloatField(screen_pixel_density, xdpi_id);
    const float ydpi = m_env->GetFloatField(screen_pixel_density, ydpi_id);

    return {xdpi, ydpi};
}

/*
glm::vec2 ScreenParams::getScreenSizeInMeters(int width_pixels, int
height_pixels)
{
    const DisplayMetrics display_metrics = getDisplayMetrics();

    return glm::vec2{   (width_pixels / display_metrics.xdpi) * kMetersPerInch,
                        (height_pixels / display_metrics.ydpi) * kMetersPerInch
};
}
*/
}  // namespace ara::cardboard

#endif