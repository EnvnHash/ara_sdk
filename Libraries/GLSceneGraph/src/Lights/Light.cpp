
#include "Lights/Light.h"

using namespace glm;
using namespace std;

namespace ara {

Light::Light(sceneData* sd)
    : SceneNode(sd), s_near(1.f), s_far(1000.f), s_fov(45.f), s_shadowMapView(0), s_colTex(0), s_needsRecalc(true),
      s_direction(vec3(0.f, 0.f, -1.f)), s_lookAt(vec3(0.f, 0.f, 0.f)), s_view_mat(mat4(1.f)), s_proj_mat(mat4(1.f)),
      s_pvm_mat(mat4(1.f)), s_shadow_mat(mat4(1.f)) {
    m_nodeType = GLSG_SNT_LIGHT;

    // when the model Matrix of this Node changes, call the light setup() method
    pushModelMatChangedCb(this, [this](SceneNode* node) {
        s_direction = normalize(vec3(m_rotMat * vec4(0.f, 0.f, -1.f,
                                                     0.f)));  ///< needed for lightProperties /
                                                              ///< faster shader light calculation
        reinterpret_cast<Light*>(node)->setup();
        return false;  // keep in queue
    });
}

string Light::getLightShaderBlock() {
    return s_lightProp.getLightParStruct() + "layout( std140, binding=0 ) buffer Lp { LightPar lightPars[]; };\n";
}

}  // namespace ara
