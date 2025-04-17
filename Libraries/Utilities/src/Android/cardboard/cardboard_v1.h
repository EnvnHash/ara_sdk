//
// Created by sven on 18-10-22.
//

#pragma once

#include <util_common.h>

namespace ara::cardboard {

/// Device params for Cardboard V1 released at Google I/O 2014.
/// {@
constexpr float kCardboardV1InterLensDistance     = 0.06f;
constexpr float kCardboardV1TrayToLensDistance    = 0.035f;
constexpr float kCardboardV1ScreenToLensDistance  = 0.042f;
constexpr float kCardboardV1FovHalfDegrees[]      = {40.0f, 40.0f, 40.0f, 40.0f};
constexpr float kCardboardV1DistortionCoeffs[]    = {0.441f, 0.156f};
constexpr int   kCardboardV1DistortionCoeffsSize  = 2;
constexpr int   kCardboardV1VerticalAlignmentType = 0;
constexpr char  kCardboardV1Vendor[]              = "Google, Inc.";
constexpr char  kCardboardV1Model[]               = "Cardboard v1";
/// @}

std::vector<uint8_t> getCardboardV1DeviceParams();
}  // namespace ara::cardboard
