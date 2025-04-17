#include "Lights/LIStandardSpot.h"

using namespace glm;
using namespace std;

namespace ara {

LIStandardSpot::LIStandardSpot(sceneData* sd) : Light(sd) {
    setName(getTypeName<LIStandardSpot>());
    m_transFlag |= GLSG_NO_SCALE;
}

void LIStandardSpot::setup(bool force) {
    if (s_sd && s_sd->winViewport.z != 0 && s_sd->winViewport.w != 0) {
        s_fov = 90.f;

        s_lightProp.setAmbientColor(0.f, 0.f, 0.f);
        s_lightProp.setColor(0.5f, 0.5f, 0.5f);

        s_lightProp.setPosition(m_transVec.x, m_transVec.y, m_transVec.z);
        s_lightProp.setConstantAttenuation(0.0f);
        s_lightProp.setLinearAttenuation(0.0f);
        s_lightProp.setQuadraticAttenuation(0.025f);
        s_lightProp.setSpotCosCutoff(0.3f);
        s_lightProp.setSpotExponent(0.002f);

        s_lightProp.setEyeDirection(-s_direction.x, -s_direction.y, -s_direction.z);
        s_lightProp.setConeDirection(s_direction.x, s_direction.y,
                                     s_direction.z);  // must point opposite of eye dir
        s_lightProp.setDirection(s_direction.x, s_direction.y,
                                 s_direction.z);   // by scaling this, the intensity varies
        s_lightProp.setHalfVector(0.f, 1.f, 0.f);  // init with anything
        s_lightProp.setLightMode(0.f);             // spot light
        s_lightProp.setFloat("m_aspect", 16.f / 9.f);
        s_lightProp.setFloat("fovY",
                             radians(s_fov) * 0.5f);  // apply * 0.5f already here and not in the
                                                      // shader, for brightness calc

        float sc          = 0.5f;
        scale_bias_matrix = mat4(vec4(sc, 0.0f, 0.0f, 0.0f), vec4(0.0f, sc, 0.0f, 0.0f), vec4(0.0f, 0.0f, sc, 0.0f),
                                 vec4(sc, sc, sc, 1.0f));

        s_near            = 0.1f;
        linearDepthScalar = 1.0f / (s_far - s_near);  //  remap depth values to be linear

        vec3 lightCenter = m_transVec + s_direction;

        // rotate up vector
        vec3 upVec = normalize(vec3(m_rotMat * vec4(0.f, 1.f, 0.f, 0.f)));
        s_view_mat = glm::lookAt(m_transVec, lightCenter, upVec);

        float aspect = float(s_sd->winViewport.z) / float(s_sd->winViewport.w);
        s_proj_mat   = perspective(radians(s_fov), aspect, s_near, s_far);
        s_pvm_mat    = s_proj_mat * s_view_mat;

        s_shadow_mat  = scale_bias_matrix * s_proj_mat * s_view_mat;
        s_needsRecalc = true;
    }
}

}  // namespace ara
