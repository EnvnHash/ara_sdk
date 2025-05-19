//
// Created by sven on 18-10-22.
//

#pragma once

#include "Meshes/Mesh.h"
#include "Utils/Stereo/PolynomialRadialDistortion.h"
#include "Utils/Stereo/StereoDeviceParams.h"
#include "Utils/Stereo/StereoScreenParams.h"

namespace ara {

constexpr float kDefaultBorderSizeMeters = 0.003f;
class DistortionMesh;

class LensDistortion {
public:
    // All values in tanangle units.
    struct ViewportParams {
        float width;
        float height;
        float x_eye_offset;
        float y_eye_offset;
    };

    LensDistortion(const StereoDeviceParams &device_params, const StereoScreenParams &screen_params);

    std::unique_ptr<DistortionMesh> createDistortionMesh(stereoEye eye, const PolynomialRadialDistortion &distortion,
                                                         const std::array<float, 4> &fov) const;

    // Tan angle units. "DistortedUvForUndistoredUv" goes through the forward distort function. I.e. the lens.
    // UndistortedUvForDistortedUv uses the inverse distort function.
    [[nodiscard]] std::array<float, 2> distortedUvForUndistortedUv(const std::array<float, 2> &in, stereoEye eye) const;
    [[nodiscard]] std::array<float, 2> undistortedUvForDistortedUv(const std::array<float, 2> &in, stereoEye eye) const;
    [[nodiscard]] glm::mat4            getEyeProjectionMatrix(stereoEye eye, float z_near, float z_far) const;
    [[nodiscard]] Mesh                &getDistortionMesh(stereoEye eye) const;
    [[nodiscard]] glm::vec2            getLensOffs(stereoEye eye) const;
    [[nodiscard]] glm::vec2            getLensSize(stereoEye eye) const;

    void                 getEyeFieldOfView(stereoEye eye, float *field_of_view) const;
    glm::mat4            &getEyeFromHeadMatrix(stereoEye eye) { return m_eye_from_head_matrix[(int)eye]; }
    std::array<float, 4> &getFov(stereoEye eye) { return m_fov[(int)eye]; }

    static float getViewEyeOffs(stereoEye eye) {
        return StereoDeviceParams::inter_lens_distance() * (eye == stereoEye::left ? 0.5f : -0.5f);
    }

private:
    static float getYEyeOffsetMeters(float screen_height_meters);

    static std::array<float, 4> calculateFov(const PolynomialRadialDistortion &distortion, float screen_width_meters,
                                      float screen_height_meters);

    void calculateViewportParameters(stereoEye eye, const std::array<float, 4> &fov, ViewportParams *screen_params,
                                     ViewportParams *texture_params) const;

    StereoDeviceParams m_device_params;
    StereoScreenParams m_screen_params;

    std::array<std::array<float, 4>, 2>         m_fov{0.f};  // L, R, B, T
    std::array<glm::mat4, 2>                    m_eye_from_head_matrix{};
    std::unique_ptr<DistortionMesh>             m_left_mesh;
    std::unique_ptr<DistortionMesh>             m_right_mesh;
    std::unique_ptr<PolynomialRadialDistortion> m_distortion;
    glm::vec2                                   m_screen_meters{0.f};
};

}  // namespace ara
