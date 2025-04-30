/**
 *	Standard ShadowMap Generator
 *
 *  Created by Sven Hahne on 12.07.14.
 *
 */

#include "ShadowMapStd.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

ShadowMapStd::ShadowMapStd(CameraSet* _cs, int _scrWidth, int _scrHeight, sceneData* _scd)
    : ShadowMap(_cs->getGLBase()) {
    s_cs              = _cs;
    s_shadow_map_coef = 1.f;

    s_scrWidth  = static_cast<int>((float)_scrWidth);
    s_scrHeight = static_cast<int>((float)_scrHeight);

    // s_fbo for saving the depth information
    s_fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth, s_scrHeight, 1, GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, 1, GL_REPEAT, false});

    // set the necessary texture parameters for the depth textures
    glBindTexture(GL_TEXTURE_2D, s_fbo->getDepthImg());
    // setup filtering, this will allow PCF (Percentage Closer Filter)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // setup depth comparison
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // setup wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::string vert = s_glbase->shaderCollector().getShaderHeader();
    vert += "// ShadowMapStd vertex Shader\n";
    vert += "layout(location = 0) in vec4 position; \n";

    for (unsigned int i = 0; i < 4; i++) vert += "uniform mat4 " + getStdMatrixNames()[i] + "; \n";

    vert += "void main() { \n ";
    vert += getStdPvmMult() + "}";

    std::string frag = s_glbase->shaderCollector().getShaderHeader();
    frag += "// ShadowMapStd fragment Shader\n";

    frag += STRINGIFY(layout(location = 0) out vec4 color; void main() { color = vec4(1.0); });

    s_shadowShader = s_glbase->shaderCollector().add("ShadowMapStd", vert.c_str(), frag.c_str());
}

ShadowMapStd::~ShadowMapStd() {}

void ShadowMapStd::begin() {
    s_fbo->bind();
    s_shadowShader->begin();

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(4.1f, 4.f);
}

void ShadowMapStd::end() {
    glDisable(GL_POLYGON_OFFSET_FILL);

    s_shadowShader->end();
    s_fbo->unbind();
}

}  // namespace ara
