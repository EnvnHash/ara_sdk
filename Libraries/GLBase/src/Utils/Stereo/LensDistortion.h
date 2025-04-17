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

    LensDistortion(StereoDeviceParams &device_params, StereoScreenParams &screen_params);

    std::unique_ptr<DistortionMesh> createDistortionMesh(StereoEye eye, const PolynomialRadialDistortion &distortion,
                                                         const std::array<float, 4> &fov);

    // Tan angle units. "DistortedUvForUndistoredUv" goes through the forward
    // distort function. I.e. the lens. UndistortedUvForDistortedUv uses the
    // inverse distort function.
    std::array<float, 2> distortedUvForUndistortedUv(const std::array<float, 2> &in, StereoEye eye);
    std::array<float, 2> undistortedUvForDistortedUv(const std::array<float, 2> &in, StereoEye eye);
    glm::mat4            getEyeProjectionMatrix(StereoEye eye, float z_near, float z_far);
    void                 getEyeFieldOfView(StereoEye eye, float *field_of_view) const;
    Mesh                &getDistortionMesh(StereoEye eye) const;
    glm::vec2            getLensOffs(StereoEye eye);
    glm::vec2            getLensSize(StereoEye eye);

    glm::mat4            &getEyeFromHeadMatrix(StereoEye eye) { return m_eye_from_head_matrix[(int)eye]; }
    std::array<float, 4> &getFov(StereoEye eye) { return m_fov[(int)eye]; }

    float getViewEyeOffs(StereoEye eye) {
        return m_device_params.inter_lens_distance() * (eye == StereoEye::left ? 0.5f : -0.5f);
    }

private:
    float getYEyeOffsetMeters(float screen_height_meters);

    std::array<float, 4> calculateFov(const PolynomialRadialDistortion &distortion, float screen_width_meters,
                                      float screen_height_meters);

    void calculateViewportParameters(StereoEye eye, const std::array<float, 4> &fov, ViewportParams *screen_params,
                                     ViewportParams *texture_params);

    StereoDeviceParams m_device_params;
    StereoScreenParams m_screen_params;

    std::array<std::array<float, 4>, 2>         m_fov{0.f};  // L, R, B, T
    std::array<glm::mat4, 2>                    m_eye_from_head_matrix;
    std::unique_ptr<DistortionMesh>             m_left_mesh;
    std::unique_ptr<DistortionMesh>             m_right_mesh;
    std::unique_ptr<PolynomialRadialDistortion> m_distortion;
    glm::vec2                                   m_screen_meters{0.f};
};

}  // namespace ara
