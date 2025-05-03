
#include "Lights/Light.h"

using namespace glm;
using namespace std;

namespace ara {

Light::Light(sceneData* sd) : SceneNode(sd) {
    m_nodeType = sceneNodeType::light;

    // when the model Matrix of this Node changes, call the light setup() method
    pushModelMatChangedCb(this, [this](SceneNode* node) {
        s_direction = normalize(vec3{m_rotMat * vec4{0.f, 0.f, -1.f, 0.f}});    ///< needed for lightProperties /
                                                                                            ///< faster shader light calculation
        reinterpret_cast<Light*>(node)->setup(false);
        return false;  // keep in queue
    });
}

string Light::getLightShaderBlock() {
    return s_lightProp.getLightParStruct() + "layout( std140, binding=0 ) buffer Lp { LightPar lightPars[]; };\n";
}

}  // namespace ara
