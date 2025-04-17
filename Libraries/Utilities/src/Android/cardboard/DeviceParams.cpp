//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#include "DeviceParams.h"

#include <Android/cardboard/jni_utils.h>
#include <Log.h>

namespace ara::cardboard {

DeviceParams::~DeviceParams() {
    if (!m_env) return;
    m_env->DeleteGlobalRef(m_java_device_params);
}

void DeviceParams::init(JNIEnv *env, jobject context) {
    m_env                       = env;
    m_context                   = context;
    m_device_params_utils_class = reinterpret_cast<jclass>(
        env->NewGlobalRef(LoadJClass(env, "com/google/cardboard/sdk/deviceparams/DeviceParamsUtils")));
}

void DeviceParams::ParseFromArray(const uint8_t *encoded_device_params, int size) {
    jmethodID mid = m_env->GetStaticMethodID(m_device_params_utils_class, "parseCardboardDeviceParams",
                                             "([B)Lcom/google/cardboard/proto/CardboardDevice$DeviceParams;");

    jbyteArray encoded_device_params_array = m_env->NewByteArray(size);
    m_env->SetByteArrayRegion(encoded_device_params_array, 0, size,
                              const_cast<jbyte *>(reinterpret_cast<const jbyte *>(encoded_device_params)));

    jobject device_params_obj =
        m_env->CallStaticObjectMethod(m_device_params_utils_class, mid, encoded_device_params_array);

    if (m_java_device_params != nullptr) {
        m_env->DeleteGlobalRef(m_java_device_params);
    }

    m_java_device_params = m_env->NewGlobalRef(device_params_obj);
}

// wrapped within a reusable function or macro.
float DeviceParams::screen_to_lens_distance() const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getScreenToLensDistance", "()F");
    CheckExceptionInJava(m_env);
    const float screen_to_lens_distance = m_env->CallFloatMethod(m_java_device_params, mid);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve ScreenToLensDistance from device parameters. "
                "Using Cardboard Viewer v1 parameter.";
        return kCardboardV1ScreenToLensDistance;
    }
    return screen_to_lens_distance;
}

float DeviceParams::inter_lens_distance() const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getInterLensDistance", "()F");
    CheckExceptionInJava(m_env);

    const float inter_lens_distance = m_env->CallFloatMethod(m_java_device_params, mid);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve InterLensDistance from device parameters. "
                "Using Cardboard Viewer v1 parameter.";
        return kCardboardV1InterLensDistance;
    }
    return inter_lens_distance;
}

float DeviceParams::tray_to_lens_distance() const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getTrayToLensDistance", "()F");
    CheckExceptionInJava(m_env);

    const float tray_to_lens_distance = m_env->CallFloatMethod(m_java_device_params, mid);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve TrayToLensDistance from device parameters. "
                "Using Cardboard Viewer v1 parameter.";
        return kCardboardV1TrayToLensDistance;
    }
    return tray_to_lens_distance;
}

int DeviceParams::vertical_alignment() const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getVerticalAlignment",
                                       "()Lcom/google/cardboard/proto/"
                                       "CardboardDevice$DeviceParams$VerticalAlignmentType;");
    CheckExceptionInJava(m_env);
    jobject vertical_alignment = m_env->CallObjectMethod(m_java_device_params, mid);
    CheckExceptionInJava(m_env);
    jmethodID get_enum_value_mid = m_env->GetMethodID(m_env->GetObjectClass(vertical_alignment), "ordinal", "()I");
    CheckExceptionInJava(m_env);

    const int vertical_alignment_type = m_env->CallIntMethod(vertical_alignment, get_enum_value_mid);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve VerticalAlignmentType from device parameters. "
                "Using Cardboard Viewer v1 parameter.";
        return kCardboardV1VerticalAlignmentType;
    }
    return vertical_alignment_type;
}

float DeviceParams::distortion_coefficients(int index) const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getDistortionCoefficients", "(I)F");
    CheckExceptionInJava(m_env);

    const float distortion_coefficient = m_env->CallFloatMethod(m_java_device_params, mid, index);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve DistortionCoefficient from device parameters. "
                "Using Cardboard Viewer v1 parameter.";
        return kCardboardV1DistortionCoeffs[index];
    }
    return distortion_coefficient;
}

int DeviceParams::distortion_coefficients_size() const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getDistortionCoefficientsCount", "()I");
    CheckExceptionInJava(m_env);

    const int distortion_coefficients_size = m_env->CallIntMethod(m_java_device_params, mid);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve DistortionCoefficientsCount from device "
                "parameters. Using Cardboard Viewer v1 parameter.";
        return kCardboardV1DistortionCoeffsSize;
    }
    return distortion_coefficients_size;
}

float DeviceParams::left_eye_field_of_view_angles(int index) const {
    jclass cls = m_env->GetObjectClass(m_java_device_params);
    CheckExceptionInJava(m_env);
    jmethodID mid = m_env->GetMethodID(cls, "getLeftEyeFieldOfViewAngles", "(I)F");
    CheckExceptionInJava(m_env);

    const float left_eye_field_of_view_angle = m_env->CallFloatMethod(m_java_device_params, mid, index);

    if (CheckExceptionInJava(m_env)) {
        LOGE << "Cannot retrieve LeftEyeFieldOfViewAngle from device "
                "parameters. Using Cardboard Viewer v1 parameter.";
        return kCardboardV1FovHalfDegrees[index];
    }
    return left_eye_field_of_view_angle;
}

}  // namespace ara::cardboard

#endif