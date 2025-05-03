#include "Lights/LIProjector.h"

#include <Utils/FBO.h>
#include <WindowManagement/WindowBase.h>

using namespace glm;
using namespace std;

namespace ara {

LIProjector::LIProjector(sceneData* sd) : Light(sd) {
    setName(getTypeName<LIProjector>());
    m_transFlag |= GLSG_NO_SCALE;
    float ansiLumen = 125.f;
    s_lightProp.setColor(ansiLumen, ansiLumen, ansiLumen);
}

void LIProjector::setup(bool force) {
    s_lightProp.setAmbientColor(0.03f, 0.03f, 0.03f);

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
    s_lightProp.setLightMode(1.f);             // projector light
    s_lightProp.setFloat("m_aspect", m_aspect);
    s_lightProp.setFloat("throwRatio", m_throwRatio);

    float sc            = 0.5f;
    m_scale_bias_matrix = mat4(vec4(sc, 0.0f, 0.0f, 0.0f), vec4(0.0f, sc, 0.0f, 0.0f), vec4(0.0f, 0.0f, sc, 0.0f),
                               vec4(sc, sc, sc, 1.0f));

    s_near              = 0.1f;
    m_linearDepthScalar = 1.0f / (s_far - s_near);  // this helps us remap depth values to be linear

    vec3 lightCenter = m_transVec + s_direction;

    // rotate up vector
    vec3 upVec = normalize(vec3(m_rotMat * vec4(0.f, 1.f, 0.f, 0.f)));
    s_view_mat = glm::lookAt(m_transVec, lightCenter, upVec);

    // calculate the fovY n respect to throwRatio and aspect
    s_fov = static_cast<float>(atan(1.0 / (double) (2.0 * m_throwRatio * m_aspect)));

    float tt = s_near * 0.5f / m_throwRatio;

    vec2 p{m_lensShift[0] * s_near / 100.0f, m_lensShift[1] * s_near / 100.0f};
    vec2 dp{tt, tt / m_aspect};

    s_proj_mat    = frustum(p.x - dp.x, p.x + dp.x, p.y - dp.y, p.y + dp.y, s_near, s_far);
    s_pvm_mat     = s_proj_mat * s_view_mat;
    s_shadow_mat  = m_scale_bias_matrix * s_proj_mat * s_view_mat;
    s_needsRecalc = true;
}

}  // namespace ara
