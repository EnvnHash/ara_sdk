//
// Created by sven on 18-10-22.
//

#pragma once

#include <Android/cardboard/cardboard_v1.h>

namespace ara {

class StereoDeviceParams {
public:
    enum VerticalAlignmentType { BOTTOM = 0, CENTER = 1, TOP = 2 };
    static float screen_to_lens_distance() { return ara::cardboard::kCardboardV1ScreenToLensDistance; }
    static float inter_lens_distance() { return ara::cardboard::kCardboardV1InterLensDistance; }
    static float tray_to_lens_distance() { return ara::cardboard::kCardboardV1TrayToLensDistance; }
    static int   vertical_alignment() { return ara::cardboard::kCardboardV1VerticalAlignmentType; }
    static float distortion_coefficients(int index) { return ara::cardboard::kCardboardV1DistortionCoeffs[index]; }
    static int   distortion_coefficients_size() { return ara::cardboard::kCardboardV1DistortionCoeffsSize; }
    static float left_eye_field_of_view_angles(int index) { return ara::cardboard::kCardboardV1FovHalfDegrees[index]; }
};

}  // namespace ara
