//
// Created by sven on 18-10-22.
//

#pragma once

#ifdef __ANDROID__

#include <util_common.h>

#include "Android/cardboard/cardboard_v1.h"

namespace ara::cardboard {

class DeviceParams {
public:
    enum VerticalAlignmentType { BOTTOM = 0, CENTER = 1, TOP = 2 };

    ~DeviceParams();

    // Initializes JavaVM and Android activity context.
    //
    // @param[in]      env                     JNIEnv pointer
    // @param[in]      context                 Android activity context
    void init(JNIEnv* env, jobject context);

    // Parses device parameters from serialized buffer.
    //
    // @param[in]      encoded_device_params   Device parameters byte buffer.
    // @param[in]      size                    Buffer length in bytes.
    void ParseFromArray(const uint8_t* encoded_device_params, int size);

    // Device parameters getter methods.
    float screen_to_lens_distance() const;
    float inter_lens_distance() const;
    float tray_to_lens_distance() const;
    int   vertical_alignment() const;
    float distortion_coefficients(int index) const;
    int   distortion_coefficients_size() const;
    float left_eye_field_of_view_angles(int index) const;

private:
    jobject m_java_device_params        = {nullptr};
    JNIEnv* m_env                       = nullptr;
    jobject m_context                   = nullptr;
    jclass  m_device_params_utils_class = nullptr;
};

}  // namespace ara::cardboard

#endif