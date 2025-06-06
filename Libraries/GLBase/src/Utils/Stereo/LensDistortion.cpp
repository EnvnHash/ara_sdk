/*
 * Copyright 2019 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Utils/Stereo/LensDistortion.h"

#include "Utils/Stereo/DistortionMesh.h"

using namespace std;
using namespace glm;

namespace ara {

LensDistortion::LensDistortion(const StereoDeviceParams &device_params, const StereoScreenParams &screen_params) {
    m_device_params = device_params;
    m_screen_params = screen_params;

    m_eye_from_head_matrix[static_cast<int>(stereoEye::left)] = translate(vec3{StereoDeviceParams::inter_lens_distance() * 0.5f, 0.f, 0.f});
    m_eye_from_head_matrix[static_cast<int>(stereoEye::right)] =
        translate(vec3{-StereoDeviceParams::inter_lens_distance() * 0.5f, 0.f, 0.f});

    std::vector distortion_coefficients(StereoDeviceParams::distortion_coefficients_size(), 0.0f);

    for (int i = 0; i < StereoDeviceParams::distortion_coefficients_size(); i++) {
        distortion_coefficients.at(i) = StereoDeviceParams::distortion_coefficients(i);
    }

    m_distortion    = std::make_unique<PolynomialRadialDistortion>(distortion_coefficients);
    m_screen_meters = m_screen_params.getScreenSizeInMeters();

    m_fov[static_cast<int>(stereoEye::left)] = calculateFov(*m_distortion, m_screen_meters.x, m_screen_meters.y);

    // Mirror fov for right eye.
    m_fov[static_cast<int>(stereoEye::right)]    = m_fov[static_cast<int>(stereoEye::left)];
    m_fov[static_cast<int>(stereoEye::right)][0] = m_fov[static_cast<int>(stereoEye::left)][1];
    m_fov[static_cast<int>(stereoEye::right)][1] = m_fov[static_cast<int>(stereoEye::left)][0];

    m_left_mesh  = createDistortionMesh(stereoEye::left, *m_distortion, m_fov[static_cast<int>(stereoEye::left)]);
    m_right_mesh = createDistortionMesh(stereoEye::right, *m_distortion, m_fov[static_cast<int>(stereoEye::right)]);
}

mat4 LensDistortion::getEyeProjectionMatrix(stereoEye eye, float z_near, float z_far) const {
    return perspective(m_fov[static_cast<int>(eye)][3] + m_fov[static_cast<int>(eye)][2], m_fov[static_cast<int>(eye)][0] / m_fov[static_cast<int>(eye)][2], z_near,
                            z_far);
}

void LensDistortion::getEyeFieldOfView(stereoEye eye, float *field_of_view) const {
    std::copy_n(m_fov[static_cast<int>(eye)].data(), 4, field_of_view);
}

vec2 LensDistortion::getLensOffs(stereoEye eye) const {
    if (eye == stereoEye::left) {
        return (m_left_mesh->m_max - m_left_mesh->m_min) * 0.5f + m_left_mesh->m_min;
    } else {
        return (m_right_mesh->m_max - m_right_mesh->m_min) * 0.5f + m_right_mesh->m_min;
    }
}

vec2 LensDistortion::getLensSize(stereoEye eye) const {
    if (eye == stereoEye::left) {
        return (m_left_mesh->m_max - m_left_mesh->m_min) * 0.5f;
    } else {
        return (m_right_mesh->m_max - m_right_mesh->m_min) * 0.5f;
    }
}

Mesh &LensDistortion::getDistortionMesh(stereoEye eye) const {
    return eye == stereoEye::left ? m_left_mesh->getMesh() : m_right_mesh->getMesh();
}

std::array<float, 2> LensDistortion::distortedUvForUndistortedUv(const std::array<float, 2> &in, stereoEye eye) const {
    if (m_screen_meters.x == 0 || m_screen_meters.y == 0) {
        return {0, 0};
    }

    ViewportParams screen_params{}, texture_params{};

    calculateViewportParameters(eye, m_fov[static_cast<int>(eye)], &screen_params, &texture_params);

    // Convert input from normalized [0, 1] screen coordinates to eye-centered tan angle units.
    std::array<float, 2> undistorted_uv_tanangle = {in[0] * screen_params.width - screen_params.x_eye_offset,
                                                    in[1] * screen_params.height - screen_params.y_eye_offset};

    auto distorted_uv_tanangle = m_distortion->Distort(undistorted_uv_tanangle);

    // Convert output from tanangle units to normalized [0, 1] pre distort texture space.
    return {(distorted_uv_tanangle[0] + texture_params.x_eye_offset) / texture_params.width,
            (distorted_uv_tanangle[1] + texture_params.y_eye_offset) / texture_params.height};
}

std::array<float, 2> LensDistortion::undistortedUvForDistortedUv(const std::array<float, 2> &in, stereoEye eye) const {
    if (m_screen_meters.x == 0 || m_screen_meters.y == 0) {
        return {0, 0};
    }

    ViewportParams screen_params{}, texture_params{};

    calculateViewportParameters(eye, m_fov[static_cast<int>(eye)], &screen_params, &texture_params);

    // Convert input from normalized [0, 1] pre distort texture space to eye-centered tan angle units.
    std::array distorted_uv_tanangle = {in[0] * texture_params.width - texture_params.x_eye_offset,
                                        in[1] * texture_params.height - texture_params.y_eye_offset};

    std::array<float, 2> undistorted_uv_tanangle = m_distortion->DistortInverse(distorted_uv_tanangle);

    // Convert output from tan angle units to normalized [0, 1] screen
    // coordinates.
    return {(undistorted_uv_tanangle[0] + screen_params.x_eye_offset) / screen_params.width,
            (undistorted_uv_tanangle[1] + screen_params.y_eye_offset) / screen_params.height};
}

std::array<float, 4> LensDistortion::calculateFov(const PolynomialRadialDistortion &distortion,
                                                  float screen_width_meters, float screen_height_meters) {
    // FOV angles in device parameters are in degrees so they are converted to radians for posterior use.
    std::array<float, 4> device_fov = {
        radians(StereoDeviceParams::left_eye_field_of_view_angles(0)),
        radians(StereoDeviceParams::left_eye_field_of_view_angles(1)),
        radians(StereoDeviceParams::left_eye_field_of_view_angles(2)),
        radians(StereoDeviceParams::left_eye_field_of_view_angles(3)),
    };

    const float eye_to_screen_distance = StereoDeviceParams::screen_to_lens_distance();
    const float outer_distance         = (screen_width_meters - StereoDeviceParams::inter_lens_distance()) / 2.0f;
    const float inner_distance         = StereoDeviceParams::inter_lens_distance() / 2.0f;
    const float bottom_distance        = getYEyeOffsetMeters(screen_height_meters);
    const float top_distance           = screen_height_meters - bottom_distance;

    const float outer_angle  = atan(distortion.Distort({outer_distance / eye_to_screen_distance, 0})[0]);
    const float inner_angle  = atan(distortion.Distort({inner_distance / eye_to_screen_distance, 0})[0]);
    const float bottom_angle = atan(distortion.Distort({0, bottom_distance / eye_to_screen_distance})[1]);
    const float top_angle    = atan(distortion.Distort({0, top_distance / eye_to_screen_distance})[1]);

    return {std::min(outer_angle, device_fov[0]), std::min(inner_angle, device_fov[1]),
            std::min(bottom_angle, device_fov[2]), std::min(top_angle, device_fov[3])};
}

float LensDistortion::getYEyeOffsetMeters(float screen_height_meters) {
    switch (StereoDeviceParams::vertical_alignment()) {
        case StereoDeviceParams::CENTER:
        default: return screen_height_meters / 2.0f;
        case StereoDeviceParams::BOTTOM: return StereoDeviceParams::tray_to_lens_distance() - kDefaultBorderSizeMeters;
        case StereoDeviceParams::TOP:
            return screen_height_meters - StereoDeviceParams::tray_to_lens_distance() - kDefaultBorderSizeMeters;
    }
}

std::unique_ptr<DistortionMesh> LensDistortion::createDistortionMesh(stereoEye                         eye,
                                                                     const PolynomialRadialDistortion &distortion,
                                                                     const std::array<float, 4>       &fov) const {
    ViewportParams screen_params{}, texture_params{};

    calculateViewportParameters(eye, fov, &screen_params, &texture_params);

    return make_unique<DistortionMesh>(distortion, vec2{screen_params.width, screen_params.height},
                                       vec2{screen_params.x_eye_offset, screen_params.y_eye_offset}, vec2{texture_params.width,
                                       texture_params.height}, vec2{texture_params.x_eye_offset, texture_params.y_eye_offset});
}

void LensDistortion::calculateViewportParameters(stereoEye eye, const std::array<float, 4> &fov,
                                                 ViewportParams *screen_params, ViewportParams *texture_params) const {
    screen_params->width  = m_screen_meters.x / StereoDeviceParams::screen_to_lens_distance();
    screen_params->height = m_screen_meters.y / StereoDeviceParams::screen_to_lens_distance();

    screen_params->x_eye_offset = eye == stereoEye::left
                                      ? ((m_screen_meters.x - StereoDeviceParams::inter_lens_distance()) * 0.5f) /
                                            StereoDeviceParams::screen_to_lens_distance()
                                      : ((m_screen_meters.x + StereoDeviceParams::inter_lens_distance()) * 0.5f) /
                                            StereoDeviceParams::screen_to_lens_distance();

    screen_params->y_eye_offset = getYEyeOffsetMeters(m_screen_meters.y) / StereoDeviceParams::screen_to_lens_distance();

    texture_params->width  = tan(fov[0]) + tan(fov[1]);
    texture_params->height = tan(fov[2]) + tan(fov[3]);

    texture_params->x_eye_offset = tan(fov[0]);
    texture_params->y_eye_offset = tan(fov[2]);
}

}  // namespace ara
